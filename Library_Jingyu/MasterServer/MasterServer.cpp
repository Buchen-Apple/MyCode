#include "pch.h"
#include "MasterServer.h"
#include "Log/Log.h"
#include "CrashDump/CrashDump.h"

#include <strsafe.h>

// ----------------- 매치메이킹용 출력 변수

// 로그인 시, 토큰 에러가 난 횟수
LONG g_lMatch_TokenError;

// 로그인 시, 이미 로그인 된 매칭서버가 또 로그인 요청함.
LONG g_lMatch_DuplicateLogin;

// 로그인 되지 않은 유저가 패킷을 보냄.
LONG g_lMatch_NotLoginPacket;


// -----------------------
//
// 마스터 Match 서버
// 
// -----------------------
namespace Library_Jingyu
{
	// 덤프용
	CCrashDump* g_MasterDump = CCrashDump::GetInstance();

	// 로그 저장용
	CSystemLog* g_MasterLog = CSystemLog::GetInstance();
	

	// -------------------------------------
	// 내부에서만 사용하는 함수
	// -------------------------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CMatchServer_Lan::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MasterServer_Config.ini"));
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
		// 매치메이킹 랜서버의 Config
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MATCH")) == false)
			return false;

		// BindIP
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

		// Nodelay
		if (Parser.GetValue_Int(_T("Nodelay"), &pConfig->Nodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("MaxJoinUser"), &pConfig->MaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("LogLevel"), &pConfig->LogLevel) == false)
			return false;

		// 토큰
		TCHAR tcEnterToken[33];
		if (Parser.GetValue_String(_T("EnterToken"), tcEnterToken) == false)
			return false;

		// 클라가 들고올 때는 char형으로 들고오기 때문에 변환해서 가지고 있어야한다.
		int len = WideCharToMultiByte(CP_UTF8, 0, tcEnterToken, lstrlenW(tcEnterToken), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, tcEnterToken, lstrlenW(tcEnterToken), pConfig->EnterToken, len, NULL, NULL);






