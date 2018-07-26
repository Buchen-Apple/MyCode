#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define SERVERPORT 9000
#define BUFSIZE 512

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DisplayText(char* fmt, ...);

void err_quit(char* msg);
void err_display(char* msg);

DWORD WINAPI ServerMain(LPVOID arg);
DWORD WINAPI ProcessClinet(LPVOID arg);

HINSTANCE hInst;
HWND hEdit;
CRITICAL_SECTION cs;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,  int nCmdLine)
{
	hInst = hInstance;
	InitializeCriticalSection(&cs);

	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = _T("MyWndClass");
	if (!RegisterClass(&wndclass)) return 1;

	HWND hWnd = CreateWindow(_T("MyWndClass"), _T("TCP Server"), WS_OVERLAPPEDWINDOW, 0, 0, 600, 200, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL)	return 1;
	ShowWindow(hWnd, nCmdLine);
	UpdateWindow(hWnd);

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ServerMain, NULL, 0, NULL);

	// 메시지 루프
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

	}

	DeleteCriticalSection(&cs);
	return msg.wParam;
}

// 윈 프록
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		hEdit = CreateWindow(_T("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
			0, 0, 0, 0, hWnd, (HMENU)100, hInst, NULL);

		return 0;

	case WM_SIZE:
		MoveWindow(hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;

	case WM_SETFOCUS:
		SetFocus(hEdit);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// 편집 컨트롤 출력 함수
void DisplayText(const char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE + 256];
	vsprintf(cbuf, fmt, arg);

	EnterCriticalSection(&cs);
	int nLength = GetWindowTextLength(hEdit);
	SendMessage(hEdit, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
	LeaveCriticalSection(&cs);

	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCTSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);

	DisplayText("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// TCP 서버 시작 부분
DWORD WINAPI ServerMain(LPVOID arg)
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(0x0202, &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
		err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(SOCKADDR_IN));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET clinet_sock;
	SOCKADDR_IN clienetaddr;
	int addrlen;
	HANDLE hThread;
	char TextBuff[33];


	while (1)
	{
		//accept()
		addrlen = sizeof(clienetaddr);
		clinet_sock = accept(listen_sock, (SOCKADDR*)&clienetaddr, &addrlen);
		if (clinet_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		DisplayText("\r\n[TCP server] Clinet IP : %s, Client Port : %d\r\n",
			InetNtop(AF_INET, &clienetaddr.sin_addr, TextBuff, sizeof(TextBuff)),
			ntohs(clienetaddr.sin_port));

		// 스레드 생성
		hThread = CreateThread(NULL, 0, ProcessClinet,
			(LPVOID)clinet_sock, 0, NULL);
		if (hThread == NULL)
			closesocket(clinet_sock);
		else
			CloseHandle(hThread);
	}

	// closesocket()
	closesocket(listen_sock);

	WSACleanup();
	return 0;
}

// 클라이언트와 데이터 통신
DWORD WINAPI ProcessClinet(LPVOID arg)
{
	SOCKET clinet_sock = (SOCKET)arg;
	int retval;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	char TextBuff[30];

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(clinet_sock, (SOCKADDR*)&clientaddr, &addrlen);	// 이미 연결된 클라이언트의 정보 얻기

	while (1)
	{
		// 데이터 받기
		retval = recv(clinet_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;

		// 받은 데이터 출력
		buf[retval] = '\0';
		DisplayText("[TCP %s:%d] %s\r\n",
			InetNtop(AF_INET, &clientaddr.sin_addr, TextBuff, sizeof(TextBuff)),
				ntohs(clientaddr.sin_port), buf);

		// 데이터 보내기
		retval = send(clinet_sock, buf, retval, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			break;
		}
	}

	// closesocket()
	closesocket(clinet_sock);
	DisplayText("[TCP Server] Client Disconnet. IP : %s, Port : %d\r\n",
		InetNtop(AF_INET, &clientaddr.sin_addr, TextBuff, sizeof(TextBuff)), clientaddr.sin_port);

	return 0;
}