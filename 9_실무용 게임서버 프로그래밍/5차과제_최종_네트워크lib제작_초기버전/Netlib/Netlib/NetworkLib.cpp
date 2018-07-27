#include "stdafx.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>


#include <process.h>
#include <Windows.h>

#include "NetworkLib.h"
#include "RingBuff\RingBuff.h"

using namespace Library_Jingyu;




// 생성자
CLanServer::CLanServer()
{
	m_iW_ThreadCount = 0;
	m_iA_ThreadCount = 0;
	m_hWorkerHandle = nullptr;
	m_hIOCPHandle = NULL;
	m_soListen_sock = NULL;
}

// 소멸자
CLanServer::~CLanServer()
{
	// 워커 스레드 종료

	// 워커 스레드 핸들 해제

	// 워커 스레드 핸들 배열 동적해제.
	delete[] m_hWorkerHandle;
}


// enum class
enum class CLanServer::euError : int
{
	NETWORK_LIB_ERROR__WINSTARTUP_FAIL = 0,			// 윈속 초기화 하다가 에러남
	NETWORK_LIB_ERROR__CREATE_IOCP_PORT,			// IPCP 만들다가 에러남
	NETWORK_LIB_ERROR__W_THREAD_FAIL,				// 워커스레드 만들다가 실패 
	NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL,			// 소켓 생성 실패 
	NETWORK_LIB_ERROR__BINDING_FAIL,				// 바인딩 실패
	NETWORK_LIB_ERROR__LISTEN_FAIL,					// 리슨 실패
	NETWORK_LIB_ERROR__SOCKOPT_FAIL,				// 소켓 옵션 변경 실패

};


// 세션 구조체
struct CLanServer::stSession
{
	SOCKET m_Client_sock;

	TCHAR m_IP[30];
	USHORT m_prot;

	ULONGLONG m_ullSessionID;

	LONG	m_lIOCount = 0;

	// Send가능 상태인지 체크. 1이면 Send중, 0이면 Send중 아님
	LONG	m_lSendFlag = 0;

	CRingBuff m_RecvQueue;
	CRingBuff m_SendQueue;

	OVERLAPPED m_overRecvOverlapped;
	OVERLAPPED m_overSendOverlapped;	
};

// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 네이글 옵션, 최대 접속자 수] 입력받음.
bool CLanServer::Start(TCHAR* bindIP, USHORT port, int WorkerThreadCount, bool Nagle, int MaxConnect)
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		// 각종 핸들 반환 및 동적해제 절차.
		ExitFunc(m_iW_ThreadCount);

		// 에러 함수 호출(윈속 초기화) 후 false 리턴
		//OnError((int)euError::NETWORK_LIB_ERROR__WINSTARTUP_FAIL, L"WSAStartup() Error...!");
		OnError(WSAGetLastError(), L"WSAStartup() Error...!");
		return false;
	}


	// 입출력 완료포트 생성
	// 입출력 완료 포트 생성
	m_hIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_hIOCPHandle == NULL)
	{
		// 각종 핸들 반환 및 동적해제 절차.
		ExitFunc(m_iW_ThreadCount);

		// 에러 함수 호출(입출력 완료포트 생성 실패) 후 false 리턴
		//OnError((int)euError::NETWORK_LIB_ERROR__CREATE_IOCP_PORT, L"CreateIoCompletionPort() Error...!");
		OnError(WSAGetLastError(), L"CreateIoCompletionPort() Error...!");
		return false;
	}

	// 워커 스레드 생성
	m_iW_ThreadCount = WorkerThreadCount;
	m_hWorkerHandle = new HANDLE[WorkerThreadCount];

	for (int i = 0; i < m_iW_ThreadCount; ++i)
	{
		// suspend(스레드 동작하지 않음) 상태로 생성.
		// 모든절차가 잘 끝나면 스레드 깨움
		m_hWorkerHandle[i] = (HANDLE)_beginthreadex(0, 0, WorkerThread, 0, 0, 0);
		if (m_hWorkerHandle[i] == 0)
		{
			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(i);

			// 에러 함수 호출(워커스레드 생성 실패) 후 false 리턴
			//OnError((int)euError::NETWORK_LIB_ERROR__W_THREAD_FAIL, L"_beginthreadex() WorkerThread Create Error...!");
			OnError(WSAGetLastError(), L"_beginthreadex() WorkerThread Create Error...!");
			return false;
		}
	}

	// 소켓 생성
	m_soListen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_soListen_sock == INVALID_SOCKET)
	{
		// 각종 핸들 반환 및 동적해제 절차.
		ExitFunc(m_iW_ThreadCount);

		// 에러 함수 호출(소켓 생성 실패 에러) 후 false 리턴
		// OnError((int)euError::NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL, L"socket() Error...!");
		OnError(WSAGetLastError(), L"socket() Error...!");
		return false;
		
	}

	// 바인딩
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	InetPton(AF_INET, bindIP, &serveraddr.sin_addr.s_addr);

	int retval = bind(m_soListen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		// 각종 핸들 반환 및 동적해제 절차.
		ExitFunc(m_iW_ThreadCount);

		// 에러 함수 호출(바인딩 실패) 후 false 리턴
		// OnError((int)euError::NETWORK_LIB_ERROR__BINDING_FAIL, L"bind() Error...!");
		OnError(WSAGetLastError(), L"bind() Error...!");
		return false;
	}

	// 리슨
	if (listen(m_soListen_sock, SOMAXCONN) == SOCKET_ERROR)
	{
		// 각종 핸들 반환 및 동적해제 절차.
		ExitFunc(m_iW_ThreadCount);

		// 에러 함수 호출(리슨 실패) 후 false 리턴
		// OnError((int)euError::NETWORK_LIB_ERROR__LISTEN_FAIL, L"listen() Error...!");
		OnError(WSAGetLastError(), L"listen() Error...!");
		return false;
	}

	// 연결된 리슨 소켓의 송신버퍼 크기를 0으로 변경. 그래야 정상적으로 비동기 입출력으로 실행
	// 리슨 소켓만 바꾸면 모든 클라 송신버퍼 크기는 0이된다.
	int optval = 0;
	if (setsockopt(m_soListen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval)) == SOCKET_ERROR)
	{
		// 각종 핸들 반환 및 동적해제 절차.
		ExitFunc(m_iW_ThreadCount);

		// 에러 함수 호출(소켓 옵션변경 실패) 후 false 리턴
		// OnError((int)euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL, L"setsockopt() Error...!");
		OnError(WSAGetLastError(), L"setsockopt() Error...!");
		return false;
	}

	// 엑셉트 스레드 생성
	m_hAcceptHandle = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, 0, 0, NULL);
	if (m_hAcceptHandle == NULL)
	{
		// 각종 핸들 반환 및 동적해제 절차.
		ExitFunc(m_iW_ThreadCount);

		// 에러 함수 호출(소켓 옵션변경 실패) 후 false 리턴
		OnError(WSAGetLastError(), L"_beginthreadex() Accept Thread Error...!");
		return false;
	}

	// 엑셉트 스레드 수를 1로 변경. 필요 없을수도 있지만 혹시 모르니 카운트
	m_iA_ThreadCount = 1;

	// 서버 열렸음 !!
	return true;
}

