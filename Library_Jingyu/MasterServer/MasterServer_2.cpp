#include "pch.h"
#include "MasterServer.h"

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
// 마스터 Lan서버
// 외부에서는 이 클래스에 접근하며, 이 클래스가 각 Lan서버를 시작.
// 
// -----------------------
namespace Library_Jingyu
{
	// 덤프용
	CCrashDump* g_MasterDump = CCrashDump::GetInstance();

	// 로그 저장용
	CSystemLog* g_MasterLog = CSystemLog::GetInstance();
	


	// -------------------------------------
	// 외부에서 사용 가능한 함수
	// -------------------------------------

	// 마스터 서버 시작
	// 각종 Lan서버를 시작한다.
	// 
	// Parameter : 없음
	// return : 실패 시 false 리턴
	bool CMasterServer::ServerStart()
	{
		// 각 랜서버에 MasterServer 셋팅
		m_pMatchServer->SetParent(this);
		m_pBattleServer->SetParent(this);

		// 매칭용 랜서버 시작
		if (m_pMatchServer->ServerStart() == false)
		{
			return false;
		}

		// 배틀용 랜서버 시작
		if (m_pBattleServer->ServerStart() == false)
		{
			return false;
		}

		// 서버 오픈 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"AllServerOpen...");

		return true;
	}

	// 마스터 서버 종료
	// 각종 Lan서버를 종료
	// 
	// Parameter : 없음
	// return : 없음
	void CMasterServer::ServerStop()
	{
		// 각 랜서버 종료
		// 매칭 랜서버 작동중인지 체크 후 종료
		if (m_pMatchServer->GetServerState() == true)
			m_pMatchServer->ServerStop();

		// 배틀 랜서버 작동중인지 체크 후 종료
		if (m_pBattleServer->GetServerState() == true)
			m_pBattleServer->ServerStop();

		// 서버 닫힘 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"AllServerClose...");
	}
	   

	// -------------------------------------
	// Player 관리용 자료구조 함수
	// -------------------------------------

	// Player 관리 자료구조 "2개"에 Isnert
	//
	// Parameter : ClinetKey, AccountNo
	// return : 중복될 시 false.
	bool CMasterServer::InsertPlayerFunc(UINT64 ClinetKey, UINT64 AccountNo)
	{
		// 1. ClinetKey용 uset에 추가		
		AcquireSRWLockExclusive(&m_srwl_ClientKey_Umap);		// ------- ClinetKey Exclusive 락

		auto ret_A = m_ClientKey_Uset.insert(ClinetKey);

		// 2. 중복된 키일 시 false 리턴.
		if (ret_A.second == false)
		{
			ReleaseSRWLockExclusive(&m_srwl_ClientKey_Umap);		// ------- ClinetKey Exclusive 언락
			return false;
		}

		ReleaseSRWLockExclusive(&m_srwl_ClientKey_Umap);		// ------- ClinetKey Exclusive 언락

		// 3. AccountNo용 uset에 추가
		AcquireSRWLockExclusive(&m_srwl_m_AccountNo_Umap);		// ------- AccountNo Exclusive 락

		auto ret_B = m_AccountNo_Uset.insert(AccountNo);

		ReleaseSRWLockExclusive(&m_srwl_m_AccountNo_Umap);		// ------- AccountNo Exclusive 언락

		// 4. 중복된 키일 시 false 리턴.
		if (ret_B.second == false)
			return false;

		return true;
	}

	// ClinetKey 관리용 자료구조에서 Erase
	// umap으로 관리중
	//
	// Parameter : ClinetKey
	// return : 없는 유저일 시 false
	//		  : 있는 유저일 시 true
	bool CMasterServer::ErasePlayerFunc(UINT64 ClinetKey)
	{
		// 1. umap에서 유저 검색
		AcquireSRWLockExclusive(&m_srwl_ClientKey_Umap);		// ------- Exclusive 락

		auto FindPlayer = m_ClientKey_Uset.find(ClinetKey);
		if (FindPlayer == m_ClientKey_Uset.end())
		{
			ReleaseSRWLockExclusive(&m_srwl_ClientKey_Umap);		// ------- Exclusive 언락
			return false;
		}	

		// 2. 존재한다면 umap에서 제거
		m_ClientKey_Uset.erase(FindPlayer);

		ReleaseSRWLockExclusive(&m_srwl_ClientKey_Umap);		// ------- Exclusive 언락

		return true;
	}



