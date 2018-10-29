#include "pch.h"
#include "CGameServer.h"
#include "Parser\Parser_Class.h"
#include "Protocol_Set\CommonProtocol.h"
#include "Log\Log.h"
#include "CPUUsage\CPUUsage.h"
#include "PDHClass\PDHCheck.h"

#include <process.h>


extern ULONGLONG g_ullAcceptTotal_MMO;
extern LONG	  g_lAcceptTPS_MMO;
extern LONG	g_lSendPostTPS_MMO;
extern LONG	g_lRecvTPS_MMO;

extern LONG g_lAllocNodeCount;
extern LONG g_lAllocNodeCount_Lan;

extern LONG g_lAuthModeUserCount;
extern LONG g_lGameModeUserCount;

extern LONG g_lAuthFPS;
extern LONG g_lGameFPS;

LONG g_lShowAuthFPS;
LONG g_lShowGameFPS;

// GQCS에서 세마포어 리턴 시 1 증가
extern LONG g_SemCount;


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
		
		// 게임일 경우, 로그인 응답 패킷 보냄
		if (bGame == true)
		{
			// Alloc
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 타입
			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			SendBuff->PutData((char*)&Type, 2);

			// Status
			BYTE Status = 1;
			SendBuff->PutData((char*)&Status, 1);

			// AccountNo
			SendBuff->PutData((char*)&m_Int64AccountNo, 8);

			// SendPacket
			SendPacket(SendBuff);
		}
		
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
		// 자료구조에서 제거
		//m_pParent->AccountNO_Erase(m_Int64AccountNo);

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
		// 마샬링	----------------------
		INT64 AccountNo;
		char SessionKey[64];
		int Version;

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData(SessionKey, 64);
		Packet->GetData((char*)&Version, 4);

		
		// AccountNo 자료구조에 추가.	----------------------
		// 이미 있으면(false 리턴) 중복 로그인으로 처리
		/*
		if (m_pParent->AccountNO_Insert(AccountNo) == false)
		{
			// 로그인 실패 패킷 보내고 끊기.
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 타입
			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			SendBuff->PutData((char*)&Type, 2);

			// Status
			BYTE Status = 0;		// 실패
			SendBuff->PutData((char*)&Status, 1);

			// AccountNo
			SendBuff->PutData((char*)&AccountNo, 8);

			InterlockedIncrement(&g_DuplicateCount);

			// SendPacket
			SendPacket(SendBuff, TRUE);

			return;
		}
		*/


		// 3. m_Int64AccountNo의 값이 0x0fffffffffffffff이 아니면, 뭔가 이상함.
		// 코드 잘못이라고 밖에 못 봄.
		if(m_Int64AccountNo != 0x0fffffffffffffff)
		{
			throw CException(_T("Auth_LoginPacket() --> AccountNo Error!!"));
		}

		// 4. 값 셋팅
		m_Int64AccountNo = AccountNo;

		/*
		// 로그인 응답 패킷 보내기

		// Alloc
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
		*/

		// 5. 정상이면, 모드를 AUTH_TO_GAME으로 변경 요청.	
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
		// 다르면 로그 남기고 접속 끊음.
		if (AccountNo != m_Int64AccountNo)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_GameServerLog->LogSave(L"GameServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s : AccountNo : %lld",
				L"Game_EchoTest --> AccountNo Error", AccountNo);

			Disconnect();	
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
// CMMOServer를 상속받는 게임 서버  (배틀서버)
// ---------------
namespace Library_Jingyu
{

	// Net 직렬화 버퍼 1개의 크기 (Byte)
	LONG g_lNET_BUFF_SIZE = 200;

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

		// 모니터링 서버와 통신하기 위한 LanClient 동적할당
		m_Monitor_LanClient = new CGame_MinitorClient;
		m_Monitor_LanClient->ParentSet(this);

		// U_set의 reserve 셋팅.
		// 미리 공간 잡아둔다.
		m_setAccount.reserve(m_stConfig.MaxJoinUser);

