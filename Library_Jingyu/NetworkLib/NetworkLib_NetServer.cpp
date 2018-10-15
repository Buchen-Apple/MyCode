/*
락프리 적용된 Network Library
*/

#include "pch.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include <process.h>
#include <strsafe.h>

#include "NetworkLib_NetServer.h"
#include "CrashDump\CrashDump.h"
#include "RingBuff\RingBuff.h"
#include "Log\Log.h"
#include "LockFree_Queue\LockFree_Queue.h"

ULONGLONG g_ullAcceptTotal;
LONG	  g_lAcceptTPS;
LONG	g_lSendPostTPS;

LONG g_lSemCount;


namespace Library_Jingyu
{

#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cNetLibLog = CSystemLog::GetInstance();

	// 덤프 남길 변수 하나 받기
	CCrashDump* cNetDump = CCrashDump::GetInstance();

	// 헤더 사이즈
#define dfNETWORK_PACKET_HEADER_SIZE_NETSERVER	5

	// 한 번에 샌드할 수 있는 WSABUF의 카운트
#define dfSENDPOST_MAX_WSABUF			300


	// ------------------------------
	// enum과 구조체
	// ------------------------------
	// 헤더 구조체
#pragma pack(push, 1)
	struct CNetServer::stProtocolHead
	{
		BYTE	m_Code;
		WORD	m_Len;
		BYTE	m_RandXORCode;
		BYTE	m_Checksum;
	};
#pragma pack(pop)
	

	// 세션 구조체
	struct CNetServer::stSession
	{
		// 세션과 연결된 소켓
		SOCKET m_Client_sock;

		// 해당 세션이 들어있는 배열 인덱스
		WORD m_lIndex;

		// 연결된 세션의 IP와 Port
		TCHAR m_IP[30];
		USHORT m_prot;

		// Send overlapped구조체
		OVERLAPPED m_overSendOverlapped;

		// 세션 ID. 컨텐츠와 통신 시 사용.
		ULONGLONG m_ullSessionID;

		// 해당 인덱스 배열이 릴리즈 되었는지 체크
		// TRUE이면 릴리즈 되었음.(사용중 아님), FALSE이면 릴리즈 안됐음.(사용중)
		LONG m_lReleaseFlag;

		// I/O 카운트. WSASend, WSARecv시 1씩 증가.
		// 0이되면 접속 종료된 유저로 판단.
		// 사유 : 연결된 유저는 WSARecv를 무조건 걸기 때문에 0이 될 일이 없다.
		LONG	m_lIOCount;				

		// 현재, WSASend에 몇 개의 데이터를 샌드했는가. (바이트 아님! 카운트. 주의!)
		int m_iWSASendCount;

		// Send한 직렬화 버퍼들 저장할 포인터 변수
		CProtocolBuff_Net* m_PacketArray[dfSENDPOST_MAX_WSABUF];

		// Send가능 상태인지 체크. 1이면 Send중, 0이면 Send중 아님
		LONG	m_lSendFlag;

		// Send버퍼. 락프리큐 구조. 패킷버퍼(직렬화 버퍼)의 포인터를 다룬다.
		CLF_Queue<CProtocolBuff_Net*>* m_SendQueue;

		// Recv overlapped구조체
		OVERLAPPED m_overRecvOverlapped;

		// Recv버퍼. 일반 링버퍼. 
		CRingBuff m_RecvQueue;

		// PQCS overlapped 구조체
		OVERLAPPED m_overPQCSOverlapped;

		// 워커에서 PQCS를 시도했는지 체크하는 Flag
		// TRUE면 워커에서 PQCS 시도중. FALSE면 아님.
		LONG m_lPQCSFlag;

		// 마지막 패킷 저장소
		void* m_LastPacket = nullptr;

		// 생성자 
		stSession()
		{
			m_SendQueue = new CLF_Queue<CProtocolBuff_Net*>(0, false);
			m_lIOCount = 0;
			m_lReleaseFlag = TRUE;
			m_lSendFlag = FALSE;
			m_lPQCSFlag = FALSE;
			m_iWSASendCount = 0;
		}

		// 소멸자
		~stSession()
		{
			delete m_SendQueue;
		}

	};

	

	// -----------------------------
	// 유저가 호출 하는 함수
	// -----------------------------

