#include "pch.h"
#include "CGameServer.h"
#include "Parser\Parser_Class.h"
#include "Protocol_Set\CommonProtocol.h"
#include "Log\Log.h"
#include "CPUUsage\CPUUsage.h"


extern ULONGLONG g_ullAcceptTotal_MMO;
extern LONG	  g_lAcceptTPS_MMO;
extern LONG	g_lSendPostTPS_MMO;
extern LONG	g_lRecvTPS_MMO;

extern LONG g_lAllocNodeCount;

extern LONG g_lAuthModeUserCount;
extern LONG g_lGameModeUserCount;

extern LONG g_lAuthFPS;
extern LONG g_lGameFPS;



// ------------------
// CGameSession의 함수
// (CGameServer의 이너클래스)
// ------------------
namespace Library_Jingyu
{
	// 덤프용
	CCrashDump* g_GameServerDump = CCrashDump::GetInstance();

	// 로그용
	CSystemLog* g_GameServerLog = CSystemLog::GetInstance();


	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CGameServer::CGameSession::CGameSession()
		:CMMOServer::cSession()
	{
		// 할거 없음
		m_Int64AccountNo = 0x0fffffffffffffff;
	}

	// 소멸자
	CGameServer::CGameSession::~CGameSession()
	{
		// 할거 없음
	}

	// -----------------
	// 가상함수
	// -----------------
	
	// --------------- AUTH 모드용 함수

	// 유저가 Auth 모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnAuth_ClientJoin()
	{
		// 할거 없음
	}

	// 유저가 Auth 모드에서 나감
	//
	// Parameter : Game모드로 변경된것인지 알려주는 Flag. 디폴트 false.
	// return : 없음
	void CGameServer::CGameSession::OnAuth_ClientLeave(bool bGame)
	{
		// 할거 없음 
	}

	// Auth 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CGameServer::CGameSession::OnAuth_Packet(CProtocolBuff_Net* Packet)
	{
		// 패킷 처리
		try
		{
			// 1. 타입 빼기
			WORD Type;
			Packet->GetData((char*)&Type, 2);

			// 2. 타입에 따라 분기 처리
			switch (Type)
			{
				// 로그인 요청
			case en_PACKET_CS_GAME_REQ_LOGIN:
				Auth_LoginPacket(Packet);
				break;

				// 하트비트
			case en_PACKET_CS_GAME_REQ_HEARTBEAT:
				break;

				// 그 외에는 문제있음.
			default:				
				throw CException(_T("OnAuth_Packet() --> Type Error!!"));
				break;
			}
		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_GameServerLog->LogSave(L"GameServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());	

			// 덤프
			g_GameServerDump->Crash();
		}

	}



	// --------------- GAME 모드용 함수

	// 유저가 Game모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnGame_ClientJoin()
	{
		// 할거 없음
	}

	// 유저가 Game모드에서 나감
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnGame_ClientLeave()
	{
		// 할거 없음
	}

	// Game 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CGameServer::CGameSession::OnGame_Packet(CProtocolBuff_Net* Packet)
	{
		// 타입에 따라 패킷 처리
		try
		{
			// 1. 타입 빼기
			WORD Type;
			Packet->GetData((char*)&Type, 2);

			// 2. 타입에 따라 분기문 처리
			switch (Type)
			{
				// 테스트용 에코 요청
			case en_PACKET_CS_GAME_REQ_ECHO:
				Game_EchoTest(Packet);
				break;

				// 하트비트
			case en_PACKET_CS_GAME_REQ_HEARTBEAT:
				break;

				// 이 외에는 문제 있음
			default:
				throw CException(_T("OnGame_Packet() --> Type Error!!"));
				break;
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_GameServerLog->LogSave(L"GameServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// 덤프
			g_GameServerDump->Crash();
		}

	}



	// --------------- Release 모드용 함수
	
	// Release된 유저.
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnGame_ClientRelease()
	{
		// AccountNo 초기화
		m_Int64AccountNo = 0x0fffffffffffffff;

	}

	


	// -----------------
	// 패킷 처리 함수
	// -----------------

	// 로그인 요청 
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CGameServer::CGameSession::Auth_LoginPacket(CProtocolBuff_Net* Packet)
	{
		// 1. 마샬링
		INT64 AccountNo;
		char SessionKey[64];
		int Version;

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData(SessionKey, 64);
		Packet->GetData((char*)&Version, 4);

		// 2. 검증작업 -----------
		// 지금은 없음

		// 3. 값 셋팅
		m_Int64AccountNo = AccountNo;

		// 4. 로그인 응답 패킷 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		// 타입
		WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
		SendBuff->PutData((char*)&Type, 2);

		// Status
		BYTE Status = 1;
		SendBuff->PutData((char*)&Status, 1);

		// AccountNo
		SendBuff->PutData((char*)&AccountNo, 8);

		// SendPacket
		SendPacket(SendBuff);		

		// 5. AUTH 에서 GAME으로 모드 변경
		SetMode_GAME();
	}
	   
	// 테스트용 에코 요청
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CGameServer::CGameSession::Game_EchoTest(CProtocolBuff_Net* Packet)
	{
		// 1. 마샬링
		INT64 AccountNo;
		LONGLONG SendTick;

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)&SendTick, 8);

		// 2. AccountNo 확인하기
		// 로그인요청이 오지 않은 유저라면, 밖으로 예외 던짐
		if (AccountNo != m_Int64AccountNo)
		{
			throw CException(_T("Game_EchoTest() --> AccountNo Error!!"));
			return;
		}

		// 2. 그대로 다시 보내기
		WORD Type = en_PACKET_CS_GAME_RES_ECHO;

		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&SendTick, 8);

		SendPacket(SendBuff);
	}


}

