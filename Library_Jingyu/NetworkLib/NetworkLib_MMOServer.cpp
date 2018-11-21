#include "pch.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include <process.h>
#include <strsafe.h>

#include "NetworkLib_MMOServer.h"
#include "Log\Log.h"
#include "CrashDump\CrashDump.h"
#include "Parser\Parser_Class.h"


// ------------------------
// cSession의 함수
// (MMOServer의 이너클래스)
// ------------------------
namespace Library_Jingyu
{
	// 생성자
	CMMOServer::cSession::cSession()
	{
		//m_SendQueue = new CLF_Queue<CProtocolBuff_Net*>(0, false);
		m_SendQueue = new CNormalQueue<CProtocolBuff_Net*>;
		m_CRPacketQueue = new CNormalQueue<CProtocolBuff_Net*>;
		m_lIOCount = 0;
		m_lSendFlag = FALSE;
		m_iWSASendCount = 0;
		m_euMode = euSessionModeState::MODE_NONE;
		m_lLogoutFlag = FALSE;
		m_lAuthToGameFlag = FALSE;
	}

	// 소멸자
	CMMOServer::cSession::~cSession()
	{
		delete m_SendQueue;
		delete m_CRPacketQueue;
	}


	// 지정한 유저를 끊을 때 호출하는 함수. 외부 에서 사용.
	// 라이브러리한테 끊어줘!라고 요청하는 것 뿐
	//
	// Parameter : 없음
	// return : 없음
	void CMMOServer::cSession::Disconnect()
	{
		// 해당 유저에 대해,셧다운 날린다.
		// 차후 자연스럽게 I/O카운트가 감소되어서 InDisconnect된다.
		shutdown(m_Client_sock, SD_BOTH);
	}

	// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
	// SendPacket은 그냥 아무때나 하면 된다.
	// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
	//
	// Parameter : SessionID, SendBuff, LastFlag(디폴트 FALSE)
	// return : 없음
	void CMMOServer::cSession::SendPacket(CProtocolBuff_Net* payloadBuff, LONG LastFlag)
	{
		// 1. m_LastPacket가 nullptr이 아니라면, 보내고 끊을 유저. 큐에 넣지 않는다.
		if (m_LastPacket != nullptr)
		{
			CProtocolBuff_Net::Free(payloadBuff);
			return;
		}

		// 2. 보내고 끊을 유저라면, 더 이상 패킷 넣지 않는다.
		if (m_LastPacket != nullptr)
		{
			CProtocolBuff_Net::Free(payloadBuff);
			return;
		}

		// 3. 인자로 받은 Flag가 true라면, 마지막 패킷의 주소를 보관
		if (LastFlag == TRUE)
			m_LastPacket = payloadBuff;		

		// 3. 인큐. 패킷의 "주소"를 인큐한다(8바이트)
		// 차후, 문제가 생기면 패킷 래퍼런스 카운트 증가시키는 것 고려
		m_SendQueue->Enqueue(payloadBuff);
	}

	// 해당 유저의 모드를 GAME모드로 변경하는 함수
	//
	// Parameter : 없음
	// return : 없음
	void CMMOServer::cSession::SetMode_GAME()
	{
		m_lAuthToGameFlag = TRUE;
	}


	// Auth 스레드에서 처리
	void CMMOServer::cSession::OnAuth_ClientJoin() {}
	void CMMOServer::cSession::OnAuth_ClientLeave(bool bGame) {}
	void CMMOServer::cSession::OnAuth_Packet(CProtocolBuff_Net* Packet) {}

	// Game 스레드에서 처리
	void CMMOServer::cSession::OnGame_ClientJoin() {}
	void CMMOServer::cSession::OnGame_ClientLeave() {}
	void CMMOServer::cSession::OnGame_Packet(CProtocolBuff_Net* Packet) {}

	// Release용
	void CMMOServer::cSession::OnGame_ClientRelease() {}
}



// ------------------------
// MMOServer
// ------------------------
namespace Library_Jingyu
{

#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cMMOServer_Log = CSystemLog::GetInstance();

	// 덤프 남길 변수 하나 받기
	CCrashDump* cMMOServer_Dump = CCrashDump::GetInstance();

	// 헤더 사이즈
#define dfNETWORK_PACKET_HEADER_SIZE_NETSERVER	5
	   
	// 헤더 구조체
#pragma pack(push, 1)
	struct CMMOServer::stProtocolHead
	{
		BYTE	m_Code;
		WORD	m_Len;
		BYTE	m_RandXORCode;
		BYTE	m_Checksum;
	};
#pragma pack(pop)
	  

	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CMMOServer::CMMOServer()
	{
		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			cMMOServer_Dump->Crash();

		// 값을 -1로 셋팅
		m_lSendConfigIndex = -1;

		// 서버 가동상태 false로 시작 
		m_bServerLife = false;

		// Auth에게 전달할 일감 Pool 동적할당
		m_pAcceptPool = new CMemoryPoolTLS<stAuthWork>(0, false);

		// Auth에게 전달할 일감 보관 큐
		m_pASQ = new CLF_Queue< stAuthWork*>(0);

		// 미사용 인덱스 nullptr로 셋팅.
		m_stEmptyIndexStack = nullptr;

		// 세션 배열 nullptr로 셋팅
		m_stSessionArray = nullptr;

		// 스레드 수들 0으로 셋팅
		m_iA_ThreadCount = 0;
		m_iW_ThreadCount = 0;
		m_iS_ThreadCount = 0;
		m_iAuth_ThreadCount = 0;
		m_iGame_ThreadCount = 0;

		// Auth 스레드 종료용, Game 스레드 종료용, Send 스레드 종료용, Release 스레드 종료용 이벤트
		// 
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		m_hAuthExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hGameExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hSendExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hReleaseExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	// 소멸자
	CMMOServer::~CMMOServer()
	{
		// 서버가 가동중이었으면, 서버 종료절차 진행
		if (m_bServerLife == true)
			Stop();

		// 종료용 이벤트 해제
		CloseHandle(m_hAuthExitEvent);
		CloseHandle(m_hGameExitEvent);
		CloseHandle(m_hSendExitEvent);
	}
	



	// -----------------------
	// 게터 함수
	// -----------------------

	// 서버 작동중인지 확인
	//
	// Parameter : 없음
	// return : 작동중일 시 true.
	bool CMMOServer::GetServerState()
	{
		return m_bServerLife;
	}

	// 접속 중인 세션 수 얻기
	//
	// Parameter : 없음
	// return : 접속자 수 (ULONGLONG)
	ULONGLONG CMMOServer::GetClientCount()
	{
		return m_ullJoinUserCount;
	}

	// Accept Socket Queue 안의 일감 수 얻기
	//
	// Parameter : 없음
	// return : 일감 수(LONG)
	LONG CMMOServer::GetASQ_Count()
	{
		return m_pASQ->GetInNode();
	}

	// Accept Socket Queue Pool의 총 청크 수 얻기
	// 
	// Parameter : 없음
	// return : 총 청크 수 (LONG)
	LONG CMMOServer::GetChunkCount()
	{
		return m_pAcceptPool->GetAllocChunkCount();
	}

	// Accept Socket Queue Pool의 밖에서 사용중인 청크 수 얻기
	// 
	// Parameter : 없음
	// return : 밖에서 사용중인 청크 수 (LONG)
	LONG CMMOServer::GetOutChunkCount()
	{
		return m_pAcceptPool->GetOutChunkCount();
	}


	// AcceptTotal 얻기
	ULONGLONG CMMOServer::GetAccpetTotal()
	{
		return m_ullAcceptTotal;
	}

	// AcceptTPS 얻기
	LONG CMMOServer::GetAccpetTPS()
	{
		LONG ret = InterlockedExchange(&m_lAcceptTPS, 0);

		return ret;
	}

	// SendTPS 얻기
	LONG CMMOServer::GetSendTPS()
	{
		LONG ret = InterlockedExchange(&m_lSendPostTPS, 0);

		return ret;
	}

	// RecvTPS 얻기
	LONG CMMOServer::GetRecvTPS()
	{
		LONG ret = InterlockedExchange(&m_lRecvTPS, 0);

		return ret;
	}

	// Auth모드 유저 수 얻기
	LONG CMMOServer::GetAuthModeUserCount()
	{
		return m_lAuthModeUserCount;
	}

	// Game모드 유저 수 얻기
	LONG CMMOServer::GetGameModeUserCount()
	{
		return m_lGameModeUserCount;
	}

	// AuthFPS 얻기
	LONG CMMOServer::GetAuthFPS()
	{
		LONG ret = InterlockedExchange(&m_lAuthFPS, 0);

		return ret;
	}

	// GameFPS 얻기
	LONG CMMOServer::GetGameFPS()
	{
		LONG ret = InterlockedExchange(&m_lGameFPS, 0);

		return ret;
	}




	// -----------------------
	// 내부에서만 사용하는 함수
	// -----------------------
	
	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CMMOServer::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MMOGameServer_Config.ini"));
		}
		catch (int expn)
		{
			if (expn == 1)
			{
				printf("File Open Fail...\n");
				return false;
			}
			else if (expn == 2)
			{
				printf("FileR Read Fail...\n");
				return false;
			}
		}

