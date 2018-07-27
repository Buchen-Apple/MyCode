#pragma comment(lib, "ws2_32")
#include <tchar.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <locale.h>
#include "RingBuff.h"

#define SERVERPORT 9000
#define USERMAX	500
#define WM_SOCK_ACCEPT (WM_USER+1)
#define WM_SOCK (WM_USER+2)

// 세션 구조체
// 세션 : 연결된 상대방과 통신에 사용되는 구조체
struct Session
{
	SOCKET m_Sock;
	CRingBuff* m_RecvBuff;
	CRingBuff* m_SendBuff;
	TCHAR m_ClientIP[30];
	DWORD m_ClinetPort;
	BOOL m_SendFlag;
};

// 필요 기능들 함수
BOOL AddUser(SOCKET sock, SOCKADDR_IN);		// 클라 추가
BOOL ProcClose(SOCKET);						// 클라 삭제
int IndexSearch(SOCKET);					// 소켓을 넘기면 SessionList의 인덱스를 찾아주는 함수.
BOOL ProcRead(SOCKET);						// 데이터 받기
BOOL ProcWrite(SOCKET);						// 데이터 보내기
int SendPacket(char*, int, Session*);		// 리시브 큐의 데이터를 샌드 큐에 저장한다.


// 전역 변수
HINSTANCE hInst;	// 현재 인스턴스입니다.
SOCKET listen_sock;	// 리슨소켓
Session* SessionList[USERMAX];
int dAcceptUserCount = 0;

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tmain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	setlocale(LC_ALL, "korean");

	// TODO: 여기에 코드를 입력합니다.

	// 전역 문자열을 초기화합니다.
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(NULL));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(NULL);
	wcex.lpszClassName = TEXT("WSAAsyncSelect");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(NULL));

	RegisterClassExW(&wcex);

	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(TEXT("WSAAsyncSelect"), TEXT("6차과제"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// 윈속초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_tprintf(_T("윈속 초기화 실패\n"));
		return 1;
	}

	// socket()
	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == INVALID_SOCKET)
	{
		_tprintf(_T("리슨소켓 socket() 실패\n"));
		return 1;
	}

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(SERVERPORT);
	int retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		_tprintf(_T("bind() 실패\n"));
		return 1;
	}

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
	{
		_tprintf(_T("listen() 실패\n"));
		return 1;
	}

	// listen_sock소켓을 WSAAsyncSelect()
	retval = WSAAsyncSelect(listen_sock, hWnd, WM_SOCK_ACCEPT, FD_ACCEPT);
	if (retval == SOCKET_ERROR)
	{
		_tprintf(_T("리슨소켓 WSAAsyncSelect() 실패\n"));
		return 1;
	}


	// 메시지 루프.
	MSG msg;
	
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();
	closesocket(listen_sock);
	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		int addrlen;
		int retval;

	case WM_SOCK_ACCEPT:
		// 에러 먼저 체크
		if (WSAGETSELECTERROR(lParam))
		{
			_tprintf(_T("WM_SOCK_ACCEPT 에러 발생 : %d\n"), WSAGETSELECTERROR(lParam));
			ProcClose(wParam);	// 에러가 발생하면, 해당 유저와 연결을 끊는다.
			break;
		}

		// accept() 메시지 처리
		SOCKADDR_IN clientaddr;
		SOCKET clinet_sock;
		addrlen = sizeof(clientaddr);
		clinet_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (clinet_sock == INVALID_SOCKET)
		{
			_tprintf(_T("accpet() 에러 발생!\n"));
			break;
		}

		// 접속한 클라이언트의 정보 추가 
		retval = AddUser(clinet_sock, clientaddr);
		if (retval == FALSE)
			break;

		// 접속한 클라이언트 WSAAsyncSelect()
		retval = WSAAsyncSelect(clinet_sock, hWnd, WM_SOCK, FD_READ | FD_WRITE | FD_CLOSE | FD_CONNECT);
		if (retval == SOCKET_ERROR)
		{
			_tprintf(_T("접속한 클라이언트 WSAAsyncSelect() 에러발생!\n"));
			break;
		}

		break;

	case WM_SOCK:
		// 에러 먼저 체크
		if (WSAGETSELECTERROR(lParam))
		{
			// 10053은 상대방이 접속 종료상태라는 것이다. 즉, 클라가 먼저 종료한것이니 딱히 에러는 아님. 그냥 끊으면 됨
			if(WSAGETSELECTERROR(lParam) != 10053)
				_tprintf(_T("WM_SOCK 에러 발생 : %d\n"), WSAGETSELECTERROR(lParam));

			ProcClose(wParam);
			break;
		}

		// 메시지 처리
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_READ:
			ProcRead(wParam);
			break;

		case FD_WRITE:
			// 해당 유저와 send 가능 상태가 되었다. 해당 유저의 sendFlag를 TRUE로 만든다.
			{
				int iIndex = IndexSearch(wParam);
				if (iIndex == -1)
					break;

				SessionList[iIndex]->m_SendFlag = TRUE;

				BOOL Check = ProcWrite(wParam);
				if (Check == FALSE)
					break;	// 지금은 FALSE가 나와도 뭔가 할게 없음..
			}
			
			break;

		case FD_CLOSE:
			ProcClose(wParam);
			break;

		default:
			_tprintf(_T("알 수 없는 패킷\n"));
			break;
		}
		break;	

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 인덱스 찾기
int IndexSearch(SOCKET sock)
{
	// 해당 소켓의 유저가 리스트의 몇 번 인덱스 유저인지 찾아낸다.
	int iIndex = 0;

	while (1)
	{
		if (SessionList[iIndex]->m_Sock == sock)
			break;

		iIndex++;

		if (iIndex >= dAcceptUserCount)
		{
			_tprintf(_T("없는 유저입니다\n"));
			return -1;
		}
	}

	return iIndex;
}