		// U_set용 SRW락 초기화
		InitializeSRWLock(&m_setSrwl);
	}

	CGameServer::~CGameServer()
	{
		// 모니터링 서버와 통신하는 LanClient 동적해제
		delete m_Monitor_LanClient;
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
			// GameServer의 포인터 셋팅
			m_cGameSession[i].m_pParent  = this;

			// 엔진에 세션 셋팅
			SetSession(&m_cGameSession[i], m_stConfig.MaxJoinUser);			
			++i;
		}		
		
		// 2. 게임 서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept, 
			m_stConfig.Nodelay, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
			return false;	

		// 3. 모니터링 서버와 연결되는, 랜 클라이언트 가동
		if (m_Monitor_LanClient->ClientStart(m_stConfig.MonitorServerIP, m_stConfig.MonitorServerPort, m_stConfig.MonitorClientCreateWorker,
			m_stConfig.MonitorClientActiveWorker, m_stConfig.MonitorClientNodelay) == false)
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
		// 1. 모니터링 클라 종료
		if (m_Monitor_LanClient->GetClinetState() == true)
			m_Monitor_LanClient->ClientStop();

		// 2. 서버 종료
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
		// 화면 출력할 것 셋팅
		/*
		Monitor Connect :					- 모니터링 서버와 연결 여부. 1이면 연결됨.
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

		DuplicateCount :	- 중복 로그인으로 인해 보내고 끊기를 할 시 1씩 증가

		----------------------------------------------------

		*/		

		LONG AccpetTPS = InterlockedExchange(&g_lAcceptTPS_MMO, 0);
		LONG SendTPS = InterlockedExchange(&g_lSendPostTPS_MMO, 0);
		LONG RecvTPS = InterlockedExchange(&g_lRecvTPS_MMO, 0);
		g_lShowAuthFPS = InterlockedExchange(&g_lAuthFPS, 0);
		g_lShowGameFPS = InterlockedExchange(&g_lGameFPS, 0);

		printf("========================================================\n"
				"Monitor Connect : %d\n"
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
				
				"SemCount : %d\n\n"

			"========================================================\n\n",

			// ----------- 게임 서버용
			m_Monitor_LanClient->GetClinetState(),
			GetClientCount(), g_lAuthModeUserCount, g_lGameModeUserCount, g_lAuthModeUserCount+g_lGameModeUserCount,
			g_lAllocNodeCount, GetASQ_Count(),
			g_ullAcceptTotal_MMO, AccpetTPS, SendTPS, RecvTPS,
			g_lShowAuthFPS, g_lShowGameFPS,
			CProtocolBuff_Net::GetChunkCount(), CProtocolBuff_Net::GetOutChunkCount(),
			GetChunkCount(), GetOutChunkCount(),
			g_SemCount

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
		// GameServer config 읽어오기
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
			   

		////////////////////////////////////////////////////////
		// 모니터링 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("MonitorServerIP"), pConfig->MonitorServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("MonitorServerPort"), &pConfig->MonitorServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientCreateWorker"), &pConfig->MonitorClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientActiveWorker"), &pConfig->MonitorClientActiveWorker) == false)
			return false;


		// Nodelay
		if (Parser.GetValue_Int(_T("MonitorClientNodelay"), &pConfig->MonitorClientNodelay) == false)
			return false;



		return true;
	}

	// AccountNo 자료구조 "검색용"
	//
	// Parameter : AccountNO(INT64)
	// return : 있을 시 true, 없을 시 false
	bool CGameServer::AccountNO_Find(INT64 AccountNo)
	{		
		AcquireSRWLockShared(&m_setSrwl);		// Shared 락 ------------------

		// 1. 검색
		auto Find = m_setAccount.find(AccountNo);

		// 2. 만약, 없는 AccountNo라면 false 리턴
		if (Find == m_setAccount.end())
		{
			ReleaseSRWLockShared(&m_setSrwl);		// Shared 언락  ------------------ 
			return false;
		}

		ReleaseSRWLockShared(&m_setSrwl);		// Shared 언락  ------------------ 

		// 3. 있다면 true 리턴
		return true;
	}

	// AccountNo 자료구조 "추가"
	//
	// Parameter : AccountNO(INT64)
	// return : 이미 자료구조에 존재할 시 false, 
	//			존재하며 정상적으로 추가될 시 true
	bool CGameServer::AccountNO_Insert(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_setSrwl);		// Exclusive 락 ------------------

		// 1. 추가
		auto ret = m_setAccount.insert(AccountNo);

		// 2. 만약, 이미 있다면 중복된 유저. return false.
		if (ret.second == false)
		{
			ReleaseSRWLockExclusive(&m_setSrwl);		// Exclusive 언락  ------------------ 
			return false;
		}

		// 3. 없다면, 잘 추가된것. true 리턴
		ReleaseSRWLockExclusive(&m_setSrwl);		// Exclusive 언락  ------------------ 
		return true;
	}

	// AccountNo 자료구조 "삭제"
	//
	// Parameter : AccountNO(INT64)
	// return : 없음.
	//			자료구조에 존재하지 않을 시 Crash
	void CGameServer::AccountNO_Erase(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_setSrwl);		// Exclusive 락 ------------------

		// 1. 검색
		auto Find = m_setAccount.find(AccountNo);

		// 2. 만약, 없다면 말이안됨. 
		if (Find == m_setAccount.end())
		{
			ReleaseSRWLockExclusive(&m_setSrwl);	//  Exclusive \언락 ------------------

			// 말이 안됨. 크래시
			g_GameServerDump->Crash();
		}

		// 3. 있다면 Set에서 제거	
		m_setAccount.erase(Find);

		ReleaseSRWLockExclusive(&m_setSrwl);	// ---------------- Unlock
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


