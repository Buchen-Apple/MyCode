#include "pch.h"
#include "LoginServer.h"
#include "Protocol_Set\CommonProtocol.h"
#include "Parser\Parser_Class.h"
#include "Log\Log.h"

#include <strsafe.h>

extern LONG g_lStruct_PlayerCount;

// ----------------------------------
// LoginServer(NetServer)
// ----------------------------------
namespace Library_Jingyu
{
	CCrashDump* g_LoginDump = CCrashDump::GetInstance();

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cLoginLibLog = CSystemLog::GetInstance();
	

	/////////////////////////////
	// 외부에서 호출 가능 함수
	/////////////////////////////


	// !! 테스트용 !!
	// 랜서버 접속자 얻기
	ULONGLONG CLogin_NetServer::GetLanClientCount()
	{
		return m_cLanS->GetClientCount();
	}


	// 생성자
	CLogin_NetServer::CLogin_NetServer()
	{
		// ------------------- 파일에서 필요한 정보 읽어오기
		if (SetFile(&m_stConfig) == false)
			g_LoginDump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		cLoginLibLog->SetDirectory(L"LoginServer");
		cLoginLibLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);

		// 락 초기화
		InitializeSRWLock(&srwl);

		// LanServer 동적할당
		m_cLanS = new CLogin_LanServer;		

		// Player TLS 동적할당
		m_MPlayerTLS = new CMemoryPoolTLS< stPlayer >(50, false);

		// DBConnector 셋팅
		m_AcountDB_Connector = new CBConnectorTLS(m_stConfig.DB_IP, m_stConfig.DB_User, m_stConfig.DB_Password, m_stConfig.DB_Name, m_stConfig.DB_Port);
		