// 접속한 유저 추가
BOOL AddUser(SOCKET sock, SOCKADDR_IN clientaddr)
{
	// 새로운 유저 정보를 저장할 메모리 세팅
	Session* newUser = new Session;
	if (newUser == NULL)
	{
		_tprintf(_T("AddUser() 함수 실패! 메모리 공간 부족\n"));
		return FALSE;
	}
	
	// 현재 유저 수 체크. 유저 수가 Max면 더 이상 유저 추가 불가능
	if (dAcceptUserCount == USERMAX)
	{
		_tprintf(_T("AddUser() 함수 실패! 유저가 가득 찼습니다.\n"));
		return FALSE;
	}

	// IP 주소 변환
	TCHAR tIPBuff[30];
	InetNtop(AF_INET, &clientaddr.sin_addr, tIPBuff, sizeof(tIPBuff));

	// 새로운 유저 정보 세팅
	newUser->m_Sock = sock;
	newUser->m_SendFlag = FALSE;	
	_tcscpy_s(newUser->m_ClientIP, _countof(newUser->m_ClientIP), tIPBuff);
	newUser->m_ClinetPort = ntohs(clientaddr.sin_port);
	newUser->m_RecvBuff = new CRingBuff;
	newUser->m_SendBuff = new CRingBuff;

	// 리스트에 추가
	SessionList[dAcceptUserCount++] = newUser;

	_tprintf(_T("[%s:%d] 접속!\n"), newUser->m_ClientIP, newUser->m_ClinetPort);

	return TRUE;	
}

// 연결 끊기
BOOL ProcClose(SOCKET sock)
{
	// 끊어야 할 유저가 리스트의 몇 번 인덱스 유저인지 찾아낸다.
	int iIndex = IndexSearch(sock);
	if (iIndex == -1)
		return FALSE;
	
	// 찾은 유저를 RemoveUser에 연결
	Session* RemoveUser = SessionList[iIndex];
	_tprintf(_T("[%s:%d] 접속 종료!\n"), RemoveUser->m_ClientIP, RemoveUser->m_ClinetPort);

	// 골라낸 유저를 삭제한다.
	delete RemoveUser;

	// 리스트의 빈칸을 채운다.
	if (iIndex < dAcceptUserCount - 1)
		SessionList[iIndex] = SessionList[dAcceptUserCount - 1];

	// 접속중인 유저 수 1 감소
	dAcceptUserCount--;

	return TRUE;
}