	// 서버 시작
	// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 활성화시킬 워커스레드 수, 엑셉트 스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수, 패킷 Code, XOR 1번코드, XOR 2번코드] 입력받음.
	//
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CNetServer::Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int ActiveWThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect,
							BYTE Code, BYTE XORCode1, BYTE XORCode2)
	{		

		// rand설정
		srand((UINT)time(NULL));

		// Config 데이터 셋팅
		m_bCode = Code;
		m_bXORCode_1 = XORCode1;
		m_bXORCode_2 = XORCode2;

		// 새로 시작하니까 에러코드들 초기화
		m_iOSErrorCode = 0;
		m_iMyErrorCode = (euError)0;

		// 윈속 초기화
		WSADATA wsa;
		int retval = WSAStartup(MAKEWORD(2, 2), &wsa);
		if (retval != 0)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = retval;
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WINSTARTUP_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WSAStartup() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 입출력 완료 포트 생성
		m_hIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, ActiveWThreadCount);
		if (m_hIOCPHandle == NULL)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__CREATE_IOCP_PORT;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> CreateIoCompletionPort() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 워커 스레드 생성
		m_iW_ThreadCount = WorkerThreadCount;
		m_hWorkerHandle = new HANDLE[WorkerThreadCount];

		for (int i = 0; i < m_iW_ThreadCount; ++i)
		{
			m_hWorkerHandle[i] = (HANDLE)_beginthreadex(0, 0, WorkerThread, this, 0, 0);
			if (m_hWorkerHandle[i] == 0)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__W_THREAD_CREATE_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(i);

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WorkerThread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}

		// 소켓 생성
		m_soListen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_soListen_sock == INVALID_SOCKET)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> socket() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;

		}

		// 바인딩
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		InetPton(AF_INET, bindIP, &serveraddr.sin_addr.s_addr);

		retval = bind(m_soListen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__BINDING_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> bind() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 리슨
		retval = listen(m_soListen_sock, SOMAXCONN);
		if (retval == SOCKET_ERROR)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__LISTEN_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> listen() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 연결된 리슨 소켓의 송신버퍼 크기를 0으로 변경. 그래야 정상적으로 비동기 입출력으로 실행
		// 리슨 소켓만 바꾸면 모든 클라 송신버퍼 크기는 0이된다.
		//int optval = 0;
		//retval = setsockopt(m_soListen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
		//if (optval == SOCKET_ERROR)
		//{
		//	// 윈도우 에러, 내 에러 보관
		//	m_iOSErrorCode = WSAGetLastError();
		//	m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

		//	// 각종 핸들 반환 및 동적해제 절차.
		//	ExitFunc(m_iW_ThreadCount);

		//	// 로그 찍기 (로그 레벨 : 에러)
		//	cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> setsockopt() SendBuff Size Change Error : NetError(%d), OSError(%d)",
		//		(int)m_iMyErrorCode, m_iOSErrorCode);

		//	// false 리턴
		//	return false;
		//}

		// 인자로 받은 노딜레이 옵션 사용 여부에 따라 네이글 옵션 결정
		// 이게 true면 노딜레이 사용하겠다는 것(네이글 중지시켜야함)
		if (Nodelay == true)
		{
			BOOL optval = TRUE;
			retval = setsockopt(m_soListen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));

			if (retval == SOCKET_ERROR)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = WSAGetLastError();
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> setsockopt() Nodelay apply Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}

		// 최대 접속 가능 유저 수 셋팅
		m_iMaxJoinUser = MaxConnect;

		// 세션 배열 동적할당
		m_stSessionArray = new stSession[MaxConnect];

		// 미사용 세션 관리 스택 동적할당. (락프리 스택) 
		// 그리고 미리 Max만큼 만들어두기
		m_stEmptyIndexStack = new CLF_Stack<WORD>;
		for (int i = 0; i < MaxConnect; ++i)
			m_stEmptyIndexStack->Push(i);

		// 엑셉트 스레드 생성
		m_iA_ThreadCount = AcceptThreadCount;
		m_hAcceptHandle = new HANDLE[m_iA_ThreadCount];
		for (int i = 0; i < m_iA_ThreadCount; ++i)
		{
			m_hAcceptHandle[i] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
			if (m_hAcceptHandle == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_CREATE_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Accept Thread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}



		// 서버 열렸음 !!
		m_bServerLife = true;

		// 서버 오픈 로그 찍기		
		// 이건, 상속받는 쪽에서 찍는걸로 수정. 넷서버 자체는 독립적으로 작동하지 않음.
		//cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// 서버 스탑.
	void CNetServer::Stop()
	{
		// 1. Accept 스레드 종료. 더 이상 접속을 받으면 안되니 Accept스레드 먼저 종료
		// Accept 스레드는 리슨소켓을 closesocket하면 된다.
		closesocket(m_soListen_sock);

		// Accept스레드 종료 대기
		DWORD retval = WaitForMultipleObjects(m_iA_ThreadCount, m_hAcceptHandle, TRUE, INFINITE);

		// 리턴값이 [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iW_ThreadCount - 1] 가 아니라면, 뭔가 에러가 발생한 것. 에러 찍는다
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iW_ThreadCount - 1)
		{
			// 에러 값이 WAIT_FAILED일 경우, GetLastError()로 확인해야함.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();

			// 그게 아니라면 리턴값에 이미 에러가 들어있음.
			else
				m_iOSErrorCode = retval;

			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Accept Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Accept Thread EXIT Error");
		}

		// 2. 모든 유저에게 Shutdown
		// 모든 유저에게 셧다운 날림
		for (int i = 0; i < m_iMaxJoinUser; ++i)
		{
			if (m_stSessionArray[i].m_lReleaseFlag == FALSE)
			{
				shutdown(m_stSessionArray[i].m_Client_sock, SD_BOTH);
			}
		}

		// 모든 유저가 종료되었는지 체크
		while (1)
		{
			if (m_ullJoinUserCount == 0)
				break;

			Sleep(1);
		}

		// 3. 워커 스레드 종료
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			PostQueuedCompletionStatus(m_hIOCPHandle, 0, 0, 0);

		// 워커스레드 종료 대기
		retval = WaitForMultipleObjects(m_iW_ThreadCount, m_hWorkerHandle, TRUE, INFINITE);

		// 리턴값이 [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iW_ThreadCount - 1] 가 아니라면, 뭔가 에러가 발생한 것. 에러 찍는다
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iW_ThreadCount - 1)
		{
			// 에러 값이 WAIT_FAILED일 경우, GetLastError()로 확인해야함.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();

			// 그게 아니라면 리턴값에 이미 에러가 들어있음.
			else
				m_iOSErrorCode = retval;

			// 내 에러 셋팅
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT;

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Worker Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT, L"Stop() --> Worker Thread EXIT Error");
		}

		// 4. 각종 리소스 반환
		// 1) 엑셉트 스레드 핸들 반환
		for (int i = 0; i < m_iA_ThreadCount; ++i)
			CloseHandle(m_hAcceptHandle[i]);

		// 2) 워커 스레드 핸들 반환
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			CloseHandle(m_hWorkerHandle[i]);

		// 3) 워커 스레드 배열, 엑셉트 스레드 배열 동적해제
		delete[] m_hWorkerHandle;
		delete[] m_hAcceptHandle;

		// 4) IOCP핸들 반환
		CloseHandle(m_hIOCPHandle);

		// 5) 윈속 해제
		WSACleanup();

		// 6) 세션 배열, 세션 미사용 인덱스 관리 스택 동적해제
		delete[] m_stSessionArray;
		delete m_stEmptyIndexStack;

		// 5. 서버 가동중 아님 상태로 변경
		m_bServerLife = false;

		// 6. 각종 변수 초기화
		Reset();

		// 7. 서버 종료 로그 찍기		
		cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");
	}

	// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
	// SendPacket은 그냥 아무때나 하면 된다.
	// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
	//
	// Parameter : SessionID, SendBuff, LastFlag(디폴트 FALSE)
	// return : 없음
	void CNetServer::SendPacket(ULONGLONG SessionID, CProtocolBuff_Net* payloadBuff, LONG LastFlag)
	{
		// 세션 락 걸기(락 아니지만 락처럼 사용함) ----------------
		stSession* NowSession = GetSessionLOCK(SessionID);
		if (NowSession == nullptr)
		{
			// 이 때는, 큐에도 넣지 못했기 때문에, 인자로 받은 직렬화버퍼를 Free한다.
			// ref 카운트가 0이 되면 메모리풀에 반환
			CProtocolBuff_Net::Free(payloadBuff);
			return;
		}

		// 1. m_LastPacket가 nullptr이 아니라면, 보내고 끊을 유저. 큐에 넣지 않는다.
		if (NowSession->m_LastPacket != nullptr)
		{
			CProtocolBuff_Net::Free(payloadBuff);
			return;
		}

		// 2. 인자로 받은 Flag가 true라면, 마지막 패킷의 주소를 보관
		if (LastFlag == TRUE)
			NowSession->m_LastPacket = payloadBuff;		

		// 3. 헤더를 넣어 패킷 완성
		payloadBuff->Encode(m_bCode, m_bXORCode_1, m_bXORCode_2);

		// 4. 인큐. 패킷의 "주소"를 인큐한다(8바이트)
		// 직렬화 버퍼 레퍼런스 카운트 1 증가
		payloadBuff->Add();
		NowSession->m_SendQueue->Enqueue(payloadBuff);

		// 5. 직렬화 버퍼 레퍼런스 카운트 1 감소. 0 되면 메모리풀에 반환
		CProtocolBuff_Net::Free(payloadBuff);

		// 6. PQCS전에 인터락으로 하나 증가. 여기서 증가시킨 것은 워커에서 감소
		InterlockedIncrement(&NowSession->m_lIOCount);
		
		// 7. PQCS
		PostQueuedCompletionStatus(m_hIOCPHandle, 0, (ULONG_PTR)NowSession, &NowSession->m_overPQCSOverlapped);

		// 8. 세션 락 해제(락 아니지만 락처럼 사용) ----------------------
		// 여기서 false가 리턴되면 이미 다른곳에서 삭제되었어야 했는데 SendPacket이 I/O카운트를 올림으로 인해 삭제되지 못한 유저였음.
		// 근데 따로 리턴값 받지 않고 있음
		GetSessionUnLOCK(NowSession);				   		
	}

	// 지정한 유저를 끊을 때 호출하는 함수. 외부 에서 사용.
	// 라이브러리한테 끊어줘!라고 요청하는 것 뿐
	//
	// Parameter : SessionID
	// return : 없음
	void CNetServer::Disconnect(ULONGLONG SessionID)
	{
		// 1. 세션 락 
		stSession* DeleteSession = GetSessionLOCK(SessionID);
		if (DeleteSession == nullptr)
			return;

		// 2. 끊어야하는 유저는 셧다운 날린다.
		// 차후 자연스럽게 I/O카운트가 감소되어서 디스커넥트된다.
		shutdown(DeleteSession->m_Client_sock, SD_BOTH);

		// 3. 세션 락 해제
		// 여기서 삭제된 유저는, 정상적으로 삭제된 유저일 수도 있기 때문에 (shutdown 날렸으니!) false체크 안한다.
		GetSessionUnLOCK(DeleteSession);
	}

	///// 각종 게터함수들 /////
	// 윈도우 에러 얻기
	int CNetServer::WinGetLastError()
	{
		return m_iOSErrorCode;
	}

	// 내 에러 얻기
	int CNetServer::NetLibGetLastError()
	{
		return (int)m_iMyErrorCode;
	}

	// 현재 접속자 수 얻기
	ULONGLONG CNetServer::GetClientCount()
	{
		return m_ullJoinUserCount;
	}

	// 서버 가동상태 얻기
	// return true : 가동중
	// return false : 가동중 아님
	bool CNetServer::GetServerState()
	{
		return m_bServerLife;
	}

	// 미사용 세션 관리 스택의 노드 얻기
	LONG CNetServer::GetStackNodeCount()
	{
		return m_stEmptyIndexStack->GetInNode();
	}






	// -----------------------------
	// Lib 내부에서만 사용하는 함수
	// -----------------------------
	// 생성자
	CNetServer::CNetServer()
	{
		// 서버 가동상태 false로 시작 
		m_bServerLife = false;
	}

	// 소멸자
	CNetServer::~CNetServer()
	{
		// 서버가 가동중이었으면, 서버 종료절차 진행
		if (m_bServerLife == true)
			Stop();
	}

	// 실제로 접속종료 시키는 함수
	void CNetServer::InDisconnect(stSession* DeleteSession)
	{
		// ReleaseFlag와 I/O카운트 둘 다 0이라면 'ReleaseFlag'만 TRUE로 바꾼다!
		// I/O카운트는 안바꾼다.
		// 
		// FALSE가 리턴되는 것은, 이미 DeleteSession->m_lReleaseFlag와 I/OCount가 0이었다는 의미. 
		// 로직을 타야한다.
		if (InterlockedCompareExchange64((LONG64*)&DeleteSession->m_lReleaseFlag, TRUE, FALSE) == FALSE)
		{
			// 컨텐츠쪽에 알려주기 위한 세션 ID 받아둠.
			// 해당 세션을 스택에 반납한 다음에 유저에게 알려주기 때문에 미리 받아둔다.
			ULONGLONG sessionID = DeleteSession->m_ullSessionID;

			DeleteSession->m_ullSessionID = 0xffffffffffffffff;

			// 해당 세션의 'Send 직렬화 버퍼(Send했던 직렬화 버퍼 모음. 아직 완료통지 못받은 직렬화버퍼들)'에 있는 데이터를 Free한다.
			for (int i = 0; i < DeleteSession->m_iWSASendCount; ++i)
				CProtocolBuff_Net::Free(DeleteSession->m_PacketArray[i]);

			// SendCount 초기화
			DeleteSession->m_iWSASendCount = 0;			

			int UseSize = DeleteSession->m_SendQueue->GetInNode();

			CProtocolBuff_Net* Payload;
			int i = 0;
			while (i < UseSize)
			{
				// 디큐 후, 직렬화 버퍼 메모리풀에 Free한다.
				if (DeleteSession->m_SendQueue->Dequeue(Payload) == -1)
					cNetDump->Crash();

				CProtocolBuff_Net::Free(Payload);

				i++;
			}			

			// 큐 초기화
			DeleteSession->m_RecvQueue.ClearBuffer();			

			// 클로즈 소켓
			closesocket(DeleteSession->m_Client_sock);

			// 접속 중 유저 수 감소
			InterlockedDecrement(&m_ullJoinUserCount);

			// 컨텐츠 쪽에 종료된 유저 알려줌 
			// 컨텐츠와 통신할 때는 세션키를 이용해 통신한다. 그래서 인자로 세션키를 넘겨준다.
			OnClientLeave(sessionID);

			// 미사용 인덱스 스택에 반납
			m_stEmptyIndexStack->Push(DeleteSession->m_lIndex);			
		}
		
		return;	
	}

	// Start에서 에러가 날 시 호출하는 함수.
	// 1. 입출력 완료포트 핸들 반환
	// 2. 워커스레드 종료, 핸들반환, 핸들 배열 동적해제
	// 3. 엑셉트 스레드 종료, 핸들 반환
	// 4. 리슨소켓 닫기
	// 5. 윈속 해제
	void CNetServer::ExitFunc(int w_ThreadCount)
	{
		// 1. 입출력 완료포트 해제 절차.
		// null이 아닐때만! (즉, 입출력 완료포트를 만들었을 경우!)
		// 해당 함수는 어디서나 호출되기때문에, 지금 이걸 해야하는지 항상 확인.
		if (m_hIOCPHandle != NULL)
			CloseHandle(m_hIOCPHandle);

		// 2. 워커스레드 종료 절차. Count가 0이 아닐때만!
		if (w_ThreadCount > 0)
		{
			// 종료 메시지를 보낸다
			for (int h = 0; h < w_ThreadCount; ++h)
				PostQueuedCompletionStatus(m_hIOCPHandle, 0, 0, 0);

			// 모든 워커스레드 종료 대기.
			WaitForMultipleObjects(w_ThreadCount, m_hWorkerHandle, TRUE, INFINITE);

			// 모든 워커스레드가 종료됐으면 핸들 반환
			for (int h = 0; h < w_ThreadCount; ++h)
				CloseHandle(m_hWorkerHandle[h]);

			// 워커스레드 핸들 배열 동적해제. 
			// Count가 0보다 크다면 무조건 동적할당을 한 적이 있음
			delete[] m_hWorkerHandle;
		}

		// 3. 엑셉트 스레드 종료, 핸들 반환
		if (m_iA_ThreadCount > 0)
			CloseHandle(m_hAcceptHandle);

		// 4. 리슨소켓 닫기
		if (m_soListen_sock != NULL)
			closesocket(m_soListen_sock);

		// 5. 윈속 해제
		// 윈속 초기화 하지 않은 상태에서 WSAClenup() 호출해도 아무 문제 없음
		WSACleanup();
	}

	// 워커스레드
	UINT WINAPI	CNetServer::WorkerThread(LPVOID lParam)
	{
		DWORD cbTransferred;
		stSession* stNowSession;
		OVERLAPPED* overlapped;

		CNetServer* g_This = (CNetServer*)lParam;

		while (1)
		{
			// 변수들 초기화
			cbTransferred = 0;
			stNowSession = nullptr;
			overlapped = nullptr;

			// GQCS 완료 시 함수 호출
			g_This->OnWorkerThreadEnd();

			// 비동기 입출력 완료 대기
			// GQCS 대기
			GetQueuedCompletionStatus(g_This->m_hIOCPHandle, &cbTransferred, (PULONG_PTR)&stNowSession, &overlapped, INFINITE);

					
			// --------------
			// 완료 체크
			// --------------
			// overlapped가 nullptr이라면, IOCP 에러임.
			if (overlapped == NULL)
			{
				// 셋 다 0이면 스레드 종료.
				if (cbTransferred == 0 && stNowSession == NULL)
				{
					// printf("워커 스레드 정상종료\n");

					break;
				}
				// 그게 아니면 IOCP 에러 발생한 것

				// 윈도우 에러, 내 에러 보관
				g_This->m_iOSErrorCode = GetLastError();
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__IOCP_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, L"IOCP_Error --> GQCS return Error : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// OnError 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__IOCP_ERROR, tcErrorString);

				break;
			}

			// GQCS 깨어날 시 함수호출
			g_This->OnWorkerThreadBegin();


			// -----------------
			// PQCS 요청 로직
			// -----------------
			if (&stNowSession->m_overPQCSOverlapped == overlapped)
			{
				// 1. PQCS Flag 체크
				// 여기서 TRUE가 리턴되는 것은, 이미 stNowSession->m_lPQCSFlag가 1(PQCS중)이었다는 것.
				if (InterlockedExchange(&stNowSession->m_lPQCSFlag, TRUE) == TRUE)
				{
					// 그렇다면 다시 PQCS를 건다.
					PostQueuedCompletionStatus(g_This->m_hIOCPHandle, 0, (ULONG_PTR)stNowSession, &stNowSession->m_overPQCSOverlapped);
					continue;
				}

				//  2. SendPost()
				g_This->SendPost(stNowSession);

				// 3. PQCSFlag 되돌림
				stNowSession->m_lPQCSFlag = FALSE;

				// 4. SendPacket에서 증가시켰던 I/O 카운트 감소
				if (InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
					g_This->InDisconnect(stNowSession);					

				continue;
			}


			// -----------------
			// Recv 로직
			// -----------------
			// WSArecv()가 완료된 경우, 받은 데이터가 0이 아니면 로직 처리
			else if (&stNowSession->m_overRecvOverlapped == overlapped && cbTransferred > 0)
			{
				// rear 이동
				stNowSession->m_RecvQueue.MoveWritePos(cbTransferred);

				// 1. 링버퍼 후, 패킷 처리
				g_This->RecvProc(stNowSession);

				// 2. 리시브 다시 걸기.
				// 리시브 큐가 꽉찼으면 접속 끊는다.
				if (g_This->RecvPost(stNowSession) == 1)
				{
					// I/O 카운트 감소시켰는데 0이되면 shutdown날릴것도 없이 InDisconnect.
					if (InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
					{
						g_This->InDisconnect(stNowSession);
						continue;
					}

					// 0이 아니라면 서버측에서 접속 끊는다.
					shutdown(stNowSession->m_Client_sock, SD_BOTH);
					continue;
				}

			}

			// -----------------
			// Send 로직
			// -----------------
			// WSAsend()가 완료된 경우, 받은 데이터가 0이 아니면 로직처리
			else if (&stNowSession->m_overSendOverlapped == overlapped && cbTransferred > 0)
			{
				// !! 테스트 출력용 !!
				// sendpostTPS 추가
				InterlockedAdd(&g_lSendPostTPS, stNowSession->m_iWSASendCount);

				// 1. 샌드 완료됐다고 컨텐츠에 알려줌
				g_This->OnSend(stNowSession->m_ullSessionID, cbTransferred);		

				// 2. 보내고 끊을 유저일 경우 로직
				if (stNowSession->m_LastPacket != nullptr)
				{
					// 직렬화 버퍼 해제
					int i = 0;
					bool Flag = false;
					while (i < stNowSession->m_iWSASendCount)
					{
						CProtocolBuff_Net::Free(stNowSession->m_PacketArray[i]);

						// 마지막 패킷이 잘 갔으면, falg를 True로 바꾼다.
						if (stNowSession->m_PacketArray[i] == stNowSession->m_LastPacket)
							Flag = true;

						++i;
					}

					stNowSession->m_iWSASendCount = 0;  // 보낸 카운트 0으로 만듬.

					// 보낸게 잘 갔으면, 해당 유저는 접속을 끊는다.
					if (Flag == true)
					{
						// I/O 카운트 감소시켰는데 0이되면 shutdown날릴것도 없이 InDisconnect.
						if (InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
						{
							g_This->InDisconnect(stNowSession);
							continue;
						}

						// 0이 아니라면 서버측에서 접속 끊는다.
						shutdown(stNowSession->m_Client_sock, SD_BOTH);
						continue;
					}
				}

				// 3. 보내고 끊을 유저가 아닐 경우 로직
				else
				{
					// 직렬화 버퍼 해제
					int i = 0;
					while (i < stNowSession->m_iWSASendCount)
					{
						CProtocolBuff_Net::Free(stNowSession->m_PacketArray[i]);
						++i;
					}

					stNowSession->m_iWSASendCount = 0;  // 보낸 카운트 0으로 만듬.	
				}
												

				// 4. 샌드 가능 상태로 변경
				stNowSession->m_lSendFlag = FALSE;				

				// 5. 다시 샌드 시도
				g_This->SendPost(stNowSession);
			}

			// -----------------
			// I/O카운트 감소 및 삭제 처리
			// -----------------
			// I/O카운트 감소 후, 0이라면접속 종료
			if(InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
				g_This->InDisconnect(stNowSession);
		}
		return 0;
	}

	// Accept 스레드
	UINT WINAPI	CNetServer::AcceptThread(LPVOID lParam)
	{
		// --------------------------
		// Accept 파트
		// --------------------------
		SOCKET client_sock;
		SOCKADDR_IN	clientaddr;
		int addrlen = sizeof(clientaddr);

		CNetServer* g_This = (CNetServer*)lParam;

		// 유저가 접속할 때 마다 1씩 증가하는 고유한 키.
		ULONGLONG ullUniqueSessionID = 0;
		TCHAR tcTempIP[30];
		USHORT port;
		WORD iIndex;

		while (1)
		{
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			client_sock = accept(g_This->m_soListen_sock, (SOCKADDR*)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET)
			{
				int Error = WSAGetLastError();
				// 10004번(WSAEINTR) 에러면 정상종료. 함수호출이 중단되었다는 것.
				// 10038번(WSAEINTR) 은 소켓이 아닌 항목에 작업을 시도했다는 것. 이미 리슨소켓은 closesocket된것이니 에러 아님.
				if (Error == WSAEINTR || Error == WSAENOTSOCK)
				{
					// Accept 스레드 정상 종료
					break;
				}

				// 그게 아니라면 OnError 함수 호출
				// 윈도우 에러, 내 에러 보관
				g_This->m_iOSErrorCode = Error;
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, L"accpet(). Abonormal_exit : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT, tcErrorString);

				break;
			}
					
			InterlockedIncrement(&g_lAcceptTPS); // 테스트용!!

			// ------------------
			// 최대 접속자 수 이상 접속 불가
			// ------------------
			if (g_This->m_iMaxJoinUser <= g_This->m_ullJoinUserCount)
			{
				closesocket(client_sock);

				// 내 에러 보관
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__JOIN_USER_FULL;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, L"accpet(). User Full!!!! (%lld)",
					g_This->m_ullJoinUserCount);

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__JOIN_USER_FULL, tcErrorString);

				// continue
				continue;				
			}

			g_ullAcceptTotal++;	// 테스트용!!

			// ------------------
			// IP와 포트 알아오기.
			// ------------------
			InetNtop(AF_INET, &clientaddr.sin_addr, tcTempIP, 30);
			port = ntohs(clientaddr.sin_port);




			// ------------------
			// 접속 직후, IP등을 판단해서 무언가 추가 작업이 필요할 경우가 있을 수도 있으니 호출
			// ------------------		
			// false면 접속거부, 
			// true면 접속 계속 진행. true에서 할게 있으면 OnConnectionRequest함수 인자로 뭔가를 던진다.
			if (g_This->OnConnectionRequest(tcTempIP, port) == false)
				continue;




			// ------------------
			// 세션 구조체 생성 후 셋팅
			// ------------------
			// 1) 미사용 인덱스 알아오기
			iIndex = g_This->m_stEmptyIndexStack->Pop();	

			// 2) I/O 카운트 증가
			// 삭제 방어
			InterlockedIncrement(&g_This->m_stSessionArray[iIndex].m_lIOCount);

			// 3) 정보 셋팅하기
			// -- 세션 ID(믹스 키)와 인덱스 할당
			ULONGLONG MixKey = ((ullUniqueSessionID << 16) | iIndex);
			ullUniqueSessionID++;

			g_This->m_stSessionArray[iIndex].m_ullSessionID = MixKey;
			g_This->m_stSessionArray[iIndex].m_lIndex = iIndex;

			// -- LastPacket 초기화
			g_This->m_stSessionArray[iIndex].m_LastPacket = nullptr;
			
			// -- 소켓
			g_This->m_stSessionArray[iIndex].m_Client_sock = client_sock;						

			// -- IP와 Port
			StringCchCopy(g_This->m_stSessionArray[iIndex].m_IP, _MyCountof(g_This->m_stSessionArray[iIndex].m_IP), tcTempIP);
			g_This->m_stSessionArray[iIndex].m_prot = port;

			// -- SendFlag 초기화
			g_This->m_stSessionArray[iIndex].m_lSendFlag = FALSE;

			// 3) 해당 세션, 사용중으로 변경
			// 셋팅이 모두 끝났으면 릴리즈 해제 상태로 변경.
			g_This->m_stSessionArray[iIndex].m_lReleaseFlag = FALSE;


			// ------------------
			// IOCP 연결
			// ------------------
			// 소켓과 IOCP 연결
			if (CreateIoCompletionPort((HANDLE)client_sock, g_This->m_hIOCPHandle, (ULONG_PTR)&g_This->m_stSessionArray[iIndex], 0) == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				g_This->m_iOSErrorCode = WSAGetLastError();
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_IOCPCONNECT_FAIL;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, L"accpet(). IOCP_Connect Error : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);				

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_IOCPCONNECT_FAIL, tcErrorString);

				break;
			}

			// 접속자 수 증가. disconnect에서도 사용되는 변수이기 때문에 인터락 사용
			InterlockedIncrement(&g_This->m_ullJoinUserCount);



			// ------------------
			// 모든 접속절차가 완료되었으니 접속 후 처리 함수 호출.
			// ------------------
			g_This->OnClientJoin(MixKey);



			// ------------------
			// 비동기 입출력 시작
			// ------------------
			// 리시브 버퍼가 꽉찼으면 1리턴
			int ret = g_This->RecvPost(&g_This->m_stSessionArray[iIndex]);

			// 증가시켰던, I/O카운트 --. 0이라면 삭제처리
			if(InterlockedDecrement(&g_This->m_stSessionArray[iIndex].m_lIOCount) == 0)
				g_This->InDisconnect(&g_This->m_stSessionArray[iIndex]);		

			// I/O카운트 감소시켰는데 0이 아니라면, ret 체크.
			// ret가 1이라면 접속 끊는다.
			else if (ret == 1)
				shutdown(g_This->m_stSessionArray[iIndex].m_Client_sock, SD_BOTH);
		}

		return 0;
	}

	// SendPacket, Disconnect 등 외부에서 호출하는 함수에서, 락거는 함수.
	// 실제 락은 아니지만 락처럼 사용.
	//
	// Parameter : SessionID
	// return : 성공적으로 세션 찾았을 시, 해당 세션 포인터
	//		  : I/O카운트가 0이되어 삭제된 유저는, nullptr
	CNetServer::stSession* 	CNetServer::GetSessionLOCK(ULONGLONG SessionID)
	{
		// 1. SessionID로 세션 알아오기	
		stSession* retSession = &m_stSessionArray[(WORD)SessionID];

		// 2. I/O 카운트 1 증가.	
		if (InterlockedIncrement(&retSession->m_lIOCount) == 1)
		{
			// I/O 카운트가 1이라면 다시 --
			// 감소한 값이 0이면서, inDIsconnect 호출
			if(InterlockedDecrement(&retSession->m_lIOCount) == 0)
				InDisconnect(retSession);			

			return nullptr;
		}	

		// 3. Release Flag 체크
		if (retSession->m_lReleaseFlag == TRUE)
		{
			if (InterlockedDecrement(&retSession->m_lIOCount) == 0)
				InDisconnect(retSession);

			return nullptr;
		}

		// 4. 내가 원하던 세션이 맞는지 체크
		if (retSession->m_ullSessionID != SessionID)
		{
			// 아니라면 I/O 카운트 1 감소
			// 감소한 값이 0이면서, inDIsconnect 호출
			if (InterlockedDecrement(&retSession->m_lIOCount) == 0)
				InDisconnect(retSession);

			return nullptr;
		}								

		// 5. 정상적으로 있는 유저고, 안전하게 처리된 유저라면 해당 유저의 포인터 반환
		return retSession;
	}

	// SendPacket, Disconnect 등 외부에서 호출하는 함수에서, 락 해제하는 함수
	// 실제 락은 아니지만 락처럼 사용.
	//
	// parameter : 세션 포인터
	// return : 성공적으로 해제 시, true
	//		  : I/O카운트가 0이되어 삭제된 유저는, false
	bool 	CNetServer::GetSessionUnLOCK(stSession* NowSession)
	{
		// 1. I/O 카운트 1 감소
		if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
		{
			InDisconnect(NowSession);
			return false;
		}

		return true;
	}
	   

	// 각종 변수들을 리셋시킨다.
	// Stop() 함수에서 사용.
	void CNetServer::Reset()
	{
		m_iW_ThreadCount = 0;
		m_iA_ThreadCount = 0;
		m_hWorkerHandle = nullptr;
		m_hIOCPHandle = 0;
		m_soListen_sock = 0;
		m_iMaxJoinUser = 0;
		m_ullJoinUserCount = 0;
	}






	// ------------
	// Lib 내부에서만 사용하는 리시브 관련 함수들
	// ------------
	// RecvProc 함수. 큐의 내용 체크 후 PacketProc으로 넘긴다.
	void CNetServer::RecvProc(stSession* NowSession)
	{
		// -----------------
		// Recv 큐 관련 처리
		// -----------------

		while (1)
		{
			// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
			// 헤더의 구조 -----------> [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] 
			stProtocolHead Header;

			// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
			int UseSize = NowSession->m_RecvQueue.GetUseSize();
			if (UseSize < dfNETWORK_PACKET_HEADER_SIZE_NETSERVER)
			{
				break;
			}

			// 2. 헤더를 Peek으로 확인한다. 
			// 버퍼가 비어있는건 말이 안된다. 이미 위에서 있다고 검사했기 때문에. Crash 남김
			if (NowSession->m_RecvQueue.Peek((char*)&Header, dfNETWORK_PACKET_HEADER_SIZE_NETSERVER) == -1)
				cNetDump->Crash();

			// 3. 헤더의 코드 확인. 내 것이 맞는지
			if(Header.m_Code != m_bCode)
			{
				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__RECV_CODE_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_CodeError.UserID : %lld, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);				

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__RECV_CODE_ERROR, tcErrorString);

				// 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);					

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;
			}

			// 4. 헤더 안에 들어있는 Len(페이로드 길이)가 너무 크진 않은지
			WORD PayloadLen = Header.m_Len;
			if (PayloadLen > 512)
			{
				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__RECV_LENBIG_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_LenBig.UserID : %lld, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__RECV_LENBIG_ERROR, tcErrorString);

				// 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;

			}


			// 5. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size)
			// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.			
			if (UseSize < (dfNETWORK_PACKET_HEADER_SIZE_NETSERVER + PayloadLen))
				break;

			// 6. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
			NowSession->m_RecvQueue.RemoveData(dfNETWORK_PACKET_HEADER_SIZE_NETSERVER);

			// 7. 직렬화 버퍼의 rear는 무조건 5부터(앞에 5바이트는 헤더공간)부터 시작한다.
			// 때문에 clear()를 이용해 rear를 0으로 만들어둔다.
			CProtocolBuff_Net* PayloadBuff = CProtocolBuff_Net::Alloc();
			PayloadBuff->Clear();

			// 8. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
			int DequeueSize = NowSession->m_RecvQueue.Dequeue(PayloadBuff->GetBufferPtr(), PayloadLen);

			// 버퍼가 비어있거나, 내가 원하는만큼 데이터가 없었다면, 말이안됨. (위 if문에서는 있다고 했는데 여기오니 없다는것)
			// 로직문제로 보고 서버 종료.
			if ((DequeueSize == -1) || (DequeueSize != PayloadLen))		
				cNetDump->Crash();
			

			// 9. 읽어온 만큼 rear를 이동시킨다. 
			PayloadBuff->MoveWritePos(DequeueSize);

			// 10. 헤더 Decode
			if (PayloadBuff->Decode(PayloadLen, Header.m_RandXORCode, Header.m_Checksum, m_bXORCode_1, m_bXORCode_2) == false)
			{
				// 할당받은 패킷 Free
				CProtocolBuff_Net::Free(PayloadBuff);

				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__RECV_CHECKSUM_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_Empry.UserID : %lld, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__RECV_CHECKSUM_ERROR, tcErrorString);				

				// 끊어야하니 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);	

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;
			}

			// 11. Recv받은 데이터의 헤더 타입에 따라 분기처리.
			OnRecv(NowSession->m_ullSessionID, PayloadBuff);

			CProtocolBuff_Net::Free(PayloadBuff);
		}

		return;
	}

	// RecvPost 함수. 비동기 입출력 시작
	//
	// return 0 : 성공적으로 WSARecv() 완료
	// return 1 : RecvQ가 꽉찬 유저
	// return 2 : I/O 카운트가 0이되어 삭제된 유저
	int CNetServer::RecvPost(stSession* NowSession)
	{
		// ------------------
		// 비동기 입출력 시작
		// ------------------
		// 1. WSABUF 셋팅.
		WSABUF wsabuf[2];
		int wsabufCount = 0;

		int FreeSize = NowSession->m_RecvQueue.GetFreeSize();
		int Size = NowSession->m_RecvQueue.GetNotBrokenPutSize();

		// 리시브 링버퍼 크기 확인
		// 빈공간이 없으면 1리턴
		if (FreeSize <= 0)
			return 1;

		if (Size < FreeSize)
		{
			wsabuf[0].buf = NowSession->m_RecvQueue.GetRearBufferPtr();
			wsabuf[0].len = Size;

			wsabuf[1].buf = NowSession->m_RecvQueue.GetBufferPtr();
			wsabuf[1].len = FreeSize - Size;
			wsabufCount = 2;

		}
		else
		{
			wsabuf[0].buf = NowSession->m_RecvQueue.GetRearBufferPtr();
			wsabuf[0].len = Size;

			wsabufCount = 1;
		}

		// 2. Overlapped 구조체 초기화
		ZeroMemory(&NowSession->m_overRecvOverlapped, sizeof(NowSession->m_overRecvOverlapped));

		// 3. WSARecv()
		DWORD recvBytes = 0, flags = 0;
		InterlockedIncrement(&NowSession->m_lIOCount);

		// 4. 에러 처리
		if (WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_overRecvOverlapped, NULL) == SOCKET_ERROR)
		{
			int Error = WSAGetLastError();

			// 비동기 입출력이 시작된게 아니라면
			if (Error != WSA_IO_PENDING)
			{
				if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
				{
					InDisconnect(NowSession);
					return 2;
				}				

				// 에러가 버퍼 부족이라면, I/O카운트 차감이 끝이 아니라 끊어야한다.
				if (Error == WSAENOBUFS)
				{
					// 내 에러, 윈도우에러 보관
					m_iOSErrorCode = Error;
					m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

					// 에러 스트링 만들고
					TCHAR tcErrorString[300];
					StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %lld, [%s:%d]"),
						NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

					// 로그 찍기 (로그 레벨 : 에러)
					cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSARecv --> %s : NetError(%d), OSError(%d)",
						tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

					// 에러 함수 호출
					OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);

					// 끊어야하니 셧다운 호출
					shutdown(NowSession->m_Client_sock, SD_BOTH);		
				}
			}
		}

		return 0;
	}






	// ------------
	// Lib 내부에서만 사용하는 샌드 관련 함수들
	// ------------
	// 샌드 버퍼의 데이터 WSASend() 하기
	//
	// return 0 : 성공적으로 WSASend() 완료 or WSASend가 실패했지만 종료된 유저는 아님. or UseSize가 0이라 할게 없음.
	// return 1 : SendFlag가 TRUE(누가 이미 샌드중)임.
	// return 2 : I/O카운트가 0이되어서 종료된 유저
	int CNetServer::SendPost(stSession* NowSession)
	{
		while (1)
		{	
			// ------------------
			// send 가능 상태인지 체크
			// ------------------
			// 1. SendFlag(1번인자)가 를 TRUE(2번인자)로 변경.
			// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)이었다는 것.
			if (InterlockedExchange(&NowSession->m_lSendFlag, TRUE) == TRUE)
				return 1;
			
			// 2. SendBuff에 데이터가 있는지 확인
			// 여기서 구한 UseSize는 이제 스냅샷 이다. 
			int UseSize = NowSession->m_SendQueue->GetInNode();
			if (UseSize == 0)
			{
				// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
				NowSession->m_lSendFlag = FALSE;

				// 3. 진짜로 사이즈가 없는지 다시한번 체크. 락 풀고 왔는데, 컨텍스트 스위칭 일어나서 다른 스레드가 건드렸을 가능성
				// 사이즈 있으면 위로 올라가서 한번 더 시도
				if (NowSession->m_SendQueue->GetInNode() > 0)
					continue;

				break;
			}			


			// ------------------
			// Send 준비하기
			// ------------------
			// 1. WSABUF 셋팅.
			WSABUF wsabuf[dfSENDPOST_MAX_WSABUF];

			if (UseSize > dfSENDPOST_MAX_WSABUF)
				UseSize = dfSENDPOST_MAX_WSABUF;

			int i = 0;
			while (i < UseSize)
			{
				if (NowSession->m_SendQueue->Dequeue(NowSession->m_PacketArray[i]) == -1)
					cNetDump->Crash();	
				

				// WSABUF에 복사하기
				wsabuf[i].buf = NowSession->m_PacketArray[i]->GetBufferPtr();
				wsabuf[i].len = NowSession->m_PacketArray[i]->GetUseSize();

				i++;
			}			

			NowSession->m_iWSASendCount = UseSize;

			// 4. Overlapped 구조체 초기화
			ZeroMemory(&NowSession->m_overSendOverlapped, sizeof(NowSession->m_overSendOverlapped));

			// 5. WSASend()
			DWORD SendBytes = 0, flags = 0;
			InterlockedIncrement(&NowSession->m_lIOCount);

			// 6. 에러 처리
			if (WSASend(NowSession->m_Client_sock, wsabuf, UseSize, &SendBytes, flags, &NowSession->m_overSendOverlapped, NULL) == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();

				// 비동기 입출력이 시작된게 아니라면
				if (Error != WSA_IO_PENDING)
				{
					if(InterlockedDecrement(&NowSession->m_lIOCount) == 0)
					{
						InDisconnect(NowSession);
						return 2;
					}
					
					// 에러가 버퍼 부족이라면
					if (Error == WSAENOBUFS)
					{
						// 내 에러, 윈도우에러 보관
						m_iOSErrorCode = Error;
						m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

						// 에러 스트링 만들고
						TCHAR tcErrorString[300];
						StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %lld, [%s:%d]"),
							NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

						// 로그 찍기 (로그 레벨 : 에러)
						cNetLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d), OSError(%d)",
							tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);
						
						// 에러 함수 호출
						OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);

						// 끊는다.
						shutdown(NowSession->m_Client_sock, SD_BOTH);
					}
				}
			}
			break;
		}

		return 0;
	}

}