		////////////////////////////////////////////////////////
		// 배틀 랜서버의 Config
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("BATTLE")) == false)
			return false;

		// BindIP
		if (Parser.GetValue_String(_T("BattleBindIP"), pConfig->BattleBindIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("BattlePort"), &pConfig->BattlePort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("BattleCreateWorker"), &pConfig->BattleCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("BattleActiveWorker"), &pConfig->BattleActiveWorker) == false)
			return false;

		// 생성 엑셉트
		if (Parser.GetValue_Int(_T("BattleCreateAccept"), &pConfig->BattleCreateAccept) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("BattleNodelay"), &pConfig->BattleNodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("BattleMaxJoinUser"), &pConfig->BattleMaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("BattleLogLevel"), &pConfig->BattleLogLevel) == false)
			return false;

		// 토큰
		TCHAR tcBattleEnterToken[33];
		if (Parser.GetValue_String(_T("EnterToken"), tcBattleEnterToken) == false)
			return false;

		// 클라가 들고올 때는 char형으로 들고오기 때문에 변환해서 가지고 있어야한다.
		len = WideCharToMultiByte(CP_UTF8, 0, tcBattleEnterToken, lstrlenW(tcBattleEnterToken), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, tcBattleEnterToken, lstrlenW(tcBattleEnterToken), pConfig->BattleEnterToken, len, NULL, NULL);


		return true;

	}



	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 서버 시작
	//
	// Parameter : 없음
	// return : 실패 시 false.
	bool CMatchServer_Lan::ServerStart()
	{
		// 매칭 랜서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, 
			m_stConfig.CreateAccept, m_stConfig.Nodelay, m_stConfig.MaxJoinUser) == false)
		{
			return false;
		}

		// 매칭 서버 오픈 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"MatchServerOpen...");

		// 배틀 랜서버 시작
		if(pBattleServer->ServerStart() == false)
		{
			return false;
		}		

		return true;
	}
	
	// 서버 종료
	//
	// Parameter : 없음
	// return : 없음
	void CMatchServer_Lan::ServerStop()
	{
		// 매칭 랜서버 종료
		Stop();

		// 매칭 서버 닫힘 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"MatchServerClose...");

		// 배틀 랜서버 종료
		if (pBattleServer->GetServerState() == true)
			pBattleServer->ServerStop();
	}



	// -----------------
	// 접속한 매칭 서버 관리용 자료구조 함수
	// -----------------

	// 접속자 관리 자료구조에 Insert
	// umap으로 관리중
	//
	// Parameter : SessionID, stMatching*
	// return : 실패 시 false 리턴
	bool CMatchServer_Lan::InsertMatchServerFunc(ULONGLONG SessionID, stMatching* insertServer)
	{
		// 1. umap에 추가		
		AcquireSRWLockExclusive(&m_srwl_MatchServer_Umap);		// ------- Exclusive 락

		auto ret = m_MatchServer_Umap.insert(make_pair(SessionID, insertServer));

		// 2. 중복된 키일 시 false 리턴.
		if (ret.second == false)
		{
			ReleaseSRWLockExclusive(&m_srwl_MatchServer_Umap);		// ------- Exclusive 언락
			return false;
		}
		return true;

	}


	// 접속자 관리 자료구조에서, 유저 검색
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID
	// return : 검색 성공 시, stMatchingr*
	//		  : 검색 실패 시 nullptr
	CMatchServer_Lan::stMatching* CMatchServer_Lan::FindMatchServerFunc(ULONGLONG SessionID)
	{
		// 1. umap에서 검색
		AcquireSRWLockShared(&m_srwl_MatchServer_Umap);		// ------- Shared 락

		auto FindPlayer = m_MatchServer_Umap.find(SessionID);

		ReleaseSRWLockShared(&m_srwl_MatchServer_Umap);		// ------- Shared 언락

		// 2. 검색 실패 시 nullptr 리턴
		if (FindPlayer == m_MatchServer_Umap.end())
			return nullptr;

		// 3. 검색 성공 시, 찾은 stPlayer* 리턴
		return FindPlayer->second;

	}


	// 접속자 관리 자료구조에서 Erase
	// umap으로 관리중
	//
	// Parameter : SessionID
	// return : 성공 시, 제거된 stMatching*
	//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
	CMatchServer_Lan::stMatching* CMatchServer_Lan::EraseMatchServerFunc(ULONGLONG SessionID)
	{
		// 1. umap에서 유저 검색
		AcquireSRWLockExclusive(&m_srwl_MatchServer_Umap);		// ------- Exclusive 락

		auto FindPlayer = m_MatchServer_Umap.find(SessionID);
		if (FindPlayer == m_MatchServer_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_srwl_MatchServer_Umap);		// ------- Exclusive 언락
			return nullptr;
		}

		// 2. 둘 다에서 존재한다면, 리턴할 값 받아두기
		stMatching* ret = FindPlayer->second;

		// 3. umap에서 제거
		m_MatchServer_Umap.erase(FindPlayer);

		ReleaseSRWLockExclusive(&m_srwl_MatchServer_Umap);		// ------- Exclusive 언락

		return ret;

	}





	// -----------------
	// 접속한 매칭 서버 관리용 자료구조 함수(로그인 한 유저 관리)
	// -----------------

	// 로그인 한 유저 관리 자료구조에 Insert
	// uset으로 관리중
	//
	// Parameter : ServerNo
	// return : 실패 시 false 리턴
	bool CMatchServer_Lan::InsertLoginMatchServerFunc(int ServerNo)
	{
		// 1. uset에 추가		
		AcquireSRWLockExclusive(&m_srwl_LoginMatServer_Uset);		// ------- Exclusive 락

		auto ret = m_LoginMatServer_Uset.insert(ServerNo);

		ReleaseSRWLockExclusive(&m_srwl_LoginMatServer_Uset);		// ------- Exclusive 언락

		// 2. 중복된 키일 시 false 리턴.
		if (ret.second == false)		
			return false;

		return true;
	}

	// 로그인 한 유저 관리 자료구조에서 Erase
	// uset으로 관리중
	//
	// Parameter : ServerNo
	// return : 실패 시 false 리턴
	bool CMatchServer_Lan::EraseLoginMatchServerFunc(int ServerNo)
	{
		// 1. uset에서 검색
		AcquireSRWLockExclusive(&m_srwl_LoginMatServer_Uset);	// ------- Exclusive 락

		auto FindPlayer = m_LoginMatServer_Uset.find(ServerNo);
		if (FindPlayer == m_LoginMatServer_Uset.end())
		{
			// 못찾았으면 false 리턴
			ReleaseSRWLockExclusive(&m_srwl_LoginMatServer_Uset);	// ------- Exclusive 언락

			return false;
		}

		// 2. 잘 찾았으면 erase
		m_LoginMatServer_Uset.erase(FindPlayer);

		ReleaseSRWLockExclusive(&m_srwl_LoginMatServer_Uset);	// ------- Exclusive 언락

		// 3. true 리턴
		return true;

	}





	// -----------------
	// 패킷 처리용 함수
	// -----------------

	// Login패킷 처리
	//
	// Parameter : SessionID, Payload
	// return : 없음
	void CMatchServer_Lan::Packet_Login(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 마샬링
		int ServerNo;
		char MasterToken[32];

		Payload->GetData((char*)&ServerNo, 4);
		Payload->GetData(MasterToken, 32);

		// 2. 입장 토큰키 비교.		
		if (memcmp(MasterToken, m_stConfig.BattleEnterToken, 32) != 0)
		{
			InterlockedIncrement(&g_lMatch_TokenError);

			g_MasterDump->Crash();

			/*
			// 다를 경우, 로그 찍고, 응답 없이 끊는다.
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Packet_Login() --> EnterToken Error. SessionID : %lld, ServerNo : %d", SessionID, ServerNo);

			Disconnect(SessionID);

			return;
			*/
		}

		// 3. 중복로그인 체크를 위해 자료구조에 Insert
		if (InsertLoginMatchServerFunc(ServerNo) == false)
		{
			InterlockedIncrement(&g_lMatch_DuplicateLogin);

			g_MasterDump->Crash();

			/*
			// false가 리턴되는 것은 이미 접속중인 매칭 서버.
			// 로그 찍고, 응답 없이 끊는다.
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Packet_Login() --> Duplicate Login. ServerNo : %d", ServerNo);

			Disconnect(SessionID);

			return;
			*/
		}

		// 4. 로그인 중이 아니라면, SessionID를 이용해 검색
		stMatching* NowUser = FindMatchServerFunc(SessionID);
		if (NowUser == nullptr)
			g_MasterDump->Crash();

		// 5. 이 유저가 로그인 중인지 체크
		// 말도 안되는 상황이지만 혹시를 위해 체크
		if (NowUser->m_bLoginCheck == true)
			g_MasterDump->Crash();

		// 6. 로그인 Flag 변경
		NowUser->m_bLoginCheck = true;

		// 7. 정보 셋팅
		NowUser->m_iServerNo = ServerNo;

		// 8. 응답패킷 보냄
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_MAT_MAS_RES_SERVER_ON;
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerNo, 4);

		SendPacket(SessionID, SendBuff);
	}
	
	// 방 정보 요청
	// 정보 뽑은 후, Battle로 전달
	//
	// Parameter : SessionID, Payload
	// return : 없음
	void CMatchServer_Lan::Relay_RoomInfo(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 매칭 서버 검색
		stMatching* NowUser = FindMatchServerFunc(SessionID);
		if (NowUser == nullptr)
			g_MasterDump->Crash();


		// 2. 매칭 서버의 로그인 상태 확인
		if (NowUser->m_bLoginCheck == false)
		{
			InterlockedIncrement(&g_lMatch_NotLoginPacket);

			// 로그 남기고 접속 끊음.
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Packet_RoomInfo() --> Not Login Packet. ServerNo : %d", NowUser->m_iServerNo);

			Disconnect(SessionID);

			return;
		}

		// 3. 마샬링
		UINT64 ClinetKey;
		Payload->GetData((char*)&ClinetKey, 8);

		// 4. 배틀용 랜서버로 릴레이
		// 이제 처리는 배틀용 랜서버가 담당
		pBattleServer->Relay_Battle_Room_Info(SessionID, ClinetKey, NowUser->m_iServerNo);
	}
	
	// 방 입장 성공
	//
	// Parameter : SessionID, Payload
	// return : 없음
	void CMatchServer_Lan::Packet_RoomEnter_OK(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 매칭 서버 검색
		stMatching* NowUser = FindMatchServerFunc(SessionID);
		if (NowUser == nullptr)
			g_MasterDump->Crash();

		// 2. 매칭 서버의 로그인 상태 확인
		if (NowUser->m_bLoginCheck == false)
		{
			InterlockedIncrement(&g_lMatch_NotLoginPacket);

			// 로그 남기고 접속 끊음.
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Packet_RoomEnter_OK() --> Not Login Packet. ServerNo : %d", NowUser->m_iServerNo);

			Disconnect(SessionID);

			return;
		}

		// 3. 마샬링
		WORD BattleServerNo;
		int RoomNo;
		UINT64 ClinetKey;

		Payload->GetData((char*)&BattleServerNo, 2);
		Payload->GetData((char*)&RoomNo, 4);
		Payload->GetData((char*)&ClinetKey, 8);
		
		// ------------ 배틀 랜 서버의 자료구조에서 삭제
		// 1. 배틀의 함수 호출. 0이 리턴되어야 정상
		// 내부에서 stPlayer*를 Free까지 한다.
		if(pBattleServer->RoomEnter_OK_Func(ClinetKey, RoomNo, BattleServerNo) != 0)
			g_MasterDump->Crash();

	}
	
	// 방 입장 실패
	//
	// Parameterr : SessionID, Payload
	// return : 없음
	void CMatchServer_Lan::Packet_RoomEnter_Fail(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 매칭 서버 검색
		stMatching* NowUser = FindMatchServerFunc(SessionID);
		if (NowUser == nullptr)
			g_MasterDump->Crash();

		// 2. 매칭 서버의 로그인 상태 확인
		if (NowUser->m_bLoginCheck == false)
		{
			InterlockedIncrement(&g_lMatch_NotLoginPacket);

			// 로그 남기고 접속 끊음.
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Packet_RoomEnter_OK() --> Not Login Packet. ServerNo : %d", NowUser->m_iServerNo);

			Disconnect(SessionID);

			return;
		}

		// 3. 마샬링
		UINT64 ClinetKey;
		Payload->GetData((char*)&ClinetKey, 8);

		// ------------ 배틀 랜 서버의 자료구조에서 삭제
		// 1. 배틀의 함수 호출. 0이 리턴되어야 정상인 것.
		// Room을 체크해 카운트 증감처리
		// 내부에서 stPlayer*를 Free까지 한다.
		if (pBattleServer->RoomEnter_Fail_Func(ClinetKey) != 0)
			g_MasterDump->Crash();
	}









	// -----------------------
	// 가상함수
	// -----------------------

	bool CMatchServer_Lan::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	void CMatchServer_Lan::OnClientJoin(ULONGLONG SessionID)
	{
		// 1. CMatchingServer* Alloc
		stMatching* NewJoin = m_TLSPool_MatchServer->Alloc();

		// 2. SessionID 셋팅
		NewJoin->m_ullSessionID = SessionID;

		// 3. Insert
		if (InsertMatchServerFunc(SessionID, NewJoin) == false)
			g_MasterDump->Crash();

	}

	void CMatchServer_Lan::OnClientLeave(ULONGLONG SessionID)
	{
		// 1. 자료구조에서 매칭서버 제거
		stMatching* NowUser = EraseMatchServerFunc(SessionID);
		if (NowUser == nullptr)
			g_MasterDump->Crash();

		// 2. 배틀 랜 서버의 stPlayer* 중, 해당 매칭서버가 동적할당 한게 남아있으면 모두 지워준다.
		pBattleServer->MatchLeave(NowUser->m_iServerNo);

		// 3. 로그인 패킷 여부
		if (NowUser->m_bLoginCheck == true)
		{
			// 로그인되었던 매칭 서버라면, uset에서도 제거
			if (EraseLoginMatchServerFunc(NowUser->m_iServerNo) == false)
				g_MasterDump->Crash();

			// 초기화
			NowUser->m_bLoginCheck = false;
		}			

		// 4. 반환
		m_TLSPool_MatchServer->Free(NowUser);
	}

	void CMatchServer_Lan::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 타입 빼오기
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 타입에 따라 분기처리
		try
		{
			switch (Type)
			{
				// 방 정보 요청
			case en_PACKET_MAT_MAS_REQ_GAME_ROOM:
				Relay_RoomInfo(SessionID, Payload);
				break;

				// 유저 방 입장 성공
			case en_PACKET_MAT_MAS_REQ_ROOM_ENTER_SUCCESS:
				Packet_RoomEnter_OK(SessionID, Payload);
				break;

				// 유저 방 입장 실패
			case en_PACKET_MAT_MAS_REQ_ROOM_ENTER_FAIL:
				Packet_RoomEnter_Fail(SessionID, Payload);
				break;

				// 매치메이킹 로그인 요청
			case en_PACKET_MAT_MAS_REQ_SERVER_ON:
				Packet_Login(SessionID, Payload);
				break;

				// 타입 에러
			default:
				TCHAR ErrStr[1024];
				StringCchPrintf(ErrStr, 1024, _T("MatchAread OnRecv(). TypeError. Type : %d, SessionID : %lld"),
					Type, SessionID);

				throw CException(ErrStr);
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// Crash
			g_MasterDump->Crash();

			// 접속 끊기 요청
			//Disconnect(SessionID);
		}
	}

	void CMatchServer_Lan::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	void CMatchServer_Lan::OnWorkerThreadBegin()
	{}

	void CMatchServer_Lan::OnWorkerThreadEnd()
	{}

	void CMatchServer_Lan::OnError(int error, const TCHAR* errorStr)
	{}



	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CMatchServer_Lan::CMatchServer_Lan()
		:CLanServer()
	{
		//  Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			g_MasterDump->Crash();

		// 로그 저장할 파일 셋팅
		g_MasterLog->SetDirectory(L"MasterServer");
		g_MasterLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);
		
		// 자료구조 공간 미리 잡아두기
		m_MatchServer_Umap.reserve(100);
		m_LoginMatServer_Uset.rehash(100);

		// 락 초기화		
		InitializeSRWLock(&m_srwl_MatchServer_Umap);
		InitializeSRWLock(&m_srwl_LoginMatServer_Uset);

		// TLS 동적할당
		m_TLSPool_MatchServer = new CMemoryPoolTLS<stMatching>(0, false);

		// 배틀 서버 동적할당
		pBattleServer = new CBattleServer_Lan;

	}

	// 소멸자
	CMatchServer_Lan::~CMatchServer_Lan()
	{
		// TLS 동적 해제
		delete m_TLSPool_MatchServer;	
	}

}


