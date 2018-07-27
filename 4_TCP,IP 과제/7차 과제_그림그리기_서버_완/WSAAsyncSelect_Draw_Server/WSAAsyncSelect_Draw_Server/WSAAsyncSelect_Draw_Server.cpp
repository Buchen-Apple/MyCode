#pragma comment(lib, "ws2_32")
#include <tchar.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <locale.h>
#include "RingBuff.h"

#define SERVERPORT 25000
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

// 페이로드
#pragma pack(push, 1)
struct st_Draw_Packet
{
	int iStartX;
	int iStartY;
	int iEndX;
	int iEndY;
};
#pragma pack(pop)

// 필요 기능들 함수
BOOL AddUser(SOCKET sock, SOCKADDR_IN);		// 클라 추가
BOOL ProcClose(SOCKET);						// 클라 삭제
int IndexSearch(SOCKET);					// 소켓을 넘기면 SessionList의 인덱스를 찾아주는 함수.
BOOL ProcRead(SOCKET);						// 데이터 받기
BOOL ProcWrite(SOCKET);						// 데이터 보내기
BOOL ProcWriteBroadCast(SOCKET);			// 브로드 캐스트용 보내기. 모든 유저에게 보낸다. (나에게 패킷을 보낸 유저 제외 이런거 없음. 걍 다 보냄)
int SendPacket(char*, int, Session*);		// 리시브 큐의 데이터를 샌드 큐에 저장한다.
BOOL ProcPacket(SOCKET);					// 리시브 큐의 데이터 처리

// 전역 변수
HINSTANCE hInst;	// 현재 인스턴스입니다.
SOCKET listen_sock;	// 리슨소켓
Session* SessionList[USERMAX];
int dAcceptUserCount = 0;		// Accept한 유저 수

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

	_tprintf(_T("서버 준비 완료\n"));

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


// 윈 프록
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
			if (WSAGETSELECTERROR(lParam) != 10053)
				_tprintf(_T("WM_SOCK 에러 발생 : %d\n"), WSAGETSELECTERROR(lParam));

			ProcClose(wParam);
			break;
		}

		// 메시지 처리
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_READ:
			ProcRead(wParam);
			ProcPacket(wParam);
			break;

		case FD_WRITE:
			// 해당 유저와 send 가능 상태가 되었다. 해당 유저의 sendFlag를 TRUE로 만든다.
		{
			int iIndex = IndexSearch(wParam);
			if (iIndex == -1)
				break;

			SessionList[iIndex]->m_SendFlag = TRUE;

			// 그리고, 혹시 Write할게 있으면 한다.
			BOOL Check = ProcWriteBroadCast(wParam);
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
			// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있거나 꽉차서 recv 불가능 상태. recv할게 없다는 것이니 while문 종료
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


	// 샌드 패킷의 데이터를, 상대에게 send()
	int BuffArray = 0;
	int iSize = sendUser->m_SendBuff->GetUseSize();

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

// 브로드 캐스트용 데이터 보내기
BOOL ProcWriteBroadCast(SOCKET sock)
{
	// 어떤 유저의 데이터를 보낼것인지 찾아낸다.
	int iIndex = IndexSearch(sock);
	if (iIndex == -1)
		return FALSE;

	// 골라낸 유저를 sendUser에 저장
	Session* sendUser = SessionList[iIndex];

	// 보낼 총 사이즈를 구한다. (샌드 버퍼에 있는 모든 데이터 다 보낸다)
	int iSize = sendUser->m_SendBuff->GetUseSize();
	int BuffArray = 0;

	// 브로드 캐스트
	while (iSize > 0)
	{
		// 일단 임시 버퍼로 데이터 복사(이 때는 디큐가 아니라 Peek)
		char Buff[1000];
		int PeekSize = sendUser->m_SendBuff->Peek(&Buff[BuffArray], iSize);
		if (PeekSize == -1)
		{
			_tprintf(_T("ProcWrite(). 샌드 큐가 비어있는 유저\n"));
			return FALSE;
		}

		iSize -= PeekSize;
		BuffArray += PeekSize;		

		// 이번에 Peek 한 데이터를 모든 유저에게 데이터 보내기.
		for (int i = 0; i < dAcceptUserCount; ++i)
		{
			// 해당 유저의 SendFlag가 FALSE라면 샌드 불가능 상태.
			if (SessionList[i]->m_SendFlag == FALSE)
			{
				_tprintf(_T("ProcWrite(). m_SendFlag가 FALSE인 유저\n"));
				continue;
			}

			// 유저 1명 선택
			Session* goalUser = SessionList[i];

			// 임시 버퍼에 있는 데이터를, 유저 1명에게 send()
			int BuffArray = 0;
			int TempPeekSize = PeekSize;

			while (TempPeekSize > 0)
			{
				int sendCheck = send(goalUser->m_Sock, &Buff[BuffArray], TempPeekSize, 0);

				// 에러가 발생했으면
				if (sendCheck == SOCKET_ERROR)
				{
					// 우드블럭인지 체크. 우드블럭이라면, Send불가능 상태가 된 것이니(내 소켓 버퍼나 상대 소켓 버퍼가 가득 찬 상황?)
					// 샌드 플레그를 FASLE로 만들고 나간다.
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						_tprintf(_T("ProcWrite(). WSAEWOULDBLOCK이 뜬 유저\n"));
						sendUser->m_SendFlag = FALSE;
						continue;
					}

					// 그 외 에러는 그냥 끊는다.
					ProcClose(goalUser->m_Sock);
				}

				TempPeekSize -= sendCheck;
				BuffArray += sendCheck;
			}	
		}

		// 모든 유저에게 샌드후, 실제로 Peek 한 사이즈 만큼 Remove한다
		sendUser->m_SendBuff->RemoveData(PeekSize);
	}	

	return TRUE;

}

