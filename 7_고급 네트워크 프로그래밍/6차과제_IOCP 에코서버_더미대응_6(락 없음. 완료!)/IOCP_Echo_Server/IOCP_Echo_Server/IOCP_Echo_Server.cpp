// IOCP_Echo_Server.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <process.h>
#include <clocale>

#include "NetworkFunc.h"

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#include <conio.h>

// 입출력 완료포트 변수
HANDLE	g_hcp;

int AcceptCount;

long IOCountMinusCount;
long JoinUser;
long SessionDeleteCount;
int ThreadCount;

int		g_LogLevel;			// 로그 출력 레벨.
TCHAR	g_szLogBuff[1024];	//  로그 출력용 임시 버퍼

// 워커 스레드
UINT	WINAPI	WorkerThread(LPVOID lParam);

int _tmain()
{
	system("mode con: cols=100 lines=20");

	_tsetlocale(LC_ALL, L"korean");

	timeBeginPeriod(1);

	InitializeSRWLock(&g_Session_map_srwl);

	_tprintf(L"스레드 수 입력 : ");
	_tscanf_s(L"%d", &ThreadCount);

	// 로그 레벨 입력받기
	fputs("LogLevel (0:Debuf / 1:Warning / 2:Error) : ", stdout);
	scanf_s("%d", &g_LogLevel);

	// 윈속 초기화
	WSADATA	wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 입출력 완료 포트 생성
	g_hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (g_hcp == NULL)
		return 1;


	// 작업자 스레드 생성
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	HANDLE* hThread;
	hThread = new HANDLE[ThreadCount];

	for (int i = 0; i < ThreadCount; ++i)
	{
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, 0, 0, NULL);
		if (hThread[i] == NULL)
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] _beginthreadex() 에러\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			return 1;
		}
	}

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == INVALID_SOCKET)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] listen_sock() 에러\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

		return 1;
	}

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVER_PORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] bind() 에러\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

		return 1;
	}

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] listen() 에러\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

		return 1;
	}

	// 논블락 소켓으로 변경
	ULONG on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] ioctlsocket() NoneBlock Change Fail...\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

		return 1;
	}

	// 연결된 리슨 소켓의 송신버퍼 크기를 0으로 변경. 그래야 정상적으로 비동기 입출력으로 실행
	// 리슨 소켓만 바꾸면 모든 클라 송신버퍼 크기는 0이된다.
	int optval = 0;
	retval = setsockopt(listen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] setsockopt() 에러\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

		return 1;
	}

	// 서버 열렸음
	SYSTEMTIME lst;
	GetLocalTime(&lst);

	_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] Server Open... Port : %d\n",
		lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, SERVER_PORT);


	// --------------------------
	// Accept 파트
	// --------------------------
	SOCKET client_sock;
	SOCKADDR_IN	clientaddr;
	int addrlen;

	LONGLONG StartTime = GetTickCount64();;


	while (true)
	{
		if (GetTickCount64() - StartTime >= 1000)
		{
			StartTime = GetTickCount64();	
			_tprintf(L"총 접속수 : %d, 마이너스카운트 : %d, 현재접속수 : %d, 접속해제수 : %d, 스레드 수 : %d\n", AcceptCount, IOCountMinusCount, JoinUser, SessionDeleteCount, ThreadCount);
		}


		// -------------
		// 서버 종료 체크
		// -------------
		if (_kbhit())
		{
			int Key = _getch();

			if (Key == 'q' || Key == 'Q')
			{
				_tprintf(L"Q키 누름. 서버 종료\n");

				// 모든 유저에게 셧다운 날림
				LockSession();

				map<SOCKET, stSession*>::iterator itor = map_Session.begin();
				for (; itor != map_Session.end(); ++itor)
					shutdown(itor->second->m_Client_sock, SD_BOTH);		

				UnlockSession();

				break;
			}
		}

		
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			int Error = WSAGetLastError();

			// 우드블럭 에러가 떴다면, 다시 처음으로 돌아간다.
			if (Error == WSAEWOULDBLOCK)
				continue;
				
			_tprintf(L"accept() 에러 %d\n", WSAGetLastError());
			break;
		}

		InterlockedIncrement(&JoinUser);					

		// ------------------
		// 세션 구조체 생성 후 셋팅
		// ------------------
		stSession* NewSession = new stSession;
		InetNtop(AF_INET, &clientaddr.sin_addr, NewSession->m_IP, 30);
		NewSession->m_prot = ntohs(clientaddr.sin_port);
		NewSession->m_Client_sock = client_sock;

		// ------------------
		// map 등록 후 IOCP 연결
		// ------------------
		// 셋팅된 구조체를 map에 등록
		LockSession();
		map_Session.insert(pair<SOCKET, stSession*>(client_sock, NewSession));
		UnlockSession();
				

		// 소켓과 IOCP 연결
		CreateIoCompletionPort((HANDLE)client_sock, g_hcp, (ULONG_PTR)NewSession, 0);


		// -----------
		// 접속 내용 화면에 출력
		// -----------
		//_tprintf(L"[TCP 서버] 클라이언트 접속 : IP 주소=%s, 포트=%d\n", IP, NewSession->m_prot);


		// ------------------
		// 비동기 입출력 시작
		// ------------------
		//RecvPost(NewSession);
		RecvPost_Accept(NewSession);
		AcceptCount++;		
		
	}


	// ---------------
	// 스레드 안전 종료 체크
	// ---------------
	// 모든 유저가 종료되었는지 체크
	while (1)
	{
		if (map_Session.size() == 0)
			break;		

		Sleep(1);
	}

	GetLocalTime(&lst);
	_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d]모든 유저 종료\n",
		lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);


	// 종료되었으면 스레드도 종료
	for(int i=0; i<ThreadCount; ++i)
		PostQueuedCompletionStatus(g_hcp, 0, 0, 0);

	WaitForMultipleObjects(ThreadCount, hThread, TRUE, INFINITE);

	for (int i = 0; i < ThreadCount; ++i)
		CloseHandle(hThread[i]);

	delete[] hThread;
		
	closesocket(listen_sock);
	WSACleanup();
	timeEndPeriod(1);

	GetLocalTime(&lst);
	_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] 서버 정상 종료\n",
		lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

    return 0;
}