	// -------------------------------------
	// 내부에서만 사용하는 함수
	// -------------------------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CMasterServer::SetFile(stConfigFile* pConfig)
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




	// -------------------------------------
	// 생성자와 소멸자
	// -------------------------------------

	// 생성자
	CMasterServer::CMasterServer()
	{
		// 각 랜 서버 동적할당
		m_pMatchServer = new CMatchServer_Lan;
		m_pBattleServer = new CBattleServer_Lan;

		//  Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			g_MasterDump->Crash();

		// 로그 저장할 파일 셋팅
		g_MasterLog->SetDirectory(L"MasterServer");
		g_MasterLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);


		// 락 초기화
		InitializeSRWLock(&m_srwl_BattleServer_Umap);
		InitializeSRWLock(&m_srwl_ClientKey_Umap);
		InitializeSRWLock(&m_srwl_m_AccountNo_Umap);
		InitializeSRWLock(&m_srwl_Room_Umap);
		InitializeSRWLock(&m_srwl_Room_pq);

		// TLS 동적할당
		m_TLSPool_BattleServer = new CMemoryPoolTLS<CMasterServer::CBattleServer>(0, false);
		m_TLSPool_Room = new CMemoryPoolTLS<CRoom>(0, false);
		

		// 자료구조 공간 미리 잡아두기
		m_BattleServer_Umap.reserve(1000);
		m_ClientKey_Uset.reserve(5000);
		m_AccountNo_Uset.reserve(5000);
		m_Room_Umap.reserve(5000);
	}
	
	// 소멸자
	CMasterServer::~CMasterServer()
	{
		// 매칭 랜서버 작동중인지 체크 후 종료
		if (m_pMatchServer->GetServerState() == true)
			m_pMatchServer->ServerStop();

		// 배틀 랜서버 작동중인지 체크 후 종료
		if (m_pBattleServer->GetServerState() == true)
			m_pBattleServer->ServerStop();
		
		delete m_pMatchServer;
		delete m_pBattleServer;

		// TLS 동적 해제
		delete m_TLSPool_Room;
		delete m_TLSPool_BattleServer;
	}
}




// -----------------------
//
// 매치메이킹과 연결되는 Lan 서버
// 
// -----------------------
namespace Library_Jingyu
{	
	// -----------------
	// umap용 자료구조 관리 함수
	// -----------------

	// 접속자 관리 자료구조에 Insert
	// umap으로 관리중
	//
	// Parameter : SessionID, CMatchingServer*
	// return : 실패 시 false 리턴
	bool CMatchServer_Lan::InsertPlayerFunc(ULONGLONG SessionID, CMasterServer::CMatchingServer* insertServer)
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
	// return : 검색 성공 시, CMatchingServer*
	//		  : 검색 실패 시 nullptr
	CMasterServer::CMatchingServer* CMatchServer_Lan::FindPlayerFunc(ULONGLONG SessionID)
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
	// return : 성공 시, 제거된 CMatchingServer*
	//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
	CMasterServer::CMatchingServer* CMatchServer_Lan::ErasePlayerFunc(ULONGLONG SessionID)
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
		CMasterServer::CMatchingServer* ret = FindPlayer->second;

		// 3. umap에서 제거
		m_MatchServer_Umap.erase(FindPlayer);

		ReleaseSRWLockExclusive(&m_srwl_MatchServer_Umap);		// ------- Exclusive 언락