// char의 데이터를 size만큼 Session의 샌드 버퍼에 넣기.
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

// 리시브 큐의 데이터 처리
BOOL ProcPacket(SOCKET sock)
{
	// 데이터를 처리할 유저를 찾아낸다.리스트의 몇 번 인덱스 유저인지 찾아낸다.
	int iIndex = IndexSearch(sock);
	if (iIndex == -1)
		return FALSE;

	// User 변수에다가, 현재 처리할 유저를 담는다.
	Session* User = SessionList[iIndex];

	// 유저의 리시브 큐의 가장 앞 헤더 확인. 현재, 헤더에는 페이로드의 사이즈가 들어있다.
	unsigned short header = 0;
	int Check = User->m_RecvBuff->Peek((char*)&header, 2);
	if (Check == -1)
	{
		_tprintf(_T("ProcPacket(). 리시브 큐가 비어있음\n"));
		return FALSE;
	}

	while (1)
	{
		// 패킷 1개가 완성될 수 있으면 그때 패킷 처리
		if (User->m_RecvBuff->GetUseSize() >= header + 2)
		{
			int DequeueSize;
			int BuffSize = 2;
			int BuffArray = 0;

			// 반복하며 총 2바이트 헤더를 임시버퍼로 꺼내옴. (반복하는 이유는, 버퍼가 잘려서 한 번에 2바이트를 못읽어올 수도 있기 때문에.
			while (BuffSize > 0)
			{				
				DequeueSize = User->m_RecvBuff->Dequeue((char*)&header + BuffArray, BuffSize);
				if (DequeueSize == -1)
				{
					_tprintf(_T("ProcPacket(). 리시브 큐가 비어있음(헤더)\n"));
					return FALSE;
				}

				BuffSize -= DequeueSize;
				BuffArray += DequeueSize;
			}			

			
			st_Draw_Packet DrawInfo;
			BuffSize = header;
			BuffArray = 0;

			// 마찬가지로 헤더에 들어있는 값(페이로드의 길이) 만큼 버퍼로 꺼내옴.
			while (BuffSize > 0)
			{
				DequeueSize = User->m_RecvBuff->Dequeue((char*)&DrawInfo + BuffArray, BuffSize);
				if (DequeueSize == -1)
				{
					_tprintf(_T("ProcPacket(). 리시브 큐가 비어있음(페이로드)\n"));
					return FALSE;
				}

				BuffSize -= DequeueSize;
				BuffArray += DequeueSize;
			}
			

			// 헤더, 페이로드의 데이터를 샌드패킷에 넣는다.
			int check = SendPacket((char*)&header, 2, User);
			if (check == -1)
				return FALSE;

			check = SendPacket((char*)&DrawInfo, header, User);
			if (check == -1)
				return FALSE;

			// 현재 패킷처리는 상대에게 샌드하는거밖에 없음.
			//ProcWrite(User->m_Sock);
			ProcWriteBroadCast(User->m_Sock);
		}

		// 패킷 1개가 완성되지 않으면 빠져나감.
		else
			break;

	}	

	return TRUE;
}

