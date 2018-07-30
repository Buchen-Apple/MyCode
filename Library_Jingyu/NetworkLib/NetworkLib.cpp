#include "stdafx.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include <process.h>
#include <Windows.h>
#include <strsafe.h>

#include "NetworkLib.h"
#include "RingBuff\RingBuff.h"
#include "Log\Log.h"



namespace Library_Jingyu
{
	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cNetLibLog = CSystemLog::GetInstance();	


	// 헤더 사이즈
	#define dfNETWORK_PACKET_HEADER_SIZE	2


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
		NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL	,		// Enqueue사이즈가 꽉찬 유저
		NETWORK_LIB_ERROR__WSASEND_FAIL,				// SendPost에서 WSASend 실패
		NETWORK_LIB_ERROR__WSAENOBUFS,					// WSASend, WSARecv시 버퍼사이즈 부족
		NETWORK_LIB_ERROR__EMPTY_RECV_BUFF,				// Recv 완료통지가 왔는데, 리시브 버퍼가 비어있다고 나오는 유저.
		NETWORK_LIB_ERROR__A_THREAD_ABONORMAL_EXIT,		// 엑셉트 스레드 비정상 종료. 보통 accept()함수에서 이상한 에러가 나온것.
		NETWORK_LIB_ERROR__W_THREAD_ABONORMAL_EXIT,		// 워커 스레드 비정상 종료. 
		NETWORK_LIB_ERROR__WFSO_ERROR,					// WaitForSingleObject 에러.
		NETWORK_LIB_ERROR__IOCP_IO_FAIL					// IOCP에서 I/O 실패 에러. 이 때는, 일정 횟수는 I/O를 재시도한다.
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
		
		// I/O가 외부적인 요인(물리적인 연결 에러라던가..)으로 실패했을 때 약 5회정도 재시도 해본다. 
		// 이 시도한 횟수를 저장할 변수. 실패할 때 마다 1씩 증가하다가, 5가되면 더 이상 접속시도 안함
		// I/O가 성공하면 그 즉시 0이된다.
		LONG	m_lReyryCount;

		// 내 구조체 동기화 객체.
		SRWLOCK m_srwStruct_srwl;
		
		CRingBuff m_RecvQueue;
		CRingBuff m_SendQueue;

		OVERLAPPED m_overRecvOverlapped;
		OVERLAPPED m_overSendOverlapped;

		// 생성자
		stSession()
		{
			InitializeSRWLock(&m_srwStruct_srwl);
			m_lIOCount = 0;
			m_lSendFlag = 0;
			m_lReyryCount = 0;
		}

	private:
		// 락 걸기
		void LockSession_Func()
		{
			AcquireSRWLockExclusive(&m_srwStruct_srwl);
		}

		// 락 풀기
		void UnlockSession_Func()
		{
			ReleaseSRWLockExclusive(&m_srwStruct_srwl);
		}