		////////////////////////////////////////////////////////
		// MMOServer config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MMOSERVER")) == false)
			return false;

		// Auth 스레드의, 유저 1명당 패킷 처리 수 
		if (Parser.GetValue_Int(_T("AuthPacketCount"), &pConfig->AuthPacket_Count) == false)
			return false;

		// Auth 스레드 슬립 
		if (Parser.GetValue_Int(_T("AuthSleep"), &pConfig->AuthSleep) == false)
			return false;

		// Auth 스레드의, 1프레임에 Accept Socket Queue에서 빼는 패킷 수
		if (Parser.GetValue_Int(_T("AuthNewUSerPacketCount"), &pConfig->AuthNewUser_PacketCount) == false)
			return false;



		// Game 스레드의, 유저 1명당 패킷 처리 수 
		if (Parser.GetValue_Int(_T("GamePacketCount"), &pConfig->GamePacket_Count) == false)
			return false;

		// Game 스레드 슬립
		if (Parser.GetValue_Int(_T("GameSleep"), &pConfig->GameSleep) == false)
			return false;

		// 1프레임에 AUTH_IN_GAME 에서 GAME으로 변경되는 수
		if (Parser.GetValue_Int(_T("GameNewUSerPacketCount"), &pConfig->GameNewUser_PacketCount) == false)
			return false;



		// Release 스레드 슬립
		if (Parser.GetValue_Int(_T("ReleaseSleep"), &pConfig->ReleaseSleep) == false)
			return false;



		// Send 스레드 슬립
		if (Parser.GetValue_Int(_T("SendSleep"), &pConfig->SendSleep) == false)
			return false;

		// Send 스레드 생성 수
		if (Parser.GetValue_Int(_T("MaxSendThread"), &pConfig->CreateSendThreadCount) == false)
			return false;


