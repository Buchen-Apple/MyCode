#include "pch.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include <process.h>
#include <strsafe.h>


#include "NetworkLib_MMOServer.h"
#include "LockFree_Queue\LockFree_Queue.h"
#include "RingBuff\RingBuff.h"
#include "Log\Log.h"
#include "CrashDump\CrashDump.h"
#include "NormalTemplate_Queue\Normal_Queue_Template.h"

ULONGLONG g_ullAcceptTotal_MMO;
LONG	  g_lAcceptTPS_MMO;
LONG	g_lSendPostTPS_MMO;



namespace Library_Jingyu
{

#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cMMOServer_Log = CSystemLog::GetInstance();

	// 덤프 남길 변수 하나 받기
	CCrashDump* cMMOServer_Dump = CCrashDump::GetInstance();

	// 한 번에 샌드할 수 있는 WSABUF의 카운트
#define dfSENDPOST_MAX_WSABUF			300

	// ------------------
	// 이너 클래스 
	// ------------------
	struct CMMOServer::stSession
	{
		friend class CMMOServer;

	protected:
		// -----------
		// 상속자 접근 가능 변수
		// -----------

		// Auth To Game Falg
		// true면 Game모드로 전환될 유저
		LONG m_lAuthToGameFlag;

	private:
		// -----------
		// 멤버 변수
		// -----------

		// 유저의 모드 상태
		euSessionModeState m_euMode;

		// 로그아웃 플래그
		// true면 로그아웃 될 유저.
		LONG m_lLogoutFlag;		

		// CompleteRecvPacket(Queue)
		CNormalQueue<CProtocolBuff_Net*>* m_CRPacketQueue;

		// 세션과 연결된 소켓
		SOCKET m_Client_sock;

		// 해당 세션이 들어있는 배열 인덱스
		WORD m_lIndex;

		// 연결된 세션의 IP와 Port
		TCHAR m_IP[30];
		USHORT m_prot;

		// Send overlapped구조체
		OVERLAPPED m_overSendOverlapped;

		// 세션 ID. 컨텐츠와 통신 시 사용.
		ULONGLONG m_ullSessionID;

		// I/O 카운트. WSASend, WSARecv시 1씩 증가.
		// 0이되면 접속 종료된 유저로 판단.
		// 사유 : 연결된 유저는 WSARecv를 무조건 걸기 때문에 0이 될 일이 없다.
		LONG	m_lIOCount;

		// 현재, WSASend에 몇 개의 데이터를 샌드했는가. (바이트 아님! 카운트. 주의!)
		int m_iWSASendCount;

		// Send한 직렬화 버퍼들 저장할 포인터 변수
		CProtocolBuff_Net* m_PacketArray[dfSENDPOST_MAX_WSABUF];

		// Send가능 상태인지 체크. 1이면 Send중, 0이면 Send중 아님
		LONG	m_lSendFlag;

		// Send버퍼. 락프리큐 구조. 패킷버퍼(직렬화 버퍼)의 포인터를 다룬다.
		CLF_Queue<CProtocolBuff_Net*>* m_SendQueue;

		// Recv overlapped구조체
		OVERLAPPED m_overRecvOverlapped;

		// Recv버퍼. 일반 링버퍼. 
		CRingBuff m_RecvQueue;			

	public:
		// -----------------
		// 생성자와 소멸자
		// -----------------
		stSession()
		{
			m_SendQueue = new CLF_Queue<CProtocolBuff_Net*>(0, false);
			m_CRPacketQueue = new CNormalQueue<CProtocolBuff_Net*>;
			m_lIOCount = 0;
			m_lSendFlag = FALSE;
			m_iWSASendCount = 0;
			m_euMode = euSessionModeState::MODE_NONE;
			m_lLogoutFlag = FALSE;
			m_lAuthToGameFlag = FALSE;
		}

		~stSession()
		{
			delete m_SendQueue;
			delete m_CRPacketQueue;
		}

	public:
		// -----------------
		// 순수 가상함수
		// -----------------

		// Auth 스레드에서 처리
		virtual void OnAuth_ClientJoin() = 0;
		virtual void OnAuth_ClientLeave() = 0;
		virtual void OnAuth_Packet(CProtocolBuff_Net* Packet) = 0;