// ---------------
// CGame_MinitorClient
// CLanClienet를 상속받는 모니터링 클라
// ---------------
namespace Library_Jingyu
{
	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	CGame_MinitorClient::CGame_MinitorClient()
		:CLanClient()
	{
		// 모니터링 서버 정보전송 스레드를 종료시킬 이벤트 생성
		//
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		m_hMonitorThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	}

	CGame_MinitorClient::~CGame_MinitorClient()
	{
		// 아직 연결이 되어있으면, 연결 해제
		if (GetClinetState() == true)
			ClientStop();

		// 이벤트 삭제
		CloseHandle(m_hMonitorThreadExitEvent);
	}



	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 시작 함수
	// 내부적으로, 상속받은 CLanClient의 Start호출.
	//
	// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
	// return : 성공 시 true , 실패 시 falsel 
	bool CGame_MinitorClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 모니터링 서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
			return false;

		// 모니터링 서버로 정보 전송할 스레드 생성
		m_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);

		return true;
	}

	// 종료 함수
	// 내부적으로, 상속받은 CLanClient의 Stop호출.
	// 추가로, 리소스 해제 등
	//
	// Parameter : 없음
	// return : 없음
	void CGame_MinitorClient::ClientStop()
	{
		// 1. 모니터링 서버 정보전송 스레드 종료
		SetEvent(m_hMonitorThreadExitEvent);

		// 종료 대기
		if (WaitForSingleObject(m_hMonitorThread, INFINITE) == WAIT_FAILED)
		{
			DWORD Error = GetLastError();
			printf("MonitorThread Exit Error!!! (%d) \n", Error);
		}

		// 2. 스레드 핸들 반환
		CloseHandle(m_hMonitorThread);

		// 3. 모니터링 서버와 연결 종료
		Stop();
	}
	
	// 게임서버의 this를 입력받는 함수
	// 
	// Parameter : 게임 서버의 this
	// return : 없음
	void CGame_MinitorClient::ParentSet(CGameServer* ChatThis)
	{
		m_GameServer_this = ChatThis;
	}




	// -----------------------
	// 내부에서만 사용하는 기능 함수
	// -----------------------

	// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
	UINT	WINAPI CGame_MinitorClient::MonitorThread(LPVOID lParam)
	{
		// this 받아두기
		CGame_MinitorClient* g_This = (CGame_MinitorClient*)lParam;

		// 종료 신호 이벤트 받아두기
		HANDLE* hEvent = &g_This->m_hMonitorThreadExitEvent;

		// CPU 사용율 체크 클래스 (채팅서버 소프트웨어)
		CCpuUsage_Process CProcessCPU;

		// CPU 사용율 체크 클래스 (하드웨어)
		CCpuUsage_Processor CProcessorCPU;

		// PDH용 클래스
		CPDH	CPdh;

		while (1)
		{
			// 1초에 한번 깨어나서 정보를 보낸다.
			DWORD Check = WaitForSingleObject(*hEvent, 1000);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("MoniterThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 만약, 종료 신호가 왔다면 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;


			// 그게 아니라면, 일을 한다.

			// 프로세서, 프로세스 CPU 사용율, PDH 정보 갱신
			CProcessorCPU.UpdateCpuTime();
			CProcessCPU.UpdateCpuTime();
			CPdh.SetInfo();

			// ----------------------------------
			// 하드웨어 정보 보내기 (프로세서)
			// ----------------------------------
			int TimeStamp = (int)(time(NULL));

			// 1. 하드웨어 CPU 사용률 전체
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL, (int)CProcessorCPU.ProcessorTotal(), TimeStamp);

			// 2. 하드웨어 사용가능 메모리 (MByte)
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY, (int)CPdh.Get_AVA_Mem(), TimeStamp);

			// 3. 하드웨어 이더넷 수신 바이트 (KByte)
			int iData = (int)(CPdh.Get_Net_Recv() / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV, iData, TimeStamp);

			// 4. 하드웨어 이더넷 송신 바이트 (KByte)
			iData = (int)(CPdh.Get_Net_Send() / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND, iData, TimeStamp);

			// 5. 하드웨어 논페이지 메모리 사용량 (MByte)
			iData = (int)(CPdh.Get_NonPaged_Mem() / 1024 / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY, iData, TimeStamp);



			// ----------------------------------
			// 게임서버 정보 보내기
			// ----------------------------------

			// 게임서버가 On일 경우, 게임서버 정보 보낸다.
			if (g_This->m_GameServer_this->GetServerState() == true)
			{			
				// 1. 게임서버 ON		
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, TRUE, TimeStamp);

				// 2. 게임서버 CPU 사용률 (커널 + 유저)
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_CPU, (int)CProcessCPU.ProcessTotal(), TimeStamp);

				// 3. 게임서버 메모리 유저 커밋 사용량 (Private) MByte
				int Data = (int)(CPdh.Get_UserCommit() / 1024 / 1024);
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, Data, TimeStamp);

				// 4. 게임서버 패킷풀 사용량
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, g_lAllocNodeCount + g_lAllocNodeCount_Lan, TimeStamp);

				// 5. Auth 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, g_lShowAuthFPS, TimeStamp);

				// 6. Game 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, g_lShowGameFPS, TimeStamp);

				// 7. 게임서버 접속 세션전체
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, (int)g_This->m_GameServer_this->GetClientCount(), TimeStamp);

				// 8. Auth 스레드 모드 인원
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH, g_lAuthModeUserCount, TimeStamp);

				// 9. Game 스레드 모드 인원
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME, g_lGameModeUserCount, TimeStamp);

				// 10. 대기방 수
				
				// 11. 플레이 방 수
			}

		}

		return 0;
	}

	// 모니터링 서버로 데이터 전송
	//
	// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
	// return : 없음
	void CGame_MinitorClient::InfoSend(BYTE DataType, int DataValue, int TimeStamp)
	{
		WORD Type = en_PACKET_SS_MONITOR_DATA_UPDATE;

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&DataType, 1);
		SendBuff->PutData((char*)&DataValue, 4);
		SendBuff->PutData((char*)&TimeStamp, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}


	// -----------------------
	// 순수 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CGame_MinitorClient::OnConnect(ULONGLONG SessionID)
	{
		m_ullSessionID = SessionID;

		// 모니터링 서버(Lan)로 로그인 패킷 보냄
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_SS_MONITOR_LOGIN;
		int ServerNo = dfSERVER_NO;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerNo, 4);

		SendPacket(SessionID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CGame_MinitorClient::OnDisconnect(ULONGLONG SessionID)
	{}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, CProtocolBuff_Lan*
	// return : 없음
	void CGame_MinitorClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CGame_MinitorClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGame_MinitorClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGame_MinitorClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CGame_MinitorClient::OnError(int error, const TCHAR* errorStr)
	{}



}