// 중간에 무언가 에러났을때 호출하는 함수
// 1. 윈속 해제
// 2. 입출력 완료포트 핸들 반환
// 3. 워커스레드 종료, 핸들반환, 핸들 배열 동적해제
// 4. 엑셉트 스레드 종료, 핸들 반환
// 5. 리슨소켓 닫기
void CLanServer::ExitFunc(int w_ThreadCount)
{
	// 1. 윈속 해제
	// 윈속 초기화 하지 않은 상태에서 WSAClenup() 호출해도 아무 문제 없음
	WSACleanup();

	// 2. 입출력 완료포트 해제 절차.
	// null이 아닐때만! (즉, 입출력 완료포트를 만들었을 경우!)
	// 해당 함수는 어디서나 호출되기때문에, 지금 이걸 해야하는지 항상 확인.
	if (m_hIOCPHandle != NULL)
		CloseHandle(m_hIOCPHandle);

	// 3. 워커스레드 종료 절차. Count가 0이 아닐때만!
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

	// 4. 엑셉트 스레드 종료, 핸들 반환
	if (m_iA_ThreadCount > 0)
		CloseHandle(m_hAcceptHandle);

	// 5. 리슨소켓 닫기
	if (m_soListen_sock != NULL)
		closesocket(m_soListen_sock);
}


// 워커스레드
UINT WINAPI	CLanServer::WorkerThread(LPVOID lParam)
{
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

	while (true)
	{
		addrlen = sizeof(clientaddr);
		client_sock = accept(m_soListen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			// Accept 스레드 정상 종료
			break;
		}

		//InterlockedIncrement(&JoinUser);

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
		map_Session.insert(pair<ULONGLONG, stSession*>(NewSession->m_ullSessionID, NewSession));
		UnlockSession();


		// 소켓과 IOCP 연결
		CreateIoCompletionPort((HANDLE)client_sock, m_hIOCPHandle, (ULONG_PTR)NewSession, 0);


		// -----------
		// 접속 내용 화면에 출력
		// -----------
		//_tprintf(L"[TCP 서버] 클라이언트 접속 : IP 주소=%s, 포트=%d\n", IP, NewSession->m_prot);


		// ------------------
		// 비동기 입출력 시작
		// ------------------
		/*RecvPost_Accept(NewSession);
		AcceptCount++;*/
	}

	return 0;
}