	#define	Struct_Lock()	LockSession_Func()
	#define Struct_Unlock()	UnlockSession_Func()
	
	};




	// -----------------------------
	// 유저가 호출 하는 함수
	// -----------------------------
	// 서버 시작
	// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수] 입력받음.
	//
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CLanServer::Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, bool Nodelay, int MaxConnect)
	{			

		// 각종 변수 초기화 함수
		Init();

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


		// 입출력 완료포트 생성
		// 입출력 완료 포트 생성
		m_hIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
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
		if (listen(m_soListen_sock, SOMAXCONN) == SOCKET_ERROR)
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
		if (setsockopt(m_soListen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval)) == SOCKET_ERROR)
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
			if (setsockopt(m_soListen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval)) == SOCKET_ERROR)
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

		// 링거 건다.
		/*LINGER Loptval;
		Loptval.l_onoff = 1;
		Loptval.l_linger = 0;
		setsockopt(m_soListen_sock, SOL_SOCKET, SO_LINGER, (char*)&Loptval, sizeof(Loptval));*/

		// 엑셉트 스레드 생성
		m_hAcceptHandle = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
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

		// 엑셉트 스레드 수를 1로 변경. 필요 없을수도 있지만 혹시 모르니 카운트
		m_iA_ThreadCount = 1;

		// 서버 열렸음 !!
		m_bServerLife = true;		

		// 서버 오픈 로그 찍기		
		cNetLibLog->LogSave(L"System", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// 서버 스탑.
	void CLanServer::Stop()
	{
		// 1. Accept 스레드 종료. 더 이상 접속을 받으면 안되니 Accept스레드 먼저 종료
		// Accept 스레드는 리슨소켓을 closesocket하면 된다.
		closesocket(m_soListen_sock);
		DWORD retval = WaitForSingleObject(m_hAcceptHandle, INFINITE);
		if (retval != WAIT_OBJECT_0)
		{
			m_iOSErrorCode = GetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;
			
			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Accept Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Accept Thread EXIT Error");
		}

		// 2. 모든 유저에게 Shutdown
		// 모든 유저에게 셧다운 날림
		Lock_Map();

		map<ULONGLONG, stSession*>::iterator itor = map_Session.begin();
		for (; itor != map_Session.end(); ++itor)
			shutdown(itor->second->m_Client_sock, SD_BOTH);		

		Unlock_Map();

		// 모든 유저가 종료되었는지 체크
		while (1)
		{
			if (m_ullJoinUserCount == 0)
				break;

			Sleep(2);
		}

		// 3. 워커 스레드 종료
		for (int i = 0; i<m_iW_ThreadCount; ++i)
			PostQueuedCompletionStatus(m_hIOCPHandle, 0, 0, 0);

		// 워커스레드 종료 대기
		retval = WaitForMultipleObjects(m_iW_ThreadCount, m_hWorkerHandle, TRUE, INFINITE);

		// 리턴값이 [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iW_ThreadCount - 1] 가 아니라면, 뭔가 에러가 발생한 것. 에러 찍고 걍 리턴한다.
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
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__W_THREAD_ABONORMAL_EXIT;

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Worker Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Worker Thread EXIT Error");
		}

		// 4. 각종 리소스 반환
		// 1) 엑셉트 스레드 핸들 반환
		CloseHandle(m_hAcceptHandle);

		// 2) 워커 스레드 핸들 반환
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			CloseHandle(m_hWorkerHandle[i]);

		// 3) 워커 스레드 배열 동적해제
		delete[] m_hWorkerHandle;

		// 4) IOCP핸들 반환
		CloseHandle(m_hIOCPHandle);

		// 5) 윈속 해제
		WSACleanup();

		// 5. 서버 가동중 아님 상태로 변경
		m_bServerLife = false;
	}

	// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
	// SendPacket은 그냥 아무때나 하면 된다.
	// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
	//
	// return true : SendQ에 성공적으로 데이터 넣음.
	// return true : SendQ에 데이터 넣기 실패.
	bool CLanServer::SendPacket(ULONGLONG ClinetID, CProtocolBuff* payloadBuff)
	{
		// 1. ClinetID로 세션구조체 알아오기
		stSession* NowSession = FineSessionPtr(ClinetID);
		if (NowSession == nullptr)
		{
			// 유저가 호출한 함수는, 에러 확인이 가능하기 때문에 OnError함수 호출 안함.
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__NOT_FIND_CLINET;

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Not Fine Clinet : [%s : %d] NetError(%d)",
				NowSession->m_IP, NowSession->m_prot, (int)m_iMyErrorCode);

			return false;
		}
	
		// 2. 헤더 만들기 (페이로드 사이즈가 들어간다)
		WORD Header = payloadBuff->GetUseSize();
	
		// 3. 넣기 (헤더, 페이로드를 2번 인큐하는거..어떻게 안될까)
		NowSession->m_SendQueue.EnterLOCK();  // 락 ---------------------------	

		// 헤더 인큐
		int EnqueueCheck = NowSession->m_SendQueue.Enqueue((char*)&Header, dfNETWORK_PACKET_HEADER_SIZE);
		if (EnqueueCheck == -1)
		{
			NowSession->m_SendQueue.LeaveLOCK();  // 락 해제 ---------------------------	

			// 해당 유저는 접속을 끊는다.
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			// 유저가 호출한 함수는, 에러 확인이 가능하기 때문에 OnError함수 호출 안함.
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL;

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Send Queue Full Clinet : [%s : %d] NetError(%d)",
				NowSession->m_IP, NowSession->m_prot, (int)m_iMyErrorCode);
			
			return false;
		}

		// 페이로드 인큐
		EnqueueCheck = NowSession->m_SendQueue.Enqueue(payloadBuff->GetBufferPtr(), Header);
		if (EnqueueCheck == -1)
		{
			NowSession->m_SendQueue.LeaveLOCK();  // 락 해제 ---------------------------	

			 // 해당 유저는 접속을 끊는다.
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			// 유저가 호출한 함수는, 에러 확인이 가능하기 때문에 OnError함수 호출 안함.
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL;	

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Send Queue Full Clinet : [%s : %d] NetError(%d)",
				NowSession->m_IP, NowSession->m_prot, (int)m_iMyErrorCode);

			return false;
		}

		NowSession->m_SendQueue.LeaveLOCK();  // 락 해제 ---------------------------	

		// 4. SendPost시도
		SendPost(NowSession);

		return true;
	}

	// 지정한 유저를 끊을 때 호출하는 함수. 외부 에서 사용.
	// 라이브러리한테 끊어줘!라고 요청하는 것 뿐
	//
	// return true : 해당 유저에게 셧다운 잘 날림.
	// return false : 접속중이지 않은 유저를 접속해제하려고 함.
	bool CLanServer::Disconnect(ULONGLONG ClinetID)
	{
		// 유저 찾는다.
		stSession* NowSession = FineSessionPtr(ClinetID);

		// 유저 못찾았으면 에러코드 남기고 false 리턴
		if (NowSession == nullptr)
		{
			// 내 에러 남김. (윈도우 에러는 없음)
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__NOT_FIND_CLINET;

			// 로그 찍기 (로그 레벨 : 에러)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Not Fine Clinet : [%s : %d] NetError(%d)",
				NowSession->m_IP, NowSession->m_prot, (int)m_iMyErrorCode);

			return false;
		}

		// 끊어야하는 유저는 셧다운 날린다.
		// 차후 자연스럽게 I/O카운트가 감소되어서 디스커넥트된다.
		shutdown(NowSession->m_Client_sock, SD_BOTH);

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

		InitializeSRWLock(&m_srwSession_map_srwl);
	}

	// 소멸자
	CLanServer::~CLanServer()
	{
		// 서버 가동상태 false로 변경
		m_bServerLife = false;

		// 접속 중인 유저가 있으면 Stop함수 호출.
		if (m_ullJoinUserCount != 0)
		{
			Stop();
		}
	}

	// 실제로 접속종료 시키는 함수
	void CLanServer::InDisconnect(stSession* DeleteSession)
	{
		ULONGLONG sessionID = DeleteSession->m_ullSessionID;
	
		// map에서 제외시키기
		Lock_Map();
		size_t retval = map_Session.erase(sessionID);
		Unlock_Map();

		// 만약, 없는 유저라면 그냥 삭제된걸로 치고, 리턴한다.
		if (retval == 0)
		{		
			// 유저 함수 호출
			OnClientLeave(sessionID);			
			return;
		}	

		// 클로즈 소켓, 세션 동적해제	
		closesocket(DeleteSession->m_Client_sock);
		delete DeleteSession;

		// 유저 수 감소
		InterlockedDecrement(&m_ullJoinUserCount);

		// 유저 함수 호출
		OnClientLeave(sessionID);
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
		int retval;

		DWORD cbTransferred;
		stSession* NowSession;
		OVERLAPPED* overlapped;

		CLanServer* g_This = (CLanServer*)lParam;

		while (1)
		{
			// 변수들 초기화
			cbTransferred = 0;
			NowSession = nullptr;
			overlapped = nullptr;

			// GQCS 완료 시 함수 호출
			g_This->OnWorkerThreadEnd();

			// 비동기 입출력 완료 대기
			// GQCS 대기
			retval = GetQueuedCompletionStatus(g_This->m_hIOCPHandle, &cbTransferred, (PULONG_PTR)&NowSession, &overlapped, INFINITE);

			// GQCS 깨어날 시 함수호출
			g_This->OnWorkerThreadBegin();

			// --------------
			// 완료 체크
			// --------------
			// overlapped가 nullptr이라면, IOCP 에러임.
			if (overlapped == NULL)
			{
				// 셋 다 0이면 스레드 종료.
				if (cbTransferred == 0 && NowSession == NULL)
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

				// break 안한다. 해당 유저는 자연스럽게 I/O가 줄어들어서 종료.
			}

			/*
			 overlapped가 nullptr이 아니면서 리턴값이 false면, I/O에 대한 에러일 수 있으니 GetLastError 하고 일정 횟수 I/O 재요청. (물리적인 연결 실패 등으로 인한 에러?)
			 근데.. 이거 의미없는거같다. 걍 체크 안하는게 맞는거같기도 하고.
			 접속 -> 끊고 -> 다시 접속을 반복하다 보니까 접속이 끊긴 클라에 대해 ERROR_NETNAME_DELETED(해당 네트워크를 더 이상 사용할수 없음)이게 계속 뜨는데,
			 일단 위 에러는 제외한다고 쳐도, 이미 연결 끊긴 유저에게 재접속 기회를 주는게 맞나 모르겠다. 물리적으로 끊겼으면 그냥 끊긴걸로 처리?
			 클라에게 알려주는 용도로(에러표시용)으로 남겨둬야할까? 고민중...
			*/
			//else if(retval == false && GetLastError() != ERROR_NETNAME_DELETED)
			//{
			//	// 윈도우 에러, 내 에러 보관
			//	g_This->m_iOSErrorCode = GetLastError();
			//	g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__IOCP_IO_FAIL;

			//	// 실패에 대해 재전송 시도
			//	// m_lReyryCount가 5가 아닐 때만 시도한다.
			//	if (NowSession->m_lReyryCount != 5)
			//	{
			//		InterlockedIncrement(&NowSession->m_lReyryCount);

			//		// Recv가 실패했을 경우, 리시브를 다시 걸어본다.
			//		if (&NowSession->m_overRecvOverlapped == overlapped)
			//			g_This->RecvPost(NowSession);

			//		// Send가 실패했을 경우, 샌드를 다시 걸어본다.
			//		else if (&NowSession->m_overSendOverlapped == overlapped)
			//			g_This->SendPost(NowSession);
			//	}

			//  // 로그 찍기 (로그 레벨 : 에러)
			//  cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"IOCP_IO_Fail --> GQCS return Error : [%s : %d] NetError(%d), OSError(%d)",
			//	NowSession->m_IP, NowSession->m_prot, (int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

			//	// 에러 발생 함수 호출
			//	g_This->OnError((int)euError::NETWORK_LIB_ERROR__IOCP_IO_FAIL, L"IOCP_IO_Fail");				
			//}

			// -----------------
			// Recv 로직
			// -----------------
			// WSArecv()가 완료된 경우, 받은 데이터가 0이 아니면 로직 처리
			if (&NowSession->m_overRecvOverlapped == overlapped && cbTransferred > 0)
			{
				// m_lReyryCount값을 0으로 변경
				//InterlockedExchange(&NowSession->m_lReyryCount, 0);

				// rear 이동
				NowSession->m_RecvQueue.MoveWritePos(cbTransferred);

				// 1. 링버퍼 후, 패킷 처리
				g_This->RecvProc(NowSession);

				// 2. 리시브 다시 걸기
				g_This->RecvPost(NowSession);
			}

			// -----------------
			// Send 로직
			// -----------------
			// WSAsend()가 완료된 경우, 받은 데이터가 0이 아니면 로직처리
			else if (&NowSession->m_overSendOverlapped == overlapped && cbTransferred > 0)
			{
				// m_lReyryCount값을 0으로 변경
				//InterlockedExchange(&NowSession->m_lReyryCount, 0);

				// 1. front 이동	
				NowSession->m_SendQueue.EnterLOCK();		// 락 -----------------------

				NowSession->m_SendQueue.RemoveData(cbTransferred);

				NowSession->m_SendQueue.LeaveLOCK();		// 락 해제 ------------------

				// 2. 샌드 가능 상태로 변경
				NowSession->m_lSendFlag = 0;

				// 3. 다시 샌드 시도
				g_This->SendPost(NowSession);

				// 4. 샌드 완료됐다고 컨텐츠에 알려줌
				g_This->OnSend(NowSession->m_ullSessionID, cbTransferred);
			}

			// -----------------
			// I/O카운트 감소 및 삭 제 처리
			// -----------------
			// I/O카운트 감소 후, 0이라면접속 종료
			long NowVal = InterlockedDecrement(&NowSession->m_lIOCount);
			if (NowVal == 0)
				g_This->InDisconnect(NowSession);
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

		while (true)
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
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_ABONORMAL_EXIT;

				// 로그 찍기 (로그 레벨 : 에러)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"accpet(). Abonormal_exit : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABONORMAL_EXIT, L"accpet(). Abonormal_exit");

				break;
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
			bool Check = g_This->OnConnectionRequest(tcTempIP, port);
		
			// false면 접속거부, 
			// true면 접속 계속 진행. true에서 할게 있으면 OnConnectionRequest함수 인자로 뭔가를 던진다.
			if (Check == false)
				continue;


			// ------------------
			// 세션 구조체 생성 후 셋팅
			// ------------------
			stSession* NewSession = new stSession;
			StringCchCopy(NewSession->m_IP, _MyCountof(NewSession->m_IP), tcTempIP);
			NewSession->m_prot = port;
			NewSession->m_Client_sock = client_sock;
			NewSession->m_ullSessionID = g_This->m_ullSessionID++;
						

			// ------------------
			// IOCP 연결
			// ------------------
			// 소켓과 IOCP 연결
			CreateIoCompletionPort((HANDLE)client_sock, g_This->m_hIOCPHandle, (ULONG_PTR)NewSession, 0);
					

			// ------------------
			// 비동기 입출력 시작
			// ------------------
			// 반환값이 false라면, 리시브 걸다가 실패한 것 (즉, 정상적으로 접속되지 않은 유저) 이기 때문에
			// 1. NewSession 동적해제
			// 2. 맵에 추가하지 않음.
			// 3. 여기서 다시 Accept하러 돌아감.
			if (g_This->RecvPost_Accept(NewSession) == false)
			{
				delete NewSession;
				continue;
			}


			// ------------------
			// 여기까지 오면 정상적으로 접속된 유저
			// map 등록 후, 접속자 수 추가
			// ------------------
			// 셋팅된 구조체를 map에 등록
			g_This->Lock_Map();
			g_This->map_Session.insert(pair<ULONGLONG, stSession*>(NewSession->m_ullSessionID, NewSession));
			g_This->Unlock_Map();

			// 접속자 수 증가. disconnect에서도 사용되는 변수이기 때문에 인터락 사용
			InterlockedIncrement(&g_This->m_ullJoinUserCount);


			// ------------------
			// 모든 접속절차가 완료되었으니 접속 후 처리 함수 호출.
			// ------------------
			g_This->OnClientJoin(NewSession->m_ullSessionID);
		}

		return 0;
	}

	// map에 락 걸기
	void CLanServer::LockMap_Func()
	{
		AcquireSRWLockExclusive(&m_srwSession_map_srwl);
	}

	// map에 락 풀기
	void CLanServer::UnlockMap_Func()
	{
		ReleaseSRWLockExclusive(&m_srwSession_map_srwl);
	}

	// ClinetID로 Session구조체 찾기 함수
	CLanServer::stSession* CLanServer::FineSessionPtr(ULONGLONG ClinetID)
	{
		// 락 ------------------
		Lock_Map();
		map <ULONGLONG, stSession*>::iterator iter;

		iter = map_Session.find(ClinetID);
		if (iter == map_Session.end())
		{
			// 락 끝 ------------------
			Unlock_Map();
			return nullptr;
		}

		stSession* NowSession = iter->second;

		Unlock_Map();
		// 락 끝 ------------------

		return NowSession;
	}

	// 각종 변수들을 초기값으로 초기화
	void CLanServer::Init()
	{	
		m_iW_ThreadCount = 0;
		m_iA_ThreadCount = 0;
		m_hWorkerHandle = nullptr;
		m_hIOCPHandle = 0;
		m_soListen_sock = 0;
		m_iOSErrorCode = 0;
		m_iMyErrorCode = (euError)0;
		m_ullSessionID = 0;
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
			WORD Header_PaylaodSize = 0;

			// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
			int UseSize = NowSession->m_RecvQueue.GetUseSize();
			if (UseSize < dfNETWORK_PACKET_HEADER_SIZE)
			{
				break;
			}

			// 2. 헤더를 Peek으로 확인한다.  Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 
			// 버퍼가 비어있으면 접속 끊음.
			int PeekSize = NowSession->m_RecvQueue.Peek((char*)&Header_PaylaodSize, dfNETWORK_PACKET_HEADER_SIZE);
			if (PeekSize == -1)
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

			int DequeueSize = NowSession->m_RecvQueue.Dequeue(PayloadBuff.GetBufferPtr(), Header_PaylaodSize);
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

	// Accept용 RecvProc함수
	// return true : 성공적으로 WSARecv() 완료
	// return false : WSARecv()에서 WSA_IO_PENDING 외의 에러 발생
	bool CLanServer::RecvPost_Accept(stSession* NowSession)
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
		int retval = WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_overRecvOverlapped, NULL);


		// 4. 에러 처리
		if (retval == SOCKET_ERROR)
		{
			int Error = WSAGetLastError();

			// 비동기 입출력이 시작된게 아니라면
			if (Error != WSA_IO_PENDING)
			{
				// I/O카운트 1감소.
				long Nowval = InterlockedDecrement(&NowSession->m_lIOCount);

				// I/O 카운트가 0이라면 접속 종료.
				if (Nowval == 0)
					InDisconnect(NowSession);

				// 버퍼 부족이라면, I/O카운트 차감이 끝이 아니라 끊어야한다.
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
					cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSARecv--> %s,  NetError(%d), OSError(%d)",
						tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

					// 에러 함수 호출
					OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);
				}

				return false;
			}
		}

		return true;
	}

	// RecvPost 함수. 비동기 입출력 시작
	void CLanServer::RecvPost(stSession* NowSession)
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
		int retval = WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_overRecvOverlapped, NULL);

		// 4. 에러 처리
		if (retval == SOCKET_ERROR)
		{
			int Error = WSAGetLastError();

			// 비동기 입출력이 시작된게 아니라면
			if (Error != WSA_IO_PENDING)
			{
				InterlockedDecrement(&NowSession->m_lIOCount);

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
	}






	// ------------
	// Lib 내부에서만 사용하는 샌드 관련 함수들
	// ------------
	// 샌드 버퍼의 데이터 WSASend() 하기
	void CLanServer::SendPost(stSession* NowSession)
	{
		while (1)
		{
			// ------------------
			// send 가능 상태인지 체크
			// ------------------
			// 1. SendFlag(1번인자)가 0(3번인자)과 같다면, SendFlag(1번인자)를 1(2번인자)으로 변경
			// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)이었다는 것.
			if (InterlockedCompareExchange(&NowSession->m_lSendFlag, TRUE, FALSE) == TRUE)
			{
				// 이땐 샌드중이니 그냥 리턴
				return;
			}

			// 2. SendBuff에 데이터가 있는지 확인
			// 여기서 구한 UseSize는 이제 스냅샷 이다. 아래에서 버퍼 셋팅할때도 사용한다.
			int UseSize = NowSession->m_SendQueue.GetUseSize();
			if (UseSize == 0)
			{
				// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
				NowSession->m_lSendFlag = 0;

				// 3. 진짜로 사이즈가 없는지 다시한번 체크. 락 풀고 왔는데, 컨텍스트 스위칭 일어나서 다른 스레드가 건드렸을 가능성
				// 사이즈 있으면 위로 올라가서 한번 더 시도
				if (NowSession->m_SendQueue.GetUseSize() > 0)
				{
					continue;
				}

				break;
			}

			// ------------------
			// Send 준비하기
			// ------------------
			// 1. WSABUF 셋팅.
			WSABUF wsabuf[2];
			int wsabufCount = 0;

			// 2. BrokenSize 구하기
			// !!!!여기 주의!!! 
			// 만약 BrokenSize를, UseSize보다 먼저 구해오면 흐름이 깨진다!!! 
			// 
			// ------------
			// 시나리오...
			// [Front = 0, rear = 10]인 상황에 BrokenSize를 구하면 10이 나온다. (BrokenSize = 10)
			// 그 사이에 컨텍스트 스위칭이 일어나서 다른 스레드가 데이터를 넣는다. (Fornt = 0, rear = 20)
			// 다시 컨텍스트 스위칭으로 해당 스레드로 돌아온 후 UseSize를 구하면 20이 나온다 (Front = 0, rear = 20, BrokenSize = 10, UseSize = 20)
			// ------------
			// 
			// 이 상황에서, BrokenSize와 UseSize를 비교하면, 끊긴 상황이 아닌데 버퍼를 2개 사용하게된다.
			// 이 경우, [0]번 버퍼에 &Buff[0]이 들어가고 사이즈가 10, [1]번 버퍼에도 &Buff[0]이 들어가고 사이즈가 10이 들어가는 상황이 발생한다.
			// 동일한 데이터가 가게 된다!! 
			// 이 경우, 상대쪽에서는 보내지 않은 데이터?라서 에러를 보낼수도 있고.. 혹은 그냥 뭐가 이상하게 처리되서 서버로 샌드를 안할수도 있고 그런다.
			// 주의하자!!!!!

			int BrokenSize = NowSession->m_SendQueue.GetNotBrokenGetSize();

			// 3. UseSize가 더 크다면, 버퍼가 끊겼다는 소리. 2개를 끊어서 보낸다.
			if (BrokenSize <  UseSize)
			{
				// fornt 위치의 버퍼를 포인터로 던짐(나는 1칸 앞에 쓰기 때문에, 이 안에서 1칸 앞까지 계산해줌)
				wsabuf[0].buf = NowSession->m_SendQueue.GetFrontBufferPtr();
				wsabuf[0].len = BrokenSize;

				wsabuf[1].buf = NowSession->m_SendQueue.GetBufferPtr();;
				wsabuf[1].len = UseSize - BrokenSize;
				wsabufCount = 2;
			}

			// 3-2. 그게 아니라면, WSABUF를 1개만 셋팅한다.
			else
			{
				// fornt 위치의 버퍼를 포인터로 던짐(나는 1칸 앞에 쓰기 때문에, 이 안에서 1칸 앞까지 계산해줌)
				wsabuf[0].buf = NowSession->m_SendQueue.GetFrontBufferPtr();
				wsabuf[0].len = BrokenSize;

				wsabufCount = 1;
			}

			// 4. Overlapped 구조체 초기화
			ZeroMemory(&NowSession->m_overSendOverlapped, sizeof(NowSession->m_overSendOverlapped));

			// 5. WSASend()
			DWORD SendBytes = 0, flags = 0;
			InterlockedIncrement(&NowSession->m_lIOCount);
			int retval = WSASend(NowSession->m_Client_sock, wsabuf, wsabufCount, &SendBytes, flags, &NowSession->m_overSendOverlapped, NULL);

			// 6. 에러 처리
			if (retval == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();

				// 비동기 입출력이 시작된게 아니라면
				if (Error != WSA_IO_PENDING)
				{
					// IOcount 하나 감소
					InterlockedDecrement(&NowSession->m_lIOCount);

					// 에러가 버퍼 부족이라면
					if (Error == WSAENOBUFS)
					{
						InterlockedDecrement(&NowSession->m_lIOCount);

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
						cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d), OSError(%d)",
							tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

						// 에러 함수 호출
						OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);
					}

					// 버퍼만 체크
					//else if (Error != WSAESHUTDOWN && Error != WSAECONNRESET && Error != WSAECONNABORTED && Error != WSAEMFILE)
					//{
					//	// 일단 끊어야하니 셧다운 호출
					//	shutdown(NowSession->m_Client_sock, SD_BOTH);

					//	// 내 에러, 윈도우에러 보관
					//	m_iOSErrorCode = Error;
					//	m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSASEND_FAIL;

					//	// 에러 스트링 만들고
					//	TCHAR tcErrorString[300];
					//	StringCchPrintf(tcErrorString, 300, _T("WSASendFail... UserID : %d, [%s:%d]"),
					//		NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

					//	// 에러 함수 호출
					//	OnError((int)euError::NETWORK_LIB_ERROR__WSASEND_FAIL, tcErrorString);
					//}		
				}
			}
			break;
		}
	}

}


