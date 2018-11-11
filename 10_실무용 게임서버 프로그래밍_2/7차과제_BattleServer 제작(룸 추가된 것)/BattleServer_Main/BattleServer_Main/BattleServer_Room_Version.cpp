#include "pch.h"
#include "BattleServer_Room_Version.h"
#include "Parser\Parser_Class.h"
#include "Protocol_Set\CommonProtocol_2.h"		// 차후 일반으로 변경 필요
#include "Log\Log.h"
#include "CPUUsage\CPUUsage.h"
#include "PDHClass\PDHCheck.h"

#include "shDB_Communicate.h"

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

#include <process.h>

using namespace rapidjson;

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

// 배틀서버 입장 토큰 에러
LONG g_lBattleEnterTokenError;

// auth의 로그인 패킷에서, DB 쿼리 시 결과로 -10(사용자 없음)이 올 경우
LONG g_lQuery_Result_Not_Find;

// auth의 로그인 패킷에서, DB 쿼리 시 결과로 -10도 아닌데 1이 아닌 에러일 경우 
LONG g_lTempError;

// auth의 로그인 패킷에서, 유저의 SessionKey(토큰)이 다를 경우
LONG g_lTokenError;

// auth의 로그인 패킷에서, 유저가 들고 온 버전이 다를 경우
LONG g_lVerError;

// auth의 로그인 패킷에서, 중복 로그인 시
LONG g_DuplicateCount;

// GQCS에서 세마포어 리턴 시 1 증가
extern LONG g_SemCount;





// ------------------
// CGameSession의 함수
// (CBattleServer_Room의 이너클래스)
// ------------------
namespace Library_Jingyu
{
	// 덤프용
	CCrashDump* g_BattleServer_Room_Dump = CCrashDump::GetInstance();

	// 로그용
	CSystemLog* g_BattleServer_RoomLog = CSystemLog::GetInstance();


	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CBattleServer_Room::CGameSession::CGameSession()
		:CMMOServer::cSession()
	{
		// ClientKey 초기값 셋팅
		m_int64ClientKey = m_i64Default_CK;
	}

	// 소멸자
	CBattleServer_Room::CGameSession::~CGameSession()
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
	void CBattleServer_Room::CGameSession::OnAuth_ClientJoin()
	{
		// 할거 없음
	}

	// 유저가 Auth 모드에서 나감
	//
	// Parameter : Game모드로 변경된것인지 알려주는 Flag. 디폴트 false(변경되지 않음)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_ClientLeave(bool bGame)
	{
		// 모드 변경일 경우
		if (bGame == true)
		{
			
		}

		// 실제 게임 종료일 경우
		else
		{
			// ClinetKey가 초기값이 아니라면, 로그인 패킷을 보낸 유저.
			// 로그인 패킷을 보낸 유저는, AccountNo 자료구조에 무조건 들어있음.
			if (m_int64ClientKey != m_i64Default_CK)
			{
				if (m_pParent->EraseAccountNoFunc(m_Int64AccountNo) == false)
					g_BattleServer_Room_Dump->Crash();
			}

			// ClientKey 초기값으로 셋팅.
			// !! 이걸 안하면, Release되는 중에 HTTP응답이 올 경우 !!
			// !! Auth_Update에서 이미 샌드큐가 모두 정리된 유저에게 또 SendPacket 가능성 !!
			// !! 이 경우, 전혀 엉뚱한 유저에게 Send가 될 수 있음 !!
			m_int64ClientKey = m_i64Default_CK;
		}

	}