// -----------------------
//
// 마스터 Battle 서버
// 
// -----------------------
namespace Library_Jingyu
{

	// ----------------------------------------
	// 접속한 배틀서버 관리용 자료구조 함수
	// ----------------------------------------

	// 배틀서버 관리 자료구조에 배틀서버 Insert
	//
	// Parameter : SessionID, stBattle*
	// return : 성공 시 true
	//		  : 실패 시 false(중복키)
	bool CBattleServer_Lan::InsertBattleServerFunc(ULONGLONG SessionID, stBattle* InsertBattle)
	{
		AcquireSRWLockExclusive(&m_srwl_BattleServer_Umap);		// ----- Battle서버 Umap Exclusive 락
		
		// 1. 자료구조에 Insert
		auto ret = m_BattleServer_Umap.insert(make_pair(SessionID, InsertBattle));

		ReleaseSRWLockExclusive(&m_srwl_BattleServer_Umap);		// ----- Battle서버 Umap Exclusive 언락
	
		// 2. 중복 키라면 false 리턴
		if (ret.second == false)
			return false;

		return true;	
	}

	// 배틀서버 관리 자료구조에서 배틀서버 erase
	//
	// Parameter : SessionID
	// return : 성공 시 stBattle*
	//		  : 실패 시 nullptr
	CBattleServer_Lan::stBattle* CBattleServer_Lan::EraseBattleServerFunc(ULONGLONG SessionID)
	{
		AcquireSRWLockExclusive(&m_srwl_BattleServer_Umap);		// ----- Battle서버 Umap Exclusive 락

		// 1. 자료구조에서 find
		auto FIndBattle = m_BattleServer_Umap.find(SessionID);

		// 못찾았으면 nullptr 리턴
		if (FIndBattle == m_BattleServer_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_srwl_BattleServer_Umap);		// ----- Battle서버 Umap Exclusive 언락
			return nullptr;
		}