		// LanServer에 this 셋팅
		m_cLanS->ParentSet(this);
	}

	// 소멸자
	CLogin_NetServer::~CLogin_NetServer()
	{
		// LanServer 동적해제
		delete m_cLanS;

		// Player TLS 동적해제
		delete m_MPlayerTLS;

		// DBConnector 삭제
		delete m_AcountDB_Connector;
	}


	// 로그인 서버 시작 함수
	// 내부적으로 NetServer의 Start 호출
	// 
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CLogin_NetServer::ServerStart()
	{
		// ------------------- 랜 서버 가동
		if (m_cLanS->Start(m_stConfig.LanBindIP, m_stConfig.LanPort, m_stConfig.LanCreateWorker, m_stConfig.LanActiveWorker,
			m_stConfig.LanCreateAccept, m_stConfig.LanNodelay, m_stConfig.LanMaxJoinUser) == false)
			return false;

		// ------------------- 넷서버 가동
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept, m_stConfig.Nodelay, m_stConfig.MaxJoinUser,
			m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
			return false;

		// 서버 오픈 로그 찍기		
		cLoginLibLog->LogSave(L"LoginServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// 로그인 서버 종료 함수
	//
	// Parameter : 없음
	// return : 없음
	void CLogin_NetServer::ServerStop()
	{
		// 넷서버 스탑 (엑셉트, 워커 종료)
		Stop();
		
		// ------------------- 랜 서버 스탑
		m_cLanS->Stop();


		// ------------- 디비 저장
		// 현재 없음.


		// ------------- 리소스 초기화		

		// Playermap에 있는 모든 유저 반환
		// 넷서버가 스탑되면서 이미 모든 유저는 Disconnect 되었다
		auto itor = m_umapJoinUser.begin();

		while (itor != m_umapJoinUser.end())
		{
			// 메모리풀에 반환
			m_MPlayerTLS->Free(itor->second);
			InterlockedAdd(&g_lStruct_PlayerCount, -1);
		}

		// umap 초기화
		m_umapJoinUser.clear();
	}







	/////////////////////////////
	// 기능 처리 함수
	/////////////////////////////

	// 파일에서 Config 정보 읽어오기
	// 
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CLogin_NetServer::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("LoginServer_Config.ini"));
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
		// LoginServer config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("LOGINSERVER")) == false)
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
		// 각종 IP config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("IPCONFIG")) == false)
			return false;

		/*
		// 게임서버 IP
		if (Parser.GetValue_String(_T("GameServerIP"), pConfig->GameServerIP) == false)
			return false;

		// 게임서버 Port
		if (Parser.GetValue_Int(_T("GameServerPort"), &pConfig->GameServerPort) == false)
			return false;
		*/

		// 채팅서버 IP
		if (Parser.GetValue_String(_T("ChatServerIP"), pConfig->ChatServerIP) == false)
			return false;

		// 채팅서버 Port
		if (Parser.GetValue_Int(_T("ChatServerPort"), &pConfig->ChatServerPort) == false)
			return false;

		
		////////////////////////////////////////////////////////
		// DB config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("ACCOUNTDBCONFIG")) == false)
			return false;


		// IP
		if (Parser.GetValue_String(_T("DBIP"), pConfig->DB_IP) == false)
			return false;

		// User
		if (Parser.GetValue_String(_T("DBUser"), pConfig->DB_User) == false)
			return false;

		// Password
		if (Parser.GetValue_String(_T("DBPassword"), pConfig->DB_Password) == false)
			return false;

		// Name
		if (Parser.GetValue_String(_T("DBName"), pConfig->DB_Name) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("DBPort"), &pConfig->DB_Port) == false)
			return false;

		

		////////////////////////////////////////////////////////
		// LanServer Config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("LANSERVER")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("LanBindIP"), pConfig->LanBindIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("LanPort"), &pConfig->LanPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("LanCreateWorker"), &pConfig->LanCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("LanActiveWorker"), &pConfig->LanActiveWorker) == false)
			return false;

		// 생성 엑셉트
		if (Parser.GetValue_Int(_T("LanCreateAccept"), &pConfig->LanCreateAccept) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("LanNodelay"), &pConfig->LanNodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("LanMaxJoinUser"), &pConfig->LanMaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("LanLogLevel"), &pConfig->LanLogLevel) == false)
			return false;


		return true;
	}


	   	  








	/////////////////////////////
	// 자료구조 관리 함수
	/////////////////////////////

	// 유저 관리 자료구조에서 유저 검색
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID
	// return : 검색 성공 시 stPlayer*
	//		  : 실패 시 nullptr
	CLogin_NetServer::stPlayer* CLogin_NetServer::FindPlayerFunc(ULONGLONG SessionID)
	{
		// 1. 유저 찾기
		AcquireSRWLockShared(&srwl);		// ----------- 락
		auto NowPlayer = m_umapJoinUser.find(SessionID);

		// 2. 원하는 유저를 못찾았으면, nullptr 리턴
		if (NowPlayer == m_umapJoinUser.end())
		{
			ReleaseSRWLockShared(&srwl);		// ----------- 언락
			return nullptr;
		}

		// 3. 원하는 유저를 찾았으면 해당 유저를 리턴
		stPlayer* retPlayer = NowPlayer->second;

		ReleaseSRWLockShared(&srwl);		// ----------- 언락

		return retPlayer;
	}
	   
	// 유저 관리 자료구조에, 새로 접속한 유저 추가
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID, stPlayer*
	// return : 추가 성공 시, true
	//		  : SessionID가 중복될 시 false
	bool CLogin_NetServer::InsertPlayerFunc(ULONGLONG SessionID, stPlayer* InsertPlayer)
	{
		AcquireSRWLockExclusive(&srwl);		// ----------- 락
		auto ret = m_umapJoinUser.insert(make_pair(SessionID, InsertPlayer));
		ReleaseSRWLockExclusive(&srwl);		// ----------- 언락

		// 중복된 키일 시 false 리턴.
		if (ret.second == false)
			return false;

		return true;
	}
	
	// 유저 관리 자료구조에 있는, 유저의 정보 갱신
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID, AccountNo, UserID, NickName
	// return : 없음
	void CLogin_NetServer::SetPlayerFunc(ULONGLONG SessionID, INT64 AccountNo, char* UserID, char* NickName)
	{
		// 1. 유저 찾기
		AcquireSRWLockExclusive(&srwl);		// ----------- 락
		auto NowPlayer = m_umapJoinUser.find(SessionID);

		// 원하는 유저를 못찾았으면, 아직 접속도 안한 유저?가 이상한 패킷을 보낸것.
		// 무시한다
		if (NowPlayer == m_umapJoinUser.end())
		{
			ReleaseSRWLockExclusive(&srwl);		// ----------- 언락
			return;
		}

		// 2. 정보 갱신 (AccountNo, ID, NickName)
		NowPlayer->second->m_i64AccountNo = AccountNo;

		int len = (int)strlen(UserID);
		MultiByteToWideChar(CP_UTF8, 0, UserID, (int)strlen(UserID), NowPlayer->second->m_wcID, len);

		len = (int)strlen(NickName);
		MultiByteToWideChar(CP_UTF8, 0, NickName, (int)strlen(NickName), NowPlayer->second->m_wcNickName, len);


		ReleaseSRWLockExclusive(&srwl);		// ----------- 언락
	}

	// 유저 관리 자료구조에 있는, 유저 삭제
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID
	// return : 성공 시, stPlayer*
	//		  : 실패 시, nullptr
	CLogin_NetServer::stPlayer* CLogin_NetServer::ErasePlayerFunc(ULONGLONG SessionID)
	{
		// 1. 유저 찾기
		AcquireSRWLockExclusive(&srwl);		// ----------- 락
		auto FindPlayer = m_umapJoinUser.find(SessionID);

		// 원하는 유저를 못찾았으면, 이미 종료했거나 없는 유저.
		// 무시한다
		if (FindPlayer == m_umapJoinUser.end())
		{
			ReleaseSRWLockExclusive(&srwl);		// ----------- 언락
			return nullptr;
		}

		// 2. 리턴할 값 받아둔다.
		stPlayer* ret = FindPlayer->second;

		// 3. 자료구조에서 erase
		m_umapJoinUser.erase(FindPlayer);

		ReleaseSRWLockExclusive(&srwl);		// ----------- 언락

		// 4. 값 리턴
		return ret;
	}




	/////////////////////////////
	// Lan의 패킷 처리 함수
	/////////////////////////////	

	// LanClient에게 받은 패킷 처리 함수 (메인)
	//
	// Parameter : 패킷 타입, AccountNo, SessionID(LanClient에게 보냈던 Parameter)
	// return : 없음
	void CLogin_NetServer::LanClientPacketFunc(WORD Type, INT64 AccountNo, ULONGLONG SessionID)
	{
		// Type에 따라 분기문 처리

		switch (Type)
		{
			// 게임,채팅에게 보낸 응답 완료 시, 유저에게 성공패킷 Send
		case en_PACKET_SS_RES_NEW_CLIENT_LOGIN:
			Success_Packet(SessionID, AccountNo, dfLOGIN_STATUS_OK);
			break;

			// 내가 지정한 것 외 패킷 응답이 오면 말이 안됨. 
			// 내부통신이기 때문에 무조건 안전해야함. 크래시
		default:
			g_LoginDump->Crash();
			break;
		}

	}




	/////////////////////////////
	// Net의 패킷 처리 함수
	/////////////////////////////

	// 로그인 요청
	// 
	// Parameter : SessionID, Packet*
	// return : 없음
	void CLogin_NetServer::LoginPacketFunc(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1. AccountNo, Token 마샬링
		INT64 AccountNo;
		char Token[64];

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData(Token, 64);

		// 2. DB와 정보 검증
		// 검증 내용
		// -- account 테이블에 유저가 존재하는가
		// -- account 유저가 가진 토큰키가 일치하는가
		// -- status 상태가 로그아웃 중인가

		// 쿼리 날리기
		char cQurey[200] = "SELECT * FROM `v_account` WHERE `accountno` = %d\0";
		m_AcountDB_Connector->Query(cQurey, AccountNo);

		MYSQL_ROW rowData = m_AcountDB_Connector->FetchRow();

		// -- account 테이블에 유저가 존재하는가
		if (rowData == NULL)
		{
			// 쿼리 정리
			m_AcountDB_Connector->FreeResult();

			// 존재하지 않는다면, "존재하지 않음" 메시지를 Send한다.
			Fail_Packet(SessionID, AccountNo, dfLOGIN_STATUS_ACCOUNT_MISS);

			return;
		}

		// -- account 유저가 가진 토큰키가 일치하는가
		// 값이 NULL 이거나 일치하지 않으면 메시지 보냄
		if(rowData[3] == NULL ||
			memcmp(rowData[3], Token, 64) != 0)
		{
			/*
			// 일치하지 않는다면 "토큰 키 일치하지 않음" 메시지를 Send한다.
			Fail_Packet(SessionID, AccountNo, dfLOGIN_STATUS_SESSION_MISS, rowData[1], rowData[2]);

			// 쿼리 정리
			m_AcountDB_Connector->FreeResult();
			return;
			*/
		}

		// -- status 상태가 로그아웃 중인가
		if (*rowData[4] != '0')
		{
			// 쿼리 정리
			m_AcountDB_Connector->FreeResult();

			// 로그 아웃 중이 아니라면, "이전 정보가 남아있음" 메시지를 Send한다.
			Fail_Packet(SessionID, AccountNo, dfLOGIN_STATUS_STATUS_MISS, rowData[1], rowData[2]);
			
			return;
		}		

		// 3. 플레이어 셋팅
		SetPlayerFunc(SessionID, AccountNo, rowData[1], rowData[2]);

		// 4. LanServer를 통해, 각 서버에 정보 전달
		m_cLanS->UserJoinSend(AccountNo, Token, SessionID);

		// 쿼리 정리
		m_AcountDB_Connector->FreeResult();

	}   	
	   	  
	// "성공" 패킷 만들고 보내기
	//
	// Parameter : SessionID, AccountNo, Status, UserID, NickName
	// return : 없음
	void CLogin_NetServer::Success_Packet(ULONGLONG SessionID, INT64 AccountNo, BYTE Status)
	{
		// 1. 유저 존재여부 체크
		stPlayer* NowPlayer = FindPlayerFunc(SessionID);

		// 없는 유저라면, 이미 로그아웃 한 유저
		// 처리 없이 리턴.
		if (NowPlayer == nullptr)
			return;

		// 2. AccountNo가 같은지 체크
		// 다르다면, 덤프. 위에서 유저 존재여부를 체크했기 때문에 여기서 걸리면 안됨.
		if (NowPlayer->m_i64AccountNo != AccountNo)
			g_LoginDump->Crash();

		// 3. 직렬화버퍼 Alloc
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		// 4. 값 셋팅
		WORD Type = en_PACKET_CS_LOGIN_RES_LOGIN;
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&Status, 1);

		SendBuff->PutData((char*)&NowPlayer->m_wcID, 40);
		SendBuff->PutData((char*)&NowPlayer->m_wcNickName, 40);
		
		SendBuff->MoveWritePos(34);

		/*SendBuff->PutData((char*)&m_stConfig.GameServerIP, 32);
		SendBuff->PutData((char*)&m_stConfig.GameServerPort, 2);*/

		SendBuff->PutData((char*)&m_stConfig.ChatServerIP, 32);
		SendBuff->PutData((char*)&m_stConfig.ChatServerPort, 2);

		// 5. SendPacket()
		// 보내고 끊기.
		SendPacket(SessionID, SendBuff, TRUE);
	}

	// "실패" 패킷 만들고 보내기
	//
	// Parameter : SessionID, AccountNo, Status, UserID, NickName
	// return : 없음
	void CLogin_NetServer::Fail_Packet(ULONGLONG SessionID, INT64 AccountNo, BYTE Status, char* UserID, char* NickName)
	{
		// 1. 직렬화버퍼 Alloc
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		// 2. 값 셋팅
		WORD Type = en_PACKET_CS_LOGIN_RES_LOGIN;
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&Status, 1);

		// ID와 Password가 nullptr이라면, 직렬화버퍼의 rear만 움직인다.
		// (AccountNo가 없던가 그 외 이유로 ID와 닉네임을 확인 불가능한 상황.)
		if (UserID == nullptr)
		{
			SendBuff->MoveWritePos(80);
		}

		else
		{
			SendBuff->PutData((char*)&UserID, 40);
			SendBuff->PutData((char*)&NickName, 40);
		}

		// 어차피 유저는 접속 못하기 때문에, IP도 필요없다.
		// 그 만큼의 Rear만 움직인다.
		SendBuff->MoveWritePos(68);

		// 3. SendPacket()
		// 보내고 끊기
		SendPacket(SessionID, SendBuff, TRUE);
	}









	// Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CLogin_NetServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CLogin_NetServer::OnClientJoin(ULONGLONG SessionID)
	{
		// 1. 플레이어 TLS에서 Alloc
		stPlayer* JoinPlayer = m_MPlayerTLS->Alloc();
		InterlockedAdd(&g_lStruct_PlayerCount, 1);

		// 2. umap에 유저 추가
		if (InsertPlayerFunc(SessionID, JoinPlayer) == false)
		{
			printf("duplication SessionID!!\n");
			g_LoginDump->Crash();
		}

	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CLogin_NetServer::OnClientLeave(ULONGLONG SessionID)
	{
		// 1. 유저 삭제
		stPlayer* ErasePlayer = ErasePlayerFunc(SessionID);
		if (ErasePlayer == nullptr)
			g_LoginDump->Crash();

		// 2. stPlayer* 반환
		m_MPlayerTLS->Free(ErasePlayer);
		InterlockedAdd(&g_lStruct_PlayerCount, -1);
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CLogin_NetServer::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{		
		try
		{
			// 1. Type만 마샬링
			WORD Type;
			Payload->GetData((char*)&Type, 2);

			// 2. Type에 따라 분기처리
			switch (Type)
			{
				// 로그인 요청
			case en_PACKET_CS_LOGIN_REQ_LOGIN:
				LoginPacketFunc(SessionID, Payload);
				break;

				// 로그인 요청이 아니면 접속 끊음
				// 현재는 로그인 요청 외에 타입이 올 이유가 없음
			default:
				throw CException(_T("OnRecv(). TypeError"));
				break;
			}

		}
		catch (CException& exc)
		{
			char* pExc = exc.GetExceptionText();		

			// 로그 찍기 (로그 레벨 : 에러)
			cLoginLibLog->LogSave(L"LoginServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)pExc);	

			// 접속 끊기 요청
			Disconnect(SessionID);
		}
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CLogin_NetServer::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{
		// 할것 없음
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_NetServer::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_NetServer::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CLogin_NetServer::OnError(int error, const TCHAR* errorStr)
	{}





}


// ----------------------------------
// LoginServer(LanServer)
// ----------------------------------
namespace Library_Jingyu
{
	// 부모 셋팅
	//
	// Parameter : CLogin_NetServer*
	// return : 없음
	void CLogin_LanServer::ParentSet(CLogin_NetServer* parent)
	{
		m_cParentS = parent;
	}


	// GameServer, ChatServer의 LanClient에 새로운 유저 접속 알림
	// 
	// Parameter : AccountNo, Token(64Byte), ULONGLONG Parameter
	// return : 없음
	void CLogin_LanServer::UserJoinSend(INT64 AccountNo, char* Token, ULONGLONG Parameter)
	{
		// !! 인자로 받은 Parameter는 NetServer의 SessionID !!

		// 1. SendBuff Alloc()
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		// 2. 서버에 접속한 모든 유저(LanClient)에게 보낼 데이터 셋팅
		WORD Type = en_PACKET_SS_REQ_NEW_CLIENT_LOGIN;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData(Token, 64);
		SendBuff->PutData((char*)&Parameter, 8);

		// 3. 배열 내에 카운트가 0이라면, "서비스 중인 서버가 없음" 패킷 보냄
		
		AcquireSRWLockShared(&srwl);		// 락 ------------------

		if (m_iArrayCount == 0)
		{
			ReleaseSRWLockShared(&srwl);	 // 락 해제 ------------------
			m_cParentS->Fail_Packet(Parameter, AccountNo, dfLOGIN_STATUS_NOSERVER);
			return;
		}

		// 4. 데이터 Send하기
		// 보내기 전에 접속한 유저 수 만큼 레퍼런스 카운트 증가
		SendBuff->Add(m_iArrayCount);

		// 접속자 수 만큼 돌면서 Send
		int index = 0;
		while (index < m_iArrayCount)
		{
			SendPacket(m_arrayJoinServer[index], SendBuff);
			index++;
		}	

		ReleaseSRWLockShared(&srwl);	 // 락 해제 ------------------

		// 5. 모두 Send했으니 Free
		CProtocolBuff_Lan::Free(SendBuff);
	}


	// 생성자
	CLogin_LanServer::CLogin_LanServer()
	{
		// 락 초기화
		InitializeSRWLock(&srwl);

		m_iArrayCount = 0;
	}

	// 소멸자
	CLogin_LanServer::~CLogin_LanServer()
	{
		// 하는거 없음
	}




	// -----------------------------------
	// 가상함수
	// -----------------------------------

	// Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CLogin_LanServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CLogin_LanServer::OnClientJoin(ULONGLONG SessionID)
	{
		// 1. 배열에 유저 추가
		AcquireSRWLockExclusive(&srwl);		// ------- 락

		m_arrayJoinServer[m_iArrayCount] = SessionID;
		m_iArrayCount++;

		ReleaseSRWLockExclusive(&srwl);		// ------- 락 해제
	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CLogin_LanServer::OnClientLeave(ULONGLONG SessionID)
	{
		// 1. 배열에서 유저 제거		
		int Tempindex = 0;

		AcquireSRWLockExclusive(&srwl);		// ------- 락

		// 접속한 유저가 1명이라면, 카운트만 1 줄이고 끝
		if (m_iArrayCount == 1)
		{
			m_iArrayCount--;
			ReleaseSRWLockExclusive(&srwl);		// ------- 락 해제
			return;
		}

		// 유저가 1명 이상이라면 로직 처리
		while (Tempindex < m_iArrayCount)
		{
			// 원하는 유저를 찾았으면
			if (m_arrayJoinServer[Tempindex] == SessionID)
			{
				// 만약, 해당 유저의 위치가 마지막이라면 카운트만 1 줄이고 끝
				if (Tempindex == (m_iArrayCount - 1))
				{
					m_iArrayCount--;
					break;
				}

				// 마지막 위치가 아니라면, 마지막 위치를 삭제할 위치에 넣은 후 카운트 감소
				m_arrayJoinServer[Tempindex] = m_arrayJoinServer[m_iArrayCount - 1];
				m_iArrayCount--;
				break;
			}

			Tempindex++;
		}

		ReleaseSRWLockExclusive(&srwl);		// ------- 락 해제
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CLogin_LanServer::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 패킷 마샬링
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 2. 타입에 따라 처리
		switch (Type)
		{
			// 다른 서버가 로그인 랜서버로 로그인
			// 현재는 할거 없음.
			// 차후 게임서버가 추가되면, 게임서버인지 랜서버인지?에 따라 달라질듯. 보내주는 정보가 달라지려나?
		case en_PACKET_SS_LOGINSERVER_LOGIN:			
			break;

			// 게임 or 채팅에서 응답이 왔다면, 넷 서버로 중계
		case en_PACKET_SS_RES_NEW_CLIENT_LOGIN:
		{
			INT64 AccountNo;
			ULONGLONG Parameter;

			Payload->GetData((char*)&AccountNo, 8);
			Payload->GetData((char*)&Parameter, 8);

			// 2. Net의 Lan 패킷처리 함수 호출
			m_cParentS->LanClientPacketFunc(Type, AccountNo, Parameter);
		}
			break;

			// 그 외의 것은 잘못된것. 덤프
		default:
			g_LoginDump->Crash();
			break;
		}
		
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CLogin_LanServer::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{

	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_LanServer::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_LanServer::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CLogin_LanServer::OnError(int error, const TCHAR* errorStr)
	{

	}




}


