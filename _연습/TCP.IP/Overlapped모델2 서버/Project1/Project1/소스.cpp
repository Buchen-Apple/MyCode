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

SOCKET client_sock;
HANDLE hReadEvent, hWriteEvent;

// 비동기 입출력 시작과 처리 함수
UINT WINAPI WorkerThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);


int main()
{
	int retval;
	setlocale(LC_ALL, "korean");

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(0x0202, &wsa) != 0)
		return 1;

	// 소켓 생성
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

	// listen
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		return 1;

	// 이벤트 객체 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (hReadEvent == NULL)
		return 1;

	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hWriteEvent == NULL)
		return 1;

	// 스레드 생성
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, NULL, 0, NULL);
	if (hThread == NULL)
		return 1;
	CloseHandle(hThread);

	while (1)
	{
		// worker thread에서 clinet_sock의 값을 읽었을 경우, 신호 상태가 된다.
		WaitForSingleObject(hReadEvent, INFINITE);

		//accept
		client_sock = accept(listen_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET)
			break;

		SetEvent(hWriteEvent);
	}


	// 각종 해제
	closesocket(listen_sock);
	WSACleanup();

	return 0;
}

UINT WINAPI WorkerThread(LPVOID arg)
{
	int retval;

	while (1)
	{
		while (1)
		{
			// alertable wait
			DWORD result = WaitForSingleObjectEx(hWriteEvent, INFINITE, TRUE);
			if (result == WAIT_OBJECT_0)	// WAIT_OBJECT_0는 새로운 클라가 접속했다는 뜻이다.
				break;
			if (result != WAIT_IO_COMPLETION)	// WAIT_IO_COMPLETION는 비동기 입출력과 이에 따른 완료 루틴이 완료되었다는 뜻. 현재 여기서는 WAIT_IO_COMPLETION가 아닐때 스레드 빠져나감.
				return 1;						// 즉, WAIT_IO_COMPLETION이 아니라면 다시 alertable wait 상태로 돌아간다.

		}

		// 접속한 클라이언트 정보 출력
		SOCKADDR_IN clientaddr;
		TCHAR TextBuff[30];

		int addr = sizeof(clientaddr);
		getpeername(client_sock, (SOCKADDR*)&clientaddr, &addr);
		InetNtop(AF_INET, &clientaddr.sin_addr, TextBuff, sizeof(TextBuff));
		_tprintf(_T("connect client. IP : %s, port : %d\n"), TextBuff, ntohs(clientaddr.sin_port));

		// 소켓 정보 구조체 할당과 초기화
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == NULL)
			return 1;

		ZeroMemory(&ptr->overlapped, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		SetEvent(hReadEvent);	// 메인 스레드에게, 읽기 완료했다는 것을 전달한다.
		ptr->recvbytes = ptr->sendbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		// 비동기 입출력 시작
		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, CompletionRoutine);
		if (retval == SOCKET_ERROR)
			if (WSAGetLastError() != WSA_IO_PENDING)
				return 1;
	}

	return 0;
}

void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	int retval;

	// 클라 정보 얻기
	SOCKETINFO* ptr = (SOCKETINFO*)lpOverlapped; 
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

	// 비동기 입출력 결과 확인
	if (dwError != 0 || cbTransferred == 0)
		//dwError가 0이 아닌 경우 : 에러 발생. cbTransferred가 0인 경우 : 전송된 데이터 수가 0. 즉, 상대방과 접속 종료
	{
		closesocket(ptr->sock);
		delete ptr;
		return;
	}

	// 데이터 전송량 갱신
	if (ptr->recvbytes == 0)
	{
		ptr->recvbytes = cbTransferred;
		ptr->sendbytes = 0;

		// 받은 데이터 출력
		ptr->buf[ptr->recvbytes] = '\0';
		TCHAR TextBuff[30];
		InetNtop(AF_INET, &clientaddr.sin_addr, TextBuff, sizeof(TextBuff));

		_tprintf(_T("[%s:%d] "), TextBuff, ntohs(clientaddr.sin_port));
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
		retval = WSASend(ptr->sock, &ptr->wsabuf, 1, &sendbytes, 0, &ptr->overlapped, CompletionRoutine);
		if (retval == SOCKET_ERROR)
			if (WSAGetLastError() != WSA_IO_PENDING)
				return;
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
		retval = WSARecv(ptr->sock, &ptr->wsabuf, 1, &recvbytes, &flags, &ptr->overlapped, CompletionRoutine);
		if (retval == SOCKET_ERROR)
			if (WSAGetLastError() != WSA_IO_PENDING)
				return;

	}
}