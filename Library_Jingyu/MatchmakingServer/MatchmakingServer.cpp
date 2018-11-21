#include "pch.h"
#include "MatchmakingServer.h"
#include "Parser/Parser_Class.h"
#include "Log/Log.h"
#include "Protocol_Set/CommonProtocol_2.h"		// 궁극적으로는 CommonProtocol.h로 이름 변경 필요. 지금은 채팅서버에서 로그인 서버를 이용하는 프로토콜이 있어서 _2로 만듬.

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

#include <process.h>
#include <strsafe.h>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace rapidjson;


// ---------------------------------------------
// 
// 매치메이킹 Net서버
//
// ---------------------------------------------
namespace Library_Jingyu
{
	// 매치메이킹 Net서버에서 사용할 Net 직렬화 버퍼 크기.
	LONG g_lNET_BUFF_SIZE = 512;

	// 매치메이킹 덤프용
	CCrashDump* gMatchServerDump = CCrashDump::GetInstance();

	// 매치메이킹 로그용
	CSystemLog* cMatchServerLog = CSystemLog::GetInstance();



	// -------------------------------------
	// 내부에서만 사용하는 함수
	// -------------------------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool Matchmaking_Net_Server::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MatchmakingServer_Config.ini"));
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
		// Matchmaking Net서버의 config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MATCHNETSERVER")) == false)
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
		// MatchDB의 Config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MATCHDB")) == false)
			return false;

		// DBIP
		if (Parser.GetValue_String(_T("DBIP"), pConfig->DB_IP) == false)
			return false;

		// DBUser
		if (Parser.GetValue_String(_T("DBUser"), pConfig->DB_User) == false)
			return false;

		// DBPassword
		if (Parser.GetValue_String(_T("DBPassword"), pConfig->DB_Password) == false)
			return false;

		// DBName
		if (Parser.GetValue_String(_T("DBName"), pConfig->DB_Name) == false)
			return false;

		// DBPort
		if (Parser.GetValue_Int(_T("DBPort"), &pConfig->DB_Port) == false)
			return false;

		// Matchmaking DB에 쏘는 하트비트
		if (Parser.GetValue_Int(_T("MatchDBHeartbeat"), &pConfig->MatchDBHeartbeat) == false)
			return false;

		// Matchmaking DB에 Connect 유저 갱신하는 변경인원
		if (Parser.GetValue_Int(_T("MatchDBConnectUserChange"), &m_iMatchDBConnectUserChange) == false)
			return false;



		////////////////////////////////////////////////////////
		// MatchClient의 Config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MATCHLANCLINET")) == false)
			return false;

		// MasterServerIP
		if (Parser.GetValue_String(_T("MasterServerIP"), pConfig->MasterServerIP) == false)
			return false;