		return true;
	}


	// Start에서 에러가 날 시 호출하는 함수.
	//
	// Parameter : 워커스레드의 수
	// return : 없음
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

		// 4. 샌드 스레드 종료, 핸들 반환
		if (m_iS_ThreadCount > 0)
			CloseHandle(m_hSendHandle);

		// 5. 어스 스레드 종료, 핸들 반환
		if (m_iAuth_ThreadCount > 0)
			CloseHandle(m_hAuthHandle);

		// 6. 게임 스레드 종료, 핸들 반환
		if (m_iGame_ThreadCount > 0)
			CloseHandle(m_hGameHandle);

		// 7. 리슨소켓 닫기
		if (m_soListen_sock != NULL)
			closesocket(m_soListen_sock);

		// 8. 미사용 인덱스 동적해제.
		if (m_stEmptyIndexStack != nullptr)
			delete m_stEmptyIndexStack;

		// 9. 윈속 해제
		// 윈속 초기화 하지 않은 상태에서 WSAClenup() 호출해도 아무 문제 없음
		WSACleanup();
	}

	// 각종 변수들을 리셋시킨다.
	// Stop() 함수에서 사용.
	void CMMOServer::Reset()
	{
		// 미사용 인덱스 nullptr로 셋팅.
		m_stEmptyIndexStack = nullptr;

		// 스레드 수들 0으로 셋팅
		m_iA_ThreadCount = 0;
		m_iW_ThreadCount = 0;
		m_iS_ThreadCount = 0;
		m_iAuth_ThreadCount = 0;
		m_iGame_ThreadCount = 0;

		m_hIOCPHandle = 0;
		m_soListen_sock = 0;
		m_iMaxJoinUser = 0;
		m_ullJoinUserCount = 0;
	}
	   	
	// RecvProc 함수.
	// 완성된 패킷을 인자로받은 Session의 CompletionRecvPacekt (Queue)에 넣는다.
	//
	// Parameter : 세션 포인터
	// return : 없음
	void CMMOServer::RecvProc(cSession* NowSession)
	{
		// -----------------
		// Recv 큐 관련 처리
		// -----------------

		while (1)
		{
			// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
			// 헤더의 구조 -----------> [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] 
			stProtocolHead Header;

			// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
			int UseSize = NowSession->m_RecvQueue.GetUseSize();
			if (UseSize < dfNETWORK_PACKET_HEADER_SIZE_NETSERVER)
			{
				break;
			}

			// 2. 헤더를 Peek으로 확인한다. 
			// 버퍼가 비어있는건 말이 안된다. 이미 위에서 있다고 검사했기 때문에. Crash 남김
			if (NowSession->m_RecvQueue.Peek((char*)&Header, dfNETWORK_PACKET_HEADER_SIZE_NETSERVER) == -1)
				cMMOServer_Dump->Crash();

			// 3. 헤더의 코드 확인. 내 것이 맞는지
			if (Header.m_Code != m_bCode)
			{
				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__RECV_CODE_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_CodeError.UserID : %lld, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__RECV_CODE_ERROR, tcErrorString);

				// 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;
			}

			// 4. 헤더 안에 들어있는 Len(페이로드 길이)가 너무 크진 않은지
			WORD PayloadLen = Header.m_Len;
			if (PayloadLen > 512)
			{
				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__RECV_LENBIG_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_LenBig.UserID : %lld, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__RECV_LENBIG_ERROR, tcErrorString);

				// 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;

			}


			// 5. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size)
			// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.			
			if (UseSize < (dfNETWORK_PACKET_HEADER_SIZE_NETSERVER + PayloadLen))
				break;


			// 6. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
			NowSession->m_RecvQueue.RemoveData(dfNETWORK_PACKET_HEADER_SIZE_NETSERVER);


			// 7. 직렬화 버퍼의 rear는 무조건 5부터(앞에 5바이트는 헤더공간)부터 시작한다.
			// 때문에 clear()를 이용해 rear를 0으로 만들어둔다.
			CProtocolBuff_Net* PayloadBuff = CProtocolBuff_Net::Alloc();
			PayloadBuff->Clear();


			// 8. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
			int DequeueSize = NowSession->m_RecvQueue.Dequeue(PayloadBuff->GetBufferPtr(), PayloadLen);

			// 버퍼가 비어있거나, 내가 원하는만큼 데이터가 없었다면, 말이안됨. (위 if문에서는 있다고 했는데 여기오니 없다는것)
			// 로직문제로 보고 서버 종료.
			if ((DequeueSize == -1) || (DequeueSize != PayloadLen))
				cMMOServer_Dump->Crash();


			// 9. 읽어온 만큼 rear를 이동시킨다. 
			PayloadBuff->MoveWritePos(DequeueSize);


			// 10. 헤더 Decode
			if (PayloadBuff->Decode(PayloadLen, Header.m_RandXORCode, Header.m_Checksum, m_bXORCode_1, m_bXORCode_2) == false)
			{
				// 할당받은 패킷 Free
				CProtocolBuff_Net::Free(PayloadBuff);

				// 내 에러 보관. 윈도우 에러는 없음.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__RECV_CHECKSUM_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_Empry.UserID : %lld, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// 에러 함수 호출
				OnError((int)euError::NETWORK_LIB_ERROR__RECV_CHECKSUM_ERROR, tcErrorString);

				// 끊어야하니 셧다운 호출
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// 접속이 끊길 유저이니 더는 아무것도 안하고 리턴
				return;
			}


			// 11. 완성된 패킷을 CompletionRecvPacekt (Queue)에 넣는다.
			InterlockedIncrement(&m_lRecvTPS);
			NowSession->m_CRPacketQueue->Enqueue(PayloadBuff);
		}

		return;
	}
	
	// RecvPost함수
	//
	// return 0 : 성공적으로 WSARecv() 완료
	// return 1 : RecvQ가 꽉찬 유저
	// return 2 : I/O 카운트가 0이되어 삭제된 유저
	int CMMOServer::RecvPost(cSession* NowSession)
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
					cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSARecv --> %s : NetError(%d), OSError(%d)",
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
		CMMOServer* g_This = (CMMOServer*)lParam;

		DWORD cbTransferred;
		cSession* stNowSession;
		OVERLAPPED* overlapped;

		while (1)
		{
			// 변수들 초기화
			cbTransferred = 0;
			stNowSession = nullptr;
			overlapped = nullptr;

			// GQCS 완료 시 함수 호출
			g_This->OnWorkerThreadEnd();

			// 비동기 입출력 완료 대기
			// GQCS 대기
			GetQueuedCompletionStatus(g_This->m_hIOCPHandle, &cbTransferred, (PULONG_PTR)&stNowSession, &overlapped, INFINITE);


			// --------------
			// 완료 체크
			// --------------
			// overlapped가 nullptr이라면, IOCP 에러임.
			if (overlapped == NULL)
			{
				// 셋 다 0이면 스레드 종료.
				if (cbTransferred == 0 && stNowSession == NULL)
				{
					// printf("워커 스레드 정상종료\n");

					break;
				}
				// 그게 아니면 IOCP 에러 발생한 것

				// 윈도우 에러, 내 에러 보관
				g_This->m_iOSErrorCode = GetLastError();
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__IOCP_ERROR;

				// 에러 스트링 만들고
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, L"IOCP_Error --> GQCS return Error : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// OnError 호출
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__IOCP_ERROR, tcErrorString);

				break;
			}

			// GQCS 깨어날 시 함수호출
			g_This->OnWorkerThreadBegin();

			// -----------------
			// Recv 로직
			// -----------------
			// WSArecv()가 완료된 경우, 받은 데이터가 0이 아니면 로직 처리
			if (&stNowSession->m_overRecvOverlapped == overlapped && cbTransferred > 0)
			{
				// rear 이동
				stNowSession->m_RecvQueue.MoveWritePos(cbTransferred);

				// 1. 링버퍼 후, 패킷 처리
				g_This->RecvProc(stNowSession);

				// 2. 리시브 다시 걸기.
				// 리시브 큐가 꽉찼으면 접속 끊는다.
				if (g_This->RecvPost(stNowSession) == 1)
				{
					cMMOServer_Dump->Crash();

					// 셧다운
					shutdown(stNowSession->m_Client_sock, SD_BOTH);					
				}

			}

			// -----------------
			// Send 로직
			// -----------------
			// WSAsend()가 완료된 경우, 받은 데이터가 0이 아니면 로직처리
			else if (&stNowSession->m_overSendOverlapped == overlapped && cbTransferred > 0)
			{
				// !! 테스트 출력용 !!
				// sendpostTPS 추가
				InterlockedAdd(&g_This->m_lSendPostTPS, stNowSession->m_iWSASendCount);

				// 1. 보내고 끊을 유저일 경우 로직
				if (stNowSession->m_LastPacket != nullptr)
				{
					// 직렬화 버퍼 해제
					int i = 0;
					bool Flag = false;
					while (i < stNowSession->m_iWSASendCount)
					{
						// 마지막 패킷이 잘 갔으면, flag를 True로 바꾼다.
						if (stNowSession->m_PacketArray[i] == stNowSession->m_LastPacket)
							Flag = true;

						CProtocolBuff_Net::Free(stNowSession->m_PacketArray[i]);

						++i;
					}

					// 보낸 카운트 0으로 만듬.
					stNowSession->m_iWSASendCount = 0;  

					// 샌드 가능 상태로 변경
					stNowSession->m_lSendFlag = FALSE;

					// 보낸게 잘 갔으면, 해당 유저는 접속을 끊는다.
					if (Flag == true)
					{
						// 셧다운 날림
						// 아래에서 I/O카운트를 1 감소시켜서 0이 되도록 유도
						shutdown(stNowSession->m_Client_sock, SD_BOTH);
					}
				}

				// 2. 보내고 끊을 유저가 아닐 경우 로직
				else
				{
					// 직렬화 버퍼 해제
					int i = 0;
					while (i < stNowSession->m_iWSASendCount)
					{
						CProtocolBuff_Net::Free(stNowSession->m_PacketArray[i]);
						++i;
					}

					// 보낸 카운트 0으로 만듬.	
					stNowSession->m_iWSASendCount = 0; 

					// 샌드 가능 상태로 변경
					stNowSession->m_lSendFlag = FALSE;
				}						
			}

			// -----------------
			// I/O카운트 감소 및 삭제 처리
			// -----------------
			// I/O카운트 감소 후, 0이라면접속 로그아웃 플래그 변경
			if (InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
				stNowSession->m_lLogoutFlag = TRUE;
		}

		return 0;
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

		// -------------- 필요한 변수 받아두기

		// 리슨소켓
		SOCKET Listen_sock = g_This->m_soListen_sock;

		// Accept Socket Pool
		CMemoryPoolTLS<stAuthWork>* AcceptPool = g_This->m_pAcceptPool;

		// Accept Socket Queue
		CLF_Queue<stAuthWork*>* pASQ = g_This->m_pASQ;

		while (1)
		{
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			client_sock = accept(Listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
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

			++g_This->m_ullAcceptTotal;	// 테스트용!!
			InterlockedIncrement(&g_This->m_lAcceptTPS); // 테스트용!!

			
			// ------------------
			// 일감 Alloc
			// ------------------
			stAuthWork* NowWork = AcceptPool->Alloc();



			// ------------------
			// 일감에 정보 셋팅
			// ------------------
			InetNtop(AF_INET, &clientaddr.sin_addr, NowWork->m_tcIP, 30);
			NowWork->m_usPort = ntohs(clientaddr.sin_port);	
			NowWork->m_clienet_Socket = client_sock;


			// Auth에게 전달
			pASQ->Enqueue(NowWork);

		}

		return 0;
	}

	// Auth 스레드
	UINT	WINAPI	CMMOServer::AuthThread(LPVOID lParam)
	{
		CMMOServer* g_This = (CMMOServer*)lParam;

		// 유저가 접속할 때 마다 1씩 증가하는 고유한 키.
		ULONGLONG ullUniqueSessionID = 0;

		// 인덱스 보관 변수
		WORD wIndex;	

		// --------- 필요한 정보들 로컬로 받아두기

		// AUTH모드에서 1명의 유저에게, 1프레임에 처리하는 패킷의 최대 수. 
		const int PACKET_WORK_COUNT = g_This->m_stConfig.AuthPacket_Count;

		// 최대 접속 가능 유저, 로컬로 받아두기
		const int MAX_USER = g_This->m_iMaxJoinUser;

		// Sleep 인자로 받아두기
		const int SLEEP_VALUE = g_This->m_stConfig.AuthSleep;

		// 1프레임에 처리하는 Accept Socket Queue의 수
		const int NEWUSER_PACKET_COUNT = g_This->m_stConfig.AuthNewUser_PacketCount;

		// 종료 이벤트
		HANDLE* ExitEvent = &g_This->m_hAuthExitEvent;

		// Accept Socket Pool
		CMemoryPoolTLS<stAuthWork>* pAcceptPool = g_This->m_pAcceptPool;

		// Accept Socket Queue
		CLF_Queue<stAuthWork*>* pASQ = g_This->m_pASQ;

		// 현재 접속중인 유저 수
		ULONGLONG* ullJoinUserCount = &g_This->m_ullJoinUserCount;

		// 세션 관리 배열
		cSession** SessionArray = g_This->m_stSessionArray;

		// 미사용 인덱스 관리 스택
		CLF_Stack<WORD>* EmptyIndexStack = g_This->m_stEmptyIndexStack;

		while (1)
		{
			// ------------------
			// iSleepValuea 만큼 자다가 일어난다.
			// ------------------	
			DWORD Ret = WaitForSingleObject(*ExitEvent, SLEEP_VALUE);

			// 이상한 신호라면 
			if (Ret == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("AuthThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 정상 종료
			else if (Ret == WAIT_OBJECT_0)
				break;

			// ------------------
			// Part 1. 신규 접속자 패킷 처리
			// ------------------
			int iSize = pASQ->GetInNode();

			// 사이즈가 NEWUSER_PACKET_COUNT보다 크다면 NEWUSER_PACKET_COUNT로 만든다.
			if (iSize > NEWUSER_PACKET_COUNT)
				iSize = NEWUSER_PACKET_COUNT;

			stAuthWork* NowWork;

			// 일감큐에 일이 있을때만 일을 한다.
			// 하나 처리할 때 마다 iSize를 1씩 차감시킨다.
			while (iSize > 0)
			{
				// 1. 일감 디큐
				if (pASQ->Dequeue(NowWork) == -1)
					cMMOServer_Dump->Crash();


				// 2. 최대 접속자 수 이상 접속 불가
				if (MAX_USER <= *ullJoinUserCount)
				{
					closesocket(NowWork->m_clienet_Socket);

					// NowWork 반환
					pAcceptPool->Free(NowWork);

					// 내 에러 보관
					g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__JOIN_USER_FULL;

					// 에러 스트링 만들고
					TCHAR tcErrorString[300];
					StringCchPrintf(tcErrorString, 300, L"AuthThread(). User Full!!!! (%lld)",
						*ullJoinUserCount);

					// 에러 발생 함수 호출
					g_This->OnError((int)euError::NETWORK_LIB_ERROR__JOIN_USER_FULL, tcErrorString);

					// continue
					continue;
				}
				

				// 3. 접속 직후, IP등을 판단해서 무언가 추가 작업이 필요할 경우가 있을 수도 있으니 호출	
				// false면 접속거부, 
				// true면 접속 계속 진행. true에서 할게 있으면 OnConnectionRequest함수 인자로 뭔가를 던진다.
				if (g_This->OnConnectionRequest(NowWork->m_tcIP, NowWork->m_usPort) == false)
				{
					// NowWork 반환
					pAcceptPool->Free(NowWork);

					continue;
				}				

				// 4. 세션 구조체 생성 후 셋팅
				// 1) 미사용 인덱스 알아오기
				wIndex = EmptyIndexStack->Pop();

				// 2) I/O 카운트 증가
				// 삭제 방어
				InterlockedIncrement(&SessionArray[wIndex]->m_lIOCount);

				// 3) 정보 셋팅하기
				// -- 세션 ID(믹스 키)와 인덱스 할당
				ULONGLONG MixKey = ((ullUniqueSessionID << 16) | wIndex);
				ullUniqueSessionID++;

				SessionArray[wIndex]->m_ullSessionID = MixKey;
				SessionArray[wIndex]->m_lIndex = wIndex;

				// -- LastPacket 초기화
				SessionArray[wIndex]->m_LastPacket = nullptr;

				// -- 소켓
				SessionArray[wIndex]->m_Client_sock = NowWork->m_clienet_Socket;

				// -- IP와 Port
				StringCchCopy(SessionArray[wIndex]->m_IP, _MyCountof(SessionArray[wIndex]->m_IP), NowWork->m_tcIP);
				SessionArray[wIndex]->m_prot = NowWork->m_usPort;

				// -- SendFlag 초기화
				SessionArray[wIndex]->m_lSendFlag = FALSE;				

				// -- 로그아웃 Flag 초기화
				SessionArray[wIndex]->m_lLogoutFlag = FALSE;

				// -- Auth TO Game Flag 초기화
				SessionArray[wIndex]->m_lAuthToGameFlag = FALSE;

				// 4) 셋팅이 모두 끝났으면 Auth 상태로 변경

				// Auth 모드 유저 수 증가
				++g_This->m_lAuthModeUserCount;

				SessionArray[wIndex]->m_euMode = euSessionModeState::MODE_AUTH;



				// 5. IOCP 연결
				// 소켓과 IOCP 연결
				if (CreateIoCompletionPort((HANDLE)NowWork->m_clienet_Socket, g_This->m_hIOCPHandle, (ULONG_PTR)SessionArray[wIndex], 0) == NULL)
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


				// 6. 접속자 수 증가. InDisconnect에서도 사용되는 변수이기 때문에 인터락 사용
				InterlockedIncrement(ullJoinUserCount);

				
				// 7. NowWork 다 썻으면 반환
				pAcceptPool->Free(NowWork);
							   				

				// 8. 모든 접속절차가 완료되었으니 접속 후 처리 함수 호출.
				SessionArray[wIndex]->OnAuth_ClientJoin();


				// 9. 비동기 입출력 시작
				// 리시브 버퍼가 꽉찼으면 1리턴
				int ret = g_This->RecvPost(SessionArray[wIndex]);

				// 증가시켰던, I/O카운트 --. 0이라면 로그아웃 플래그 변경.
				if (InterlockedDecrement(&SessionArray[wIndex]->m_lIOCount) == 0)
					SessionArray[wIndex]->m_lLogoutFlag = TRUE;

				// I/O카운트 감소시켰는데 0이 아니라면, ret 체크.
				// ret가 1이라면 접속 끊는다.
				else if (ret == 1)
					shutdown(SessionArray[wIndex]->m_Client_sock, SD_BOTH);


				--iSize;
			}



			// ------------------			
			// Part 2. AUTH에서 GAME으로 모드 전환
			// Part 3. AUTH모드 세션들 패킷 처리 + Logout Flag 처리
			// ------------------
			int iIndex = MAX_USER - 1;

			// While문 한번에 iIndex 1 차감.
			// 배열의 가장 뒤부터 처리한다. (ex. 최대 유저 수가 7000이면, 6999번째 배열부터 시작)
			while (iIndex >= 0)
			{
				cSession* NowSession = g_This->m_stSessionArray[iIndex];

				// 해당 유저가 MODE_AUTH인지 확인 ---------------------------
				if (NowSession->m_euMode == euSessionModeState::MODE_AUTH)
				{
					// Auth TO Game 플래그 확인
					// AUTH에서 GAME으로 모드 전환
					if (NowSession->m_lAuthToGameFlag == TRUE)
					{
						// Auth 모드 유저 수 감소
						--g_This->m_lAuthModeUserCount;

						// 나갔다고 알려준다.
						// 모드 변경 후 알려주면, OnAuth_ClientLeave가 호출되기도 전에, GAME쪽에서 먼저 OnGame_ClinetJoin 뜰 가능성
						NowSession->OnAuth_ClientLeave(true);

						// MODE_AUTH_TO_GAME으로 모드 변경
						NowSession->m_euMode = euSessionModeState::MODE_AUTH_TO_GAME;
					}

					// AUTH모드인게 확정이라면
					else
					{
						// LogOutFlag 체크
						// FALSE라면 정상 로직 처리. Auth에서 종료될 유저
						if (NowSession->m_lLogoutFlag == FALSE)
						{
							// 1. CompleteRecvPacket 큐의 사이즈 확인.
							int iQSize = NowSession->m_CRPacketQueue->GetNodeSize();

							// 2. 큐에 노드가 1개 이상 있으면, 패킷 처리
							if (iQSize > 0)
							{
								// !! 패킷 처리 횟수 제한을 두기 위해, PACKET_WORK_COUNT보다 노드의 수가 더 많으면, PACKET_WORK_COUNT로 변경 !!
								if (iQSize > PACKET_WORK_COUNT)
									iQSize = PACKET_WORK_COUNT;

								CProtocolBuff_Net* NowPacket;

								while (iQSize > 0)
								{
									// 노드 빼기
									if (NowSession->m_CRPacketQueue->Dequeue(NowPacket) == -1)
										cMMOServer_Dump->Crash();

									// 패킷 처리
									NowSession->OnAuth_Packet(NowPacket);

									// 패킷 래퍼런스 카운트 1 감소
									CProtocolBuff_Net::Free(NowPacket);

									// 패킷 1개 처리했으니 남은 수 감소.
									--iQSize;
								}
							}
						}

						// TRUE라면 끊길 유저
						else
						{
							// SendFlag 체크
							if (NowSession->m_lSendFlag == FALSE)
							{
								// Auth 모드 유저 수 감소
								--g_This->m_lAuthModeUserCount;

								// 나갔다고 알려준다.
								// 모드 변경 후 알려주면, 아직 알려주기 전에, GAME쪽에서 Release되어 Release가 먼저 뜰 가능성.
								NowSession->OnAuth_ClientLeave();

								// MODE_WAIT_LOGOUT으로 모드 변경
								NowSession->m_euMode = euSessionModeState::MODE_WAIT_LOGOUT;
							}
						}
					}
							
				}				
				
				--iIndex;
			}
			

			// ------------------
			// Part 4. OnAuth_Update
			// ------------------
			g_This->OnAuth_Update();			

			InterlockedIncrement(&g_This->m_lAuthFPS);
		}

		return 0;		
	}

	// Game 스레드
	UINT	WINAPI	CMMOServer::GameThread(LPVOID lParam)
	{
		CMMOServer* g_This = (CMMOServer*)lParam;

		// ------- 필요한 정보들 인자로 받아두기 -------

		// 1프레임에, 1명의 유저당 몇 개의 패킷을 처리할 것인가
		const int PACKET_WORK_COUNT = g_This->m_stConfig.GamePacket_Count;

		// 최대 접속 가능 유저, 로컬로 받아두기
		int MAX_USER = g_This->m_iMaxJoinUser;

		// Sleep 인자로 받아두기
		const int SLEEP_VALUE = g_This->m_stConfig.GameSleep;

		// 1프레임 동안, AUTH_IN_GAME에서 GAME으로 변경될 수 있는 유저 수
		const int NEWUSER_PACKET_COUNT = g_This->m_stConfig.GameNewUser_PacketCount;

		// 종료 이벤트
		HANDLE* ExitEvent = &g_This->m_hGameExitEvent;

		// 세션 관리 배열
		cSession** SessionArray = g_This->m_stSessionArray;

		while (1)
		{
			// ------------------
			// SLEEP_VALUE만큼 자다가 일어난다.
			// ------------------	
			DWORD Ret = WaitForSingleObject(*ExitEvent, SLEEP_VALUE);

			// 이상한 신호라면 
			if (Ret == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("GameThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 정상 종료
			else if (Ret == WAIT_OBJECT_0)
				break;


			// ------------------
			// Part 1. Game 모드로 전환
			// Part 2. Game 모드 세션들 패킷 처리 + Logout Flag 처리			
			// ------------------
			int iIndex = MAX_USER - 1;
			int iModeChangeCount = NEWUSER_PACKET_COUNT;			
			while (iIndex >= 0)
			{
				cSession* NowSession = SessionArray[iIndex];

				// Part 1. Game 모드로 전환
				// 유저가 MODE_AUTH_TO_GAME 모드인지 확인 -----------------
				// 그리고 모드 체인지 유저 수가 0보다 큰지 체크
				if (NowSession->m_euMode == euSessionModeState::MODE_AUTH_TO_GAME &&
					iModeChangeCount > 0)
				{
					// Game 모드 유저 수 증가
					++g_This->m_lGameModeUserCount;

					// 맞다면, OnGame_ClientJoint 호출
					NowSession->OnGame_ClientJoin();

					// 모드 변경
					NowSession->m_euMode = euSessionModeState::MODE_GAME;

					// 모드 체인지 유저 수 감소.
					// 0이되면 이번 프레임에 더 이상 모드 체인지 안함
					--iModeChangeCount;					
				}

				// Part 2. Game 모드 세션들 패킷 처리 + Logout Flag 처리
				// 유저가 MODE_GAME 모드인지 확인 -----------------
				if (NowSession->m_euMode == euSessionModeState::MODE_GAME)
				{
					// LogOutFlag 체크
					// FALSE라면 정상 로직 처리
					if (NowSession->m_lLogoutFlag == FALSE)
					{
						// 1. CompleteRecvPacket 큐의 사이즈 확인.
						int iQSize = NowSession->m_CRPacketQueue->GetNodeSize();

						// 2. 큐에 노드가 1개 이상 있으면, 패킷 처리
						if (iQSize > 0)
						{
							// !! 패킷 처리 횟수 제한을 두기 위해, PACKET_WORK_COUNT보다 노드의 수가 더 많으면, PACKET_WORK_COUNT로 변경 !!
							if (iQSize > PACKET_WORK_COUNT)
								iQSize = PACKET_WORK_COUNT;

							CProtocolBuff_Net* NowPacket;

							while (iQSize > 0)
							{
								// 노드 빼기
								if (NowSession->m_CRPacketQueue->Dequeue(NowPacket) == -1)
									cMMOServer_Dump->Crash();

								// 패킷 처리
								NowSession->OnGame_Packet(NowPacket);

								// 패킷 래퍼런스 카운트 1 감소
								CProtocolBuff_Net::Free(NowPacket);

								// 패킷 1개 처리했으니 남은 수 감소.
								--iQSize;
							}
						}

					}

					// LogOutFlag가 TRUE라면 끊길 유저. 
					else
					{
						// SendFlag 체크
						if (NowSession->m_lSendFlag == FALSE)
						{
							// Game 모드 유저 수 감소
							--g_This->m_lGameModeUserCount;

							// Game모드에서 나갔음을 알려준다.
							NowSession->OnGame_ClientLeave();

							// 맞다면, MODE_WAIT_LOGOUT으로 모드 변경
							NowSession->m_euMode = euSessionModeState::MODE_WAIT_LOGOUT;
						}
					}					
				}				

				--iIndex;
			}					

			// ------------------
			// Part 3. Game 모드의 업데이트 처리
			// ------------------
			g_This->OnGame_Update();								   			

			InterlockedIncrement(&g_This->m_lGameFPS);
		}

		return 0;
	}

	// Send 스레드
	UINT	WINAPI	CMMOServer::SendThread(LPVOID lParam)
	{
		CMMOServer* g_This = (CMMOServer*)lParam;

		// 최대 접속 가능 유저, 로컬로 받아두기
		int iMaxUser = g_This->m_iMaxJoinUser;

		// 해당 샌드 스레드가 처리할 인덱스의 Start와 End 받기
		LONG Index = InterlockedIncrement(&g_This->m_lSendConfigIndex);
		int iStartIndex = g_This->m_stSendConfig[Index].m_iStartIndex;
		int iEndIndex = g_This->m_stSendConfig[Index].m_iEndIndex;

		// 종료 이벤트
		HANDLE* ExitEvent = &g_This->m_hSendExitEvent;

		// 세션 관리 배열
		cSession** SessionArray = g_This->m_stSessionArray;

		// Sleep 인자로 받아두기
		const int SLEEP_VALUE = g_This->m_stConfig.SendSleep;

		// Encode할 변수들 받아두기
		BYTE Head = g_This->m_bCode;
		BYTE XORCode1 = g_This->m_bXORCode_1;
		BYTE XORCode2 = g_This->m_bXORCode_2;

		while (1)
		{
			// ------------------
			// 1미리 세컨드마다 1회씩 일어난다.
			// ------------------	
			DWORD Ret = WaitForSingleObject(*ExitEvent, SLEEP_VALUE);

			// 이상한 신호라면 
			if (Ret == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("SendThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 정상 종료
			else if (Ret == WAIT_OBJECT_0)
				break;

			
			// ------------------
			// Start와 End Index를 기준으로 Send 시도
			// ------------------
			int TempStartIndex = iStartIndex;
			int TempEndIndex = iEndIndex;

			while (TempEndIndex >= TempStartIndex)
			{
				cSession* NowSession = SessionArray[TempEndIndex];
				
				
				// ------------------
				// 패킷 포인터 배열 카운트가 0인지 체크
				// 0이 아니면, 아직 이전 Send에 대한 완료통지가 안왔거나, WSASend를 걸었는데 저쪽에서 접속을 끊어서, 종료될 유저.
				// 저쪽에서 연결을 끊으면, GQCS에서 튀어나오는데, 워커에서 어떤 로직도 타지 않고 I/O카운트만 1 줄이고 끝남.
				// ------------------
				if (NowSession->m_iWSASendCount != 0)
				{
					--TempEndIndex;
					continue;
				}	
				

				// ------------------
				// send 가능 상태인지 체크
				// ------------------
				// SendFlag(1번인자)를 TRUE(2번인자)로 변경.
				// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)라는것.
				// 즉, Release를 탈 유저.
				//if (InterlockedExchange(&NowSession->m_lSendFlag, TRUE) == TRUE)
				//{
				//	--TempEndIndex;
				//	continue;
				//}	

				if (NowSession->m_lSendFlag == TRUE)
				{
					--TempEndIndex;
					continue;
				}

				NowSession->m_lSendFlag = TRUE;

				// LogoutFlag가 TRUE라면 로그아웃 될 유저.
				// 이걸로, InDisconnect 중에 Send 안되도록 명확히 방어.
				if (NowSession->m_lLogoutFlag == TRUE)
				{
					// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
					NowSession->m_lSendFlag = FALSE;

					--TempEndIndex;
					continue;
				}				
				
				// 모드가 Auth 혹은 Game이어야 함. 아니면 종료될 유저.				
				if (NowSession->m_euMode != euSessionModeState::MODE_AUTH &&
					NowSession->m_euMode != euSessionModeState::MODE_GAME)
				{
					// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
					NowSession->m_lSendFlag = FALSE;

					--TempEndIndex;
					continue;
				}					

				// ------------------
				// SendBuff에 데이터가 있는지 확인
				// ------------------
				//int UseSize = NowSession->m_SendQueue->GetInNode();
				int UseSize = NowSession->m_SendQueue->GetNodeSize();
				if (UseSize == 0)
				{
					// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
					NowSession->m_lSendFlag = FALSE;

					--TempEndIndex;
					continue;
				}


				// ------------------
				// 데이터가 있다면 Send하기
				// ------------------

				// 1. WSABUF 셋팅.
				WSABUF wsabuf[dfSENDPOST_MAX_WSABUF];

				if (UseSize > dfSENDPOST_MAX_WSABUF)
					UseSize = dfSENDPOST_MAX_WSABUF;

				if (NowSession->m_iWSASendCount > 0)
					cMMOServer_Dump->Crash();

				int iPacketIndex = UseSize -1;
				while (iPacketIndex >= 0)
				{
					if (NowSession->m_SendQueue->Dequeue(NowSession->m_PacketArray[iPacketIndex]) == -1)
						cMMOServer_Dump->Crash();

					// 헤더를 넣어서, 패킷 완성하기		
					NowSession->m_PacketArray[iPacketIndex]->Encode(Head, XORCode1, XORCode2);

					// WSABUF에 포인터 복사
					wsabuf[iPacketIndex].buf = NowSession->m_PacketArray[iPacketIndex]->GetBufferPtr();
					wsabuf[iPacketIndex].len = NowSession->m_PacketArray[iPacketIndex]->GetUseSize();

					--iPacketIndex;
				}

				NowSession->m_iWSASendCount = UseSize;

				// 2. Overlapped 구조체 초기화
				ZeroMemory(&NowSession->m_overSendOverlapped, sizeof(NowSession->m_overSendOverlapped));

				// 3. WSASend()
				DWORD SendBytes = 0, flags = 0;
				InterlockedIncrement(&NowSession->m_lIOCount);

				// 4. 에러 처리
				if (WSASend(NowSession->m_Client_sock, wsabuf, UseSize, &SendBytes, flags, &NowSession->m_overSendOverlapped, NULL) == SOCKET_ERROR)
				{
					int Error = WSAGetLastError();

					// 비동기 입출력이 시작된게 아니라면
					if (Error != WSA_IO_PENDING)
					{
						// I/O카운트 1 감소
						// 0이면 삭제될 유저.
						if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
						{		
							// 로그아웃 플래그 변경
							NowSession->m_lLogoutFlag = TRUE;	
						}

						// 에러가 버퍼 부족이라면
						else if (Error == WSAENOBUFS)
						{
							// 내 에러, 윈도우에러 보관
							g_This->m_iOSErrorCode = Error;
							g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

							// 에러 스트링 만들고
							TCHAR tcErrorString[300];
							StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %lld, [%s:%d]"),
								NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

							// 로그 찍기 (로그 레벨 : 에러)
							cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d), OSError(%d)",
								tcErrorString, (int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

							// 에러 함수 호출
							g_This->OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);

							// 끊는다.
							shutdown(NowSession->m_Client_sock, SD_BOTH);
						}

						// 샌드 가능 상태로 돌림.
						NowSession->m_lSendFlag = FALSE;
					}
				}

				--TempEndIndex;
			}
			
					
		}

		return 0;
	}
	
	// Release 스레드
	UINT WINAPI	CMMOServer::ReleaseThread(LPVOID lParam)
	{
		CMMOServer* g_This = (CMMOServer*)lParam;

		// ------- 필요한 정보들 인자로 받아두기 -------	

		// 최대 접속 가능 유저, 로컬로 받아두기
		int MAX_USER = g_This->m_iMaxJoinUser;

		// Sleep 인자로 받아두기
		const int SLEEP_VALUE = g_This->m_stConfig.ReleaseSleep;

		// 종료 이벤트
		HANDLE* ExitEvent = &g_This->m_hReleaseExitEvent;

		// 현재 접속중인 유저 수
		ULONGLONG* ullJoinUserCount = &g_This->m_ullJoinUserCount;		

		// 세션 관리 배열
		cSession** SessionArray = g_This->m_stSessionArray;

		// 미사용 인덱스 관리 스택
		CLF_Stack<WORD>* EmptyIndexStack = g_This->m_stEmptyIndexStack;

		while (1)
		{
			// ------------------
			// SLEEP_VALUE만큼 자다가 일어난다.
			// ------------------	
			DWORD Ret = WaitForSingleObject(*ExitEvent, SLEEP_VALUE);

			// 이상한 신호라면 
			if (Ret == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("ReleaseThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 정상 종료
			else if (Ret == WAIT_OBJECT_0)
				break;

			// ------------------
			// Part 1. Release 처리
			// ------------------
			int iIndex = MAX_USER - 1;
			while (iIndex >= 0)
			{
				cSession* NowSession = SessionArray[iIndex];

				// Part 1. Release 처리
				// 유저가 MODE_WAIT_LOGOUT 모드인지 확인 -----------------
				
				if (NowSession->m_euMode == euSessionModeState::MODE_WAIT_LOGOUT)
				{
					/*
					// SendFlag를 TRUE로 만든다.
					// 내가 TRUE로 만들었다면 로직 진행
					if (InterlockedExchange(&NowSession->m_lSendFlag, TRUE) == FALSE)
					{
						// 모드 초기화
						NowSession->m_euMode = euSessionModeState::MODE_NONE;

						// 해당 세션의 'Send 직렬화 버퍼(Send했던 직렬화 버퍼 모음. 아직 완료통지 못받은 직렬화버퍼들)'에 있는 데이터를 Free한다.
						int i = NowSession->m_iWSASendCount - 1;
						while (i >= 0)
						{
							CProtocolBuff_Net::Free(NowSession->m_PacketArray[i]);
							--i;
						}

						// SendCount 초기화
						NowSession->m_iWSASendCount = 0;

						// CompleteRecvPacket 비우기
						CProtocolBuff_Net* DeletePacket;

						int UseSize = NowSession->m_CRPacketQueue->GetNodeSize();
						while (UseSize > 0)
						{
							// 디큐 후, 직렬화 버퍼 메모리풀에 Free한다.
							if (NowSession->m_CRPacketQueue->Dequeue(DeletePacket) == -1)
								cMMOServer_Dump->Crash();

							CProtocolBuff_Net::Free(DeletePacket);

							--UseSize;
						}

						// 샌드 큐 비우기
						//UseSize = DeleteSession->m_SendQueue->GetInNode();
						UseSize = NowSession->m_SendQueue->GetNodeSize();
						while (UseSize > 0)
						{
							// 디큐 후, 직렬화 버퍼 메모리풀에 Free한다.
							if (NowSession->m_SendQueue->Dequeue(DeletePacket) == -1)
								cMMOServer_Dump->Crash();

							CProtocolBuff_Net::Free(DeletePacket);

							--UseSize;
						}

						// 리시브 큐 초기화
						NowSession->m_RecvQueue.ClearBuffer();

						// 릴리즈 되었다고 알려준다.
						NowSession->OnGame_ClientRelease();

						// 클로즈 소켓
						closesocket(NowSession->m_Client_sock);

						// 미사용 인덱스 스택에 반납
						g_This->m_stEmptyIndexStack->Push(NowSession->m_lIndex);

						// 접속 중 유저 수 감소
						InterlockedDecrement(&g_This->m_ullJoinUserCount);
					}	
					*/
					
					// 모드 초기화
					NowSession->m_euMode = euSessionModeState::MODE_NONE;

					// 해당 세션의 'Send 직렬화 버퍼(Send했던 직렬화 버퍼 모음. 아직 완료통지 못받은 직렬화버퍼들)'에 있는 데이터를 Free한다.
					int i = NowSession->m_iWSASendCount - 1;
					while (i >= 0)
					{
						CProtocolBuff_Net::Free(NowSession->m_PacketArray[i]);
						--i;
					}

					// SendCount 초기화
					NowSession->m_iWSASendCount = 0;

					// CompleteRecvPacket 비우기
					CProtocolBuff_Net* DeletePacket;

					int UseSize = NowSession->m_CRPacketQueue->GetNodeSize();
					while (UseSize > 0)
					{
						// 디큐 후, 직렬화 버퍼 메모리풀에 Free한다.
						if (NowSession->m_CRPacketQueue->Dequeue(DeletePacket) == -1)
							cMMOServer_Dump->Crash();

						CProtocolBuff_Net::Free(DeletePacket);

						--UseSize;
					}

					// 샌드 큐 비우기
					//UseSize = DeleteSession->m_SendQueue->GetInNode();
					UseSize = NowSession->m_SendQueue->GetNodeSize();
					while (UseSize > 0)
					{
						// 디큐 후, 직렬화 버퍼 메모리풀에 Free한다.
						if (NowSession->m_SendQueue->Dequeue(DeletePacket) == -1)
							cMMOServer_Dump->Crash();

						CProtocolBuff_Net::Free(DeletePacket);

						--UseSize;
					}

					// 리시브 큐 초기화
					NowSession->m_RecvQueue.ClearBuffer();

					// 클로즈 소켓
					closesocket(NowSession->m_Client_sock);

					// 릴리즈 되었다고 알려준다.
					NowSession->OnGame_ClientRelease();					

					// 미사용 인덱스 스택에 반납
					EmptyIndexStack->Push(NowSession->m_lIndex);

					// 접속 중 유저 수 감소
					InterlockedDecrement(ullJoinUserCount);
				}

				--iIndex;
			}

		}

		return 0;
	}




	// -----------------------
	// 상속 관계에서만 호출 가능한 함수
	// -----------------------
	
	// 서버 시작
	// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 활성화시킬 워커스레드 수, 엑셉트 스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수, 패킷 Code, XOR 1번코드, XOR 2번코드] 입력받음.
	//
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CMMOServer::Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int ActiveWThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect,
		BYTE Code, BYTE XORCode1, BYTE XORCode2)
	{
		// 카운트 변수 초기화
		m_ullAcceptTotal = 0;
		m_lAcceptTPS = 0;
		m_lSendPostTPS = 0;
		m_lRecvTPS = 0;

		m_lAuthModeUserCount = 0;
		m_lGameModeUserCount = 0;

		m_lAuthFPS = 0;
		m_lGameFPS = 0;


		// rand설정
		srand((UINT)time(NULL));

		// Config 데이터 셋팅
		m_bCode = Code;
		m_bXORCode_1 = XORCode1;
		m_bXORCode_2 = XORCode2;

		// 새로 시작하니까 에러코드들 초기화
		m_iOSErrorCode = 0;
		m_iMyErrorCode = (euError)0;

		// 최대 접속 가능 유저 수 셋팅
		m_iMaxJoinUser = MaxConnect;

		// 미사용 세션 관리 스택 동적할당. (락프리 스택) 
		// 그리고 미리 Max만큼 만들어두기
		m_stEmptyIndexStack = new CLF_Stack<WORD>;
		for (int i = 0; i < MaxConnect; ++i)
			m_stEmptyIndexStack->Push(i);	


		// ------------------- SendConfig 셋팅
		m_stSendConfig = new stSendConfig[m_stConfig.CreateSendThreadCount];

		int Value = MaxConnect / m_stConfig.CreateSendThreadCount;

		int Start = 0;
		int End = Value -1;

		for (int i = 0; i < m_stConfig.CreateSendThreadCount; ++i)
		{
			m_stSendConfig[i].m_iStartIndex = Start;
			m_stSendConfig[i].m_iEndIndex = End;

			Start = End + 1;
			End = End + Value;
		}


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
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__THREAD_CREATE_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(i);

				// 로그 찍기 (로그 레벨 : 에러)
				cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WorkerThread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}	
		

		// Send 스레드 생성
		m_iS_ThreadCount = m_stConfig.CreateSendThreadCount;
		m_hSendHandle = new HANDLE[m_iS_ThreadCount];
		for (int i = 0; i < m_iS_ThreadCount; ++i)
		{
			m_hSendHandle[i] = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
			if (m_hSendHandle == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__THREAD_CREATE_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Send Thread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}

		// Auth 스레드 생성
		m_iAuth_ThreadCount = 1;
		m_hAuthHandle = new HANDLE[m_iAuth_ThreadCount];
		for (int i = 0; i < m_iAuth_ThreadCount; ++i)
		{
			m_hAuthHandle[i] = (HANDLE)_beginthreadex(NULL, 0, AuthThread, this, 0, NULL);
			if (m_hAuthHandle == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__THREAD_CREATE_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Auth Thread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}


		// Game 스레드 생성
		m_iGame_ThreadCount = 1;
		m_hGameHandle = new HANDLE[m_iGame_ThreadCount];
		for (int i = 0; i < m_iGame_ThreadCount; ++i)
		{
			m_hGameHandle[i] = (HANDLE)_beginthreadex(NULL, 0, GameThread, this, 0, NULL);
			if (m_hGameHandle == NULL)
			{
				// 윈도우 에러, 내 에러 보관
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__THREAD_CREATE_FAIL;

				// 각종 핸들 반환 및 동적해제 절차.
				ExitFunc(m_iW_ThreadCount);

				// 로그 찍기 (로그 레벨 : 에러)
				cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Game Thread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false 리턴
				return false;
			}
		}		

		// Release 스레드 생성
		m_hReleaseHandle = (HANDLE)_beginthreadex(NULL, 0, ReleaseThread, this, 0, NULL);
		if (m_hGameHandle == NULL)
		{
			// 윈도우 에러, 내 에러 보관
			m_iOSErrorCode = errno;
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__THREAD_CREATE_FAIL;

			// 각종 핸들 반환 및 동적해제 절차.
			ExitFunc(m_iW_ThreadCount);

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Release Thread Create Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false 리턴
			return false;
		}

		// 리슨 소켓 생성
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
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__THREAD_CREATE_FAIL;

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
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Accept Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Accept Thread EXIT Error");
		}

		// 2. 모든 유저에게 Shutdown
		// 모든 유저에게 셧다운 날림
		for (int i = 0; i < m_iMaxJoinUser; ++i)
		{
			shutdown(m_stSessionArray[i]->m_Client_sock, SD_BOTH);
		}

		// 모든 유저가 종료되었는지 체크
		while (1)
		{
			if (m_ullJoinUserCount == 0)
				break;

			Sleep(1);
		}

		// 3. Auth 스레드 종료
		SetEvent(m_hAuthExitEvent);

		// Auth스레드 종료 대기
		retval = WaitForMultipleObjects(m_iAuth_ThreadCount, m_hAuthHandle, TRUE, INFINITE);

		// 리턴값이 [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iAuth_ThreadCount - 1] 가 아니라면, 뭔가 에러가 발생한 것. 에러 찍는다
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iAuth_ThreadCount - 1)
		{
			// 에러 값이 WAIT_FAILED일 경우, GetLastError()로 확인해야함.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();

			// 그게 아니라면 리턴값에 이미 에러가 들어있음.
			else
				m_iOSErrorCode = retval;

			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Auth Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Auth Thread EXIT Error");
		}


		// 4. Game 스레드 종료
		SetEvent(m_hGameHandle);

		// Game 스레드 종료 대기
		retval = WaitForMultipleObjects(m_iGame_ThreadCount, m_hGameHandle, TRUE, INFINITE);

		// 리턴값이 [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iGame_ThreadCount - 1] 가 아니라면, 뭔가 에러가 발생한 것. 에러 찍는다
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iGame_ThreadCount - 1)
		{
			// 에러 값이 WAIT_FAILED일 경우, GetLastError()로 확인해야함.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();

			// 그게 아니라면 리턴값에 이미 에러가 들어있음.
			else
				m_iOSErrorCode = retval;

			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Game Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Game Thread EXIT Error");
		}

		// 5. Send 스레드 종료
		SetEvent(m_hSendHandle);

		// Send 스레드 종료 대기
		retval = WaitForMultipleObjects(m_iS_ThreadCount, m_hSendHandle, TRUE, INFINITE);

		// 리턴값이 [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iS_ThreadCount - 1] 가 아니라면, 뭔가 에러가 발생한 것. 에러 찍는다
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iS_ThreadCount - 1)
		{
			// 에러 값이 WAIT_FAILED일 경우, GetLastError()로 확인해야함.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();

			// 그게 아니라면 리턴값에 이미 에러가 들어있음.
			else
				m_iOSErrorCode = retval;

			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Send Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Send Thread EXIT Error");
		}
		
		// 6. Release 스레드 종료
		SetEvent(m_hReleaseExitEvent);

		// Release 스레드 종료 대기
		retval = WaitForSingleObject(m_hReleaseHandle, INFINITE);

		// 리턴값이 [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iS_ThreadCount - 1] 가 아니라면, 뭔가 에러가 발생한 것. 에러 찍는다
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iS_ThreadCount - 1)
		{
			// 에러 값이 WAIT_FAILED일 경우, GetLastError()로 확인해야함.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();

			// 그게 아니라면 리턴값에 이미 에러가 들어있음.
			else
				m_iOSErrorCode = retval;

			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;

			// 로그 찍기 (로그 레벨 : 에러)
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Release Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Release Thread EXIT Error");
		}

		// 6. 워커 스레드 종료
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
			cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Worker Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// 에러 발생 함수 호출
			OnError((int)euError::NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT, L"Stop() --> Worker Thread EXIT Error");
		}

		// 7. 각종 리소스 반환

		// 1) Accept Socekt Queue 모두 반환
		stAuthWork* DeleteWork;
		for (int i = 0; i < m_pASQ->GetInNode(); ++i)
		{
			m_pASQ->Dequeue(DeleteWork);

			m_pAcceptPool->Free(DeleteWork);
		}

		// 2) 엑셉트 스레드 핸들 반환
		for (int i = 0; i < m_iA_ThreadCount; ++i)
			CloseHandle(m_hAcceptHandle[i]);

		// 3) 워커 스레드 핸들 반환
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			CloseHandle(m_hWorkerHandle[i]);

		// 4) 어스 스레드 핸들 반환
		for (int i = 0; i < m_iAuth_ThreadCount; ++i)
			CloseHandle(m_hAuthHandle[i]);

		// 5) 게임 스레드 핸들 반환
		for (int i = 0; i < m_iGame_ThreadCount; ++i)
			CloseHandle(m_hGameHandle[i]);

		// 6) 샌드 스레드 핸들 반환
		for (int i = 0; i < m_iS_ThreadCount; ++i)
			CloseHandle(m_hSendHandle[i]);
		
		// 7) 각종 스레드 배열 동적해제
		delete[] m_hWorkerHandle;
		delete[] m_hAcceptHandle;
		delete[] m_hSendHandle;
		delete[] m_hAuthHandle;
		delete[] m_hGameHandle;

		// 8) IOCP핸들 반환
		CloseHandle(m_hIOCPHandle);

		// 9) 윈속 해제
		WSACleanup();

		// 10) 세션 미사용 인덱스 관리 스택 동적해제
		delete m_stEmptyIndexStack;		

		// 9. 각종 변수 초기화
		Reset();

		// 10. 서버 가동중 아님 상태로 변경
		m_bServerLife = false;

		// 11. 서버 종료 로그 찍기		
		cMMOServer_Log->LogSave(L"MMOServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");
	}
	   
	// 세션 셋팅
	//
	// Parameter : cSession* 포인터, Max 수(최대 접속 가능 유저 수)
	// return : 없음
	void CMMOServer::SetSession(cSession* pSession, int Max)
	{
		static int iIndex = 0;	

		if(m_stSessionArray == nullptr)
			m_stSessionArray = new cSession*[Max];

		m_stSessionArray[iIndex] = pSession;	

		++iIndex;
	}
	   	  
}