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

extern CRITICAL_SECTION cs;

int AcceptCount = 0;

DWORD IOCountMinusCount = 0;

DWORD JoinUser = 0;

ULONG SessionDeleteCount = 0;

// 워커 스레드
UINT	WINAPI	WorkerThread(LPVOID lParam);

int _tmain()
{
	system("mode con: cols=100 lines=20");

	_tsetlocale(LC_ALL, L"korean");

	timeBeginPeriod(1);

	InitializeSRWLock(&g_Session_map_srwl);
	InitializeCriticalSection(&cs);


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
	hThread = new HANDLE[si.dwNumberOfProcessors];

	for (DWORD i = 0; i < 1; ++i)
	{
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, 0, 0, NULL);
		if (hThread[i] == NULL)
		{
			_fputts(L"_beginthreadex() 에러\n", stdout);
			return 1;
		}
	}

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == INVALID_SOCKET)
	{
		_fputts(L"listen_sock() 에러\n", stdout);
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
		_fputts(L"bind() 에러\n", stdout);
		return 1;
	}

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		_fputts(L"listen() 에러\n", stdout);
		return 1;
	}

	// 논블락 소켓으로 변경
	ULONG on = 1;
	retval = ioctlsocket(listen_sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR)
	{
		printf("NoneBlock Change Fail...\n");
		return 1;
	}

	//_fputts(L"서버 열었음\n", stdout);
	fputs("서버 열었음\n", stdout);


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
			printf("Accept Count : %d, IOMinusCount : %d, NowJoinUser : %d, SessionDeleteCount : %d\n", AcceptCount, IOCountMinusCount, JoinUser, SessionDeleteCount);
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
			{
				Sleep(0);
				continue;
			}
				
			_tprintf(L"accept() 에러 %d\n", WSAGetLastError());
			break;
		}

		// 연결된 클라 소켓의 송신버퍼 크기를 0으로 변경. 그래야 정상적으로 비동기 입출력으로 실행
		int optval = 0;
		retval = setsockopt(client_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
		if (retval == SOCKET_ERROR)
		{
			_fputts(L"setsockopt() 에러\n", stdout);
			break;
		}

		// IP 알아내기
		TCHAR IP[30];
		InetNtop(AF_INET, &clientaddr.sin_addr, IP, 30);

		// ------------------
		// 세션 구조체 생성 후 셋팅
		// ------------------
		stSession* NewSession = new stSession;

		_tcscpy_s(NewSession->m_IP, sizeof(NewSession->m_IP) / sizeof(TCHAR), IP);
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
		RecvPost(NewSession);
		AcceptCount++;
		InterlockedIncrement(&JoinUser);	
		
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

	_tprintf(L"모든 유저 종료!\n");

	// 종료되었으면 스레드도 종료
	for(int i=0; i<si.dwNumberOfProcessors; ++i)
		PostQueuedCompletionStatus(g_hcp, 0, 0, 0);

	WaitForMultipleObjects(si.dwNumberOfProcessors, hThread, TRUE, INFINITE);

	for (int i = 0; i < si.dwNumberOfProcessors; ++i)
		CloseHandle(hThread[i]);

	delete hThread;
		
	LeaveCriticalSection(&cs);
	closesocket(listen_sock);
	WSACleanup();
	timeEndPeriod(1);

	_tprintf(L"서버 정상 종료\n");
	_gettch();
    return 0;
}

// 워커 스레드
UINT WINAPI	WorkerThread(LPVOID lParam)
{
	int retval;

	DWORD cbTransferred = 0;
	stSession* NowSession;
	OVERLAPPED* overlapped;

	while (1)
	{
		// 변수들 초기화
		cbTransferred = 0;
		ZeroMemory(&NowSession, sizeof(NowSession));
		ZeroMemory(&overlapped, sizeof(overlapped));

		// 비동기 입출력 완료 대기
		// GQCS 대기
		retval = GetQueuedCompletionStatus(g_hcp, &cbTransferred, (PULONG_PTR)&NowSession, (LPOVERLAPPED*)&overlapped, INFINITE);

		// 셋 다 0이면 스레드 종료.
		if (cbTransferred == 0 && NowSession == NULL && overlapped == NULL)
		{
			_tprintf(L"쓰레드 종료!\n");
			break;
		}		


		// --------------
		// 완료 체크
		// --------------
		// overlapped가 nullptr이라면, IOCP 에러임.
		if (overlapped == NULL)
		{
			_fputts(L"워커 스레드. IOCP 에러 발생.\n", stdout);
			_gettch();
		}

		// overlapped가 nullptr이 아니라면 내 요청 관련 에러 체크
		else
		{		
			// 보낸, 받은 바이트 수가 0이라면 I/O카운트 하나 감소
			if (cbTransferred == 0)
			{
				int abc = 10;
				int NowVal = InterlockedDecrement(&NowSession->m_IOCount);

				if (NowVal == 0)
				{
					Disconnect(NowSession);
					continue;
				}

				else if (NowVal < 0)
					InterlockedIncrement(&IOCountMinusCount);
			}
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

			// 2. 리시브 새로 걸기
			// false가 리턴되면, 해당 유저 삭제된것. continue한다
			RecvPost(NowSession);
		}

		// -----------------
		// Send 로직
		// -----------------
		// WSAsend()가 완료된 경우, 받은 데이터가 0이 아니면 로직처리
		else if (&NowSession->m_SendOverlapped == overlapped && cbTransferred > 0)
		{			
			// 1. front 이동
			NowSession->m_SendBuff.EnterLOCK();		// 락 -----------------------

			NowSession->m_SendBuff.RemoveData(cbTransferred);				

			NowSession->m_SendBuff.LeaveLOCK();		// 락 해제 -----------------------
			
			// 2. 샌드 가능 상태로 변경
			// SendFlag(1번인자)가 1(3번인자)과 같다면, SendFlag(1번인자)를 0(2번인자)으로 변경
			//int abc = InterlockedCompareExchange(&NowSession->m_SendFlag, FALSE, TRUE);			

			InterlockedExchange(&NowSession->m_SendFlag, FALSE);		

			// 3. SendPost 호출.
			// false라 리턴되면 해당 유저 삭제된 것. continue한다
			if (SendPost(NowSession) == false)
				continue;
		}

		// -----------------
		// I/O카운트 감소 및 삭 제 처리
		// -----------------
		// I/O카운트 감소 후, 0이라면접속 종료
		int NowVal = InterlockedDecrement(&NowSession->m_IOCount);
		if (NowVal == 0)
			Disconnect(NowSession);

		else if (NowVal < 0)
			InterlockedIncrement(&IOCountMinusCount);

	}
	return 0;
}