		// MasterServerPort
		if (Parser.GetValue_Int(_T("MasterServerPort"), &pConfig->MasterServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("ClientCreateWorker"), &pConfig->ClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ClientActiveWorker"), &pConfig->ClientActiveWorker) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("ClientNodelay"), &pConfig->ClientNodelay) == false)
			return false;




		////////////////////////////////////////////////////////
		// 기본 CONFIG 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CONFIG")) == false)
			return false;		

		// ServerNo
		if (Parser.GetValue_Int(_T("ServerNo"), &m_iServerNo) == false)
			return false;

		// VerCode
		if (Parser.GetValue_Int(_T("VerCode"), &m_uiVer_Code) == false)
			return false;

		// ServerIP
		TCHAR tcServerIP[20];
		if (Parser.GetValue_String(_T("ServerIP"), tcServerIP) == false)
			return false;

		// ServerIP UTF-8로 변환
		int len = WideCharToMultiByte(CP_UTF8, 0, tcServerIP, lstrlenW(tcServerIP), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, tcServerIP, lstrlenW(tcServerIP), m_cServerIP, len, NULL, NULL);

		// MasterToken 
		TCHAR tcToken[33];
		if (Parser.GetValue_String(_T("MasterToken"), tcToken) == false)
			return false;

		// 클라가 들고올 때는 char형으로 들고오기 때문에 변환해서 가지고 있어야한다.
		len = WideCharToMultiByte(CP_UTF8, 0, tcToken, lstrlenW(tcToken), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, tcToken, lstrlenW(tcToken), MasterToken, len, NULL, NULL);

		return true;
	}
	   
	// 매치메이킹 DB에, 초기 정보를 Insert하는 함수.
	// 이미, 데이터가 존재하는 경우, 정보를 Update한다.
	// 
	// Parameter : 없음
	// return : 없음
	void Matchmaking_Net_Server::ServerInfo_DBInsert()
	{		
		// 1. Insert 쿼리 날린다.	
		char cQurey[200] = "INSERT INTO `matchmaking_status`.`server` VALUES(%d, '%s', %d, 0, NOW())\0";
		m_MatchDBcon->Query_Save(cQurey, m_iServerNo, m_cServerIP, m_stConfig.Port);


		// 2. 에러 확인
		int Error = m_MatchDBcon->GetLastError();

		// 에러가 0이 아닐 경우, Insert가 실패한 것.
		if (Error != 0)
		{
			// 만약 중복 키 에러라면, 이미 데이터가 존재한다는 것. Update 쿼리 날림
			if (Error == 1062)
			{
				char cUpdateQuery[200] = "UPDATE `matchmaking_status`.`server` SET `heartbeat` = NOW(), `connectuser` = 10 WHERE `serverno` = %d\0";
				m_MatchDBcon->Query_Save(cUpdateQuery, m_iServerNo);

				// 에러 체크
				Error = m_MatchDBcon->GetLastError();				
				if (Error != 0)				
					gMatchServerDump->Crash();
			}
			
			else
			{
				// 중복키가 아닌 에러가 발생했으면 크래시	
				gMatchServerDump->Crash();
			}
		}	
	}

	// ClientKey 만드는 함수
	//
	// Parameter : 없음
	// return : ClientKey(UINT64)
	UINT64 Matchmaking_Net_Server::CreateClientKey()
	{
		// 하위 4바이트에 ServerNo. 상위 4바이트에 m_ClientKeyAdd의 값이 들어간다.
		UINT64 TempKey = InterlockedIncrement(&m_ClientKeyAdd);
		return ((TempKey << 16) | m_iServerNo);
	}

	// 접속한 모든 유저에게 shutdown 하는 함수
	//
	// Parameter : 없음
	// return : 없음
	void Matchmaking_Net_Server::AllShutdown()
	{
		AcquireSRWLockShared(&m_srwlPlayer);	// ----- m_umapPlayer에 Shared 락

		// 모든 유저에게 Shutdown 날림.

		auto itor_Now = m_umapPlayer.begin();
		auto itor_End = m_umapPlayer.end();

		while (1)
		{
			if (itor_Now == itor_End)
				break;
			
			Disconnect(itor_Now->second->m_ullSessionID);
			++itor_Now;
		}

		ReleaseSRWLockShared(&m_srwlPlayer);	// ----- m_umapPlayer에 Shared 언락
	}




	// -------------------------------------
	// 스레드
	// -------------------------------------

	// matchmakingDB에 일정 시간마다 하트비트를 쏘는 스레드.
	//
	// 외부에서, 이벤트로 일을 시키기도 한다.
	UINT WINAPI Matchmaking_Net_Server::DBHeartbeatThread(LPVOID lParam)
	{
		Matchmaking_Net_Server* g_This = (Matchmaking_Net_Server*)lParam;

		// 종료용 이벤트, 일하기용 이벤트 로컬로 받아두기
		HANDLE hExitEvent[2] = { g_This->hDB_HBThread_ExitEvent, g_This->hDB_HBThread_WorkerEvent };

		// 하트비트 갱신 시간 받아두기
		int iHeartbeatTime = g_This->m_stConfig.MatchDBHeartbeat;

		// 서버 넘버 받아두기
		int iServerNo = g_This->m_iServerNo;

		// umapPlayer 포인터 받아두기. connectuser 측정 용도
		unordered_map<ULONGLONG, stPlayer*>* pUmapPlayer = &g_This->m_umapPlayer;

		// MatchmakingDB와 연결. 로컬로 받아둠
		CBConnectorTLS* pMatchDBcon = g_This->m_MatchDBcon;

		while (1)
		{
			// 메치메이킹DB의 하트비트 갱신 시간만큼 잔다.
			DWORD Check = WaitForMultipleObjects(2, hExitEvent, FALSE, iHeartbeatTime);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"DBHeartbeatThread Exit Error!!!(%d)", Error);
				break;
			}

			// 만약, 종료 신호가 왔다면 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;

			// -------------------------------
			// 무엇도 아니라면 할일 한다. (시간이 되어서 깨어났거나, 누군가 일을 시켰거나)
			// -------------------------------

			// 1. 쿼리 날리기
			char cQurey[200] = "UPDATE `matchmaking_status`.`server` SET `heartbeat` = NOW(), `connectuser` = %lld WHERE `serverno` = %d\0";
			pMatchDBcon->Query_Save(cQurey, pUmapPlayer->size(), iServerNo);

			// 2. 에러 확인
			int Error = pMatchDBcon->GetLastError();
			if (Error != 0)
			{
				// 에러가 발생했다면 로그 남기고 크래시
				cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM,
					L"DBHeartbeatThread() --> Query Error. %s(%d)", pMatchDBcon->GetLastErrorMsg(), pMatchDBcon->GetLastError());

				gMatchServerDump->Crash();
			}
		}

		return 0;
	}





	// -------------------------------------
	// 자료구조 추가,삭제,검색용 함수
	// -------------------------------------

	// Player 관리 자료구조 "2개"에, 유저 추가
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID, ClientKey, stPlayer*
	// return : 추가 성공 시, true
	//		  : SessionID가 중복될 시(이미 접속중인 유저) false
	bool Matchmaking_Net_Server::InsertPlayerFunc(ULONGLONG SessionID, UINT64 ClientKey, stPlayer* insertPlayer)
	{
		// 1. SessionID용 umap에 추가		
		AcquireSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 락

		auto ret_A = m_umapPlayer.insert(make_pair(SessionID, insertPlayer));		

		// 2. 중복된 키일 시 false 리턴.
		if (ret_A.second == false)
		{
			ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락
			return false;
		}
		
		// 3. ClientKey용 uamp에 추가
		auto ret_B = m_umapPlayer_ClientKey.insert(make_pair(ClientKey, insertPlayer));

		ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락

		// 4. 중복된 키일 시 false 리턴.
		if (ret_B.second == false)
			return false;

		return true;
	}


	// Player 관리 자료구조에서, 유저 검색
	// 현재 map으로 관리중
	// !!SessionID!! 를 이용해 검색
	// 
	// Parameter : SessionID
	// return : 검색 성공 시, stPalyer*
	//		  : 검색 실패 시 nullptr
	Matchmaking_Net_Server::stPlayer*  Matchmaking_Net_Server::FindPlayerFunc(ULONGLONG SessionID)
	{
		// 1. umap에서 검색
		AcquireSRWLockShared(&m_srwlPlayer);		// ------- Shared 락

		auto FindPlayer = m_umapPlayer.find(SessionID);

		ReleaseSRWLockShared(&m_srwlPlayer);		// ------- Shared 언락

		// 2. 검색 실패 시 nullptr 리턴
		if (FindPlayer == m_umapPlayer.end())
			return nullptr;

		// 3. 검색 성공 시, 찾은 stPlayer* 리턴
		return FindPlayer->second;
	}

	// Player 관리 자료구조에서, 유저 검색
	// !!ClientKey!! 를 이용해 검색
	// 현재 umap으로 관리중
	// 
	// Parameter : ClientKey
	// return : 검색 성공 시, stPalyer*
	//		  : 검색 실패 시 nullptr
	Matchmaking_Net_Server::stPlayer* Matchmaking_Net_Server::FindPlayerFunc_ClientKey(UINT64 ClientKey)
	{
		// 1. umap에서 검색
		AcquireSRWLockShared(&m_srwlPlayer);		// ------- Shared 락

		auto FindPlayer = m_umapPlayer_ClientKey.find(ClientKey);

		ReleaseSRWLockShared(&m_srwlPlayer);		// ------- Shared 언락

		// 2. 검색 실패 시 nullptr 리턴
		if (FindPlayer == m_umapPlayer_ClientKey.end())
			return nullptr;

		// 3. 검색 성공 시, 찾은 stPlayer* 리턴
		return FindPlayer->second;
	}


	// Player 관리 자료구조 "2개"에서, 유저 제거 (검색 후 제거)
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID
	// return : 성공 시, 제거된 유저 stPalyer*
	//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
	Matchmaking_Net_Server::stPlayer* Matchmaking_Net_Server::ErasePlayerFunc(ULONGLONG SessionID)
	{
		// 1. SessionID용 umap에서 유저 검색
		AcquireSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 락

		auto FindPlayer_A = m_umapPlayer.find(SessionID);
		if (FindPlayer_A == m_umapPlayer.end())
		{
			ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락
			return nullptr;
		}

		// 2. ClientKey 용 umap에서 유저 검색
		auto FindPlayer_B = m_umapPlayer_ClientKey.find(FindPlayer_A->second->m_ui64ClientKey);
		if (FindPlayer_B == m_umapPlayer_ClientKey.end())
		{
			ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락
			return nullptr;
		}
		
		// 3. 둘 다에서 존재한다면, 리턴할 값 받아두기
		stPlayer* ret = FindPlayer_B->second;

		// 3. SessionID용 uamp, ClinetKey용 umap에서 제거
		m_umapPlayer.erase(FindPlayer_A);
		m_umapPlayer_ClientKey.erase(FindPlayer_B);

		ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락

		return ret;
	}






	// -------------------------------------
	// 패킷 처리용 함수
	// -------------------------------------

	// 클라의 로그인 요청 받음
	//
	// Parameter : SessionID, Payload
	// return : 없음. 문제가 생기면, 내부에서 throw 던짐
	void Matchmaking_Net_Server::Packet_Match_Login(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		// 1. 마샬링
		INT64 AccountNo;
		char Token[64];
		UINT Ver_Code;

		Payload->GetData((char*)&AccountNo, 8);
		Payload->GetData(Token, 64);
		Payload->GetData((char*)&Ver_Code, 4);

		// 2. 플레이어 검색.		
		stPlayer* NowPlayer = FindPlayerFunc(SessionID);
		if (NowPlayer == nullptr)
			gMatchServerDump->Crash();


		// 3. 만약 이미 Login 된 유저거나, 배틀 방에 입장된 유저라면 Crash
		if (NowPlayer->m_bLoginCheck == true ||
			NowPlayer->m_bBattleRoomEnterCheck == true)
			gMatchServerDump->Crash();


		// 4. AccountNo로 DB에서 찾아오기
		TCHAR RequestBody[2000];
		TCHAR Body[1000];

		ZeroMemory(RequestBody, sizeof(RequestBody));
		ZeroMemory(Body, sizeof(Body));

		swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld}", AccountNo);
		if (m_HTTP_Post->HTTP_ReqANDRes((TCHAR*)_T("Contents/Select_account.php"), Body, RequestBody) == false)
			gMatchServerDump->Crash();

		// Json데이터 파싱하기 (UTF-16)
		GenericDocument<UTF16<>> Doc;
		Doc.Parse(RequestBody);		



		// 5. DB 결과 체크
		int iResult = Doc[_T("result")].GetInt();

		// 결과가 1이 아니라면, 
		if (iResult != 1)
		{
			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status;

			// 결과가 -10일 경우 (회원가입 자체가 안되어 있음)	-----------------
			if (iResult == -10)
			{
				Status = 3;
				InterlockedIncrement(&m_lAccountError);
			}

			// 그 외 기타 에러일 경우
			else
			{
				Status = 4;
				InterlockedIncrement(&m_lTempError);
			}

			CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

			SendData->PutData((char*)&Type, 2);
			SendData->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendData);
			return;
		}		


		// 6. 결과가 1이라면, 토큰키와 버전 체크
		// 토큰키 비교 --------------------
		const TCHAR* tDBToekn = Doc[_T("sessionkey")].GetString();

		char DBToken[64];
		int len = (int)_tcslen(tDBToekn);
		WideCharToMultiByte(CP_UTF8, 0, tDBToekn, (int)_tcslen(tDBToekn), DBToken, len, NULL, NULL);
		
		if (memcmp(DBToken, Token, 64) != 0)
		{
			// 토큰이 다를경우 status 2(토큰 오류)를 보낸다.
			InterlockedIncrement(&m_lTokenError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = 2;			

			CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

			SendData->PutData((char*)&Type, 2);
			SendData->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendData);
			return;
		}

		// 버전 비교 --------------------
		if (m_uiVer_Code != Ver_Code)
		{
			// 버전이 다를경우 status 5(버전 오류)를 보낸다.
			InterlockedIncrement(&m_lVerError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = 5;

			CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

			SendData->PutData((char*)&Type, 2);
			SendData->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendData);
			return;
		}




		// 7. 여기까지 왔으면 정상적인 플레이어. 셋팅 시작
		// 1) AccountNo 셋팅
		NowPlayer->m_i64AccountNo = AccountNo;	

		// 2) 로그인 상태로 변경
		NowPlayer->m_bLoginCheck = true;

		// 로그인 유저 수 증가
		InterlockedIncrement(&m_lLoginUser);


		// 8. 정상 패킷 응답
		WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
		BYTE Status = 1; // 성공

		CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

		SendData->PutData((char*)&Type, 2);
		SendData->PutData((char*)&Status, 1);

		SendPacket(SessionID, SendData);

		return;
	}
	
	// 방 입장 성공 (클라 --> 매칭 Net서버)
	// 마스터에게 방 입장 성공 패킷 보냄
	//
	// Parameter : SessionID, Payload
	// return : 없음. 문제가 생기면, 내부에서 throw 던짐
	void Matchmaking_Net_Server::Packet_Battle_EnterOK(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		// 1. 마샬링
		WORD BattleServerNo;
		int RoomNo;

		Payload->GetData((char*)&BattleServerNo, 2);
		Payload->GetData((char*)&RoomNo, 4);


		// 2. 유저 검색
		stPlayer* NowPlayer = FindPlayerFunc(SessionID);
		if (NowPlayer == nullptr)
			gMatchServerDump->Crash();


		// 3. 해당 유저의 방 입장 성공 Flag 변경
		// 만약, 이미 방 입장 성공 패킷을 보낸 유저라면 Crash()
		if (NowPlayer->m_bBattleRoomEnterCheck == true)
			gMatchServerDump->Crash();
		
		NowPlayer->m_bBattleRoomEnterCheck = true;


		// 4. 마스터 서버에게 보낼 패킷 제작 (Lan 직렬화 버퍼 사용)
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_MAT_MAS_REQ_ROOM_ENTER_SUCCESS;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&BattleServerNo, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&NowPlayer->m_ui64ClientKey, 8);

		// 5. 마스터에게 Send
		m_pLanClient->SendPacket(m_pLanClient->m_ullClientID, SendBuff);
	}


	// 방 입장 실패
	// 마스터에게 방 입장 실패 패킷 보냄
	//
	// Parameter : ClinetKey
	// return : 없음
	void Matchmaking_Net_Server::Packet_Battle_EnterFail(UINT64 ClientKey)
	{
		// 1. 마스터에게 보낼 패킷 셋팅
		WORD Type = en_PACKET_MAT_MAS_REQ_ROOM_ENTER_FAIL;

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ClientKey, 8);

		// 2. 마스터로 패킷 보내기
		m_pLanClient->SendPacket(m_pLanClient->m_ullClientID, SendBuff);
	}



	// -------------------------------------
	// 외부에서 사용 가능한 함수
	// -------------------------------------

	// 서버 Start 함수
	//
	// Parameter : 없음
	// return : 실패 시 false 리턴
	bool Matchmaking_Net_Server::ServerStart()
	{
		m_lTokenError = 0;
		m_lAccountError = 0;
		m_lTempError = 0;
		m_lVerError = 0;
		m_lLoginUser = 0;
		m_lstPlayer_AllocCount = 0;
		m_lNot_BattleRoom_Enter = 0;


		// ------------------- 매치메이킹 DB에 초기 데이터 생성
		ServerInfo_DBInsert();

		// ------------------- 하트비트 스레드 생성
		hDB_HBThread = (HANDLE)_beginthreadex(NULL, 0, DBHeartbeatThread, this, 0, 0);
		if (hDB_HBThread == 0)
		{
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, 
				L"ServerStart() --> HeartBeatThread Create Fail...");

			return false;
		}

		

		//------------------- 마스터와 연결되는 랜 클라 가동 및 this 전달
		m_pLanClient->SetParent(this);
		if (m_pLanClient->ClientStart() == false)
			return false;

		// ------------------- 넷서버 가동
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept, m_stConfig.Nodelay, m_stConfig.MaxJoinUser,
			m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
		{
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen Fail...");
			return false;
		}

		// 서버 오픈 로그 찍기		
		cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");
		return true;
	}

	// 매치메이킹 서버 종료 함수
	//
	// Parameter : 없음
	// return : 없음
	void Matchmaking_Net_Server::ServerStop()
	{
		// 넷서버 스탑 (엑셉트, 워커 종료)
		Stop();	

		// 하트비트 스레드 종료 신호
		SetEvent(hDB_HBThread_ExitEvent);

		// 하트비트 스레드 종료 대기
		if (WaitForSingleObject(hDB_HBThread, INFINITE) == WAIT_FAILED)
		{
			// 종료에 실패한다면, 에러 찍기
			DWORD Error = GetLastError();
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, 
				L"ServerStop() --> DBHeartbeatThread Exit Error!!!(%d)", Error);

			gMatchServerDump->Crash();
		}

		// 서버 종료 로그 찍기		
		cMatchServerLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");
	}

	// 출력용 함수
	//
	// Parameter : 없음
	// return : 없음
	void Matchmaking_Net_Server::ShowPrintf()
	{
		// 화면 출력할 것 셋팅
		/*
		SessionNum : 	- NetServer 의 세션수
		PacketPool_Net : 	- 외부에서 사용 중인 Net 직렬화 버퍼의 수

		PlayerData_Pool :	- Player 구조체 할당량
		Player Count : 		- Contents 파트 Player 개수

		Accept Total :		- Accept 전체 카운트 (accept 리턴시 +1)
		Accept TPS :		- Accept 처리 횟수
		Send TPS			- 초당 Send완료 횟수. (완료통지에서 증가)

		Net_BuffChunkAlloc_Count : - Net 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		Chat_MessageChunkAlloc_Count : - 플레이어 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)

		TokenError : 		- DB에서 찾아온 토큰과 로그인 요청한 유저가 들고온 토큰이 다름
		AccountError : 		- Selecet.account.php에 쿼리 날렸는데, -10 에러가 뜸(회원가입하지 않은 유저)
		TempError :			- Selecet.account.php에 쿼리 날렸는데, -10 외에 기타 에러가 뜸
		VerError :			- 로그인 요청한 유저가 들고온 VerCode와 서버가 들고있는 VerCode가 다름
		NotRoomEnter :		- 배틀 방 입장 성공 패킷을 안보내고 끊은 유저 수

		*/

		printf("========================================================\n"
			"SessionNum : %lld\n"
			"PacketPool_Net : %d\n\n"

			"PlayerData_Pool : %d\n"
			"Player Count : %lld\n\n"

			"Accept Total : %lld\n"
			"Accept TPS : %d\n"
			"Send TPS : %d\n\n"

			"Net_BuffChunkAlloc_Count : %d (Out : %d)\n"
			"Chat_PlayerChunkAlloc_Count : %d (Out : %d)\n\n"

			"TokenError : %d\n"
			"AccountError : %d\n"
			"TempError : %d\n"
			"VerError : %d\n"
			"NotRoomEnter : %d\n\n"

			"========================================================\n\n",

			// ------------ 매치메이킹 Net 서버용
			GetClientCount(), 
			CProtocolBuff_Net::GetNodeCount(),

			m_lstPlayer_AllocCount, 
			m_umapPlayer.size(),

			GetAcceptTotal(),
			GetAccpetTPS(), 
			GetSendTPS(),

			CProtocolBuff_Net::GetChunkCount(), CProtocolBuff_Net::GetOutChunkCount(),
			m_PlayerPool->GetAllocChunkCount(), m_PlayerPool->GetOutChunkCount(),

			m_lTokenError, 
			m_lAccountError, 
			m_lTempError, 
			m_lVerError, 
			m_lNot_BattleRoom_Enter);

	}





	// -------------------------------------
	// NetServer의 가상함수
	// -------------------------------------

	bool Matchmaking_Net_Server::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 유저 접속(로그인은 안된 상태)
	void Matchmaking_Net_Server::OnClientJoin(ULONGLONG SessionID)
	{
		// 1. stPlayer TLS에서 구조체 할당받음
		stPlayer* NowPlayer = m_PlayerPool->Alloc();
		InterlockedIncrement(&m_lstPlayer_AllocCount);

		// 2. stPlayer에 SessionID, ClientKey 셋팅
		NowPlayer->m_ullSessionID = SessionID;	
		UINT64 TempCKey = CreateClientKey();
		NowPlayer->m_ui64ClientKey = TempCKey;

		// 3. Player 관리 umap에 추가.
		if (InsertPlayerFunc(SessionID, TempCKey, NowPlayer) == false)
			gMatchServerDump->Crash();

		// 4. umap에 유저 수 체크.
		// umap의 유저수가 m_ChangeConnectUser +m_iMatchDBConnectUserChange 보다 많다면, 이전 값 기준 100명 이상 들어온것.
		// 매치메이킹 DB에 하트비트 한다.	
		// 카운트를 명확하게 하기 위해 락 사용	
		size_t NowumapCount = (int)m_umapPlayer.size();
		
		AcquireSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 락 -----------

		if (m_ChangeConnectUser + m_iMatchDBConnectUserChange <= NowumapCount)
		{
			m_ChangeConnectUser = NowumapCount;
			ReleaseSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 언락 -----------

			SetEvent(hDB_HBThread_WorkerEvent);
		}
		
		else
			ReleaseSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 언락 -----------
	}

	// 유저 나감
	void Matchmaking_Net_Server::OnClientLeave(ULONGLONG SessionID)
	{
		// 1. umap에서 유저 삭제
		stPlayer* ErasePlayer = ErasePlayerFunc(SessionID);
		if (ErasePlayer == nullptr)
			gMatchServerDump->Crash();

		// 2. umap 유저 수 체크.
		// umap의 유저수가 m_ChangeConnectUser - m_iMatchDBConnectUserChange 보다 적다면, 이전 값 기준 100명 이상 나간것.
		// 매치메이킹 DB에 하트비트 한다.	
		// 카운트를 명확하게 하기 위해 락 사용	
		size_t NowumapCount = m_umapPlayer.size();

		AcquireSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 락 -----------

		if (m_ChangeConnectUser - m_iMatchDBConnectUserChange >= NowumapCount)
		{
			m_ChangeConnectUser = NowumapCount;
			ReleaseSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 언락 -----------

			SetEvent(hDB_HBThread_WorkerEvent);
		}

		else
			ReleaseSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 언락 -----------

		// 3. 만약, 로그인 상태의 유저가 나갔다면 (로그인 패킷까지 보낸 유저) g_LoginUser--
		if (ErasePlayer->m_bLoginCheck == true)
			InterlockedDecrement(&m_lLoginUser);

		// 4. 유저 로그인 상태를 false로 만듬.
		ErasePlayer->m_bLoginCheck = false;

		// 5. 배틀 방 입장 패킷을 안보낸 유저라면, 실패 패킷을 마스터에게 보냄
		if (ErasePlayer->m_bBattleRoomEnterCheck == false)
		{
			InterlockedIncrement(&m_lNot_BattleRoom_Enter);
			Packet_Battle_EnterFail(ErasePlayer->m_ui64ClientKey);
		}

		// 6. 배틀방 입장 플래그 변경
		ErasePlayer->m_bBattleRoomEnterCheck = false;


		// 7. 플레이어 구조체 반환
		m_PlayerPool->Free(ErasePlayer);
		InterlockedDecrement(&m_lstPlayer_AllocCount);		
	}

	// 새로운 패킷 받음
	void Matchmaking_Net_Server::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		// 1. 패킷의 Type 빼옴
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 2. Type에 따라 분기문 처리
		try
		{
			switch (Type)
			{
				// 매치메이킹 서버로 로그인 요청
			case en_PACKET_CS_MATCH_REQ_LOGIN:
				Packet_Match_Login(SessionID, Payload);
				break;

				// 방 정보 요청
				// LanClinet의 함수 호출
			case en_PACKET_CS_MATCH_REQ_GAME_ROOM:
				m_pLanClient->Packet_Battle_Info(SessionID);
				break;

				// 배틀 서버의 방 입장 성공 알림
			case en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER:
				Packet_Battle_EnterOK(SessionID, Payload);
				break;

				// 이상한 타입의 패킷이 오면 끊는다.
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
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());	

			// Crash
			gMatchServerDump->Crash();

			// 접속 끊기 요청
			//Disconnect(SessionID);
		}
	}


	void Matchmaking_Net_Server::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	void Matchmaking_Net_Server::OnWorkerThreadBegin()
	{}

	void Matchmaking_Net_Server::OnWorkerThreadEnd()
	{}

	void Matchmaking_Net_Server::OnError(int error, const TCHAR* errorStr)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
			errorStr, error);		
	}




	// -------------------------------------
	// 생성자와 소멸자
	// -------------------------------------

	// 생성자
	Matchmaking_Net_Server::Matchmaking_Net_Server()
		:CNetServer()
	{
		// m_ClientKeyAdd 셋팅
		m_ClientKeyAdd = 0;

		// m_ChangeConnectUser 셋팅
		m_ChangeConnectUser = 0;

		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			gMatchServerDump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		cMatchServerLog->SetDirectory(L"MatchServer");
		cMatchServerLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);

		// DBThread 종료용 이벤트, 일 시키기용 이벤트 생성
		// 
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		hDB_HBThread_ExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		hDB_HBThread_WorkerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		// SRWLOCK 초기화
		InitializeSRWLock(&m_srwlPlayer);
		InitializeSRWLock(&m_srwlChangeConnectUser);

		// stPlayer 구조체를 다루는 TLS 동적할당
		m_PlayerPool = new CMemoryPoolTLS<stPlayer>(0, false);

		// DB_Connector TLS 버전
		// MatchmakingDB와 연결
		m_MatchDBcon = new CBConnectorTLS(m_stConfig.DB_IP, m_stConfig.DB_User, m_stConfig.DB_Password,	m_stConfig.DB_Name, m_stConfig.DB_Port);
		
		// HTTP_Exchange 동적할당
		m_HTTP_Post = new HTTP_Exchange((TCHAR*)_T("127.0.0.1"), 80);

		// 플레이어를 관리하는 umap의 용량을 할당해둔다.
		m_umapPlayer.reserve(m_stConfig.MaxJoinUser);	
		m_umapPlayer_ClientKey.reserve(m_stConfig.MaxJoinUser);

		// 마스터와 통신하는 Lan클라 동적할당
		m_pLanClient = new Matchmaking_Lan_Client();

		// 시간
		timeBeginPeriod(1);
	}

	// 소멸자
	Matchmaking_Net_Server::~Matchmaking_Net_Server()
	{
		// 서버가 아직 가동중이라면 서버 종료
		// Net 라이브러리가 가동중인지만 알면 됨.
		if (GetServerState() == true)
			ServerStop();

		// 구조체 메시지 TLS 메모리 풀 동적해제
		delete m_PlayerPool;

		// DBConnector 동적해제
		delete m_MatchDBcon;

		// HTTP_Exchange 동적해제
		delete m_HTTP_Post;

		// DBThread 종료용 이벤트 반환
		CloseHandle(hDB_HBThread_ExitEvent);

		// DBThread 일 시키기용 이벤트 반환
		CloseHandle(hDB_HBThread_WorkerEvent);

		// 시간
		timeEndPeriod(1);
	}
	
}