// 데이터 받기
BOOL ProcRead(SOCKET sock)
{	
	DWORD retval;
	DWORD total = 0;

	// 데이터를 받아야 할 유저가 리스트의 몇 번 인덱스 유저인지 찾아낸다.
	int iIndex = IndexSearch(sock);
	if (iIndex == -1)
		return FALSE;


	// 골라낸 유저를 recvUser에 저장
	Session* recvUser = SessionList[iIndex];

	// recv()
	while (1)
	{
		// 임시 버퍼로 recv()
		char Buff[1000];
		retval = recv(recvUser->m_Sock, Buff, sizeof(Buff), 0);

		// 에러가 발생했다면 에러 처리
		if (retval == SOCKET_ERROR)
		{
			// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있다는 것. recv할게 없다는 것이니 while문 종료
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;

			// WSAEWOULDBLOCK에러가 아니면 그냥 끊는다. 뭔가 이상한놈이다.
			else
			{
				_tprintf(_T("ProcRead(). recv 후, 이상한 유저\n"));
				ProcClose(recvUser->m_Sock);
				return FALSE;
			}
		}
		else if (retval == 0)
		{
			_tprintf(_T("ProcRead(). 연결 종료된 유저\n"));
			ProcClose(recvUser->m_Sock);
			return FALSE;
		}

		// while문 종료 후, Peek을 위해 총 얼마를 받아왔는지 데이터 크기 저장.
		total += retval;

		// recv()한 데이터를 해당 유저의 recvBuff에 저장한다.
		DWORD BuffArray = 0;

		while (retval > 0)
		{
			int EnqueueCheck = recvUser->m_RecvBuff->Enqueue(&Buff[BuffArray], retval);

			// Enqueue() 함수는, 버퍼가 가득 찼으면 -1을 리턴한다.
			// 버퍼가 가득찰 정도면 뭔가 이상한 놈이니 그냥 끊는다.
			if (EnqueueCheck == -1)
			{
				_tprintf(_T("ProcRead(). 버퍼가 가득 찬 유저\n"));
				ProcClose(recvUser->m_Sock);
				return FALSE;
			}

			retval -= EnqueueCheck;
			BuffArray += EnqueueCheck;
		}
	}

	// Peek으로 받은 데이터를 보여줌.
	char* PeekBuff = new char[total];
	DWORD BuffArray = 0;
	int tempTotal = total;

	while (tempTotal > 0)
	{
		int PeekCheck = recvUser->m_RecvBuff->Peek(&PeekBuff[BuffArray], tempTotal);
		// -1이면 버퍼가 비어있는것.
		if (PeekCheck == -1)
		{
			_tprintf(_T("ProcRead(). Peek 실패\n"));
			return FALSE;
		}

		tempTotal -= PeekCheck;
		BuffArray += PeekCheck;
	}	

	PeekBuff[total] = '\0';

	_tprintf(_T("[%s:%d] "), recvUser->m_ClientIP, recvUser->m_ClinetPort);
	printf("%s\n", PeekBuff);	

	ProcWrite(recvUser->m_Sock);

	return TRUE;
}