// ---------------
// CGameServer
// CMMOServer를 상속받는 게임 서버
// ---------------
namespace Library_Jingyu
{
	// -----------------
	// 생성자와 소멸자
	// -----------------
	CGameServer::CGameServer()
		:CMMOServer()
	{
		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			g_GameServerDump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		g_GameServerLog->SetDirectory(L"GameServer");
		g_GameServerLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);		
	}

	CGameServer::~CGameServer()
	{
	}

	// GameServerStart
	// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
	//
	// Parameter : 없음
	// return : 실패 시 false
	bool CGameServer::GameServerStart()
	{
		// 1. 세션 셋팅
		m_cGameSession = new CGameSession[m_stConfig.MaxJoinUser];	

		int i = 0;
		while (i < m_stConfig.MaxJoinUser)
		{
			SetSession(&m_cGameSession[i], m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2);			
			++i;
		}
		
		
		// 2. 서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept, 
			m_stConfig.Nodelay, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
			return false;	

		// 서버 오픈 로그 찍기		
		g_GameServerLog->LogSave(L"GameServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// GameServerStop
	// 내부적으로 Stop 실행
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::GameServerStop()
	{
		// 1. 서버 종료
		if (GetServerState() == true)
			Stop();

		// 2. 세션 삭제
		delete[] m_cGameSession;
	}

	// 출력용 함수
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::ShowPrintf()
	{
		// 해당 프로세스의 사용량 체크할 클래스
		static CCpuUsage_Process ProcessUsage;

		// 화면 출력할 것 셋팅
		/*
		Total SessionNum : 					- MMOServer 의 세션수
		AuthMode SessionNum :				- Auth 모드의 유저 수
		GameMode SessionNum (Auth + Game) :	- Game 모드의 유저 수 (Auth + Game모드 유저 수)

		PacketPool_Net : 		- 외부에서 사용 중인 Net 직렬화 버퍼의 수	
		Accept Socket Queue :	- Accept Socket Queue 안의 일감 수

		Accept Total :		- Accept 전체 카운트 (accept 리턴시 +1)
		Accept TPS :		- 초당 Accept 처리 횟수
		Auth FPS :			- 초당 Auth 스레드 처리 횟수
		Game FPS :			- 초당 Game 스레드 처리 횟수

		Send TPS			- 초당 Send완료 횟수. (완료통지에서 증가)
		Recv TPS			- 초당 Recv완료 횟수. (패킷 1개가 완성되었을 때 증가. RecvProc에서 패킷에 넣기 전에 1씩 증가)		

		Net_BuffChunkAlloc_Count : - Net 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		ASQPool_ChunkAlloc_Count : - Accept Socket Queue에 들어가는 일감 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)

		----------------------------------------------------
		CPU usage [MMOServer:%.1f%% U:%.1f%% K:%.1f%%] - 프로세스 사용량.

		*/

		LONG AccpetTPS = g_lAcceptTPS_MMO;
		LONG SendTPS = g_lSendPostTPS_MMO;
		LONG RecvTPS = g_lRecvTPS_MMO;
		LONG AuthFPS = g_lAuthFPS;
		LONG GameFPS = g_lGameFPS;
		InterlockedExchange(&g_lAcceptTPS_MMO, 0);
		InterlockedExchange(&g_lSendPostTPS_MMO, 0);
		InterlockedExchange(&g_lRecvTPS_MMO, 0);
		InterlockedExchange(&g_lAuthFPS, 0);
		InterlockedExchange(&g_lGameFPS, 0);

		// 출력 전에, 프로세스 사용량 갱신
		ProcessUsage.UpdateCpuTime();

		printf("========================================================\n"
				"Total SessionNum : %lld\n"
				"AuthMode SessionNum : %d\n"
				"GameMode SessionNum : %d (Auth + Game : %d)\n\n"

				"PacketPool_Net : %d\n"
				"Accept Socket Queue : %d\n\n"

				"Accept Total : %lld\n"
				"Accept TPS : %d\n"
				"Send TPS : %d\n"
				"Recv TPS : %d\n\n"

				"Auth FPS : %d\n"
				"Game FPS : %d\n\n"				

				"Net_BuffChunkAlloc_Count : %d (Out : %d)\n"
				"ASQPool_ChunkAlloc_Count : %d (Out : %d)\n\n"

			"========================================================\n\n"
			"CPU usage [MMOServer:%.1f%% U:%.1f%% K:%.1f%%]\n",


			// ----------- 게임 서버용
			GetClientCount(), g_lAuthModeUserCount, g_lGameModeUserCount, g_lAuthModeUserCount+g_lGameModeUserCount,
			g_lAllocNodeCount, GetASQ_Count(),
			g_ullAcceptTotal_MMO, AccpetTPS, SendTPS, RecvTPS,
			AuthFPS, GameFPS,
			CProtocolBuff_Net::GetChunkCount(), CProtocolBuff_Net::GetOutChunkCount(),
			GetChunkCount(), GetOutChunkCount(),

			// ----------- 프로세스 사용량 
			ProcessUsage.ProcessTotal(), ProcessUsage.ProcessUser(), ProcessUsage.ProcessKernel()
			);
	}






	// -----------------------
	// 내부에서만 사용하는 함수
	// -----------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CGameServer::SetFile(stConfigFile* pConfig)
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
		// ChatServer config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MMOGAMESERVER")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("BindIP"), pConfig->BindIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("Port"), &pConfig->Port) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("CreateWorker"), &pConfig->CreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ActiveWorker"), &pConfig->ActiveWorker) == false)
			return false;

		// 생성 엑셉트
		if (Parser.GetValue_Int(_T("CreateAccept"), &pConfig->CreateAccept) == false)
			return false;

		// 헤더 코드
		if (Parser.GetValue_Int(_T("HeadCode"), &pConfig->HeadCode) == false)
			return false;

		// xorcode1
		if (Parser.GetValue_Int(_T("XorCode1"), &pConfig->XORCode1) == false)
			return false;

		// xorcode2
		if (Parser.GetValue_Int(_T("XorCode2"), &pConfig->XORCode2) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("Nodelay"), &pConfig->Nodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("MaxJoinUser"), &pConfig->MaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("LogLevel"), &pConfig->LogLevel) == false)
			return false;

		return true;
	}





	// -----------------------
	// 가상함수
	// -----------------------

	// AuthThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnAuth_Update()
	{

	}

	// GameThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnGame_Update()
	{}

	// 새로운 유저 접속 시, Auth에서 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CGameServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CGameServer::OnError(int error, const TCHAR* errorStr)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		g_GameServerLog->LogSave(L"GameServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
			errorStr, error);
	}



}