// IOCP_Echo_Server.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#pragma comment(lib, "ws2_32")
#include <WS2tcpip.h>
#include <process.h>
#include <clocale>
#include <map>

#include "RingBuff\RingBuff.h"

using namespace Library_Jingyu;
using namespace std;

#define SERVER_PORT	9000

// 클라 세션
struct stSession
{
	// 끊기 플래그. true면 끊는다.
	bool m_DisconnectFlag = false;

	// shoutdown 중인지 체크 플래그. true면 shoutdown(SEND)를 보낸 것이다.
	// SEND를 보냈으면 WSASend는 보내면 안됨.
	bool m_ShoudownFlag = false;

	SOCKET m_Client_sock;

	TCHAR m_IP[30];
	USHORT m_prot;

	OVERLAPPED m_RecvOverlapped;
	OVERLAPPED m_SendOverlapped;

	CRingBuff m_RecvBuff;
	//CRingBuff m_SendBuff; 샌드링버퍼 안쓰는중

	ULONG m_IOCount = 0;
};

SRWLOCK g_Session_map_srwl;

// 세션 저장 map.
// Key : SOCKET, Value : 세션구조체
map<SOCKET, stSession*> map_Session;

#define	LockSession()	AcquireSRWLockExclusive(&g_Session_map_srwl)
#define UnlockSession()	ReleaseSRWLockExclusive(&g_Session_map_srwl)




// 입출력 완료포트 변수
HANDLE	g_hcp;


// 워커 스레드
UINT	WINAPI	WorkerThread(LPVOID lParam);

// 접속 종료
void Disconnect(stSession* DeleteSession);

int _tmain()
{
	system("mode con: cols=100 lines=20");

	_tsetlocale(LC_ALL, L"korean");

	InitializeSRWLock(&g_Session_map_srwl);

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

	for (DWORD i = 0; i < si.dwNumberOfProcessors; ++i)
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
	if(retval == SOCKET_ERROR)
	{
		_fputts(L"listen() 에러\n", stdout);
		return 1;
	}

	_fputts(L"서버 열었음\n", stdout);
	//fputs("서버 열었음\n", stdout);


	// --------------------------
	// Accept 파트
	// --------------------------
	SOCKET client_sock;
	SOCKADDR_IN	clientaddr;
	int addrlen;
	DWORD recvBytes, flags;
	
	while (1)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			_fputts(L"accept() 에러\n", stdout);
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
		ZeroMemory(&NewSession->m_RecvOverlapped, sizeof(NewSession->m_RecvOverlapped));
		
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
		_tprintf(L"[TCP 서버] 클라이언트 접속 : IP 주소=%s, 포트=%d\n", IP, NewSession->m_prot);


		// -----------
		// 비동기 입출력 시작
		// -----------
		NewSession->m_RecvBuff.EnterLOCK();

		// 1. recv 링버퍼 포인터 얻어오기.
		char* recvBuff = NewSession->m_RecvBuff.GetBufferPtr();

		// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
		int Size = NewSession->m_RecvBuff.GetNotBrokenPutSize();

		// 3. 한 번에 쓸 수 있는 공간이 0이라면
		if (Size == 0)
		{
			// 여유공간이 있을 경우, Size를 1로 변경
			if (NewSession->m_RecvBuff.GetFreeSize() > 0)
				Size = 1;
		}

		// 4. 1칸 이동한 rear 위치 알기(실제 rear 위치가 이동하지는 않음)	
		int* rear = (int*)NewSession->m_RecvBuff.GetWriteBufferPtr();
		int TempRear = NewSession->m_RecvBuff.NextIndex(*rear, 1);


		// 5. wsabuf 셋팅
		WSABUF wsabuf;		

		wsabuf.buf = &recvBuff[TempRear];
		wsabuf.len = Size;

		flags = 0;

		// 6. recv()
		InterlockedIncrement64((LONG64*)&NewSession->m_IOCount);
		
		retval = WSARecv(client_sock, &wsabuf, 1, &recvBytes, &flags, &NewSession->m_RecvOverlapped, NULL);
		NewSession->m_RecvBuff.LeaveLOCK();

		// 7. 에러 처리
		if (retval == SOCKET_ERROR)
		{
			int Error = WSAGetLastError();

			// 비동기 입출력이 시작된게 아니라면
			if (Error != WSA_IO_PENDING)
			{
				printf("%d\n", Error);
				// 근데, 버퍼 부족이라면
				if (Error == WSAENOBUFS)
				{
					// 화면에 버퍼부족 출력 
					_tprintf(L"[TCP 서버] 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", IP, NewSession->m_prot);
				}

				InterlockedDecrement64((LONG64*)&NewSession->m_IOCount);

				NewSession->m_DisconnectFlag = true;
				shutdown(NewSession->m_Client_sock, SD_BOTH);
			}
		}

	}

	WSACleanup();
    return 0;
}