		// Game 스레드에서 처리
		virtual void OnGame_ClientJoin() = 0;
		virtual void OnGame_ClientLeave() = 0;
		virtual void OnGame_Packet(CProtocolBuff_Net* Packet) = 0;

		// Release용
		virtual void OnGame_Release() = 0;

	};


	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	CMMOServer::CMMOServer()
	{
		// 서버 가동상태 false로 시작 
		m_bServerLife = false;

		// Auth에게 전달할 일감 Pool 동적할당
		m_pAcceptPool = new CMemoryPoolTLS<stAuthWork>(0, false);

		// Auth에게 전달할 일감 보관 큐
		m_pASQ = new CLF_Queue< stAuthWork*>(0);
	}

	CMMOServer::~CMMOServer()
	{
		// 서버가 가동중이었으면, 서버 종료절차 진행
		if (m_bServerLife == true)
			Stop();
	}


	// -----------------------
	// 내부에서만 사용하는 함수
	// -----------------------

	// Start에서 에러가 날 시 호출하는 함수.
	// 1. 입출력 완료포트 핸들 반환
	// 2. 워커스레드 종료, 핸들반환, 핸들 배열 동적해제
	// 3. 엑셉트 스레드 종료, 핸들 반환
	// 4. 리슨소켓 닫기
	// 5. 윈속 해제
	void CMMOServer::ExitFunc(int w_ThreadCount)
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

		// 3. 엑셉트 스레드 종료, 핸들 반환
		if (m_iA_ThreadCount > 0)
			CloseHandle(m_hAcceptHandle);

		// 4. 리슨소켓 닫기
		if (m_soListen_sock != NULL)
			closesocket(m_soListen_sock);