// 워커 스레드
UINT WINAPI	WorkerThread(LPVOID lParam)
{
	int retval;

	bool bInitR = true;
	bool bInitS = true;

	DWORD cbTransferred;
	stSession* NowSession;
	OVERLAPPED* overlapped;

	void *R = 0;
	void *S = 0;

	while (1)
	{	
		// 변수들 초기화
		cbTransferred = 0;
		NowSession = nullptr;
		overlapped = nullptr;

		// 비동기 입출력 완료 대기
		// GQCS 대기
		retval = GetQueuedCompletionStatus(g_hcp, &cbTransferred, (PULONG_PTR)&NowSession, &overlapped, INFINITE);
		// --------------
		// 완료 체크
		// --------------
		// overlapped가 nullptr이라면, IOCP 에러임.
		if (overlapped == NULL)
		{
			// 셋 다 0이면 스레드 종료.
			if (cbTransferred == 0 && NowSession == NULL )
			{
				SYSTEMTIME lst;
				GetLocalTime(&lst);
				_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] 스레드 정상종료\n",
					lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

				break;
			}

			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] 워커 스레드. IOCP 에러 발생\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			break;
		}

		// overlapped가 nullptr이 아니라면 내 요청 관련 에러 체크
		else
		{		
			// 지금 하는거 없음.			
		}		

		// -----------------
		// Recv 로직
		// -----------------
		// WSArecv()가 완료된 경우, 받은 데이터가 0이 아니면 로직 처리
		if (&NowSession->m_RecvOverlapped == overlapped && cbTransferred > 0)
		{			
			// rear 이동
			NowSession->m_RecvBuff.MoveWritePos(cbTransferred);

			// 1. 링버퍼 후, 패킷 처리
			RecvProc(NowSession);			

			// 2. 리시브 다시 걸기
			RecvPost(NowSession);		
		}

		// -----------------
		// Send 로직
		// -----------------
		// WSAsend()가 완료된 경우, 받은 데이터가 0이 아니면 로직처리
		else if (&NowSession->m_SendOverlapped == overlapped && cbTransferred > 0)
		{				
			// 1. front 이동	
			NowSession->m_SendBuff.RemoveData(cbTransferred);
			
			// 2. 샌드 가능 상태로 변경
			NowSession->m_SendFlag = 0;

			SendPost(NowSession);
		}

		// -----------------
		// I/O카운트 감소 및 삭 제 처리
		// -----------------
		// I/O카운트 감소 후, 0이라면접속 종료
		long NowVal = InterlockedDecrement(&NowSession->m_IOCount);
		if (NowVal == 0)
			Disconnect(NowSession);

		else if (NowVal < 0)
			InterlockedIncrement(&IOCountMinusCount);

	}
	return 0;
}