		// 2. 리턴할 값 받아두기
		stBattle* EraseBattle = FIndBattle->second;

		// 3. Erase
		m_BattleServer_Umap.erase(FIndBattle);

		ReleaseSRWLockExclusive(&m_srwl_BattleServer_Umap);		// ----- Battle서버 Umap Exclusive 언락

		return EraseBattle;
	}



	// -----------------------------------------
	// 접속한 배틀서버 관리용 자료구조 함수 (Set)
	// -----------------------------------------

	// 배틀서버 Set 자료구조에 배틀서버 Insert
	//
	// Parameter : ServerNo
	// return : 성공 시 true
	//		  : 실패 시 false(중복키)
	bool CBattleServer_Lan::InsertBattleServerFunc_Set(int ServerNo)
	{
		AcquireSRWLockExclusive(&m_srwl_LoginBattleServer_Uset);	 // ----- 배틀서버 Set Exclusive 락
	
		// 1. ServerNo를 기준으로 Insert
		auto ret = m_LoginBattleServer_Uset.insert(ServerNo);

		ReleaseSRWLockExclusive(&m_srwl_LoginBattleServer_Uset);	 // ----- 배틀서버 Set Exclusive 언락

		// 2. 중복 키라면 false 리턴
		if (ret.second == false)
			return false;


		return true;	
	}

	// 배틀서버 Set 자료구조에서 배틀서버 erase
	//
	// Parameter : ServerNo
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CBattleServer_Lan::EraseBattleServerFunc_Set(int ServerNo)
	{
		AcquireSRWLockExclusive(&m_srwl_LoginBattleServer_Uset);	 // ----- 배틀서버 Set Exclusive 락

		// 1. 존재 유무 검색
		auto ret = m_LoginBattleServer_Uset.find(ServerNo);

		// 없을 경우, return false
		if (ret == m_LoginBattleServer_Uset.end())
		{
			ReleaseSRWLockExclusive(&m_srwl_LoginBattleServer_Uset);	 // ----- 배틀서버 Set Exclusive 언락
			return false;
		}

		// 2. 있을 경우 Erase
		m_LoginBattleServer_Uset.erase(ret);

		ReleaseSRWLockExclusive(&m_srwl_LoginBattleServer_Uset);	 // ----- 배틀서버 Set Exclusive 언락
		return true;
	}





	// -----------------
	// 플레이어 관리용 자료구조 함수
	// -----------------

	// 플레이어 관리 자료구조에 플레이어 Insert
	//
	// Parameter : ClientKey, stPlayer*
	// return : 정상적으로 Insert 시 true
	//		  : 실패 시 false
	bool CBattleServer_Lan::InsertPlayerFunc(UINT64 ClientKey, stPlayer* InsertPlayer)
	{
		// 1. 배틀의 플레이어 자료구조에 추가		
		AcquireSRWLockExclusive(&m_srwl_Player_Umap);		// ------- Exclusive 락

		auto ret = m_Player_Umap.insert(make_pair(ClientKey, InsertPlayer));

		ReleaseSRWLockExclusive(&m_srwl_Player_Umap);		// ------- Exclusive 언락

		// 2. 중복된 키일 시 false 리턴.
		if (ret.second == false)		
			return false;

		return true;
	}





	// -----------------
	// 내부에서만 사용하는 함수
	// -----------------

	// 매칭으로 방 입장 성공 패킷이 올 시 호출되는 함수
	// 1. RoomNo, BattleServerNo가 정말 일치하는지 확인
	// 2. 자료구조에서 Erase
	// 3. stPlayer*를 메모리풀에 Free
	//
	// !! 매칭 랜 서버에서 호출 !!
	//
	// Parameter : ClinetKey, RoomNo, BattleServerNo
	//
	// return Code
	// 0 : 정상적으로 삭제됨
	// 1 : 존재하지 않는 유저 (ClinetKey로 유저 검색 실패)
	// 2 : RoomNo 불일치
	// 3 : BattleServerNo 불일치
	int CBattleServer_Lan::RoomEnter_OK_Func(UINT64 ClinetKey, int RoomNo, int BattleServerNo)
	{
		AcquireSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 락

		// 1. 유저 검색
		auto FIndPlayer = m_Player_Umap.find(ClinetKey);

		// 존재하지 않으면 return false
		if (FIndPlayer == m_Player_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 언락
			return 1;
		}

		stPlayer* ErasePlayer = FIndPlayer->second;


		// 2. RoomNo 체크
		if (ErasePlayer->m_iJoinRoomNo != RoomNo)
		{
			ReleaseSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 언락
			return 2;
		}


		// 3. BattleServerNo 체크
		if (ErasePlayer->m_iBattleServerNo != BattleServerNo)
		{
			ReleaseSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 언락
			return 3;
		}


		// 4. 모두 일치하면 Erase
		m_Player_Umap.erase(FIndPlayer);

		ReleaseSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 언락


		// 5. stPlayer* 반환
		m_TLSPool_Player->Free(ErasePlayer);

		// 6. 정상 코드 리턴
		return 0;
	}

	// 매칭으로 방 입장 실패가 올 시 호출되는 함수
	// 1. 유저 검색
	// 2. 해당 ClinetKey가 접속한 방 찾기
	// 3. 찾은 방의 Set에서 해당 유저 제거
	// 4. stPlayer*를 메모리풀에 Free
	//
	// !! 매칭 랜 서버에서 호출 !!
	//
	// Parameter : ClinetKey
	//
	// return Code
	// 0 : 정상적으로 삭제됨
	// 1 : 존재하지 않는 유저 (ClinetKey로 유저 검색 실패)
	// 2 : Room 존재하지 않음
	// 3 : Room 내의 자료구조(Set)에서 ClientKey로 검색 실패
	int CBattleServer_Lan::RoomEnter_Fail_Func(UINT64 ClinetKey)
	{
		// 1. 유저 검색
		AcquireSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 Exclusive 락
		auto FIndPlayer = m_Player_Umap.find(ClinetKey);

		// 존재하지 않으면 return 1
		if (FIndPlayer == m_Player_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 Exclusive 언락
			return 1;
		}

		stPlayer * ErasePlayer = FIndPlayer->second;

		// 룸 Key 받아두기
		UINT64 RoomKey = ErasePlayer->m_ullRoomKey;

		// Player Erase
		m_Player_Umap.erase(FIndPlayer);

		ReleaseSRWLockExclusive(&m_srwl_Player_Umap);		// ----- Battle Player 자료구조 Exclusive 언락

		// stPlayer* 반환
		m_TLSPool_Player->Free(ErasePlayer);

		



		// 2. Room 검색
		AcquireSRWLockExclusive(&m_srwl_Room_Umap);		// ----- Room umap 자료구조 Shared 락

		auto FindRoom = m_Room_Umap.find(RoomKey);

		// 존재하지 않으면 return 2
		if (FIndPlayer == m_Player_Umap.end())
		{
			ReleaseSRWLockShared(&m_srwl_Room_Umap);		// ----- Room umap 자료구조 Shared 언락
			return 2;
		}

		stRoom* NowRoom = FindRoom->second;

		NowRoom->RoomLOCK();			// ----- 룸 1개에 대한 락




		// 3. 룸에서 해당 ClientKey의 유저 제거
		auto TempUser = NowRoom->m_uset_JoinUser.find(ClinetKey);

		// 존재하지 않으면 return 3
		if (TempUser == NowRoom->m_uset_JoinUser.end())
		{
			NowRoom->RoomUNLOCK();						// ----- 룸 1개에 대한 언락
			return 3;
		}

		// 존재하면 삭제.
		NowRoom->m_uset_JoinUser.erase(TempUser);
		



		// 4. 룸에서 유저 수 1 증가	
		NowRoom->m_iEmptyCount++;

		// 증가했는데 1보다 작다면 말이 안됨
		if (NowRoom->m_iEmptyCount < 1)
			g_MasterDump->Crash();

		// 1이라면, pq에 다시 넣는다.
		if (NowRoom->m_iEmptyCount == 1)
		{
			NowRoom->RoomUNLOCK();						// ----- 룸 1개에 대한 언락

			AcquireSRWLockExclusive(&m_srwl_Room_pq);	// ----- pq Exclusive 락

			m_Room_pq.push(NowRoom);

			ReleaseSRWLockExclusive(&m_srwl_Room_pq);	// ----- pq Exclusive 언락
		}

		else
			NowRoom->RoomUNLOCK();						// ----- 룸 1개에 대한 언락


		ReleaseSRWLockShared(&m_srwl_Room_Umap);		// ----- Room umap 자료구조 Shared 언락
				


		// 5. 정상 코드 리턴
		return 0;
	}
		
	// 매칭서버와 연결이 끊길 시, 해당 매칭서버로 인해 할당된 stPlayer*를 모두 반환하는 함수
	//
	// Parameter : 매칭 서버 No(int)
	// return : 없음
	void CBattleServer_Lan::MatchLeave(int MatchServerNo)
	{
		AcquireSRWLockExclusive(&m_srwl_Player_Umap);	// ----- 플레이어 uamp Exclusive 락

		// 순회하면서 인자로 받은 MatchServerNo를 가지고 있는 stPlayer*를 Free 한다
		auto itor_Now = m_Player_Umap.begin();
		auto itor_End = m_Player_Umap.end();

		while (1)
		{
			if (itor_Now == itor_End)
				break;

			stPlayer* NowPlayer = itor_Now->second;

			if (NowPlayer->m_iMatchServerNo == MatchServerNo)
			{
				itor_Now = m_Player_Umap.erase(itor_Now);
				m_TLSPool_Player->Free(NowPlayer);
			}

			else
				++itor_Now;
		}

		ReleaseSRWLockExclusive(&m_srwl_Player_Umap);	// ----- 플레이어 uamp Exclusive 언락
	}

	// 배틀서버와 연결이 끊길 시, 해당 매칭서버의 룸을 모두 제거한다.
	// 룸 자료구조 모두(2개)에서 제거한다.
	//
	// Parameter : BattleServerNo
	// return : 없음
	void CBattleServer_Lan::BattleLeave(int BattleServerNo)
	{
		// -------------------------- Priority Queue 처리

		// 임시 저장 공간
		vector<stRoom*> TempPlace;
		TempPlace.reserve(3000);

		AcquireSRWLockExclusive(&m_srwl_Room_pq);	 // ----- pq 룸 Exclusive 락				

		//  Priority Queue에서 해당 BattleServerNo의 룸을 뺀다.
		size_t iPqSize = m_Room_pq.size();
		while (1)
		{
			if (iPqSize == 0)
				break;

			// 1. pop
			stRoom* NowRoom = m_Room_pq.top();
			m_Room_pq.pop();

			// 2. ServerNo 확인
			// 다르면 임시 저장 공간에 넣는다.
			if (NowRoom->m_iBattleServerNo != BattleServerNo)
				TempPlace.push_back(NowRoom);

			--iPqSize;
		}

		// 임시 저장 공간에 있는 Room을 다시 모두 넣는다.
		// 임시 저장 공간에는 배틀서버No가 다른 방들만 모여있다.
		size_t i = 0;
		size_t End = TempPlace.size();
		while (1)
		{
			if (i < End)
				break;

			m_Room_pq.push(TempPlace[i]);

			++i;
		}	

		ReleaseSRWLockExclusive(&m_srwl_Room_pq);	 // ----- pq 룸 Exclusive 언락




		// -------------------------- umap 처리

		AcquireSRWLockExclusive(&m_srwl_Room_Umap);	 // ----- uamp 룸 Exclusive 락

		// 모두 순회하며, ServerNo가 같은 경우 erase 및 Free
		auto itor_Now = m_Room_Umap.begin();
		auto itor_End = m_Room_Umap.end();

		while (1)
		{
			if (itor_Now == itor_End)
				break;

			stRoom* NowRoom = itor_Now->second;

			// ServerNo가 같은 경우, Erase 및 메모리풀에 Free
			if (NowRoom->m_iBattleServerNo == BattleServerNo)
			{
				itor_Now = m_Room_Umap.erase(itor_Now);
				m_TLSPool_Room->Free(NowRoom);
			}

			else
				++itor_Now;
		}

		ReleaseSRWLockExclusive(&m_srwl_Room_Umap);	 // ----- uamp 룸 Exclusive 언락
	}




	// -------------------------------
	// 외부에서 호출 가능한 함수
	// -------------------------------

	// 서버 시작
	//
	// Parameter : 없음
	// return : 실패 시 false.
	bool CBattleServer_Lan::ServerStart()
	{
		// 배틀 랜서버 시작
		if (Start(pMatchServer->m_stConfig.BattleBindIP, pMatchServer->m_stConfig.BattlePort, pMatchServer->m_stConfig.BattleCreateWorker,
			pMatchServer->m_stConfig.BattleActiveWorker, pMatchServer->m_stConfig.BattleCreateAccept, pMatchServer->m_stConfig.BattleNodelay, 
			pMatchServer->m_stConfig.BattleMaxJoinUser) == false)
		{
			return false;
		}

		// 배틀 랜 서버 오픈 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"BattleServerOpen...");

		return true;
	}

	// 서버 종료
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Lan::ServerStop()
	{
		Stop();
		
		// 배틀 랜 서버 닫힘 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"BattleServerClose...");
	}






	// -------------------------------
	// 패킷 처리 함수
	// -------------------------------

	// 방 정보 요청 패킷 처리
	// !! 매칭 랜 서버에서 호출 !!
	//
	// Parameter : SessionID(매칭 쪽의 SessionID), ClientKey, 매칭서버의 No
	// return : 없음
	void CBattleServer_Lan::Relay_Battle_Room_Info(ULONGLONG SessionID, UINT64 ClientKey, int MatchServerNo)
	{
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		AcquireSRWLockExclusive(&m_srwl_Room_pq);	// ----- pq 룸 Exclusive 락

		// 1. 마스터 서버 내에 배정 가능한 룸이 1개 이상 존재하는지 확인
		if (m_Room_pq.empty() == true)
		{
			ReleaseSRWLockExclusive(&m_srwl_Room_pq);	// ----- pq 룸 Exclusive 언락

			// 룸이 하나도 없으면, 방 없음 패킷 리턴
			WORD Type = en_PACKET_MAT_MAS_RES_GAME_ROOM;
			BYTE Status = 0;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&ClientKey, 8);
			SendBuff->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendBuff);

			return;
		}


		// 2. 룸 얻기
		stRoom* NowRoom = m_Room_pq.top();

		NowRoom->RoomLOCK();		// ----- 룸 1개에 대한 락


		// 3. 룸 안의 여유 유저 수 확인
		// 0명이면 여기 있으면 안됨.
		if (NowRoom->m_iEmptyCount == 0)
			g_MasterDump->Crash();


		// 4. 룸의 유저 수 감소 + 룸 내 자료구조에 추가
		NowRoom->m_iEmptyCount--;

		auto ret = NowRoom->m_uset_JoinUser.insert(ClientKey);
		if(ret.second == false)
			g_MasterDump->Crash();


		// 5. 감소 후 여유 유저 수가 0명이라면, pop
		if (NowRoom->m_iEmptyCount == 0)
			m_Room_pq.pop();

		ReleaseSRWLockExclusive(&m_srwl_Room_pq);	// ----- pq 룸 Exclusive 언락


		// 6. Room에서 필요한 정보들 받아두기
		int iSendRoomNo = NowRoom->m_iRoomNo;
		int iBattleServerNo = NowRoom->m_iBattleServerNo;
		char cSendEnterToken[32];
		memcpy_s(cSendEnterToken, 32, NowRoom->m_cEnterToken, 32);

		NowRoom->RoomUNLOCK();						// ----- 룸 1개에 대한 언락


		// 7. Send할 데이터 셋팅 --- 1차
		WORD Type = en_PACKET_MAT_MAS_RES_GAME_ROOM;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ClientKey, 8);		


		// 8. 해당 룸이 있는 배틀서버의 정보 알아오기
		AcquireSRWLockShared(&m_srwl_BattleServer_Umap);	// ------------- 배틀서버 자료구조 Shared 락

		auto FindBattle = m_BattleServer_Umap.find(iBattleServerNo);

		// 배틀서버가 없으면 방 없음 패킷 보냄
		if (FindBattle == m_BattleServer_Umap.end())
		{
			ReleaseSRWLockShared(&m_srwl_BattleServer_Umap);	// ------------- 배틀서버 자료구조 Shared 언락
			
			// 방 없음 패킷 리턴
			BYTE Status = 0;

			SendBuff->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendBuff);

			return;			
		}

		stBattle* NowBattle = FindBattle->second;


		// 9. Send할 데이터 셋팅 --- 2차
		BYTE Status = 1;
		SendBuff->PutData((char*)&Status, 1);

		SendBuff->PutData((char*)&NowBattle->m_iServerNo, 2);
		SendBuff->PutData((char*)NowBattle->m_tcBattleIP, 32);
		SendBuff->PutData((char*)&NowBattle->m_wBattlePort, 2);
		SendBuff->PutData((char*)&iSendRoomNo, 4);
		SendBuff->PutData(NowBattle->m_cConnectToken, 32);
		SendBuff->PutData(cSendEnterToken, 32);

		SendBuff->PutData((char*)NowBattle->m_tcChatIP, 32);
		SendBuff->PutData((char*)NowBattle->m_wChatPort, 2);

		ReleaseSRWLockShared(&m_srwl_BattleServer_Umap);	// ------------- 배틀서버 자료구조 Shared 언락


		// 10. 배틀의 플레이어 관리 자료구조에 추가
		stPlayer* NewPlayer = m_TLSPool_Player->Alloc();

		NewPlayer->m_ui64ClinetKey = ClientKey;
		NewPlayer->m_iJoinRoomNo = iSendRoomNo;
		NewPlayer->m_iBattleServerNo = iBattleServerNo;
		NewPlayer->m_iMatchServerNo = MatchServerNo;


		// 배틀의 자료구조에 추가
		if(InsertPlayerFunc(ClientKey, NewPlayer) == false)
			g_MasterDump->Crash();	


		// 11. 매칭 Lan서버를 통해, 방 정보 Send하기
		SendPacket(SessionID, SendBuff);
	}
	




	// -----------------------
	// 가상함수
	// -----------------------

	bool CBattleServer_Lan::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	void CBattleServer_Lan::OnClientJoin(ULONGLONG SessionID)
	{
		// stBattle 할당받기
		stBattle* NewBattleServer = m_TLSPool_BattleServer->Alloc();

		// 정보 셋팅
		NewBattleServer->m_ullSessionID = SessionID;

		// 자료구조에 Insert
		InsertBattleServerFunc(SessionID, NewBattleServer);
	}

	void CBattleServer_Lan::OnClientLeave(ULONGLONG SessionID)
	{
		// 1. 배틀서버 uamp에서 Erase
		stBattle* EraseBattle =  EraseBattleServerFunc(SessionID);

		// 2. 해당 배틀서버의 룸들 모두 삭제
		BattleLeave(EraseBattle->m_iServerNo);

		// 3. 만약, 로그인 상태라면 Set에서도 제거
		if (EraseBattle->m_bLoginCheck == true)
		{
			if (EraseBattleServerFunc_Set(EraseBattle->m_iServerNo) == false)
				g_MasterDump->Crash();

			// 로그인 상태를 false로 변경
			EraseBattle->m_bLoginCheck = false;
		}

		// 4. stBattle* 반환
		m_TLSPool_BattleServer->Free(EraseBattle);
	}

	void CBattleServer_Lan::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 타입에 따라 분기 처리
		try
		{
			switch (Type)
			{
				// 신규 대기방 생성
			case en_PACKET_BAT_MAS_REQ_CREATED_ROOM:
				break;

				// 방 닫힘
			case en_PACKET_BAT_MAS_REQ_CLOSED_ROOM:
				break;

				// 유저 나감
			case en_PACKET_BAT_MAS_REQ_LEFT_USER:
				break;

				// 토큰 재발행
			case en_PACKET_BAT_MAS_REQ_CONNECT_TOKEN:
				break;

				// 배틀서버 로그인 
			case en_PACKET_BAT_MAS_REQ_SERVER_ON:
				break;

				// Type Error
			default:
				TCHAR ErrStr[1024];
				StringCchPrintf(ErrStr, 1024, _T("BattleArea OnRecv(). TypeError. Type : %d, SessionID : %lld"),
					Type, SessionID);

				throw CException(ErrStr);
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// Crash
			g_MasterDump->Crash();

			// 접속 끊기 요청
			//Disconnect(SessionID);
		}
		
	}

	void CBattleServer_Lan::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	void CBattleServer_Lan::OnWorkerThreadBegin()
	{}

	void CBattleServer_Lan::OnWorkerThreadEnd()
	{}

	void CBattleServer_Lan::OnError(int error, const TCHAR* errorStr)
	{}




	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CBattleServer_Lan::CBattleServer_Lan()
	{
		// 락 초기화
		InitializeSRWLock(&m_srwl_BattleServer_Umap);
		InitializeSRWLock(&m_srwl_Room_Umap);
		InitializeSRWLock(&m_srwl_Room_pq);
		InitializeSRWLock(&m_srwl_LoginBattleServer_Uset);

		// TLS 동적할당
		m_TLSPool_BattleServer = new CMemoryPoolTLS<stBattle>(0, false);
		m_TLSPool_Room = new CMemoryPoolTLS<stRoom>(0, false);
		m_TLSPool_Player = new CMemoryPoolTLS<stPlayer>(0, false);

		// 자료구조 공간 미리 잡아두기
		m_BattleServer_Umap.reserve(100);
		m_Room_Umap.reserve(10000);
		m_LoginBattleServer_Uset.reserve(100);
	}

	// 소멸자
	CBattleServer_Lan::~CBattleServer_Lan()
	{
		// TLS 동적해제
		delete m_TLSPool_BattleServer;
		delete m_TLSPool_Room;
		delete m_TLSPool_Player;
	}
}