// 데이터 보내기
BOOL ProcWrite(SOCKET sock)
{
	// 데이터를 보낼 유저를 찾아낸다. 리스트의 몇 번 인덱스 유저인지 찾아낸다.
	int iIndex = IndexSearch(sock);
	if (iIndex == -1)
		return FALSE;

	// 해당 유저의 SendFlag가 FALSE라면 샌드 불가능 상태.
	if (SessionList[iIndex]->m_SendFlag == FALSE)
	{
		_tprintf(_T("ProcWrite(). m_SendFlag가 FALSE인 유저\n"));
		return FALSE;
	}

	// 골라낸 유저를 recvUser에 저장
	Session* sendUser = SessionList[iIndex];

	// 샌드 할 데이터를, 리시브 큐 -> 샌드 큐로 저장.
	// 리시브 큐의 사용 중 크기 알아오기
	int iSize = sendUser->m_RecvBuff->GetUseSize();
	int BuffArray = 0;

	// 리시브 큐의 데이터를 임시 버퍼에 디큐 후, 샌드 버퍼로 인큐
	while (iSize > 0)
	{
		// 임시버퍼로 디큐
		char Buff[1000];
		int DequeueSize;
		DequeueSize = sendUser->m_RecvBuff->Dequeue(&Buff[BuffArray], iSize);

		// Dequeue()는 반환값이 -1이면 리시브 큐가 비어있는 유저이다.
		if (DequeueSize == -1)
		{
			_tprintf(_T("ProcWrite(). 리시브 큐가 비어있는 유저\n"));
			return FALSE;
		}

		// 샌드 버퍼로 인큐
		int sendpacketCheck = SendPacket(&Buff[BuffArray], DequeueSize, sendUser);
		if (sendpacketCheck == -1)
			return FALSE;

		iSize -= DequeueSize;
		BuffArray += DequeueSize;
	}
	

	// 샌드 패킷의 데이터를, 상대에게 send()
	BuffArray = 0;
	iSize = sendUser->m_SendBuff->GetUseSize();

	while (iSize > 0)
	{
		// 일단 임시 버퍼로 데이터 복사(이 때는 디큐가 아니라 Peek)
		char Buff[1000];
		int PeekSize;
		PeekSize = sendUser->m_SendBuff->Peek(&Buff[BuffArray], iSize);
		if (PeekSize == -1)
		{
			_tprintf(_T("ProcWrite(). 샌드 큐가 비어있는 유저\n"));
			return FALSE;
		}

		iSize -= PeekSize;
		BuffArray += PeekSize;

		// 임시 버퍼의 데이터를 상대에게 send
		int sendCheck = send(sendUser->m_Sock, Buff, PeekSize, 0);

		// 에러가 발생했으면
		if (sendCheck == SOCKET_ERROR)
		{
			// 우드블럭인지 체크. 우드블럭이라면, Send불가능 상태가 된 것이니(내 소켓 버퍼나 상대 소켓 버퍼가 가득 찬 상황?)
			// 샌드 플레그를 FASLE로 만들고 나간다.
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				_tprintf(_T("ProcWrite(). WSAEWOULDBLOCK이 뜬 유저\n"));
				sendUser->m_SendFlag = FALSE;
				return FALSE;
			}
			
			// 그 외 에러는 그냥 끊는다.
			ProcClose(sendUser->m_Sock);
		}

		// 샌드에 성공했으면 성공한 사이즈 만큼 Remove한다
		sendUser->m_SendBuff->RemoveData(sendCheck);
	}

	return TRUE;
}

// char의 데이터를 size만큼 Session에 넣기.
// 즉, 데이터를 샌드큐로 이동
// 반환값 : -1(버퍼가 가득찬 유저), 0(정상 종료)
int SendPacket(char* Buff, int iSize, Session* User)
{
	// Buff의 데이터를 샌드 큐로 이동시킨다.
	// Buff가 텅 빌때까지.
	int BuffArray = 0;

	while (iSize > 0)
	{
		// Buff에서 iSize만큼 User의 샌드큐로 인큐한다.
		int realEnqeueuSize = User->m_SendBuff->Enqueue(&Buff[BuffArray], iSize);

		// Enqueue() 함수는, 버퍼가 가득 찼으면 -1을 리턴한다.
		// 버퍼가 가득찰 정도면 뭔가 이상한 놈이니 그냥 끊는다.
		if (realEnqeueuSize == -1)
		{
			_tprintf(_T("ProcRead(). 버퍼가 가득 찬 유저\n"));
			ProcClose(User->m_Sock);
			return -1;
		}
		iSize -= realEnqeueuSize;
		BuffArray += realEnqeueuSize;		
	}

	return 0;
}