		return ret;
	}




	// -----------------
	// uset용 자료구조 관리 함수
	// -----------------

	// 로그인 한 유저 관리 자료구조에 Insert
	// uset으로 관리중
	//
	// Parameter : ServerNo
	// return : 실패 시 false 리턴
	bool CMatchServer_Lan::InsertLoginPlayerFunc(int ServerNo)
	{
		// 1. uset에 추가		
		AcquireSRWLockExclusive(&m_srwl_LoginMatServer_Umap);		// ------- Exclusive 락

		auto ret = m_LoginMatServer_Uset.insert(ServerNo);

		// 2. 중복된 키일 시 false 리턴.
		if (ret.second == false)
		{
			ReleaseSRWLockExclusive(&m_srwl_LoginMatServer_Umap);		// ------- Exclusive 언락
			return false;
		}

		return true;
	}

	// 로그인 한 유저 관리 자료구조에서 Erase
	// uset으로 관리중
	//
	// Parameter : ServerNo
	// return : 실패 시 false 리턴
	bool CMatchServer_Lan::EraseLoginPlayerFunc(int ServerNo)
	{
		// 1. uset에서 검색
		AcquireSRWLockExclusive(&m_srwl_LoginMatServer_Umap);	// ------- Exclusive 락

		auto FindPlayer = m_LoginMatServer_Uset.find(ServerNo);
		if (FindPlayer == m_LoginMatServer_Uset.end())
		{
			// 못찾았으면 false 리턴
			ReleaseSRWLockExclusive(&m_srwl_LoginMatServer_Umap);	// ------- Exclusive 언락

			return false;
		}

		// 2. 잘 찾았으면 erase
		m_LoginMatServer_Uset.erase(FindPlayer);

		ReleaseSRWLockExclusive(&m_srwl_LoginMatServer_Umap);	// ------- Exclusive 언락

		// 3. true 리턴
		return true;
	}



	// -----------------
	// 내부에서만 사용하는 함수
	// -----------------

	// 마스터 서버의 정보 셋팅
	//
	// Parameter : CMasterServer* 
	// return : 없음
	void CMatchServer_Lan::SetParent(CMasterServer* Parent)
	{
		pMasterServer = Parent;
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
		if (memcmp(MasterToken, pMasterServer->m_stConfig.BattleEnterToken, 32) != 0)
		{
			InterlockedIncrement(&g_lMatch_TokenError);

			// 다를 경우, 로그 찍고, 응답 없이 끊는다.
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR, 
				L"Packet_Login() --> EnterToken Error. SessionID : %lld, ServerNo : %d", SessionID, ServerNo);

			Disconnect(SessionID);

			return;
		}

		// 3. 로그인 여부 판단을 위해 자료구조에 Insert
		if (InsertLoginPlayerFunc(ServerNo) == false)
		{
			InterlockedIncrement(&g_lMatch_DuplicateLogin);
			
			// false가 리턴되는 것은 이미 접속중인 매칭 서버.
			// 로그 찍고, 응답 없이 끊는다.
			g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Packet_Login() --> Duplicate Login. ServerNo : %d", ServerNo);

			Disconnect(SessionID);

			return;
		}

		// 4. 로그인 중이 아니라면, SessionID를 이용해 검색
		CMasterServer::CMatchingServer* NowUser = FindPlayerFunc(SessionID);
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
	//
	// Parameter : SessionID, Payload
	// return : 없음
	void CMatchServer_Lan::Packet_RoomInfo(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 매칭 서버 검색
		CMasterServer::CMatchingServer* NowUser = FindPlayerFunc(SessionID);
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
		UINT64 AccountNo;

		Payload->GetData((char*)&ClinetKey, 8);
		Payload->GetData((char*)&AccountNo, 8);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();


		// ----------------- 룸 셋팅 시작
		
		AcquireSRWLockExclusive(&pMasterServer->m_srwl_Room_pq);	// ----- pq 룸 Exclusive 락

		// 1) 룸 갯수 확인
		if (pMasterServer->m_Room_pq.empty() == true)
		{
			ReleaseSRWLockExclusive(&pMasterServer->m_srwl_Room_pq);	// ----- pq 룸 Exclusive 언락

			// 룸이 하나도 없으면, 방 없음 패킷 리턴
			WORD Type = en_PACKET_MAT_MAS_RES_GAME_ROOM;
			BYTE Status = 0;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&ClinetKey, 8);
			SendBuff->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendBuff);

			return;
		}

		// 2) 룸 얻기
		CMasterServer::CRoom* NowRoom = pMasterServer->m_Room_pq.top();

		NowRoom->RoomLOCK();		// ----- 룸 1개에 대한 락

		// 3) 룸 안의 여유 유저 수 확인
		if (NowRoom->m_iEmptyCount == 0)
			g_MasterDump->Crash();

		// 4) 룸의 유저 수 감소
		NowRoom->m_iEmptyCount--;

		// 5) 감소 후 여유 유저 수가 0명이라면, pop
		if (NowRoom->m_iEmptyCount == 0)
			pMasterServer->m_Room_pq.pop();
		
		ReleaseSRWLockExclusive(&pMasterServer->m_srwl_Room_pq);	// ----- pq 룸 Exclusive 언락

		// 6) Send할 데이터 셋팅 --- 1차
		WORD Type = en_PACKET_MAT_MAS_RES_GAME_ROOM;
		BYTE Status = 1;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ClinetKey, 8);
		SendBuff->PutData((char*)&Status, 1);

		// 8) 해당 룸이 있는 배틀서버의 정보 알아오기
		AcquireSRWLockShared(&pMasterServer->m_srwl_BattleServer_Umap);	// ------------- 배틀서버 자료구조 Shared 락
		
		auto FindBattle = pMasterServer->m_BattleServer_Umap.find(NowRoom->m_iBattleServerNo);

		// 배틀서버가 없으면 Crash
		if (FindBattle == pMasterServer->m_BattleServer_Umap.end())
			g_MasterDump->Crash();

		CMasterServer::CBattleServer* NowBattle = FindBattle->second;

		// 9) Send할 데이터 셋팅 --- 2차
		SendBuff->PutData((char*)&NowBattle->m_iServerNo, 2);
		SendBuff->PutData((char*)NowBattle->m_tcBattleIP, 32);
		SendBuff->PutData((char*)&NowBattle->m_wBattlePort, 2);
		SendBuff->PutData((char*)&NowRoom->m_iRoomNo, 4);
		SendBuff->PutData(NowBattle->m_cConnectToken, 32);
		SendBuff->PutData(NowRoom->m_cEnterToken, 32);

		SendBuff->PutData((char*)NowBattle->m_tcChatIP, 32);
		SendBuff->PutData((char*)NowBattle->m_wChatPort, 2);

		ReleaseSRWLockShared(&pMasterServer->m_srwl_BattleServer_Umap);	// ------------- 배틀서버 자료구조 Shared 언락
					   		 	  	  
		// 10) 클라이언트를, 클라이언트 자료구조에 추가
		if (pMasterServer->InsertPlayerFunc(ClinetKey, AccountNo) == false)
			g_MasterDump->Crash();

		NowRoom->RoomUNLOCK();		// ----- 룸 1개에 대한 언락

		// ----------------- 룸 셋팅 끝

		// 4. 방 정보 Send하기
		SendPacket(SessionID, SendBuff);
	}


	// 방 입장 성공
	//
	// Parameter : SessionID, Payload
	// return : 없음
	void CMatchServer_Lan::Packet_RoomEnter_OK(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 매칭 서버 검색
		CMasterServer::CMatchingServer* NowUser = FindPlayerFunc(SessionID);
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

		// ----- 확인 및 Erase		

		// 1) ClinetKey 자료구조에서 제거
		if(pMasterServer->ErasePlayerFunc(ClinetKey) == false)
			g_MasterDump->Crash();


		
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
		// LanServer.Start 함수 호출
		if (Start(pMasterServer->m_stConfig.BindIP, pMasterServer->m_stConfig.Port, pMasterServer->m_stConfig.CreateWorker, 
			pMasterServer->m_stConfig.ActiveWorker, pMasterServer->m_stConfig.CreateAccept, pMasterServer->m_stConfig.Nodelay, pMasterServer->m_stConfig.MaxJoinUser) == false)
		{
			return false;
		}

		// 서버 오픈 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"MatchServerOpen...");

		return true;		
	}

	// 서버 종료
	//
	// Parameter : 없음
	// return : 없음
	void CMatchServer_Lan::ServerStop()
	{
		// LanServer.Stop()함수 호출
		Stop();

		// 서버 닫힘	
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"MatchServerClose...");
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
		CMasterServer::CMatchingServer* NewJoin = m_TLSPool_MatchServer->Alloc();

		// 2. SessionID 셋팅
		NewJoin->m_ullSessionID = SessionID;

		// 3. Insert
		if (InsertPlayerFunc(SessionID, NewJoin) == false)
			g_MasterDump->Crash();

	}

	void CMatchServer_Lan::OnClientLeave(ULONGLONG SessionID)
	{
		// 1. 자료구조에서 유저 제거
		CMasterServer::CMatchingServer* NowUser = ErasePlayerFunc(SessionID);
		if(NowUser == nullptr)
			g_MasterDump->Crash();

		// 2. 로그인 패킷 여부
		if (NowUser->m_bLoginCheck == true)
		{
			// 로그인되었던 유저라면, uset에서도 제거
			if(EraseLoginPlayerFunc(NowUser->m_iServerNo) == false)
				g_MasterDump->Crash();
		}

		// 3. 초기화
		NowUser->m_bLoginCheck = false;

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
				Packet_RoomInfo(SessionID, Payload);
				break;

				// 유저 방 입장 성공
			case en_PACKET_MAT_MAS_REQ_ROOM_ENTER_SUCCESS:
				break;

				// 유저 방 입장 실패
			case en_PACKET_MAT_MAS_REQ_ROOM_ENTER_FAIL:
				break;

				// 매치메이킹 로그인 요청
			case en_PACKET_MAT_MAS_REQ_SERVER_ON:
				Packet_Login(SessionID, Payload);
				break;				

				// 이 외 패킷은 에러 처리
			default:
				TCHAR ErrStr[1024];
				StringCchPrintf(ErrStr, 1024, _T("OnRecv(). TypeError. Type : %d, SessionID : %lld"),
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
		// 자료구조 공간 미리 잡아두기
		m_MatchServer_Umap.reserve(1000);

		// 락 초기화		
		InitializeSRWLock(&m_srwl_MatchServer_Umap);
		InitializeSRWLock(&m_srwl_LoginMatServer_Umap);

		// TLS 동적할당
		m_TLSPool_MatchServer = new CMemoryPoolTLS<CMasterServer::CMatchingServer>(0, false);

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
// 배틀과 연결되는 Lan 서버
// 
// -----------------------
namespace Library_Jingyu
{
	// -----------------
	// 내부에서만 사용하는 함수
	// -----------------

	// 마스터 서버의 정보 셋팅
	//
	// Parameter : CMasterServer* 
	// return : 없음
	void CBattleServer_Lan::SetParent(CMasterServer* Parent)
	{
		pMasterServer = Parent;
	}


	// -------------------------------------
	// 외부에서 사용 가능한 함수
	// -------------------------------------

	// 서버 시작
	// 각종 Lan서버를 시작한다.
	// 
	// Parameter : 없음
	// return : 실패 시 false 리턴
	bool CBattleServer_Lan::ServerStart()
	{
		// LanServer.Start 함수 호출
		if (Start(pMasterServer->m_stConfig.BattleBindIP, pMasterServer->m_stConfig.BattlePort, pMasterServer->m_stConfig.BattleCreateWorker,
			pMasterServer->m_stConfig.BattleActiveWorker, pMasterServer->m_stConfig.BattleCreateAccept, pMasterServer->m_stConfig.BattleNodelay, 
			pMasterServer->m_stConfig.BattleMaxJoinUser) == false)
		{
			return false;
		}

		// 서버 오픈 로그 찍기		
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"BattServerOpen...");

		return true;
	}


	// 서버 종료
	// 각종 Lan서버를 종료
	// 
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Lan::ServerStop()
	{
		// LanServer.Stop()함수 호출
		Stop();

		// 서버 닫힘	
		g_MasterLog->LogSave(L"MasterServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"BattServerClose...");
	}



	// -----------------------
	// 가상함수
	// -----------------------

	bool CBattleServer_Lan::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	void CBattleServer_Lan::OnClientJoin(ULONGLONG SessionID)
	{}

	void CBattleServer_Lan::OnClientLeave(ULONGLONG SessionID)
	{}

	void CBattleServer_Lan::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 타입 빼오기
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 타입에 따라 분기처리
		try
		{
			switch (Type)
			{
				// 신규 대기방 생성
			case en_PACKET_BAT_MAS_REQ_CREATED_ROOM:
				break;

				// 유저 1명이 방에서 나감
			case en_PACKET_BAT_MAS_REQ_LEFT_USER:
				break;

				// 방 닫힘
			case en_PACKET_BAT_MAS_REQ_CLOSED_ROOM:
				break;

				// 연결 토큰 재발행
			case en_PACKET_BAT_MAS_REQ_CONNECT_TOKEN:
				break;

				// 배틀 로그인 요청
			case en_PACKET_BAT_MAS_REQ_SERVER_ON:
				break;

				// 이 외 패킷은 에러 처리
			default:
				TCHAR ErrStr[1024];
				StringCchPrintf(ErrStr, 1024, _T("OnRecv(). TypeError. Type : %d, SessionID : %lld"),
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







	// -------------------------
	// 생성자와 소멸자
	// -------------------------

	// 생성자
	CBattleServer_Lan::CBattleServer_Lan()
	{
		
	}

	// 소멸자
	CBattleServer_Lan::~CBattleServer_Lan()
	{
		
	}

}