// 워커 스레드
UINT	WINAPI	WorkerThread(LPVOID lParam)
{
	int retval;

	DWORD cbTransferred = 0;
	stSession* NowSession;
	OVERLAPPED* overlapped;

	while (1)
	{
		// 비동기 입출력 완료 대기
		cbTransferred = 0;
		NowSession = nullptr;
		overlapped = nullptr;

		// GQCS 대기
		retval = GetQueuedCompletionStatus(g_hcp, &cbTransferred, (PULONG_PTR)&NowSession, (LPOVERLAPPED*)&overlapped, INFINITE);

		// --------------
		// 완료 체크
		// --------------
		// overlapped가 nullptr이라면, IOCP 에러임.
		if (overlapped == nullptr)
		{
			_fputts(L"워커 스레드. IOCP 에러 발생.\n", stdout);
			_gettch();
		}

		// overlapped가 nullptr이 아니라면 내 요청 관련 에러 체크
		else
		{
			// 보낸, 받은 바이트 수가 0이라면 접속 끊음
			if (cbTransferred == 0)
			{
				NowSession->m_DisconnectFlag = true;
				shutdown(NowSession->m_Client_sock, SD_BOTH);
			}
		}

		// 여기까지 오면 일단 i/o가 정상적으로 완료된 것이니, I/O 카운트 1 감소	(에러가 발생했든 안했든)
		InterlockedDecrement64((LONG64*)&NowSession->m_IOCount);	
		

		// recv()가 완료된 경우, [1. 받은 데이터가 0이 아니고] [2. 끊어야 하는 대상이 아닐 경우]에 로직 처리
		if (&NowSession->m_RecvOverlapped == overlapped 
			&& cbTransferred != 0 && NowSession->m_DisconnectFlag == false)
		{
			NowSession->m_RecvBuff.EnterLOCK();

			// 받은 바이트만큼 rear 이동
			NowSession->m_RecvBuff.MoveWritePos(cbTransferred);

			// 받은 데이터만큼 Dequeue
			char* Text = new char[cbTransferred + 1];

			int dequeueSize = NowSession->m_RecvBuff.Dequeue(Text, cbTransferred);
			NowSession->m_RecvBuff.LeaveLOCK();

			if (dequeueSize == -1)
			{
				_fputts(L"워커 스레드. Recv()디큐 중 에러 발생.\n", stdout);
				_gettch();
			}			

			Text[cbTransferred] = '\0';

			// 디큐한 데이터 출력하기
			_tprintf(L"[%s:%d] 받은 데이터 : ", NowSession->m_IP, NowSession->m_prot);
			printf("%s\n",Text);

			// 받은 문자가 shutdown이면 SEND를 보낸다.
			if (strcmp(Text, "shutdown") == 0)
			{
				NowSession->m_ShoudownFlag = true;
				shutdown(NowSession->m_Client_sock, SD_SEND);
			}

			// 받은 문자가 Thanks면 저쪽도 보낼거 다 보낸것. 이제 종료해도 된다.
			else if (strcmp(Text, "Thanks") == 0)
			{
				LockSession();				
				Disconnect(NowSession);
				map_Session.erase(NowSession->m_Client_sock);
				UnlockSession();

				continue;
			}



			// ------------------
			// 디큐한 데이터 Send하기
			// ------------------
			WSABUF wsabuf;
			DWORD SendBytes, flags = 0;

			// shutdown()으로 FIN을 보내지 않은 상황에서만 Send 한다.
			if (NowSession->m_ShoudownFlag == false)
			{
				// 1. wsabuf 셋팅
				wsabuf.buf = Text;
				wsabuf.len = cbTransferred;				

				// 2. Send()
				InterlockedIncrement64((LONG64*)&NowSession->m_IOCount);
				ZeroMemory(&NowSession->m_SendOverlapped, sizeof(NowSession->m_SendOverlapped));
				retval = WSASend(NowSession->m_Client_sock, &wsabuf, 1, &SendBytes, flags, &NowSession->m_SendOverlapped, NULL);

				// 3. 에러 처리
				if (retval == SOCKET_ERROR)
				{
					int Error = WSAGetLastError();

					// 비동기 입출력이 시작된게 아니라면
					if (Error != WSA_IO_PENDING)
					{
						printf("%d\n", Error);

						// 근데, 버퍼 부족이라면
						if (Error == WSAENOBUFS)
						{
							// 화면에 버퍼부족 출력 
							_tprintf(L"[TCP 서버] Send 중, 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", NowSession->m_IP, NowSession->m_prot);
						}

						InterlockedDecrement64((LONG64*)&NowSession->m_IOCount);

						// 그리고 접속 종료
						NowSession->m_DisconnectFlag = true;
						shutdown(NowSession->m_Client_sock, SD_BOTH);
					}
				}
			}

			// 받았던 버퍼 동적해제
			delete Text;

			// -----------
			// 그리고 다시 비동기 입출력 시작
			// -----------
			NowSession->m_RecvBuff.EnterLOCK();

			// 1. recv 링버퍼 포인터 얻어오기.
			char* recvBuff = NowSession->m_RecvBuff.GetBufferPtr();

			// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
			int Size = NowSession->m_RecvBuff.GetNotBrokenPutSize();

			// 3. 한 번에 쓸 수 있는 공간이 0이라면
			if (Size == 0)
			{
				// 여유공간이 있을 경우, Size를 1로 변경
				if (NowSession->m_RecvBuff.GetFreeSize() > 0)
					Size = 1;
			}

			// 4. 1칸 이동한 rear 위치 알기(실제 rear 위치가 이동하지는 않음)	
			int* rear = (int*)NowSession->m_RecvBuff.GetWriteBufferPtr();
			int TempRear = NowSession->m_RecvBuff.NextIndex(*rear, 1);

			// 5. wsabuf 셋팅
			wsabuf.buf = &recvBuff[TempRear];
			wsabuf.len = Size;

			flags = 0;

			// 6. recv()
			DWORD recvBytes;
			InterlockedIncrement64((LONG64*)&NowSession->m_IOCount);
			
			ZeroMemory(&NowSession->m_RecvOverlapped, sizeof(NowSession->m_RecvOverlapped));
			retval = WSARecv(NowSession->m_Client_sock, &wsabuf, 1, &recvBytes, &flags, &NowSession->m_RecvOverlapped, NULL);
			NowSession->m_RecvBuff.LeaveLOCK();

			// 7. 에러 처리
			if (retval == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();

				// 비동기 입출력이 시작된게 아니라면
				if (Error != WSA_IO_PENDING)
				{
					_tprintf(L"%d\n", Error);
					// 근데, 버퍼 부족이라면
					if (Error == WSAENOBUFS)
					{
						// 화면에 버퍼부족 출력 
						_tprintf(L"[TCP 서버] 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", NowSession->m_IP, NowSession->m_prot);
					}

					InterlockedDecrement64((LONG64*)&NowSession->m_IOCount);

					NowSession->m_DisconnectFlag = true;
					shutdown(NowSession->m_Client_sock, SD_BOTH);
				}
			}

			
		}

		// send()가 완료된 경우, 로직 처리
		else if (&NowSession->m_SendOverlapped == overlapped && cbTransferred != 0)
		{
			// 샌드 링버퍼 안쓰기 때문에 할거 없음.
		}


		// -----------------------
		// 유저 접속 끊기
		// -----------------------
		// i/o카운트가 0이고 디스커넥트 플래그가 true인 유저는 삭제한다.		
		LockSession();
		map<SOCKET, stSession*>::iterator itor = map_Session.begin();
		while (itor != map_Session.end())
		{
			if (NowSession->m_DisconnectFlag == true && NowSession->m_IOCount == 0)
			{
				// map에서 제거 후, Disconnect()				
				Disconnect(NowSession);
				itor = map_Session.erase(itor);
			}
			else
				++itor;
		}
		UnlockSession();


	}

	return 0;
}

// 접속 종료
void Disconnect(stSession* DeleteSession)
{
	// 이 함수가 호출되었을 때, 세션의 I/O Count가 0이라면 삭제한다.
	if (DeleteSession->m_IOCount > 0)
		return;

	_tprintf(L"[TCP 서버] 접속 종료 : IP 주소=%s, 포트=%d\n", DeleteSession->m_IP, DeleteSession->m_prot);

	// 클로즈 소켓, 세션 동적해제	
	closesocket(DeleteSession->m_Client_sock);

	delete DeleteSession;
}

