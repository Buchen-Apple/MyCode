#pragma comment(lib, "ws2_32")
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <WS2tcpip.h>
#include <tchar.h>
#include <locale.h>

#define SERVERPORT 9000
#define BUFSIZE 512

struct SOCKETINFO
{
	WSAOVERLAPPED overlapped;
	SOCKET sock;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
	WSABUF wsabuf;
};

// 작업자 스레드 함수
UINT WINAPI WorkerThread(LPVOID arg);

int main()
{
	setlocale(LC_ALL, "korean");
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 입출력 완료 포트(I/O Completion Port) 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);	// 소켓 디스크립터, IOCP핸들, 부가정보, 동시에 작동 가능 스레드. 순서대로 입력
	if (hcp == NULL)
		return 1;

	// CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// CPU 개수 * 2 개의 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; ++i)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL)
			return 1;

		CloseHandle(hThread);
	}

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == INVALID_SOCKET)
		return 1;

	// 바인딩
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		return 1;

	// 리슨
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		return 1;

	// 데이터 통신에 사용되는 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvbytes, flags;

	while (1)
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == SOCKET_ERROR)
			return 1;

		TCHAR TextBuf[30];
		InetNtop(AF_INET, &clientaddr.sin_addr, TextBuf, sizeof(TextBuf));
		_tprintf(_T("[TCP 서버] 클라 접속 : IP 주소=%s, 포트번호=%d\n"), TextBuf, ntohs(clientaddr.sin_port));

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		// 소켓 정보 구조체 할당
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == NULL)
			break;

		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recvbytes = ptr->sendbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		// 비동기 입출력 시작
		flags = 0;
		retval = WSARecv(client_sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
				_tprintf(_T("WSARecv() 에러\n"));

			continue;
		}

	}


	closesocket(listen_sock);
	WSACleanup();

	return 0;
}

UINT WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1)
	{
		// 비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET clinet_sock;
		SOCKETINFO* ptr;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, &clinet_sock, (LPOVERLAPPED*)&ptr, INFINITE);	

		// 클라이언트 정보 얻기
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);
		TCHAR TextBuf[30];
		InetNtop(AF_INET, (SOCKADDR*)&clientaddr.sin_addr, TextBuf, sizeof(TextBuf));

		// 비동기 입출력 결과 확인.
		// retval가 0이면 GetQueuedCompletionStatus() 실패, cbTransferred가 0이면 전송된 바이트가 0. 즉, 상대와 연결 끊김.
		// 아래에서는 에러처리를 해야한다.
		if (retval == 0 || cbTransferred == 0)
		{
			if (retval == 0)
			{
				DWORD temp1, temp2;

				// WSAGetOverlappedResult()함수는 Overlapped IO의 결과를 확인한다.
				_tprintf(_T("WSAGetOverlappedResult() 에러. %d\n"), WSAGetLastError());
			}

			closesocket(ptr->sock);
			_tprintf(_T("[TCP 서버] 클라이언트 종료 : IP 주소=%s, 포트 번호=%d\n"), TextBuf, ntohs(clientaddr.sin_port));
			delete ptr;
			continue;
		}

		// 데이터 전송량 갱신
		if (ptr->recvbytes == 0)
		{
			ptr->recvbytes = cbTransferred;
			ptr->sendbytes = 0;

			// 받은 데이터 출력
			ptr->buf[ptr->recvbytes] = '\0';
			_tprintf(_T("[%s:%d] "), TextBuf, ntohs(clientaddr.sin_port));
			printf("%s\n", ptr->buf);
		}
		else
			ptr->sendbytes += cbTransferred;

		if (ptr->recvbytes > ptr->sendbytes)
		{
			// 데이터 보내기
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
			ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;

			DWORD sendbytes;
			retval = WSASend(ptr->sock, &ptr->wsabuf, 1, &sendbytes, 0, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
					_tprintf(_T("WSASend 에러!"));
				continue;
			}

		}
		else
		{
			ptr->recvbytes = 0;

			// 데이터 받기
			ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
			ptr->wsabuf.buf = ptr->buf;
			ptr->wsabuf.len = BUFSIZE;

			DWORD recvbytes;
			DWORD flags = 0;
			retval = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, NULL);
			if (retval == SOCKET_ERROR)
			{
				if(WSAGetLastError() != WSA_IO_PENDING)
					_tprintf(_T("WSARecv 에러!"));
				continue;
			}
		}
	}

	return 0;
}