		// 5. 윈속 해제
		// 윈속 초기화 하지 않은 상태에서 WSAClenup() 호출해도 아무 문제 없음
		WSACleanup();
	}

	// 각종 변수들을 리셋시킨다.
	// Stop() 함수에서 사용.
	void CMMOServer::Reset()
	{
		m_iW_ThreadCount = 0;
		m_iA_ThreadCount = 0;
		m_hWorkerHandle = nullptr;
		m_hIOCPHandle = 0;
		m_soListen_sock = 0;
		m_iMaxJoinUser = 0;
		m_ullJoinUserCount = 0;
	}

	// RecvPost함수
	//
	// return 0 : 성공적으로 WSARecv() 완료
	// return 1 : RecvQ가 꽉찬 유저
	// return 2 : I/O 카운트가 0이되어 삭제된 유저
	int CMMOServer::RecvPost(stSession* NowSession)
	{
		// ------------------
		// 비동기 입출력 시작
		// ------------------
		// 1. WSABUF 셋팅.
		WSABUF wsabuf[2];
		int wsabufCount = 0;

		int FreeSize = NowSession->m_RecvQueue.GetFreeSize();
		int Size = NowSession->m_RecvQueue.GetNotBrokenPutSize();

		// 리시브 링버퍼 크기 확인
		// 빈공간이 없으면 1리턴
		if (FreeSize <= 0)
			return 1;

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
				if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
				{
					// 로그아웃 플래그 변경
					NowSession->m_lLogoutFlag = TRUE;
					return 2;
				}

				// 에러가 버퍼 부족이라면, I/O카운트 차감이 끝이 아니라 끊어야한다.
				if (Error == WSAENOBUFS)
				{
					// 내 에러, 윈도우에러 보관
					m_iOSErrorCode = Error;
					m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

					// 에러 스트링 만들고
					TCHAR tcErrorString[300];
					StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %lld, [%s:%d]"),
						NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

					// 로그 찍기 (로그 레벨 : 에러)
					cMMOServer_Log->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSARecv --> %s : NetError(%d), OSError(%d)",
						tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

					// 에러 함수 호출
					OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);

					// 끊어야하니 셧다운 호출
					shutdown(NowSession->m_Client_sock, SD_BOTH);
				}
			}
		}

		return 0;

	}






	// -----------------------
	// 스레드들
	// -----------------------

	// 워커 스레드
	UINT	WINAPI	CMMOServer::WorkerThread(LPVOID lParam)
	{

	}

	// Accept 스레드
	UINT	WINAPI	CMMOServer::AcceptThread(LPVOID lParam)
	{
		// --------------------------
		// Accept 파트
		// --------------------------
		SOCKET client_sock;
		SOCKADDR_IN	clientaddr;
		int addrlen = sizeof(clientaddr);

		CMMOServer* g_This = (CMMOServer*)lParam;

		// 유저가 접속할 때 마다 1씩 증가하는 고유한 키.
		ULONGLONG ullUniqueSessionID = 0;
		TCHAR tcTempIP[30];
		USHORT port;
		WORD iIndex;

		while (1)
		{
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			client_sock = accept(g_This->m_soListen_sock, (SOCKADDR*)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET)
			{
				int Error = WSAGetLastError();
				// 10004번(WSAEINTR) 에러면 정상종료. 함수호출이 중단되었다는 것.
				// 10038번(WSAEINTR) 은 소켓이 아닌 항목에 작업을 시도했다는 것. 이미 리슨소켓은 closesocket된것이니 에러 아님.
				if (Error == WSAEINTR || Error == WSAENOTSOCK)
				{
					// Accept 스레드 정상 종료
					break;
				}

				// 그게 아니라면 OnError 함수 호출
				// 윈도우 에러, 내 에러 보관
				g_This->m_iOSErrorCode = Error;
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, L"accpet(). Abonormal_exit : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT, tcErrorString);

				break;
			}

			g_ullAcceptTotal_MMO++;	// 테스트용!!
			InterlockedIncrement(&g_lAcceptTPS_MMO); // 테스트용!!

			// ------------------
			// 최대 접속자 수 이상 접속 불가
			// ------------------
			if (g_This->m_iMaxJoinUser <= g_This->m_ullJoinUserCount)
			{
				closesocket(client_sock);

				// 내 에러 보관
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__JOIN_USER_FULL;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, L"accpet(). User Full!!!! (%lld)",
					g_This->m_ullJoinUserCount);

				// 에러 발생 함수 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__JOIN_USER_FULL, tcErrorString);

				// continue
				continue;
			}
			
			// ------------------
			// 일감 Alloc
			// ------------------
			stAuthWork* NowWork = g_This->m_pAcceptPool->Alloc();



			// ------------------
			// IP와 포트 알아오기.
			// ------------------
			InetNtop(AF_INET, &clientaddr.sin_addr, NowWork->m_tcIP, 30);
			NowWork->m_usPort = ntohs(clientaddr.sin_port);	


			// Auth에게 전달
			g_This->m_pASQ->Enqueue(NowWork);

		}

	}

	// Auth 스레드
	UINT	WINAPI	CMMOServer::AuthThread(LPVOID lParam)
	{
		CMMOServer* g_This = (CMMOServer*)lParam;

		// 유저가 접속할 때 마다 1씩 증가하는 고유한 키.
		ULONGLONG ullUniqueSessionID = 0;

		// 인덱스 보관 변수
		WORD iIndex;		

		// AUTH모드에서 한 번에 처리하는 패킷의 수. 
		const int PACKET_WORK_COUNT = 1;

		while (1)
		{
			// ------------------
			// 1미리 세컨드마다 1회씩 일어난다.
			// ------------------	



			// ------------------
			// Part 1. 신규 접속자 패킷 처리
			// ------------------
			int Size = g_This->m_pASQ->GetInNode();

			int i = 0;
			stAuthWork* NowWork;
			while (i < Size)
			{				
				// 1. 일감 디큐
				if (g_This->m_pASQ->Dequeue(NowWork) == -1)
					cMMOServer_Dump->Crash();


				// 2. 접속 직후, IP등을 판단해서 무언가 추가 작업이 필요할 경우가 있을 수도 있으니 호출	
				// false면 접속거부, 
				// true면 접속 계속 진행. true에서 할게 있으면 OnConnectionRequest함수 인자로 뭔가를 던진다.
				if (g_This->OnConnectionRequest(NowWork->m_tcIP, NowWork->m_usPort) == false)
					continue;
				

				// 3. 세션 구조체 생성 후 셋팅
				// 1) 미사용 인덱스 알아오기
				iIndex = g_This->m_stEmptyIndexStack->Pop();

				// 2) I/O 카운트 증가
				// 삭제 방어
				InterlockedIncrement(&g_This->m_stSessionArray[iIndex].m_lIOCount);

				// 3) 정보 셋팅하기
				// -- 세션 ID(믹스 키)와 인덱스 할당
				ULONGLONG MixKey = ((ullUniqueSessionID << 16) | iIndex);
				ullUniqueSessionID++;

				g_This->m_stSessionArray[iIndex].m_ullSessionID = MixKey;
				g_This->m_stSessionArray[iIndex].m_lIndex = iIndex;

				// -- 소켓
				g_This->m_stSessionArray[iIndex].m_Client_sock = NowWork->m_clienet_Socket;

				// -- IP와 Port
				StringCchCopy(g_This->m_stSessionArray[iIndex].m_IP, _MyCountof(g_This->m_stSessionArray[iIndex].m_IP), NowWork->m_tcIP);
				g_This->m_stSessionArray[iIndex].m_prot = NowWork->m_usPort;

				// 4) 셋팅이 모두 끝났으면 Auth 상태로 변경
				g_This->m_stSessionArray[iIndex].m_euMode = euSessionModeState::MODE_AUTH;


				// 4. IOCP 연결
				// 소켓과 IOCP 연결
				if (CreateIoCompletionPort((HANDLE)NowWork->m_clienet_Socket, g_This->m_hIOCPHandle, (ULONG_PTR)&g_This->m_stSessionArray[iIndex], 0) == NULL)
				{
					// 윈도우 에러, 내 에러 보관
					g_This->m_iOSErrorCode = WSAGetLastError();
					g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_IOCPCONNECT_FAIL;

					// 에러 스트링 만들고
					TCHAR tcErrorString[300];
					StringCchPrintf(tcErrorString, 300, L"accpet(). IOCP_Connect Error : NetError(%d), OSError(%d)",
						(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

					// 에러 발생 함수 호출
					g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_IOCPCONNECT_FAIL, tcErrorString);

					break;
				}

				// 접속자 수 증가. disconnect에서도 사용되는 변수이기 때문에 인터락 사용
				InterlockedIncrement(&g_This->m_ullJoinUserCount);
				

				// 5. 모든 접속절차가 완료되었으니 접속 후 처리 함수 호출.
				g_This->m_stSessionArray[iIndex].OnAuth_ClientJoin();
							   

				// 6. 비동기 입출력 시작
				// 리시브 버퍼가 꽉찼으면 1리턴
				int ret = g_This->RecvPost(&g_This->m_stSessionArray[iIndex]);

				// 증가시켰던, I/O카운트 --. 0이라면 로그아웃 플래그 변경.
				if (InterlockedDecrement(&g_This->m_stSessionArray[iIndex].m_lIOCount) == 0)
					g_This->m_stSessionArray[iIndex].m_lLogoutFlag = TRUE;

				// I/O카운트 감소시켰는데 0이 아니라면, ret 체크.
				// ret가 1이라면 접속 끊는다.
				else if (ret == 1)
					shutdown(g_This->m_stSessionArray[iIndex].m_Client_sock, SD_BOTH);
			}



			// ------------------
			// Part 2. AUTH모드 세션들 패킷 처리 + Logout Flag 처리 1차
			// ------------------
			i = 0;
			while (i < g_This->m_iMaxJoinUser)
			{
				// 해당 유저가 MODE_AUTH인지 확인 ---------------------------
				if (g_This->m_stSessionArray[i].m_euMode == euSessionModeState::MODE_AUTH)
				{

					// LogOutFlag가 TRUE라면 
					if (g_This->m_stSessionArray[i].m_lLogoutFlag = TRUE)
					{
						// 모드를 MODE_LOGOUT_IN_AUTH로 변경
						g_This->m_stSessionArray[i].m_euMode = euSessionModeState::MODE_LOGOUT_IN_AUTH;
					}

					// LogOutFlag가 FALSE인 유저만 정상 로직 처리
					else
					{
						// 1. CompleteRecvPacket 큐의 사이즈 확인.
						int iQSize = g_This->m_stSessionArray[i].m_CRPacketQueue->GetNodeSize();

						// 2. 큐에 노드가 1개 이상 있으면, 패킷 처리
						if (iQSize > 0)
						{
							// !! 패킷 처리 횟수 제한을 두기 위해, PACKET_WORK_COUNT보다 노드의 수가 더 많으면, PACKET_WORK_COUNT로 변경 !!
							if (iQSize > PACKET_WORK_COUNT)
								iQSize = PACKET_WORK_COUNT;

							CProtocolBuff_Net* NowPacket;

							int TempCount = 0;
							while (TempCount < iQSize)
							{
								// 노드 빼기
								if (g_This->m_stSessionArray[i].m_CRPacketQueue->Dequeue(NowPacket) == -1)
									cMMOServer_Dump->Crash();

								// 패킷 래퍼런스 카운트 1 증가
								NowPacket->Add();

								// 패킷 처리
								g_This->m_stSessionArray[i].OnAuth_Packet(NowPacket);

								// 패킷 래퍼런스 카운트 1 감소
								CProtocolBuff_Net::Free(NowPacket);

								// 반복문 카운트 증가.
								TempCount++;
							}
						}
					}
				}				
				

				++i;
			}



			// ------------------
			// Part 3. OnAuth_Update
			// ------------------
			g_This->OnAuth_Update();



			// ------------------
			// Part 4. Logout Flag 처리 2차 + Auth To Game 처리
			// ------------------
			i = 0;
			while (i < g_This->m_iMaxJoinUser)
			{
				// 해당 유저가 MODE_LOGOUT_IN_AUTH인지 확인 ---------------------------
				if (g_This->m_stSessionArray[i].m_euMode == euSessionModeState::MODE_LOGOUT_IN_AUTH)
				{
					// SendFlag가 FALSE인지 확인
					if (InterlockedOr(&g_This->m_stSessionArray[i].m_lSendFlag, 0) == FALSE)
					{
						// 나갔다고 알려준다.
						// 모드 변경 후 알려주면, 아직 알려주기 전에, GAME쪽에서 Release되어 Release가 먼저 뜰 가능성.
						g_This->m_stSessionArray[i].OnAuth_ClientLeave();

						// MODE_WAIT_LOGOUT으로 모드 변경
						g_This->m_stSessionArray[i].m_euMode = euSessionModeState::MODE_WAIT_LOGOUT;						
					}

					// 혹은 Auth TO Game 플래그 확인
					else if (g_This->m_stSessionArray[i].m_lAuthToGameFlag == TRUE)
					{
						// 나갔다고 알려준다.
						// 모드 변경 후 알려주면, OnAuth_ClientLeave가 호출되기도 전에, GAME쪽에서 먼저 OnGame_ClinetJoin 뜰 가능성
						g_This->m_stSessionArray[i].OnAuth_ClientLeave();

						// MODE_AUTH_TO_GAME으로 모드 변경
						g_This->m_stSessionArray[i].m_euMode = euSessionModeState::MODE_AUTH_TO_GAME;

					}
				}
			}		
		}
		
	}

	// Game 스레드
	UINT	WINAPI	CMMOServer::GameThread(LPVOID lParam)
	{}



	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 세션 셋팅
	//
	// Parameter : stSession* 포인터, Max 수(최대 접속 가능 유저 수)
	// return : 없음
	void CMMOServer::SetSession(stSession* pSession, int Max)
	{
		// 세션 배열 셋팅
		for (int i = 0; i < Max; ++i)
			m_stSessionArray[i] = pSession[i];

	}





	// -----------------------
	// 상속에서만 호출 가능한 함수
	// -----------------------
	
	// 서버 시작
	// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 활성화시킬 워커스레드 수, 엑셉트 스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수, 패킷 Code, XOR 1번코드, XOR 2번코드] 입력받음.
	//
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CMMOServer::Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int ActiveWThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect,
		BYTE Code, BYTE XORCode1, BYTE XORCode2)
	{

		// rand설정
		srand((UINT)time(NULL));

		// Config 데이터 셋팅
		m_bCode = Code;
		m_bXORCode_1 = XORCode1;
		m_bXORCode_2 = XORCode2;

		// 새로 시작하니까 에러코드들 초기화
		m_iOSErrorCode = 0;
		m_iMyErrorCode = (euError)0;

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
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WSAStartup() Error : NetError(%d), OSError(%d)",
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
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> CreateIoCompletionPort() Error : NetError(%d), OSError(%d)",
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
				cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WorkerThread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}

		// 소켓 생성
		m_soListen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_soListen_sock == INVALID_SOCKET)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> socket() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;

		}

		// 바인딩
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		InetPton(AF_INET, bindIP, &serveraddr.sin_addr.s_addr);

		retval = bind(m_soListen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__BINDING_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> bind() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 리슨
		retval = listen(m_soListen_sock, SOMAXCONN);
		if (retval == SOCKET_ERROR)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__LISTEN_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> listen() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}		

		// 인자로 받은 노딜레이 옵션 사용 여부에 따라 네이글 옵션 결정
		// 이게 true면 노딜레이 사용하겠다는 것(네이글 중지시켜야함)
		if (Nodelay == true)
		{
			BOOL optval = TRUE;
			retval = setsockopt(m_soListen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));

			if (retval == SOCKET_ERROR)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = WSAGetLastError();
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> setsockopt() Nodelay apply Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}

		// 최대 접속 가능 유저 수 셋팅
		m_iMaxJoinUser = MaxConnect;

		// 미사용 세션 관리 스택 동적할당. (락프리 스택) 
		// 그리고 미리 Max만큼 만들어두기
		m_stEmptyIndexStack = new CLF_Stack<WORD>;
		for (int i = 0; i < MaxConnect; ++i)
			m_stEmptyIndexStack->Push(i);

		// 엑셉트 스레드 생성
		m_iA_ThreadCount = AcceptThreadCount;
		m_hAcceptHandle = new HANDLE[m_iA_ThreadCount];
		for (int i = 0; i < m_iA_ThreadCount; ++i)
		{
			m_hAcceptHandle[i] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
			if (m_hAcceptHandle == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_CREATE_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Accept Thread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}



		// 서버 열렸음 !!
		m_bServerLife = true;

		// 서버 오픈 로그 찍기		
		// 이건, 상속받는 쪽에서 찍는걸로 수정. 넷서버 자체는 독립적으로 작동하지 않음.
		//cNetLibLog->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}


	// 서버 스탑.
	void CMMOServer::Stop()
	{
		// 1. Accept 스레드 종료. 더 이상 접속을 받으면 안되니 Accept스레드 먼저 종료
		// Accept 스레드는 리슨소켓을 closesocket하면 된다.
		closesocket(m_soListen_sock);

		// Accept스레드 종료 대기
		DWORD retval = WaitForMultipleObjects(m_iA_ThreadCount, m_hAcceptHandle, TRUE, INFINITE);

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

			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Accept Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Accept Thread EXIT Error");
		}

		// 2. 모든 유저에게 Shutdown
		// 모든 유저에게 셧다운 날림
		for (int i = 0; i < m_iMaxJoinUser; ++i)
		{
			shutdown(m_stSessionArray[i].m_Client_sock, SD_BOTH);


			/*if (m_stSessionArray[i].m_lReleaseFlag == FALSE)
			{
				shutdown(m_stSessionArray[i].m_Client_sock, SD_BOTH);
			}*/
		}

		// 모든 유저가 종료되었는지 체크
		while (1)
		{
			if (m_ullJoinUserCount == 0)
				break;

			Sleep(1);
		}

		// 3. 워커 스레드 종료
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			PostQueuedCompletionStatus(m_hIOCPHandle, 0, 0, 0);

		// 워커스레드 종료 대기
		retval = WaitForMultipleObjects(m_iW_ThreadCount, m_hWorkerHandle, TRUE, INFINITE);

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
			cMMOServer_Log->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Worker Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT, L"Stop() --> Worker Thread EXIT Error");
		}

		// 4. 각종 리소스 반환
		// 1) 엑셉트 스레드 핸들 반환
		for (int i = 0; i < m_iA_ThreadCount; ++i)
			CloseHandle(m_hAcceptHandle[i]);

		// 2) 워커 스레드 핸들 반환
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			CloseHandle(m_hWorkerHandle[i]);

		// 3) 워커 스레드 배열, 엑셉트 스레드 배열 동적해제
		delete[] m_hWorkerHandle;
		delete[] m_hAcceptHandle;

		// 4) IOCP핸들 반환
		CloseHandle(m_hIOCPHandle);

		// 5) 윈속 해제
		WSACleanup();

		// 6) 세션 배열, 세션 미사용 인덱스 관리 스택 동적해제
		delete[] m_stSessionArray;
		delete m_stEmptyIndexStack;

		// 5. 서버 가동중 아님 상태로 변경
		m_bServerLife = false;

		// 6. 각종 변수 초기화
		Reset();

		// 7. 서버 종료 로그 찍기		
		cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");

	}



}