	// Auth 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_Packet(CProtocolBuff_Net* Packet)
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
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// 덤프
			g_BattleServer_Room_Dump->Crash();
		}

	}



	// --------------- GAME 모드용 함수

	// 유저가 Game모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientJoin()
	{
		// 할거 없음
	}

	// 유저가 Game모드에서 나감
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientLeave()
	{
		// ClinetKey가 초기값이 아니라면, 로그인 패킷을 보낸 유저.
			// 로그인 패킷을 보낸 유저는, AccountNo 자료구조에 무조건 들어있음.
		if (m_int64ClientKey != m_i64Default_CK)
		{
			if (m_pParent->EraseAccountNoFunc(m_Int64AccountNo) == false)
				g_BattleServer_Room_Dump->Crash();
		}

		// ClientKey 초기값으로 셋팅.
		// !! 이걸 안하면, Release되는 중에 HTTP응답이 올 경우 !!
		// !! Game_Update에서 이미 샌드큐가 모두 정리된 유저에게 또 SendPacket 가능성 !!
		// !! 이 경우, 전혀 엉뚱한 유저에게 Send가 될 수 있음 !!
		m_int64ClientKey = m_i64Default_CK;
	}

	// Game 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_Packet(CProtocolBuff_Net* Packet)
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
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// 덤프
			g_BattleServer_Room_Dump->Crash();
		}

	}



	// --------------- Release 모드용 함수

	// Release된 유저.
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientRelease()
	{
		
	}




	// -----------------
	// 패킷 처리 함수
	// -----------------

	// 로그인 요청 
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Auth_LoginPacket(CProtocolBuff_Net* Packet)
	{
		// 1. 현재 ClientKey가 초기값이 아니면 Crash
		if (m_int64ClientKey != m_i64Default_CK)
			g_BattleServer_Room_Dump->Crash();


		// 2. AccountNo, ClientKey 마샬링
		INT64 AccountNo;
		INT64 ClinetKey;

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)&ClinetKey, 8);


		// 3. AccountNo, ClientKey 셋팅
		m_Int64AccountNo = AccountNo;
		m_int64ClientKey = ClinetKey;


		// 4. AccountNo 자료구조에 추가.
		// 이미 있으면(false 리턴) 중복 로그인으로 처리
		// 현재 유저에게는 실패 패킷, 접속 중인 유저는 DIsconnect.
		if (m_pParent->InsertAccountNoFunc(AccountNo, this) == false)
		{
			InterlockedIncrement(&g_DuplicateCount);

			// 로그인 실패 패킷 보내고 끊기.
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 타입
			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			SendBuff->PutData((char*)&Type, 2);

			// Status
			BYTE Status = 6;		// 중복 로그인
			SendBuff->PutData((char*)&Status, 1);

			// AccountNo
			SendBuff->PutData((char*)&AccountNo, 8);			

			// SendPacket
			SendPacket(SendBuff);


			// ------------- 기존 접속중이던 중복 유저도 종료

			// 중복 로그인 처리된 유저 접속 종료 요청
			CGameSession* ReleasePlayer = m_pParent->FindAccountNoFunc(AccountNo);
			
			// 그 사이에 종료해버린 유저라면, 접속 종료 안보냄.
			if(ReleasePlayer != nullptr)
				ReleasePlayer->Disconnect();

			return;
		}


		// 5. HTTP 통신 시작
		// 통신 후, 해당 패킷에 대한 후처리는 Auth_Update에게 넘긴다.
		DB_WORK* Send = m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();

		Send->m_wWorkType = eu_LOGIN;
		Send->m_i64UniqueKey = ClinetKey;
		Send->pPointer = this;

		// 직렬화 버퍼 전달 전에, 레퍼런스 카운트 Add
		Packet->Add();
		Send->m_pBuff = Packet;

		Send->AccountNo = AccountNo;

		// Select_Account.php 요청
		m_pParent->m_shDB_Communicate.DBReadFunc(Send, en_PHP_TYPE::SELECT_ACCOUNT);
	}
}


// ----------------------------------------
// 
// MMOServer를 이용한 배틀 서버. 룸 버전
//
// ----------------------------------------
namespace Library_Jingyu
{
	// Net 직렬화 버퍼 1개의 크기 (Byte)
	LONG g_lNET_BUFF_SIZE = 512;




	// -----------------------
	// 내부에서만 사용하는 함수
	// -----------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CBattleServer_Room::SetFile(stConfigFile* pConfig)
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
		// BATTLESERVER config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("BATTLESERVER")) == false)
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
		// 기본 config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CONFIG")) == false)
			return false;

		// VerCode
		if (Parser.GetValue_Int(_T("VerCode"), &m_uiVer_Code) == false)
			return false;






		////////////////////////////////////////////////////////
		// 마스터 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MASTERLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("MasterServerIP"), pConfig->MasterServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("MasterServerPort"), &pConfig->MasterServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("MasterClientCreateWorker"), &pConfig->MasterClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("MasterClientActiveWorker"), &pConfig->MasterClientActiveWorker) == false)
			return false;


		// Nodelay
		if (Parser.GetValue_Int(_T("MasterClientNodelay"), &pConfig->MasterClientNodelay) == false)
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

