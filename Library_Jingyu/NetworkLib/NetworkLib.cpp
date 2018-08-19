/*
락프리 적용된 Network Library
*/

#include "pch.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include <process.h>
#include <strsafe.h>

#include "NetworkLib.h"
#include "CrashDump\CrashDump.h"
#include "RingBuff\RingBuff.h"
#include "Log\Log.h"


LONG g_llPacketAllocCount = 0;


namespace Library_Jingyu
{

#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cNetLibLog = CSystemLog::GetInstance();

	// 덤프 남길 변수 하나 받기
	CCrashDump* cNetDump = CCrashDump::GetInstance();

	// 패킷 동적할당, 해제 카운트
	LONGLONG g_PacketAllocCount = 0;


	// 헤더 사이즈
#define dfNETWORK_PACKET_HEADER_SIZE	2

	// 한 번에 샌드할 수 있는 WSABUF의 카운트
#define dfSENDPOST_MAX_WSABUF			300








	// ------------------------------
	// enum과 구조체
	// ------------------------------
	// enum class
	enum class CLanServer::euError : int
	{
		NETWORK_LIB_ERROR__NORMAL = 0,					// 에러 없는 기본 상태
		NETWORK_LIB_ERROR__WINSTARTUP_FAIL,				// 윈속 초기화 하다가 에러남
		NETWORK_LIB_ERROR__CREATE_IOCP_PORT,			// IPCP 만들다가 에러남
		NETWORK_LIB_ERROR__W_THREAD_CREATE_FAIL,		// 워커스레드 만들다가 실패 
		NETWORK_LIB_ERROR__A_THREAD_CREATE_FAIL,		// 엑셉트 스레드 만들다가 실패 
		NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL,			// 소켓 생성 실패 
		NETWORK_LIB_ERROR__BINDING_FAIL,				// 바인딩 실패
		NETWORK_LIB_ERROR__LISTEN_FAIL,					// 리슨 실패
		NETWORK_LIB_ERROR__SOCKOPT_FAIL,				// 소켓 옵션 변경 실패
		NETWORK_LIB_ERROR__IOCP_ERROR,					// IOCP 자체 에러
		NETWORK_LIB_ERROR__NOT_FIND_CLINET,				// map 검색 등을 할때 클라이언트를 못찾은경우.
		NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL,		// Enqueue사이즈가 꽉찬 유저
		NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY,			// Dequeue 시, 큐가 비어있는 유저. Peek을 시도하는데 큐가 비었을 상황은 없음
		NETWORK_LIB_ERROR__WSASEND_FAIL,				// SendPost에서 WSASend 실패
		NETWORK_LIB_ERROR__WSAENOBUFS,					// WSASend, WSARecv시 버퍼사이즈 부족
		NETWORK_LIB_ERROR__EMPTY_RECV_BUFF,				// Recv 완료통지가 왔는데, 리시브 버퍼가 비어있다고 나오는 유저.
		NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT,		// 엑셉트 스레드 비정상 종료. 보통 accept()함수에서 이상한 에러가 나온것.
		NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT,		// 워커 스레드 비정상 종료. 
		NETWORK_LIB_ERROR__WFSO_ERROR,					// WaitForSingleObject 에러.
		NETWORK_LIB_ERROR__IOCP_IO_FAIL,				// IOCP에서 I/O 실패 에러. 이 때는, 일정 횟수는 I/O를 재시도한다.
		NETWORK_LIB_ERROR__JOIN_USER_FULL				// 유저가 풀이라서 더 이상 접속 못받음
	};

	// 세션 구조체
	struct CLanServer::stSession
	{
		SOCKET m_Client_sock;

		TCHAR m_IP[30];
		USHORT m_prot;

		ULONGLONG m_ullSessionID;

		LONG	m_lIOCount;

		// Send가능 상태인지 체크. 1이면 Send중, 0이면 Send중 아님
		LONG	m_lSendFlag;

