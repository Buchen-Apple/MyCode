/*
락프리 적용된 LanServer
*/

#include "pch.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include <process.h>
#include <strsafe.h>

#include "NetworkLib_LanClinet.h"

#include "CrashDump\CrashDump.h"
#include "Log\Log.h"



namespace Library_Jingyu
{

#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 헤더 사이즈
#define dfNETWORK_PACKET_HEADER_SIZE	2


	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cLanClientLibLog = CSystemLog::GetInstance();

	// 덤프 남길 변수 하나 받기
	CCrashDump* cLanClientDump = CCrashDump::GetInstance();


	// -----------------------------
	// 유저가 호출 하는 함수
	// -----------------------------
	// 서버 시작
	// [서버 IP(커넥트 할 IP), 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)] 입력받음.
	//
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CLanClient::Start(const TCHAR* ConnectIP, USHORT port, int WorkerThreadCount, int ActiveWThreadCount, bool Nodelay)
	{
		// 새로 시작하니까 에러코드들 초기화
		m_iOSErrorCode = 0;
		m_iMyErrorCode = (euError)0;

		// 접속할 서버의 IP와 Port 복사해두기
		StringCchCopy(m_tcServerIP, _MyCountof(m_tcServerIP), ConnectIP);
		m_wPort = port;

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
			cLanClientLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WSAStartup() Error : NetError(%d), OSError(%d)",
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
			cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> CreateIoCompletionPort() Error : NetError(%d), OSError(%d)",
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
				cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WorkerThread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}	

		// 연결
		ConnectFunc();

		// 서버 오픈 로그 찍기		
		// 이건, 상속받는 쪽에서 찍는걸로 수정. 랜클라 자체는 독립적으로 작동하지 않음.
		// cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// 서버 스탑.
	void CLanClient::Stop()
	{	
		// 1. 클라 접속 종료 유도
		shutdown(m_stSession.m_Client_sock, SD_BOTH);

		// 종료되었는지 체크
		while (1)
		{
			if (m_ullJoinUserCount == 0)
				break;

			Sleep(1);
		}

		// 2. 워커 스레드 종료
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			PostQueuedCompletionStatus(m_hIOCPHandle, 0, 0, 0);

		// 워커스레드 종료 대기
		DWORD retval = WaitForMultipleObjects(m_iW_ThreadCount, m_hWorkerHandle, TRUE, INFINITE);

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
			cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Worker Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Worker Thread EXIT Error");
		}

		// 3. 각종 리소스 반환	

		// 1) 워커 스레드 핸들 반환
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			CloseHandle(m_hWorkerHandle[i]);

		// 3) 워커 스레드 배열 동적해제
		delete[] m_hWorkerHandle;

		// 4) IOCP핸들 반환
		CloseHandle(m_hIOCPHandle);

		// 5) 윈속 해제
		WSACleanup();

		// 4. 클라 접속중 아님 상태로 변경
		m_bClienetConnect = false;

		// 5. 각종 변수 초기화
		Reset();