	// Start
	// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
	//
	// Parameter : 없음
	// return : 실패 시 false
	bool CBattleServer_Room::BattleServerStart()
	{
		// 1. 세션 셋팅
		m_cGameSession = new CGameSession[m_stConfig.MaxJoinUser];

		int i = 0;
		while (i < m_stConfig.MaxJoinUser)
		{
			// GameServer의 포인터 셋팅
			m_cGameSession[i].m_pParent = this;

			// 엔진에 세션 셋팅
			SetSession(&m_cGameSession[i], m_stConfig.MaxJoinUser);
			++i;
		}

		// 2. Battle 서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept,
			m_stConfig.Nodelay, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
			return false;

		// 3. 모니터링 서버와 연결되는, 랜 클라이언트 가동
		if (m_Monitor_LanClient->ClientStart(m_stConfig.MonitorServerIP, m_stConfig.MonitorServerPort, m_stConfig.MonitorClientCreateWorker,
			m_stConfig.MonitorClientActiveWorker, m_stConfig.MonitorClientNodelay) == false)
			return false;

		// 서버 오픈 로그 찍기		
		g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;

	}

	// Stop
	// 내부적으로 Stop 실행
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::BattleServerStop()
	{
		// 1. 모니터링 클라 종료
		if (m_Monitor_LanClient->GetClinetState() == true)
			m_Monitor_LanClient->ClientStop();

		// 2. 서버 종료
		if (GetServerState() == true)
			Stop();

		// 3. 세션 삭제
		delete[] m_cGameSession;

	}

	// 출력용 함수
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::ShowPrintf()
	{

	}



	// -----------------------
	// 패킷 후처리 함수
	// -----------------------

