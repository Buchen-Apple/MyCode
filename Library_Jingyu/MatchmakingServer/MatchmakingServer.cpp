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

// 출력용 변수들 ------------------------

// 로그인 패킷 받았을 시, 에러.
LONG g_lTokenError;	
LONG g_lAccountError;
LONG g_lTempError;
LONG g_lVerError;

// 로그인에 성공한 유저 수
LONG g_lLoginUser;

// 플레이어 구조체 할당 수
LONG g_lstPlayer_AllocCount;

// Net 엔진에서 카운트 중인 값.
extern ULONGLONG g_ullAcceptTotal;
extern LONG	  g_lAcceptTPS;
extern LONG	g_lSendPostTPS;

// 패킷 직렬화 버퍼 사용 수 (Net)
extern LONG g_lAllocNodeCount;




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
				{
					// 에러가 발생했으면 에러 찍고 크래시
					cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM,
						L"ServerInfo_DBInsert() --> Query Error. %s(%d)", m_MatchDBcon->GetLastErrorMsg(), m_MatchDBcon->GetLastError());

					gMatchServerDump->Crash();
				}
			}
			
			else
			{
				// 중복키가 아닌 에러가 발생했으면 로그 남긴 후 크래시
				cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM,
					L"ServerInfo_DBInsert() --> Query Error. %s(%d)", m_MatchDBcon->GetLastErrorMsg(), m_MatchDBcon->GetLastError());

				gMatchServerDump->Crash();
			}
		}	
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

	// Player 관리 자료구조에, 유저 추가
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID, stPlayer*
	// return : 추가 성공 시, true
	//		  : SessionID가 중복될 시(이미 접속중인 유저) false		
	bool Matchmaking_Net_Server::InsertPlayerFunc(ULONGLONG SessionID, stPlayer* insertPlayer)
	{
		// 1. umap에 추가		
		AcquireSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 락

		auto ret = m_umapPlayer.insert(make_pair(SessionID, insertPlayer));

		ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락

		// 2. 중복된 키일 시 false 리턴.
		if (ret.second == false)
			return false;

		// 3. 아니면 true 리턴
		return true;
	}


	// Player 관리 자료구조에서, 유저 검색
	// 현재 map으로 관리중
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


	// Player 관리 자료구조에서, 유저 제거 (검색 후 제거)
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID
	// return : 성공 시, 제거된 유저 stPalyer*
	//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
	Matchmaking_Net_Server::stPlayer* Matchmaking_Net_Server::ErasePlayerFunc(ULONGLONG SessionID)
	{
		// 1. umap에서 유저 검색
		AcquireSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 락

		auto FindPlayer = m_umapPlayer.find(SessionID);
		if (FindPlayer == m_umapPlayer.end())
		{
			ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락
			return nullptr;
		}
		
		// 2. 존재한다면, 리턴할 값 받아두기
		stPlayer* ret = FindPlayer->second;

		// 3. 맵에서 제거
		m_umapPlayer.erase(FindPlayer);

		ReleaseSRWLockExclusive(&m_srwlPlayer);		// ------- Exclusive 언락

		return ret;
	}





	// -------------------------------------
	// 패킷 처리용 함수
	// -------------------------------------

	// 매치메이킹 서버로 로그인 요청
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


		// 2. AccountNo로 DB에서 찾아오기
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



		// 3. DB 결과 체크
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
				InterlockedIncrement(&g_lAccountError);
			}

			// 그 외 기타 에러일 경우
			else
			{
				Status = 4;
				InterlockedIncrement(&g_lTempError);
			}

			CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

			SendData->PutData((char*)&Type, 2);
			SendData->PutData((char*)&Status, 1);

			// 보내고 끊기
			SendPacket(SessionID, SendData, TRUE);
			return;
		}		


		// 4. 결과가 1이라면, 토큰키와 버전 체크
		// 토큰키 비교 --------------------
		const TCHAR* tDBToekn = Doc[_T("sessionkey")].GetString();

		char DBToken[64];
		int len = (int)_tcslen(tDBToekn);
		WideCharToMultiByte(CP_UTF8, 0, tDBToekn, (int)_tcslen(tDBToekn), DBToken, len, NULL, NULL);
		
		if (memcmp(DBToken, Token, 64) != 0)
		{
			// 토큰이 다를경우 status 2(토큰 오류)를 보낸다.
			InterlockedIncrement(&g_lTokenError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = 2;			

			CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

			SendData->PutData((char*)&Type, 2);
			SendData->PutData((char*)&Status, 1);

			// 보내고 끊기
			SendPacket(SessionID, SendData, TRUE);
			return;
		}

		// 버전 비교 --------------------
		if (m_uiVer_Code != Ver_Code)
		{
			// 버전이 다를경우 status 5(버전 오류)를 보낸다.
			InterlockedIncrement(&g_lVerError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Status = 5;

			CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

			SendData->PutData((char*)&Type, 2);
			SendData->PutData((char*)&Status, 1);

			// 보내고 끊기
			SendPacket(SessionID, SendData, TRUE);
			return;
		}




		// 5. 여기까지 왔으면 정상적인 플레이어. 셋팅 시작
		// 1) 플레이어 검색.
		stPlayer* NowPlayer = FindPlayerFunc(SessionID);
		if (NowPlayer == nullptr)
		{
			// 검색 실패 시 로직 에러로 본다.
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM,
				L"Packet_Match_Login Player Not Find. SessionID : %lld, AccountNo : %lld, ", 
				SessionID, AccountNo);

			gMatchServerDump->Crash();
		}

		// 2) AccountNo 셋팅
		NowPlayer->m_i64AccountNo = AccountNo;

		// 3) ClinetKey 셋팅
		// 하위 4바이트에 ServerNo. 상위 4바이트에 m_ClientKeyAdd의 값이 들어간다.
		UINT64 TempKey = InterlockedIncrement(&m_ClientKeyAdd);
		NowPlayer->m_ui64ClientKey = ((TempKey << 16) | m_iServerNo);

		// 4) 로그인 상태로 변경
		NowPlayer->m_bLoginCheck = true;

		// 로그인 유저 수 증가
		InterlockedIncrement(&g_lLoginUser);


		// 6. 정상 패킷 응답
		WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
		BYTE Status = 1; // 성공

		CProtocolBuff_Net* SendData = CProtocolBuff_Net::Alloc();

		SendData->PutData((char*)&Type, 2);
		SendData->PutData((char*)&Status, 1);

		SendPacket(SessionID, SendData);
		return;
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

		*/

		LONG AccpetTPS = InterlockedExchange(&g_lAcceptTPS, 0);
		LONG SendTPS = InterlockedExchange(&g_lSendPostTPS, 0);

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
			"VerError : %d\n\n"

			"========================================================\n\n",

			// ------------ 매치메이킹 Net 서버용
			GetClientCount(), g_lAllocNodeCount,
			g_lstPlayer_AllocCount, m_umapPlayer.size(),
			g_ullAcceptTotal, AccpetTPS, SendTPS,
			CProtocolBuff_Net::GetChunkCount(), CProtocolBuff_Net::GetOutChunkCount(),
			m_PlayerPool->GetAllocChunkCount(), m_PlayerPool->GetOutChunkCount(),
			g_lTokenError, g_lAccountError, g_lTempError, g_lVerError );		

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
		InterlockedIncrement(&g_lstPlayer_AllocCount);

		// 2. stPlayer에 SessionID 셋팅
		NowPlayer->m_ullSessionID = SessionID;	

		// 3. Player 관리 umap에 추가.
		if (InsertPlayerFunc(SessionID, NowPlayer) == false)
		{
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM,
				L"OnClientJoin() --> duplication SessionID...(SessionID : %lld)", SessionID);

			gMatchServerDump->Crash();
		}

		// 4. umap에 유저 수 체크.
		// umap의 유저수가 m_ChangeConnectUser +100 보다 많다면, 이전 값 기준 100명 이상 들어온것.
		// 매치메이킹 DB에 하트비트 한다.	
		// 카운트를 명확하게 하기 위해 락 사용	
		size_t NowumapCount = (int)m_umapPlayer.size();
		
		AcquireSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 락 -----------

		if (m_ChangeConnectUser + 100 <= NowumapCount)
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
		{
			cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM,
				L"OnClientLeave() --> Erase Fail.. (SessionID : %lld)", SessionID);

			gMatchServerDump->Crash();
		}

		// 2. umap 유저 수 체크.
		// umap의 유저수가 m_ChangeConnectUser - 100 보다 적다면, 이전 값 기준 100명 이상 나간것.
		// 매치메이킹 DB에 하트비트 한다.	
		// 카운트를 명확하게 하기 위해 락 사용	
		size_t NowumapCount = m_umapPlayer.size();

		AcquireSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 락 -----------

		if (m_ChangeConnectUser - 100 >= NowumapCount)
		{
			m_ChangeConnectUser = NowumapCount;
			ReleaseSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 언락 -----------

			SetEvent(hDB_HBThread_WorkerEvent);
		}

		else
			ReleaseSRWLockExclusive(&m_srwlChangeConnectUser);	// Exclusive 언락 -----------

		// 3. 만약, 로그인 상태의 유저가 나갔다면 (로그인 패킷까지 보낸 유저) g_LoginUser--
		if (ErasePlayer->m_bLoginCheck == true)
			InterlockedDecrement(&g_lLoginUser);

		// 4. 유저 로그인 상태를 false로 만듬.
		ErasePlayer->m_bLoginCheck = false;

		// 5. 플레이어 구조체 반환
		m_PlayerPool->Free(ErasePlayer);
		InterlockedDecrement(&g_lstPlayer_AllocCount);		
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
			case en_PACKET_CS_MATCH_REQ_GAME_ROOM:
				break;

				// 배틀 서버의 방에 입장 성공 알림
			case en_PACKET_CS_MATCH_REQ_GAME_ROOM_ENTER:
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