// ---------------------------------------------
// 
// 매치메이킹 LanClient(Master서버의 LanServer와 통신)
//
// ---------------------------------------------
namespace Library_Jingyu
{
	// -------------------------------------
	// Net 서버가 호출하는 함수
	// -------------------------------------

	// 방 정보 요청 (클라 --> 매칭 Net서버)
	// LanClient를 통해, 마스터에게 패킷 보냄
	//
	// Parameter : SessionID
	// return : 없음. 문제가 생기면, 내부에서 throw 던짐
	void Matchmaking_Lan_Client::Packet_Battle_Info(ULONGLONG SessionID)
	{
		// 1. 유저 검색
		Matchmaking_Net_Server::stPlayer* NowPlayer = m_pParent->FindPlayerFunc(SessionID);
		if (NowPlayer == nullptr)
			gMatchServerDump->Crash();

		// 2. 마스터에게 보낼 패킷 제작
		// Lan 직렬화 버퍼 사용
		WORD Type = en_PACKET_MAT_MAS_REQ_GAME_ROOM;

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&NowPlayer->m_ui64ClientKey, 8);
		SendBuff->PutData((char*)&NowPlayer->m_i64AccountNo, 8);

		// 3. 데이터 Send
		SendPacket(m_ullClientID, SendBuff);
	}





	// -------------------------------------
	// 마스터에게 받은 패킷 처리용 함수
	// -------------------------------------

	// 방 정보 요청에 대한 응답
	// 내부에서, Net서버의 SendPacket()까지 호출한다.
	// 
	// Parameter : CProtocolBuff_Lan*
	// return : 없음. 문제가 생기면, 내부에서 throw 던짐
	void Matchmaking_Lan_Client::Response_Battle_Info(CProtocolBuff_Lan* payload)
	{
		// 1. ClientKey와 Status 마샬링
		UINT64 ClinetKey;
		BYTE Status;

		payload->GetData((char*)&ClinetKey, 8);
		payload->GetData((char*)&Status, 1);

		// 2. 유저 검색
		Matchmaking_Net_Server::stPlayer* NowPlayer = m_pParent->FindPlayerFunc_ClientKey(ClinetKey);
		if (NowPlayer == nullptr)
			gMatchServerDump->Crash();

		// 3. Status 확인
		// 1이 아니면 모두 잘못된 것.
		if (Status != 1)
		{
			// Status가 0이라면 방 얻기 실패한 것.
			// 클라에게 방 정보 실패 결과 보냄
			if (Status == 0)
			{
				// Net 직렬화 버퍼 사용.
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_MATCH_RES_GAME_ROOM;
				BYTE SendStatus = 0;

				SendBuff->PutData((char*)&Type, 2);
				SendBuff->PutData((char*)&SendStatus, 1);

				// 차후 보내고 끊어야 할지 고민..
				m_pParent->SendPacket(NowPlayer->m_ullSessionID, SendBuff);

				return;
			}

			// 여기까지 오면 1도 아니고 0도 아닌것. 규약에 없는 Status. Crash
			else
				gMatchServerDump->Crash();
		}

		// 4. 상태가 1이라면, 방 정보가 온 것. 나머지 마샬링
		WORD	BattleServerNo;		
		WCHAR	IP[16];
		WORD	Port;		
		int		RoomNo;
		char	ConnectToken[32];
		char	EnterToken[32];		

		WCHAR	ChatServerIP[16];
		WORD	ChatServerPort;

		payload->GetData((char*)&BattleServerNo, 2);
		payload->GetData((char*)&IP, 32);
		payload->GetData((char*)&Port, 2);
		payload->GetData((char*)&RoomNo, 4);
		payload->GetData(ConnectToken, 32);
		payload->GetData(EnterToken, 32);

		payload->GetData((char*)&ChatServerIP, 32);
		payload->GetData((char*)&ChatServerPort, 2);


		// 5. 클라에게 보낼 정보 셋팅
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_MATCH_RES_GAME_ROOM;
		BYTE SendStatus = 0;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&SendStatus, 1);

		SendBuff->PutData((char*)&BattleServerNo, 2);
		SendBuff->PutData((char*)&IP, 32);
		SendBuff->PutData((char*)&Port, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData(ConnectToken, 32);
		SendBuff->PutData(EnterToken, 32);

		SendBuff->PutData((char*)&ChatServerIP, 32);
		SendBuff->PutData((char*)&ChatServerPort, 2);


		// 6. 클라에게 보내기 (Net으로 보내기)
		m_pParent->SendPacket(NowPlayer->m_ullSessionID, SendBuff);
	}

	// 로그인 요청에 대한 응답
	// 
	// Parameter : CProtocolBuff_Lan*
	// return : 없음. 문제가 생기면, 내부에서 throw 던짐
	void Matchmaking_Lan_Client::Response_Login(CProtocolBuff_Lan* payload)
	{
		// 이미 로그인 상태인지 체크
		if(m_bLoginCheck == true)
			gMatchServerDump->Crash();

		// 1. 마샬링
		int iServerNo;
		payload->GetData((char*)&iServerNo, 4);

		// 2. 서버 확인
		if(iServerNo != m_pParent->m_iServerNo)
			gMatchServerDump->Crash();

		// 서버가 정상적으로 맞다면 할 것 없음.
	}




	// -------------------------------------
	// 외부에서 사용 가능한 함수
	// -------------------------------------

	// 매치메이킹 LanClient 시작 함수
	//
	// Parameter : 없음
	// return : 실패 시 false 리턴
	bool Matchmaking_Lan_Client::ClientStart()
	{
		// ClientID 값 초기화
		m_ullClientID = 0xffffffffffffffff;

		// 로그인 체크 초기화
		m_bLoginCheck = false;

		// LanClient의 Start() 호출
		// 마스터의 LanServer와 연결
		//if (Start(m_pParent->m_stConfig.MasterServerIP, m_pParent->m_stConfig.MasterServerPort, m_pParent->m_stConfig.ClientCreateWorker, m_pParent->m_stConfig.ClientActiveWorker, m_pParent->m_stConfig.ClientNodelay) == false)
			//return false;

		return true;
	}

	// 매치메이킹 LanClient 종료 함수
	//
	// Parameter : 없음
	// return : 없음
	void Matchmaking_Lan_Client::ClientStop()
	{

	}




	// -------------------------------------
	// 상속 관계에서만 사용 가능한 기능 함수
	// -------------------------------------

	// 내 부모를 채워주는 함수
	void Matchmaking_Lan_Client::SetParent(Matchmaking_Net_Server* NetServer)
	{
		m_pParent = NetServer;
	}






	// -----------------------
	// Lan 클라의 가상 함수
	// -----------------------

	void Matchmaking_Lan_Client::OnConnect(ULONGLONG ClinetID)
	{
		// 내 m_ullClientID가 0xffffffffffffffff인지 체크
		if(m_ullClientID != 0xffffffffffffffff)
			gMatchServerDump->Crash();

		// 1. ClientID 받아둔다.
		m_ullClientID = ClinetID;

		// 2. 마스터 서버로 보낼 로그인 정보 셋팅
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_MAT_MAS_REQ_SERVER_ON;
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&m_pParent->m_iServerNo, 4);
		SendBuff->PutData(m_pParent->MasterToken, 32);

		// 3. 마스터 서버로 로그인 패킷 보내기
		SendPacket(ClinetID, SendBuff);
	}

	void Matchmaking_Lan_Client::OnDisconnect(ULONGLONG ClinetID)
	{
		// Net 서버와 연결된 모든 유저 접속 종료
		m_pParent->AllShutdown();

		// m_ullClientID 초기화
		m_ullClientID = 0xffffffffffffffff;

		// 로그인 Flag 초기화
		m_bLoginCheck = false;
	}

	void Matchmaking_Lan_Client::OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload)
	{
		// 내 ClientID와 인자로 받은 ClientID가 다르면 Crash
		if(m_ullClientID != ClinetID)
			gMatchServerDump->Crash();

		// 타입 빼오기
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 타입에 따라 분기처리
		try
		{
			switch (Type)
			{
				// 방 요청에 대한 응답
			case en_PACKET_MAT_MAS_RES_GAME_ROOM:
				Response_Battle_Info(Payload);
				break;

				// 로그인 요청에 대한 응답
			case en_PACKET_MAT_MAS_RES_SERVER_ON:
				Response_Login(Payload);
				break;

				// 이상한 타입의 패킷이 오면 끊는다.
			default:
				TCHAR ErrStr[1024];
				StringCchPrintf(ErrStr, 1024, _T("LanClient -> OnRecv(). TypeError. Type : %d"), Type);

				throw CException(ErrStr);
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// Crash
			gMatchServerDump->Crash();
		}
		
	}

	void Matchmaking_Lan_Client::OnSend(ULONGLONG ClinetID, DWORD SendSize)
	{}

	void Matchmaking_Lan_Client::OnWorkerThreadBegin()
	{}

	void Matchmaking_Lan_Client::OnWorkerThreadEnd()
	{}

	void Matchmaking_Lan_Client::OnError(int error, const TCHAR* errorStr)
	{}





	// -------------------------------------
	// 생성자와 소멸자
	// -------------------------------------

	// 생성자
	Matchmaking_Lan_Client::Matchmaking_Lan_Client()
		:CLanClient()
	{
		// 특별히 할 거 없음

	}

	// 소멸자
	Matchmaking_Lan_Client::~Matchmaking_Lan_Client()
	{
		// 특별히 할 거 없음
	}	
}