	// Login 패킷 후처리
	//
	// Parameter : DB_WORK_LOGIN*
	// return : 없음
	void CBattleServer_Room::Auth_LoginPacket_Last(DB_WORK_LOGIN* DBData)
	{
		// 1. ClientKey 체크
		CGameSession* NowPlayer = (CGameSession*)DBData->pPointer;

		// 다르면 이미 종료한 유저로 판단. 가능성 있는 상황
		if(NowPlayer->m_int64ClientKey != DBData->m_i64UniqueKey)
			return;			


		// 2. Json데이터 파싱하기 (UTF-16)
		GenericDocument<UTF16<>> Doc;
		Doc.Parse(DBData->m_tcRequest);

		int iResult = Doc[_T("result")].GetInt();


		// 3. DB 요청 결과 확인
		// 결과가 1이 아니라면, 
		if (iResult != 1)
		{
			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 2;

			// 결과가 -10일 경우 (회원가입 자체가 안되어 있음)
			if (iResult == -10)
				InterlockedIncrement(&g_lQuery_Result_Not_Find);

			// 그 외 기타 에러일 경우
			else
				InterlockedIncrement(&g_lTempError);

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
			return;
		}


		// 4. SessionKey, ConnectToekn, Ver_Code 마샬링
		char SessionKey[64];
		char ConnectToken[32];
		UINT VerCode;

		DBData->m_pBuff->GetData(SessionKey, 64);
		DBData->m_pBuff->GetData(ConnectToken, 32);
		DBData->m_pBuff->GetData((char*)&VerCode, 4);
		

		// 5. 토큰키 비교
		const TCHAR* tDBToekn = Doc[_T("sessionkey")].GetString();

		char DBToken[64];
		int len = (int)_tcslen(tDBToekn);
		WideCharToMultiByte(CP_UTF8, 0, tDBToekn, (int)_tcslen(tDBToekn), DBToken, len, NULL, NULL);

		if (memcmp(DBToken, SessionKey, 64) != 0)
		{
			// 토큰이 다를경우 Result 3(세션키 오류)를 보낸다.
			InterlockedIncrement(&g_lTokenError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Result = 3;

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
			return;
		}


		// 6. 배틀서버 입장 토큰 비교
		// "현재" 토큰과 먼저 비교
		if (memcmp(ConnectToken, cConnectToken_Now, 32) != 0)
		{
			// 다르다면 "이전" 토큰과 비교
			if (memcpy(ConnectToken, m_cConnectToken_Before, 32) != 0)
			{
				InterlockedIncrement(&g_lBattleEnterTokenError);

				// 그래도 다르다면 이상한 유저로 판단.
				// 배틀서버 입장 토큰이 다르다는 패킷 보낸다.
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
				BYTE Result = 3;	// 현재는 세션키 오류랑 같이 취급. 차후 수정될까?

				SendBuff->PutData((char*)&Type, 2);
				SendBuff->PutData((char*)&DBData->AccountNo, 8);
				SendBuff->PutData((char*)&Result, 1);

				NowPlayer->SendPacket(SendBuff);

				return;
			}
		}


		// 7. 버전 비교 
		if (m_uiVer_Code != VerCode)
		{
			// 버전이 다를경우 Result 5(버전 오류)를 보낸다.
			InterlockedIncrement(&g_lVerError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Result = 5;

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
			return;
		}


		// 8. 정상이면 정상 응답 패킷 보냄
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
		BYTE Result = 1;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&DBData->AccountNo, 8);
		SendBuff->PutData((char*)&Result, 1);

		NowPlayer->SendPacket(SendBuff);
	}




	// -----------------------
	// AccountNo 자료구조 관리 함수
	// -----------------------

	// AccountNo 자료구조에 유저를 추가하는 함수
	//
	// Parameter : AccountNo, CGameSession*
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CBattleServer_Room::InsertAccountNoFunc(INT64 AccountNo, CGameSession* InsertPlayer)
	{

		AcquireSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 락

		// 1. Insert
		auto ret = m_AccountNo_Umap.insert(make_pair(AccountNo, InsertPlayer));

		ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 언락

		// 2. 중복키일 시 false 리턴
		if (ret.second == false)
			return false;

		return true;
	}

	// AccountNo 자료구조에서 유저를 검색하는 함수
	//
	// Parameter : AccountNo
	// return : 성공 시 CGameSession*
	//		  : 실패 시 nullptr
	CBattleServer_Room::CGameSession* CBattleServer_Room::FindAccountNoFunc(INT64 AccountNo)
	{
		AcquireSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 락

		// 1. 검색
		auto ret = m_AccountNo_Umap.find(AccountNo);

		// 2. 없는 유저일 시 nullptr 리턴
		if (ret == m_AccountNo_Umap.end())
		{
			ReleaseSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락
			return nullptr;
		}

		// 3. 있는 유저라면 ClientKey 리턴
		CGameSession* RetPlayer = ret->second;

		ReleaseSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락

		return RetPlayer;
	}

	// AccountNo 자료구조에서 유저를 제거하는 함수
	//
	// Parameter : AccountNo
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CBattleServer_Room::EraseAccountNoFunc(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 락

		// 1. 검색
		auto ret = m_AccountNo_Umap.find(AccountNo);

		// 2. 없는 유저일 시 false 리턴
		if (ret == m_AccountNo_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 언락
			return false;
		}

		// 3. 있는 유저라면 Erase
		m_AccountNo_Umap.erase(ret);

		ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락

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
	void CBattleServer_Room::OnAuth_Update()
	{
		// ------------------- 방생성
		// !! 대기방 생성 조건 !!
		// - [현재 게임 내에 만들어진 방 수(모든 방 포함) < 최대 존재할 수 있는 방 수] 만족
		// - [현재 대기방 수 < 최대 존재할 수 있는 대기방 수] 만족

		// 1. 조건 체크
		// 현재 만들어진 총 방수
		if (m_iNowTotalRoomCount < m_iMaxTotalRoomCount)
		{
			// 현재 대기방 수 
			if (m_iNowWaitRoomCount < m_iMaxWaitRoomCount)
			{
				// 여기까지 오면 방생성 조건 만족

				// 1) 방 Alloc
				stRoom* NowRoom = m_Room_Pool->Alloc();

				// 2) 셋팅
				NowRoom->m_iRoomNo = InterlockedIncrement(&m_lGlobal_RoomNo);
				NowRoom->m_iJoinUserCount = 0;
				NowRoom->m_iJoinUserCount = NowRoom->m_iMaxJoinCount;

				// 토큰 셋팅				
				WORD Index = rand() % 64;	// 0 ~ 63 중 인덱스 골라내기				
				memcpy_s(NowRoom->m_cEnterToken, 32, m_cRoomEnterToken[Index], 32);

				// 3) 방 관리 자료구조에 추가
				 


			}
		}


		// ------------------- 토큰 재발급
		// 1. 


		// ------------------- HTTP 통신 후, 후처리
		// 1. Q 사이즈 확인
		int iQSize = m_shDB_Communicate.m_pDB_ReadComplete_Queue->GetInNode();

		// 한 프레임에 최대 50명의 업데이트 처리
		if (iQSize > 50)
			iQSize = 50;

		// 2. 있으면 로직 진행
		DB_WORK* DBData;
		while (iQSize > 0)
		{
			// 일감 디큐		
			if (m_shDB_Communicate.m_pDB_ReadComplete_Queue->Dequeue(DBData) == -1)
				g_BattleServer_Room_Dump->Crash();

			try
			{
				// 일감 타입에 따라 일감 처리
				switch (DBData->m_wWorkType)
				{
					// 로그인 패킷에 대한 후 처리
				case eu_DB_READ_TYPE::eu_LOGIN:
					Auth_LoginPacket_Last(DBData);
					break;

					// 없는 일감 타입이면 에러.
				default:
					g_BattleServer_Room_Dump->Crash();
					break;
				}

			}
			catch (CException& exc)
			{
				// 로그 찍기 (로그 레벨 : 에러)
				g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
					(TCHAR*)exc.GetExceptionText());

				// 덤프
				g_BattleServer_Room_Dump->Crash();
			}
			

			// DB_WORK 안의 직렬화 버퍼 반환
			CProtocolBuff_Net::Free(DBData->m_pBuff);

			// DB_WORK 반환
			m_shDB_Communicate.m_pDB_Work_Pool->Free(DBData);

			--iQSize;
		}

	}

	// GameThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnGame_Update()
	{

	}

	// 새로운 유저 접속 시, Auth에서 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CBattleServer_Room::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CBattleServer_Room::OnError(int error, const TCHAR* errorStr)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
			errorStr, error);
	}

	   	  

	// -----------------
	// 생성자와 소멸자
	// -----------------
	CBattleServer_Room::CBattleServer_Room()
		:CMMOServer()
	{	
		srand((UINT)time(NULL));

		m_iNowWaitRoomCount = 0;
		m_iNowTotalRoomCount = 0;
		m_lGlobal_RoomNo = 0;

		// 방 입장 토큰 만들어두기
		for (int i = 0; i < 64; ++i)
		{
			for (int h = 0; h < 32; ++h)
			{
				m_cRoomEnterToken[i][h] = (rand() % 128) + 1;
			}
		}	

		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			g_BattleServer_Room_Dump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		g_BattleServer_RoomLog->SetDirectory(L"CBattleServer_Room");
		g_BattleServer_RoomLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);
				
		// TLS 동적할당
		m_Room_Pool = new CMemoryPoolTLS<stRoom>(0, false);

		// 모니터링 서버와 통신하기 위한 LanClient 동적할당
		m_Monitor_LanClient = new CGame_MinitorClient;
		m_Monitor_LanClient->ParentSet(this);

		// reserve 셋팅.
		m_AccountNo_Umap.reserve(m_stConfig.MaxJoinUser);
		m_Room_Umap.reserve(m_iMaxTotalRoomCount);

		//SRW락 초기화
		InitializeSRWLock(&m_AccountNo_Umap_srwl);
	}

	CBattleServer_Room::~CBattleServer_Room()
	{
		// 모니터링 서버와 통신하는 LanClient 동적해제
		delete m_Monitor_LanClient;

		// TLS 동적 해제
		delete m_Room_Pool;
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

	// 배틀서버의 this를 입력받는 함수
	// 
	// Parameter : 배틀 서버의 this
	// return : 없음
	void CGame_MinitorClient::ParentSet(CBattleServer_Room* ChatThis)
	{
		m_BattleServer_this = ChatThis;
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
			// 배틀서버 정보 보내기
			// ----------------------------------

			// 배틀서버가 On일 경우, 게임서버 정보 보낸다.
			if (g_This->m_BattleServer_this->GetServerState() == true)
			{
				// 1. 배틀서버 ON		
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, TRUE, TimeStamp);

				// 2. 배틀서버 CPU 사용률 (커널 + 유저)
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_CPU, (int)CProcessCPU.ProcessTotal(), TimeStamp);

				// 3. 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
				int Data = (int)(CPdh.Get_UserCommit() / 1024 / 1024);
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, Data, TimeStamp);

				// 4. 배틀서버 패킷풀 사용량
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, g_lAllocNodeCount + g_lAllocNodeCount_Lan, TimeStamp);

				// 5. Auth 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, g_lShowAuthFPS, TimeStamp);

				// 6. Game 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, g_lShowGameFPS, TimeStamp);

				// 7. 배틀서버 접속 세션전체
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, (int)g_This->m_BattleServer_this->GetClientCount(), TimeStamp);

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