		// 6. 서버 종료 로그 찍기		
		cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");
	}

	// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
	// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
	//
	// return true : SendQ에 성공적으로 데이터 넣고 SendPost까지 성공.
	// return false : SendQ에 데이터 넣기 실패 or 원하던 유저 못찾음
	void CLanClient::SendPacket(ULONGLONG ClinetID, CProtocolBuff_Lan* payloadBuff)
	{
		// 1. 세션 락 걸기(락 아니지만 락처럼 사용함)
		stSession* NowSession = GetSessionLOCK(ClinetID);
		if (NowSession == nullptr)
		{
			// 이 때는, 큐에도 넣지 못했기 때문에, 인자로 받은 직렬화버퍼를 Free한다.
			// ref 카운트가 0이 되면 메모리풀에 반환
			CProtocolBuff_Lan::Free(payloadBuff);
			return;
		}

		// 2. 헤더를 넣어서, 패킷 완성하기
		payloadBuff->SetProtocolBuff_HeaderSet();

		// 3. 인큐. 패킷의 "주소"를 인큐한다(8바이트)
		// 직렬화 버퍼 레퍼런스 카운트 1 증가
		payloadBuff->Add();
		NowSession->m_SendQueue->Enqueue(payloadBuff);

		// 4. 직렬화 버퍼 레퍼런스 카운트 1 감소. 0 되면 메모리풀에 반환
		CProtocolBuff_Lan::Free(payloadBuff);

		// 5. SendPost시도
		SendPost(NowSession);

		// 6. 세션 락 해제(락 아니지만 락처럼 사용)
		// 여기서 false가 리턴되면 이미 다른곳에서 삭제되었어야 했는데 이 SendPacket이 I/O카운트를 올림으로 인해 삭제되지 못한 유저였음.
		// 근데 따로 리턴값 받지 않고 있음
		GetSessionUnLOCK(NowSession);
	}

	///// 각종 게터함수들 /////
	// 윈도우 에러 얻기
	int CLanClient::WinGetLastError()
	{
		return m_iOSErrorCode;
	}

	// 내 에러 얻기
	int CLanClient::NetLibGetLastError()
	{
		return (int)m_iMyErrorCode;
	}

	// 현재 접속자 수 얻기
	ULONGLONG CLanClient::GetClientCount()
	{
		return m_ullJoinUserCount;
	}

	// 클라 접속중인지 확인
	// return true : 접속중
	// return false : 접속중 아님
	bool CLanClient::GetClinetState()
	{
		return m_bClienetConnect;
	}



	// -----------------------------
	// Lib 내부에서만 사용하는 함수
	// -----------------------------
	// 생성자
	CLanClient::CLanClient()
	{
		// 클라 미접속 상태로 시작 
		m_bClienetConnect = false;
	}

	// 소멸자
	CLanClient::~CLanClient()
	{
		// 클라가 아직 서버에 접속중이라면, 접속 해제 절차
		if (m_bClienetConnect == true)
		{
			m_bClienetConnect = false;
			Stop();
		}
	}

	// 실제로 접속종료 시키는 함수
	void CLanClient::InDisconnect(stSession* DeleteSession)
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
				CProtocolBuff_Lan::Free(DeleteSession->m_PacketArray[i]);

			// SendCount 초기화
			DeleteSession->m_iWSASendCount = 0;

			int UseSize = DeleteSession->m_SendQueue->GetInNode();

			CProtocolBuff_Lan* Payload;
			int i = 0;
			while (i < UseSize)
			{
				// 디큐 후, 직렬화 버퍼 메모리풀에 Free한다.
				if (DeleteSession->m_SendQueue->Dequeue(Payload) == -1)
					cLanClientDump->Crash();

				CProtocolBuff_Lan::Free(Payload);

				i++;
			}

			// 리시브 큐 초기화
			DeleteSession->m_RecvQueue.ClearBuffer();

			// socket을 INVALID_SOCKET으로 변경
			DeleteSession->m_Client_sock = INVALID_SOCKET;

			// SendFlag 초기화
			DeleteSession->m_lSendFlag = FALSE;

			// 클로즈 소켓
			closesocket(DeleteSession->m_Client_sock);

			// 접속 중 유저 수 감소
			InterlockedDecrement(&m_ullJoinUserCount);

			// 컨텐츠 쪽 연결이 종료되었다고 알려줌.
			// 컨텐츠와 통신할 때는 세션키를 이용해 통신한다. 그래서 인자로 세션키를 넘겨준다.
			OnDisconnect(sessionID);
		}

		return;
	}

	// Start에서 에러가 날 시 호출하는 함수.
	// 1. 입출력 완료포트 핸들 반환
	// 2. 워커스레드 종료, 핸들반환, 핸들 배열 동적해제
	// 3. 윈속 해제
	void CLanClient::ExitFunc(int w_ThreadCount)
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

		// 3. 윈속 해제
		// 윈속 초기화 하지 않은 상태에서 WSAClenup() 호출해도 아무 문제 없음
		WSACleanup();
	}

	// 워커스레드
	UINT WINAPI	CLanClient::WorkerThread(LPVOID lParam)
	{
		DWORD cbTransferred;
		stSession* stNowSession;
		OVERLAPPED* overlapped;

		CLanClient* g_This = (CLanClient*)lParam;

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
			// Recv 로직
			// -----------------
			// WSArecv()가 완료된 경우, 받은 데이터가 0이 아니면 로직 처리
			if (&stNowSession->m_overRecvOverlapped == overlapped && cbTransferred > 0)
			{
				// rear 이동
				stNowSession->m_RecvQueue.MoveWritePos(cbTransferred);

				// 1. 링버퍼 후, 패킷 처리
				g_This->RecvProc(stNowSession);

				// 2. 리시브 다시 걸기.
				g_This->RecvPost(stNowSession);
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
				int i = 0;
				while (i < stNowSession->m_iWSASendCount)
				{
					CProtocolBuff_Lan::Free(stNowSession->m_PacketArray[i]);
					i++;
				}

				stNowSession->m_iWSASendCount = 0;  // 보낸 카운트 0으로 만듬.						

				// 4. 샌드 가능 상태로 변경
				stNowSession->m_lSendFlag = FALSE;

				// 5. 다시 샌드 시도
				g_This->SendPost(stNowSession);
			}

			// -----------------
			// I/O카운트 감소 및 삭제 처리
			// -----------------
			// I/O카운트 감소 후, 0이라면접속 종료
			if (InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
			{
				g_This->InDisconnect(stNowSession);
				g_This->ConnectFunc();
			}

		}
		return 0;
	}


	// 지정한 서버에 connect하는 함수
	bool CLanClient::ConnectFunc()
	{
		// RecvPost를 하다가 실패할 경우, InDisconnect가 되기 때문에 while문으로 반복한다.
		while (1)
		{
			// ------------------
			// 세션 구조체 생성 및 셋팅
			// ------------------

			// 1) I/O 카운트 증가
			// 삭제 방어
			InterlockedIncrement(&m_stSession.m_lIOCount);

			// 2) 정보 셋팅하기
			m_stSession.m_ullSessionID = InterlockedIncrement(&m_ullUniqueSessionID);


			// ------------------
			// 소켓 생성 및 셋팅
			// ------------------
			m_stSession.m_Client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (m_stSession.m_Client_sock == INVALID_SOCKET)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = WSAGetLastError();
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"ConnectFunc() --> socket() Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;

			}

			// 소켓의 송신버퍼 크기를 0으로 변경. 그래야 정상적으로 비동기 입출력으로 실행
			//int optval = 0;
			//int retval = setsockopt(m_stSession.m_Client_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
			//if (optval == SOCKET_ERROR)
			//{
			//	// 윈도우 에러, 내 에러 보관
			//	m_iOSErrorCode = WSAGetLastError();
			//	m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

			//	// 각종 핸들 반환 및 동적해제 절차.
			//	ExitFunc(m_iW_ThreadCount);

			//	// 로그 찍기 (로그 레벨 : 에러)
			//	cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"ConnectFunc() --> setsockopt() SendBuff Size Change Error : NetError(%d), OSError(%d)",
			//		(int)m_iMyErrorCode, m_iOSErrorCode);

			//	// false 리턴
			//	return false;
			//}

			// 노딜레이 옵션 사용 여부에 따라 네이글 옵션 결정 (Start에서 전달받음)
			// true면 노딜레이 사용하겠다는 것(네이글 중지시켜야함)
			if (m_bNodelay == true)
			{
				BOOL optval = TRUE;
				int retval = setsockopt(m_soListen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));

				if (retval == SOCKET_ERROR)
				{
					// 윈도우 에러, 내 에러 보관
					m_iOSErrorCode = WSAGetLastError();
					m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

					// 각종 핸들 반환 및 동적해제 절차.
					ExitFunc(m_iW_ThreadCount);

					// 로그 찍기 (로그 레벨 : 에러)
					cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"ConnectFunc() --> setsockopt() Nodelay apply Error : NetError(%d), OSError(%d)",
						(int)m_iMyErrorCode, m_iOSErrorCode);

					// false 리턴
					return false;
				}
			}


			// ------------------
			// 논블락 connect
			// ------------------
			// 논블락 소켓으로 변경
			ULONG on = 1;
			DWORD check = ioctlsocket(m_stSession.m_Client_sock, FIONBIO, &on);
			if (check == SOCKET_ERROR)
			{
				printf("NoneBlock Change Fail...\n");
				return 0;
			}

			SOCKADDR_IN clientaddr;
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			clientaddr.sin_family = AF_INET;
			clientaddr.sin_port = htons(m_wPort);
			InetPton(AF_INET, m_tcServerIP, &clientaddr.sin_addr.S_un.S_addr);

			// connect 시도
			while (1)
			{
				connect(m_stSession.m_Client_sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));

				DWORD Check = WSAGetLastError();

				// 우드블럭이 뜬 경우
				if (Check == WSAEWOULDBLOCK)
				{
					// 쓰기셋, 예외셋, 셋팅
					FD_SET wset;
					FD_SET exset;
					wset.fd_count = 0;
					exset.fd_count = 0;

					wset.fd_array[wset.fd_count] = m_stSession.m_Client_sock;
					wset.fd_count++;

					exset.fd_array[exset.fd_count] = m_stSession.m_Client_sock;
					exset.fd_count++;

					// timeval 셋팅. 500m/s  대기
					TIMEVAL tval;
					tval.tv_sec = 0;
					tval.tv_usec = 500000;

					// Select()
					DWORD retval = select(0, 0, &wset, &exset, &tval);



					// 에러 발생여부 체크
					if (retval == SOCKET_ERROR)
					{
						printf("Select error..(%d)\n", WSAGetLastError());

					}

					// 타임아웃 체크
					else if (retval == 0)
					{
						printf("Select Timeout..\n");
						printf("%d\n", WSAGetLastError());

					}

					// 반응이 있다면, 예외셋에 반응이 왔는지 체크
					else if (exset.fd_count > 0)
					{
						//예외셋 반응이면 실패한 것.
						printf("Select ---> exset problem..\n");
					}

					// 쓰기셋에 반응이 왔는지 체크
					// 여기까지 오면 성공이지만 한번 더 체크해봄
					else if (wset.fd_count > 0)
					{
						// 다시 소켓을 블락으로 변경
						ULONG on = 0;
						DWORD check = ioctlsocket(m_stSession.m_Client_sock, FIONBIO, &on);
						if (check == SOCKET_ERROR)
						{
							printf("NoneBlock Change Fail...\n");
							return false;
						}

						// 정상적으로 잘 됐으니 break;
						break;

					}

					// 실패했으면 다시 접속
					Sleep(0);
					continue;
				}

				// 이미 연결이 되었다면 break;
				else if (Check == WSAEISCONN)
					break;
			}

			// ------------------
			// IOCP 연결
			// ------------------
			// 소켓과 IOCP 연결
			if (CreateIoCompletionPort((HANDLE)m_stSession.m_Client_sock, m_hIOCPHandle, (ULONG_PTR)&m_stSession, 0) == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = WSAGetLastError();
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT;

				// 로그 찍기 (로그 레벨 : 에러)
				cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"accpet(). Abonormal_exit : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// 에러 발생 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT, L"accpet(). Abonormal_exit");

				return false;
			}

			// ------------------
			// 마지막 절차
			// ------------------
			// 셋팅이 모두 끝났으면 릴리즈 해제 상태로 변경.
			m_stSession.m_lReleaseFlag = FALSE;

			// 서버에 접속한 유저 수 증가
			InterlockedIncrement(&m_ullJoinUserCount);

			// ------------------
			// 비동기 입출력 시작
			// ------------------
			// 반환값이 false라면, 이 안에서 종료된 유저임. 근데 안받음
			RecvPost(&m_stSession);

			// 증가시켰던, I/O카운트 --. 0이라면 삭제처리
			if (InterlockedDecrement(&m_stSession.m_lIOCount) == 0)
			{
				InDisconnect(&m_stSession);

				// 삭제 후, 다시 접속하기 위해 함수 상단으로 이동
				continue;
			}

			// I/O카운트가 0이되어 삭제된 것이 아니라면 정상 Connect 된 것
			// ConClientJoin 호출
			OnConnect(m_stSession.m_ullSessionID);

			break;
		}

		return true;		
	}

	// SendPacket, Disconnect 등 외부에서 호출하는 함수에서, 락거는 함수.
	// 실제 락은 아니지만 락처럼 사용.
	//
	// Parameter : SessionID
	// return : 성공적으로 세션 찾았을 시, 해당 세션 포인터
	//			실패 시 nullptr
	CLanClient::stSession* 	CLanClient::GetSessionLOCK(ULONGLONG SessionID)
	{
		// 1. SessionID로 세션 알아오기	
		//stSession* retSession = &m_stSessionArray[(WORD)SessionID];

		// 2. I/O 카운트 1 증가.	
		if (InterlockedIncrement(&m_stSession.m_lIOCount) == 1)
		{
			// 감소한 값이 0이면서, inDIsconnect 호출
			if (InterlockedDecrement(&m_stSession.m_lIOCount) == 0)
			{
				InDisconnect(&m_stSession);
				ConnectFunc();
			}

			return nullptr;
		}

		// 3. 내가 원하던 세션이 맞는지 체크
		if (m_stSession.m_ullSessionID != SessionID)
		{
			// 아니라면 I/O 카운트 1 감소
			// 감소한 값이 0이면서, inDIsconnect 호출
			if (InterlockedDecrement(&m_stSession.m_lIOCount) == 0)
			{
				InDisconnect(&m_stSession);
				ConnectFunc();
			}

			return nullptr;
		}

		// 4. Release Flag 체크
		if (m_stSession.m_lReleaseFlag == TRUE)
		{
			// 이미 Release된 유저라면, I/O 카운트 1 감소
			// 감소한 값이 0이면서, inDIsconnect 호출
			if (InterlockedDecrement(&m_stSession.m_lIOCount) == 0)
			{
				InDisconnect(&m_stSession);
				ConnectFunc();
			}

			return nullptr;
		}

		// 5. 정상적으로 있는 유저고, 안전하게 처리된 유저라면 해당 유저의 포인터 반환
		return &m_stSession;

	}

	// SendPacket, Disconnect 등 외부에서 호출하는 함수에서, 락 해제하는 함수
	// 실제 락은 아니지만 락처럼 사용.
	//
	// parameter : 세션 포인터
	// return : 성공적으로 해제 시, true
	//		  : I/O카운트가 0이되어 삭제된 유저는, false
	bool 	CLanClient::GetSessionUnLOCK(stSession* NowSession)
	{
		// 1. I/O 카운트 1 감소
		if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
		{
			// 감소 시 0이면 삭제로직
			InDisconnect(NowSession);
			ConnectFunc();
			return false;
		}

		return true;
	}


	// 각종 변수들을 리셋시킨다.
	// Stop() 함수에서 사용.
	void CLanClient::Reset()
	{
		m_iW_ThreadCount = 0;
		m_hWorkerHandle = nullptr;
		m_hIOCPHandle = 0;
		m_soListen_sock = 0;
		m_iMaxStackCount = 0;
		m_ullJoinUserCount = 0;
	}






	// ------------
	// Lib 내부에서만 사용하는 리시브 관련 함수들
	// ------------
	// RecvProc 함수. 큐의 내용 체크 후 PacketProc으로 넘긴다.
	void CLanClient::RecvProc(stSession* NowSession)
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

			// 2. 헤더를 Peek으로 확인한다. 
			// 버퍼가 비어있는건 말이 안된다. 이미 위에서 있다고 검사했기 때문에. Crash 남김
			if (NowSession->m_RecvQueue.Peek((char*)&Header_PaylaodSize, dfNETWORK_PACKET_HEADER_SIZE) == -1)
				cLanClientDump->Crash();


			// 3. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size)
			// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
			if (UseSize < (dfNETWORK_PACKET_HEADER_SIZE + Header_PaylaodSize))
				break;

			// 4. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
			NowSession->m_RecvQueue.RemoveData(dfNETWORK_PACKET_HEADER_SIZE);

			// 5. 직렬화 버퍼의 rear는 무조건 2부터(앞에 2바이트는 헤더공간)부터 시작한다.
			// 때문에 front를 2바이트 이동시켜놔야 Size가 0이된다.
			CProtocolBuff_Lan* PayloadBuff = CProtocolBuff_Lan::Alloc();
			PayloadBuff->Clear();

			// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
			int DequeueSize = NowSession->m_RecvQueue.Dequeue(PayloadBuff->GetBufferPtr(), Header_PaylaodSize);

			// 버퍼가 비어있거나, 내가 원하는만큼 데이터가 없었다면, 말이안됨. (위 if문에서는 있다고 했는데 여기오니 없다는것)
			// 로직문제로 보고 서버 종료.
			if ((DequeueSize == -1) || (DequeueSize != Header_PaylaodSize))
				cLanClientDump->Crash();

			// 7. 읽어온 만큼 rear를 이동시킨다. 
			// 참고로, rear는 시작부터 2이다
			PayloadBuff->MoveWritePos(DequeueSize);

			// 8. Recv받은 데이터의 헤더 타입에 따라 분기처리.
			OnRecv(NowSession->m_ullSessionID, PayloadBuff);

			CProtocolBuff_Lan::Free(PayloadBuff);
		}

		return;
	}

	// RecvPost 함수. 비동기 입출력 시작
	//
	// return true : 성공적으로 WSARecv() 완료 or WSARecv가 실패했지만 종료된 유저는 아님.
	// return false : I/O카운트가 0이되어서 종료된 유저
	bool CLanClient::RecvPost(stSession* NowSession)
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
					ConnectFunc();
					return false;
				}

				// 에러가 버퍼 부족이라면, I/O카운트 차감이 끝이 아니라 끊어야한다.
				if (Error == WSAENOBUFS)
				{
					// 내 에러, 윈도우에러 보관
					m_iOSErrorCode = Error;
					m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

					// 에러 스트링 만들고
					TCHAR tcErrorString[300];
					StringCchPrintf(tcErrorString, 300, _T("RecvPost --> WSANOBUFS"));

					// 로그 찍기 (로그 레벨 : 에러)
					cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSARecv --> %s : NetError(%d), OSError(%d)",
						tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

					// 끊어야하니 셧다운 호출
					shutdown(NowSession->m_Client_sock, SD_BOTH);

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
	// return true : 성공적으로 WSASend() 완료 or WSASend가 실패했지만 종료된 유저는 아님.
	// return false : I/O카운트가 0이되어서 종료된 유저
	bool CLanClient::SendPost(stSession* NowSession)
	{
		while (1)
		{
			// ------------------
			// send 가능 상태인지 체크
			// ------------------
			// 1. SendFlag(1번인자)가 0(3번인자)과 같다면, SendFlag(1번인자)를 1(2번인자)으로 변경
			// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)이었다는 것.
			//if (InterlockedCompareExchange(&NowSession->m_lSendFlag, TRUE, FALSE) == TRUE)
			//	return true;

			// 1. SendFlag(1번인자)가 를 TRUE(2번인자)로 변경.
			// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)이었다는 것.
			if (InterlockedExchange(&NowSession->m_lSendFlag, TRUE) == TRUE)
				break;

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
					cLanClientDump->Crash();

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
					// IOcount 하나 감소. I/O 카운트가 0이라면 접속 종료.
					if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
					{
						InDisconnect(NowSession);
						ConnectFunc();
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
						StringCchPrintf(tcErrorString, 300, _T("SendPost --> WSANOBUFS."));

						// 로그 찍기 (로그 레벨 : 에러)
						cLanClientLibLog->LogSave(L"LanClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d), OSError(%d)",
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