		// 해당 인덱스 배열이 사용중인지 체크
		// 1이면 사용중, 0이면 사용중 아님
		LONG m_lUseFlag;

		// 해당 배열의 현재 인덱스
		ULONGLONG m_lIndex;

		// 현재, WSASend에 몇 개의 데이터를 샌드했는가. (바이트 아님! 카운트. 주의!)
		int m_iWSASendCount;

		CRingBuff m_RecvQueue;
		CRingBuff m_SendQueue;

		OVERLAPPED m_overRecvOverlapped;
		OVERLAPPED m_overSendOverlapped;

		// 생성자
		stSession()
		{
			m_lIOCount = 0;
			m_lSendFlag = 0;
			m_lIndex = 0;
			m_lUseFlag = FALSE;
			m_iWSASendCount = 0;
		}

		void Reset_Func()
		{
			// -- SendFlag, I/O카운트, SendCount 초기화
			m_lIOCount = 0;
			m_lSendFlag = 0;
			m_iWSASendCount = 0;

			// 큐 초기화
			m_RecvQueue.ClearBuffer();
			m_SendQueue.ClearBuffer();
		}

		// 소멸자
		// 필요없어짐
		//~stSession() {}


	};


	// -----------------------------
	// 유저가 호출 하는 함수
	// -----------------------------
	// 서버 시작
	// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 엑셉트 스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수] 입력받음.
	//
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CLanServer::Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect)
	{
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
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WSAStartup() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 입출력 완료 포트 생성
		m_hIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
		if (m_hIOCPHandle == NULL)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__CREATE_IOCP_PORT;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> CreateIoCompletionPort() Error : NetError(%d), OSError(%d)",
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
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WorkerThread Create Error : NetError(%d), OSError(%d)",
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
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> socket() Error : NetError(%d), OSError(%d)",
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
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> bind() Error : NetError(%d), OSError(%d)",
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
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> listen() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 연결된 리슨 소켓의 송신버퍼 크기를 0으로 변경. 그래야 정상적으로 비동기 입출력으로 실행
		// 리슨 소켓만 바꾸면 모든 클라 송신버퍼 크기는 0이된다.
		int optval = 0;
		retval = setsockopt(m_soListen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
		if (optval == SOCKET_ERROR)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> setsockopt() SendBuff Size Change Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

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
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> setsockopt() Nodelay apply Error : NetError(%d), OSError(%d)",
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
		m_stEmptyIndexStack = new CLF_Stack<ULONGLONG>();
		for (int i = 0; i < MaxConnect; ++i)
		{
			m_stEmptyIndexStack->Push(i);
		}

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
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Accept Thread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}



		// 서버 열렸음 !!
		m_bServerLife = true;

		// 서버 오픈 로그 찍기		
		cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// 서버 스탑.
	void CLanServer::Stop()
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
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Accept Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Accept Thread EXIT Error");
		}

		// 2. 모든 유저에게 Shutdown
		// 모든 유저에게 셧다운 날림
		for (int i = 0; i < m_iMaxJoinUser; ++i)
		{
			if (m_stSessionArray[i].m_lUseFlag == TRUE)
				shutdown(m_stSessionArray[i].m_Client_sock, SD_BOTH);
		}

		// 모든 유저가 종료되었는지 체크
		while (1)
		{
			if (m_ullJoinUserCount == 0)
				break;

			Sleep(1);
		}

		// 3. 워커 스레드 종료
		for (int i = 0; i<m_iW_ThreadCount; ++i)
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
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Worker Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Worker Thread EXIT Error");
		}

		// 4. 각종 리소스 반환
		// 1) 엑셉트 스레드 핸들 반환
		for (int i = 0; i<m_iA_ThreadCount; ++i)
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
		_tprintf_s(L"PacketCount : [%d]\n", g_llPacketAllocCount);
		cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerClose...");
	}

	// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
	// SendPacket은 그냥 아무때나 하면 된다.
	// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
	//
	// return true : SendQ에 성공적으로 데이터 넣음.
	// return false : SendQ에 데이터 넣기 실패 or 원하던 유저 못찾음
	bool CLanServer::SendPacket(ULONGLONG ClinetID, CProtocolBuff* payloadBuff)
	{
		// 1. ClinetID로 세션의 Index 알아오기
		ULONGLONG wArrayIndex = GetSessionIndex(ClinetID);

		// 2. 미사용 배열이면 뭔가 잘못된 것이니 false 리턴
		if (m_stSessionArray[wArrayIndex].m_lUseFlag == FALSE)
		{
			// 유저가 호출한 함수는, 에러 확인이 가능하기 때문에 OnError함수 호출 안함.
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__NOT_FIND_CLINET;

			// 로그 찍기 (로그 레벨 : 디버그)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_DEBUG, L"SendPacket() --> Not Fine Clinet :  NetError(%d)", (int)m_iMyErrorCode);

			return false;
		}

		// 3. 헤더를 넣어서, 패킷 완성하기
		SetProtocolBuff_HeaderSet(payloadBuff);

		// 4. 인큐. 패킷의 "주소"를 인큐한다(8바이트)
		// 직렬화 버퍼 레퍼런스 카운트 1 증가
		payloadBuff->Add();
		int EnqueueCheck = m_stSessionArray[wArrayIndex].m_SendQueue.Enqueue((char*)&payloadBuff, sizeof(void*));
		if (EnqueueCheck == -1)
		{
			// 유저가 호출한 함수는, 에러 확인이 가능하기 때문에 OnError함수 호출 안함.
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL;

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Send Queue Full Clinet : [%s : %d] NetError(%d)",
				m_stSessionArray[wArrayIndex].m_IP, m_stSessionArray[wArrayIndex].m_prot, (int)m_iMyErrorCode);

			// 직렬화 버퍼 레퍼런스 카운트 1 감소. 0 되면 삭제도 한다.
			CProtocolBuff::Free(payloadBuff);

			// 해당 유저는 접속을 끊는다.
			shutdown(m_stSessionArray[wArrayIndex].m_Client_sock, SD_BOTH);

			return false;
		}

		// 직렬화 버퍼 레퍼런스 카운트 1 감소. 0 되면 삭제도 한다.
		CProtocolBuff::Free(payloadBuff);

		// 4. SendPost시도
		bool Check = SendPost(&m_stSessionArray[wArrayIndex]);

		return Check;
	}

	// 지정한 유저를 끊을 때 호출하는 함수. 외부 에서 사용.
	// 라이브러리한테 끊어줘!라고 요청하는 것 뿐
	//
	// return true : 해당 유저에게 셧다운 잘 날림.
	// return false : 접속중이지 않은 유저를 접속해제하려고 함.
	bool CLanServer::Disconnect(ULONGLONG ClinetID)
	{
		// 1. ClinetID로 세션의 Index 알아오기
		ULONGLONG wArrayIndex = GetSessionIndex(ClinetID);

		// 2. 미사용 배열이면 뭔가 잘못된 것이니 false 리턴
		if (m_stSessionArray[wArrayIndex].m_lUseFlag == FALSE)
		{
			// 내 에러 남김. (윈도우 에러는 없음)
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__NOT_FIND_CLINET;

			// 로그 찍기 (로그 레벨 : 디버그)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_DEBUG, L"SendPacket() --> Not Fine Clinet : NetError(%d)", (int)m_iMyErrorCode);

			return false;
		}

		// 끊어야하는 유저는 셧다운 날린다.
		// 차후 자연스럽게 I/O카운트가 감소되어서 디스커넥트된다.
		shutdown(m_stSessionArray[wArrayIndex].m_Client_sock, SD_BOTH);

		return true;
	}

	///// 각종 게터함수들 /////
	// 윈도우 에러 얻기
	int CLanServer::WinGetLastError()
	{
		return m_iOSErrorCode;
	}

	// 내 에러 얻기
	int CLanServer::NetLibGetLastError()
	{
		return (int)m_iMyErrorCode;
	}

	// 현재 접속자 수 얻기
	ULONGLONG CLanServer::GetClientCount()
	{
		return m_ullJoinUserCount;
	}

	// 서버 가동상태 얻기
	// return true : 가동중
	// return false : 가동중 아님
	bool CLanServer::GetServerState()
	{
		return m_bServerLife;
	}






	// -----------------------------
	// Lib 내부에서만 사용하는 함수
	// -----------------------------
	// 생성자
	CLanServer::CLanServer()
	{
		// 로그 초기화
		cNetLibLog->SetDirectory(L"LanServer");
		cNetLibLog->SetLogLeve(CSystemLog::en_LogLevel::LEVEL_ERROR);

		// 서버 가동상태 false로 시작 
		m_bServerLife = false;

		// SRWLock 초기화
		InitializeSRWLock(&m_srwSession_stack_srwl);
	}

	// 소멸자
	CLanServer::~CLanServer()
	{
		// 서버가 가동중이었으면, 상태를 false 로 바꾸고, 서버 종료절차 진행
		if (m_bServerLife == true)
		{
			m_bServerLife = false;
			Stop();
		}
	}

	// 실제로 접속종료 시키는 함수
	void CLanServer::InDisconnect(stSession* DeleteSession)
	{

		DeleteSession->m_lUseFlag = FALSE;

		ULONGLONG sessionID = DeleteSession->m_ullSessionID;		

		// 샌드 큐에 데이터가 있으면 동적해제 한다.
		int UseSize = DeleteSession->m_SendQueue.GetUseSize();

		while (UseSize > 0)
		{
			int TempSize;

			if (UseSize > 8000)
				int TempSize = 8000;
			else
				TempSize = UseSize;

			// UseSize 사이즈 만큼 디큐
			CProtocolBuff* Payload[1000];
			int DequeueSize = DeleteSession->m_SendQueue.Dequeue((char*)Payload, TempSize);

			// 꺼낸 수 만큼 UseSize 줄임. 다음 절차를 위해.
			UseSize -= DequeueSize;

			DequeueSize = DequeueSize / 8;

			// 꺼낸 Dequeue만큼 돌면서 삭제한다.
			for (int i = 0; i < DequeueSize; ++i)
			{
				CProtocolBuff::Free(Payload[i]);
				//delete Payload[i];
				InterlockedDecrement(&g_llPacketAllocCount);
			}
		}

		// 클로즈 소켓
		closesocket(DeleteSession->m_Client_sock);		

		// 미사용 인덱스 스택에 반납
		m_stEmptyIndexStack->Push(DeleteSession->m_lIndex);


		// 유저 수 감소
		InterlockedDecrement(&m_ullJoinUserCount);

		OnClientLeave(sessionID);
		return;
	}

	// Start에서 에러가 날 시 호출하는 함수.
	// 1. 입출력 완료포트 핸들 반환
	// 2. 워커스레드 종료, 핸들반환, 핸들 배열 동적해제
	// 3. 엑셉트 스레드 종료, 핸들 반환
	// 4. 리슨소켓 닫기
	// 5. 윈속 해제
	void CLanServer::ExitFunc(int w_ThreadCount)
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
	UINT WINAPI	CLanServer::WorkerThread(LPVOID lParam)
	{
		DWORD cbTransferred;
		stSession* stNowSession;
		OVERLAPPED* overlapped;

		CLanServer* g_This = (CLanServer*)lParam;

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

			// GQCS 깨어날 시 함수호출
			g_This->OnWorkerThreadBegin();

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

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"IOCP_Error --> GQCS return Error : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);


				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__IOCP_ERROR, L"IOCP_Error");

				break;
			}

			// -----------------
			// Recv 로직
			// -----------------
			// WSArecv()가 완료된 경우, 받은 데이터가 0이 아니면 로직 처리
			if (&stNowSession->m_overRecvOverlapped == overlapped && cbTransferred > 0)
			{
				// rear 이동
				stNowSession->m_RecvQueue.MoveWritePos(cbTransferred);

				// 1. 링버퍼 후, 패킷 처리
				g_This->RecvProc(stNowSession);

				// 2. 리시브 다시 걸기. false가 리턴되면 종료된 유저이니 다시 위로 올라간다.
				if (g_This->RecvPost(stNowSession) == false)
					continue;
			}

			// -----------------
			// Send 로직
			// -----------------
			// WSAsend()가 완료된 경우, 받은 데이터가 0이 아니면 로직처리
			else if (&stNowSession->m_overSendOverlapped == overlapped && cbTransferred > 0)
			{
				// 1. 샌드 완료됐다고 컨텐츠에 알려줌
				g_This->OnSend(stNowSession->m_ullSessionID, cbTransferred);

				// 2. 보냈던 직렬화버퍼 삭제
				CProtocolBuff* TempBuff[dfSENDPOST_MAX_WSABUF];
				int DequeueSize = stNowSession->m_SendQueue.Dequeue((char*)TempBuff, stNowSession->m_iWSASendCount * 8);

				// 꺼낸 Dequeue만큼 돌면서 삭제한다.
				for (int i = 0; i < DequeueSize / 8; ++i)
				{
					CProtocolBuff::Free(TempBuff[i]);
					// delete TempBuff[i];
					InterlockedDecrement(&g_llPacketAllocCount);
				}

				stNowSession->m_iWSASendCount = 0;  // 보낸 카운트 0으로 만듬.		
				

				// 4. 샌드 가능 상태로 변경
				InterlockedExchange(&stNowSession->m_lSendFlag, FALSE);

				// 5. 다시 샌드 시도. false가 리턴되면 종료된 유저이니 다시 위로 올라간다.
				if (g_This->SendPost(stNowSession) == false)
					continue;
			}

			// -----------------
			// I/O카운트 감소 및 삭제 처리
			// -----------------
			// I/O카운트 감소 후, 0이라면접속 종료
			if (InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
				g_This->InDisconnect(stNowSession);

		}
		return 0;
	}

	// Accept 스레드
	UINT WINAPI	CLanServer::AcceptThread(LPVOID lParam)
	{
		// --------------------------
		// Accept 파트
		// --------------------------
		SOCKET client_sock;
		SOCKADDR_IN	clientaddr;
		int addrlen;

		CLanServer* g_This = (CLanServer*)lParam;

		// 유저가 접속할 때 마다 1씩 증가하는 고유한 키.
		ULONGLONG ullUniqueSessionID = 0;

		while (1)
		{
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			addrlen = sizeof(clientaddr);
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

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"accpet(). Abonormal_exit : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT, L"accpet(). Abonormal_exit");

				break;
			}

			// ------------------
			// 최대 접속자 수 이상 접속 불가
			// ------------------
			if (g_This->m_iMaxJoinUser <= g_This->m_ullJoinUserCount)
			{
				closesocket(client_sock);

				// 로그 찍기(로그 레벨 : 디버그)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_DEBUG, L"accpet(). User Full!!!!");
				continue;
			}



			// ------------------
			// IP와 포트 알아오기.
			// ------------------
			TCHAR tcTempIP[30];
			InetNtop(AF_INET, &clientaddr.sin_addr, tcTempIP, 30);
			USHORT port = ntohs(clientaddr.sin_port);




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
			ULONGLONG iIndex = g_This->m_stEmptyIndexStack->Pop();		

			 // 2) 해당 세션 배열, 사용중으로 변경
			g_This->m_stSessionArray[iIndex].m_lUseFlag = TRUE;

			// 3) 초기화하기
			// -- 소켓
			g_This->m_stSessionArray[iIndex].m_Client_sock = client_sock;

			// -- 세션키(믹스 키)와 인덱스
			ULONGLONG MixKey = InterlockedIncrement(&ullUniqueSessionID) | (iIndex << 48);
			g_This->m_stSessionArray[iIndex].m_ullSessionID = MixKey;
			g_This->m_stSessionArray[iIndex].m_lIndex = iIndex;

			// -- IP와 Port
			StringCchCopy(g_This->m_stSessionArray[iIndex].m_IP, _MyCountof(g_This->m_stSessionArray[iIndex].m_IP), tcTempIP);
			g_This->m_stSessionArray[iIndex].m_prot = port;

			// -- SendFlag, I/O카운트, 큐, Send했던 카운트 초기화
			g_This->m_stSessionArray[iIndex].Reset_Func();



			// ------------------
			// IOCP 연결
			// ------------------
			// 소켓과 IOCP 연결
			if (CreateIoCompletionPort((HANDLE)client_sock, g_This->m_hIOCPHandle, (ULONG_PTR)&g_This->m_stSessionArray[iIndex], 0) == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				g_This->m_iOSErrorCode = WSAGetLastError();
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT;

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"accpet(). Abonormal_exit : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT, L"accpet(). Abonormal_exit");

				break;
			}

			// 접속자 수 증가. disconnect에서도 사용되는 변수이기 때문에 인터락 사용
			InterlockedIncrement(&g_This->m_ullJoinUserCount);



			// ------------------
			// 모든 접속절차가 완료되었으니 접속 후 처리 함수 호출.
			// ------------------
			g_This->m_stSessionArray[iIndex].m_lIOCount++; // I/O카운트 ++. 들어가기전에 1에서 시작. 아직 recv,send 그 어떤것도 안걸었기 때문에 그냥 ++해도 안전!
			g_This->OnClientJoin(MixKey);



			// ------------------
			// 비동기 입출력 시작
			// ------------------
			// 반환값이 false라면, 이 안에서 종료된 유저임. 근데 안받음
			g_This->RecvPost(&g_This->m_stSessionArray[iIndex]);

			// I/O카운트 --. 0이라면 삭제처리
			if (InterlockedDecrement(&g_This->m_stSessionArray[iIndex].m_lIOCount) == 0)
				g_This->InDisconnect(&g_This->m_stSessionArray[iIndex]);

		}

		return 0;
	}



	// 조합된 키를 입력받으면, Index 리턴하는 함수
	WORD CLanServer::GetSessionIndex(ULONGLONG MixKey)
	{
		return MixKey >> 48;
	}

	// 조합된 키를 입력받으면, 진짜 세션키를 리턴하는 함수.
	ULONGLONG CLanServer::GetRealSessionKey(ULONGLONG MixKey)
	{
		return MixKey & 0x0000ffffffffffff;
	}

	// CProtocolBuff에 헤더 넣는 함수
	void CLanServer::SetProtocolBuff_HeaderSet(CProtocolBuff* Packet)
	{
		WORD wHeader = Packet->GetUseSize() - dfNETWORK_PACKET_HEADER_SIZE;
		memcpy_s(&Packet->GetBufferPtr()[0], dfNETWORK_PACKET_HEADER_SIZE, &wHeader, dfNETWORK_PACKET_HEADER_SIZE);
	}



	// 각종 변수들을 리셋시킨다.
	void CLanServer::Reset()
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
	void CLanServer::RecvProc(stSession* NowSession)
	{
		// -----------------
		// Recv 큐 관련 처리
		// -----------------

		while (1)
		{
			// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
			WORD Header_PaylaodSize;

			// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
			int UseSize = NowSession->m_RecvQueue.GetUseSize();
			if (UseSize < dfNETWORK_PACKET_HEADER_SIZE)
			{
				break;
			}

			// 2. 헤더를 Peek으로 확인한다.  Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 
			// 버퍼가 비어있으면 접속 끊음.
			if (NowSession->m_RecvQueue.Peek((char*)&Header_PaylaodSize, dfNETWORK_PACKET_HEADER_SIZE) == -1)
			{
				// 일단 끊어야하니 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_Empry.UserID : %d, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s, NetError(%d)",
					tcErrorString, (int)m_iMyErrorCode);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF, tcErrorString);

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;
			}

			// 3. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size)
			// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
			if (UseSize < (dfNETWORK_PACKET_HEADER_SIZE + Header_PaylaodSize))
			{
				break;
			}

			// 4. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
			NowSession->m_RecvQueue.RemoveData(dfNETWORK_PACKET_HEADER_SIZE);

			// 5. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
			CProtocolBuff PayloadBuff;
			PayloadBuff.MoveReadPos(dfNETWORK_PACKET_HEADER_SIZE);

			int DequeueSize = NowSession->m_RecvQueue.Dequeue(&PayloadBuff.GetBufferPtr()[dfNETWORK_PACKET_HEADER_SIZE], Header_PaylaodSize);
			// 버퍼가 비어있으면 접속 끊음
			if (DequeueSize == -1)
			{
				// 일단 끊어야하니 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_Empry.UserID : %d, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF, tcErrorString);

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;
			}
			PayloadBuff.MoveWritePos(DequeueSize);

			// 9. 헤더에 들어있는 타입에 따라 분기처리.
			OnRecv(NowSession->m_ullSessionID, &PayloadBuff);

		}

		return;
	}

	// RecvPost 함수. 비동기 입출력 시작
	//
	// return true : 성공적으로 WSARecv() 완료 or 어쨋든 종료된 유저는 아님
	// return false : I/O카운트가 0이되어서 종료된 유저임
	bool CLanServer::RecvPost(stSession* NowSession)
	{
		// ------------------
		// 비동기 입출력 시작
		// ------------------
		// 1. WSABUF 셋팅.
		WSABUF wsabuf[2];
		int wsabufCount = 0;

		int FreeSize = NowSession->m_RecvQueue.GetFreeSize();
		int Size = NowSession->m_RecvQueue.GetNotBrokenPutSize();

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
				// I/O카운트 1감소.I/O 카운트가 0이라면 접속 종료.
				if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
				{
					InDisconnect(NowSession);
					return false;
				}

				// 에러가 버퍼 부족이라면, I/O카운트 차감이 끝이 아니라 끊어야한다.
				if (Error == WSAENOBUFS)
				{
					// 일단 끊어야하니 셧다운 호출
					shutdown(NowSession->m_Client_sock, SD_BOTH);

					// 내 에러, 윈도우에러 보관
					m_iOSErrorCode = Error;
					m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

					// 에러 스트링 만들고
					TCHAR tcErrorString[300];
					StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %d, [%s:%d]"),
						NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

					// 로그 찍기 (로그 레벨 : 에러)
					cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSARecv --> %s : NetError(%d), OSError(%d)",
						tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

					// 에러 함수 호출
					OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);
				}
			}
		}

		return true;
	}






	// ------------
	// Lib 내부에서만 사용하는 샌드 관련 함수들
	// ------------
	// 샌드 버퍼의 데이터 WSASend() 하기
	//
	// return true : 성공적으로 WSASend() 완료 or 어쨋든 종료된 유저는 아님
	// return false : I/O카운트가 0이되어서 종료된 유저임
	bool CLanServer::SendPost(stSession* NowSession)
	{
		while (1)
		{
			// ------------------
			// send 가능 상태인지 체크
			// ------------------
			// 1. SendFlag(1번인자)가 0(3번인자)과 같다면, SendFlag(1번인자)를 1(2번인자)으로 변경
			// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)이었다는 것.
			if (InterlockedCompareExchange(&NowSession->m_lSendFlag, TRUE, FALSE) == TRUE)
				return true;

			// 2. SendBuff에 데이터가 있는지 확인
			// 여기서 구한 UseSize는 이제 스냅샷 이다. 
			int UseSize = NowSession->m_SendQueue.GetUseSize();
			if (UseSize == 0)
			{
				// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
				NowSession->m_lSendFlag = 0;

				// 3. 진짜로 사이즈가 없는지 다시한번 체크. 락 풀고 왔는데, 컨텍스트 스위칭 일어나서 다른 스레드가 건드렸을 가능성
				// 사이즈 있으면 위로 올라가서 한번 더 시도
				if (NowSession->m_SendQueue.GetUseSize() > 0)
					continue;

				break;
			}

			// ------------------
			// Send 준비하기
			// ------------------
			// 1. WSABUF 셋팅.
			WSABUF wsabuf[dfSENDPOST_MAX_WSABUF];			

			// 2. 한 번에 100개의 포인터(총 800바이트)를 꺼내도록 시도 (픽 구조)	
			if (UseSize > dfSENDPOST_MAX_WSABUF * 8)
				UseSize = dfSENDPOST_MAX_WSABUF * 8;

			// 아래 로직은 혹시나 해서 넣었지만 절대 들어오지 않음.
			else if (UseSize > 8 && UseSize % 8 != 0)
				UseSize = (UseSize / 8) * 8;

			CProtocolBuff* TempBuff[dfSENDPOST_MAX_WSABUF];
			int wsabufByte = (NowSession->m_SendQueue.Peek((char*)TempBuff, UseSize));
			if (wsabufByte == -1)
			{
				// 큐가 텅 비어있음. 위에서 있다고 왔는데 여기서 없는것은 진짜 말도안되는 에러!
				// 내 에러보관
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("QUEUE_PEEK_EMPTY. UserID : %d, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d)",
					tcErrorString, (int)m_iMyErrorCode);

				// 끊는다.
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// 샌드 Flag 0으로 변경 (샌드 가능상태)
				InterlockedExchange(&NowSession->m_lSendFlag, FALSE);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY, tcErrorString);

				return false;
			}

			int Temp = wsabufByte / 8;

			// 4. 실제로 픽 한 포인트 수(바이트 아님! 주의)만큼 돌면서 WSABUF구조체에 할당
			for (int i = 0; i < Temp; i++)
			{
				wsabuf[i].buf = TempBuff[i]->GetBufferPtr();
				wsabuf[i].len = TempBuff[i]->GetUseSize();
			}

			 NowSession->m_iWSASendCount = Temp;

			// 4. Overlapped 구조체 초기화
			ZeroMemory(&NowSession->m_overSendOverlapped, sizeof(NowSession->m_overSendOverlapped));

			// 5. WSASend()
			DWORD SendBytes = 0, flags = 0;
			InterlockedIncrement(&NowSession->m_lIOCount);

			// 6. 에러 처리
			if (WSASend(NowSession->m_Client_sock, wsabuf, Temp, &SendBytes, flags, &NowSession->m_overSendOverlapped, NULL) == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();

				// 비동기 입출력이 시작된게 아니라면
				if (Error != WSA_IO_PENDING)
				{
					// IOcount 하나 감소. I/O 카운트가 0이라면 접속 종료.
					if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
					{
						InDisconnect(NowSession);
						return false;
					}

					// 에러가 버퍼 부족이라면
					if (Error == WSAENOBUFS)
					{
						// 내 에러, 윈도우에러 보관
						m_iOSErrorCode = Error;
						m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

						// 에러 스트링 만들고
						TCHAR tcErrorString[300];
						StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %d, [%s:%d]"),
							NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

						// 로그 찍기 (로그 레벨 : 에러)
						cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d), OSError(%d)",
							tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

						// 끊는다.
						shutdown(NowSession->m_Client_sock, SD_BOTH);

						// 에러 함수 호출
						OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);
					}
				}
			}
			break;
		}

		return true;
	}

}
