#include "pch.h"
#include "MatchmakingServer.h"
#include "Parser/Parser_Class.h"
#include "Log/Log.h"
#include "Protocol_Set/CommonProtocol_2.h"		// 궁극적으로는 CommonProtocol.h로 이름 변경 필요. 지금은 채팅서버에서 로그인 서버를 이용하는 프로토콜이 있어서 _2로 만듬.


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
		// Matchmaking Lan 클라의 정보 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MATCHLANCLIENT")) == false)
			return false;		

		// ServerNo
		if (Parser.GetValue_Int(_T("ServerNo"), &pConfig->ServerNo) == false)
			return false;

		// MasterToken 
		TCHAR tcToken[33];
		if (Parser.GetValue_String(_T("MasterToken"), tcToken) == false)
			return false;

		// 클라가 들고올 때는 char형으로 들고오기 때문에 변환해서 가지고 있어야한다.
		int len = WideCharToMultiByte(CP_UTF8, 0, tcToken, lstrlenW(tcToken), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, tcToken, lstrlenW(tcToken), pConfig->MasterToken, len, NULL, NULL);

		return true;
	}
	   

	// -------------------------------------
	// 스레드
	// -------------------------------------

	// matchmakingDB에 일정 시간마다 하트비트를 쏘는 스레드.
	UINT WINAPI Matchmaking_Net_Server::DBHeartbeatThread(LPVOID lParam)
	{
		Matchmaking_Net_Server* g_This = (Matchmaking_Net_Server*)lParam;

		// 종료용 이벤트 로컬로 받아두기
		HANDLE hExitEvent = g_This->hDB_HBThread_ExitEvent;

		// 하트비트 시간 받아두기
		int iHeartbeatTime = g_This->m_stConfig.MatchDBHeartbeat;

		// DBConenct 받아두기
		CBConnectorTLS* pMatchDBcon = g_This->m_MatchDB_Connector;

		// 서버 넘버 받아두기
		int iServerNo = g_This->m_stConfig.ServerNo;

		while (1)
		{
			// 메치메이킹DB의 하트비트 갱신 시간만큼 잔다.
			DWORD Check = WaitForSingleObject(hExitEvent, iHeartbeatTime);

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


			// 무엇도 아니라면 할일 한다.

			// 1. 쿼리 날리기
			char cQurey[200] = "UPDATE `matchmaking_status`.`server` SET `heartbeat` = NOW() WHERE `serverno` = %d\0";
			pMatchDBcon->Query(cQurey, iServerNo);

			// 2. 쿼리 결과 가져오기
			MYSQL_ROW rowData = pMatchDBcon->FetchRow();

			// 실패했다면 Crash
			if (rowData == NULL)
			{
				// 실패 메시지 찍는다.
				cMatchServerLog->LogSave(L"MatchServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, 
					L"DBHeartbeatThread QueryError. Errno : %d, Msg : %s", pMatchDBcon->GetLastError(), pMatchDBcon->GetLastErrorMsg());

				gMatchServerDump->Crash();
			}

			// 3. 쿼리 정리
			pMatchDBcon->FreeResult();

		}
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

		// 서버 종료 로그 찍기		
		cMatchServerLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");
	}


	// -------------------------------------
	// NetServer의 가상함수
	// -------------------------------------

	bool Matchmaking_Net_Server::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	void Matchmaking_Net_Server::OnClientJoin(ULONGLONG SessionID)
	{}

	void Matchmaking_Net_Server::OnClientLeave(ULONGLONG SessionID)
	{}

	void Matchmaking_Net_Server::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{}

	void Matchmaking_Net_Server::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	void Matchmaking_Net_Server::OnWorkerThreadBegin()
	{}

	void Matchmaking_Net_Server::OnWorkerThreadEnd()
	{}

	void Matchmaking_Net_Server::OnError(int error, const TCHAR* errorStr)
	{}




	// -------------------------------------
	// 생성자와 소멸자
	// -------------------------------------

	// 생성자
	Matchmaking_Net_Server::Matchmaking_Net_Server()
		:CNetServer()
	{
		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			gMatchServerDump->Crash();

		// DBThread 종료용 이벤트 생성
		// 
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		hDB_HBThread_ExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		// MatchmakingDB와 Connect
		m_MatchDB_Connector = new CBConnectorTLS(m_stConfig.DB_IP, m_stConfig.DB_User, m_stConfig.DB_Password, m_stConfig.DB_Name, m_stConfig.DB_Port);

		// ------------------- 로그 저장할 파일 셋팅
		cMatchServerLog->SetDirectory(L"MatchServer");
		cMatchServerLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);
	}

	// 소멸자
	Matchmaking_Net_Server::~Matchmaking_Net_Server()
	{
		// 서버가 아직 가동중이라면 서버 종료
		// Net 라이브러리가 가동중인지만 알면 됨.
		if (GetServerState() == true)
			ServerStop();

		// MatchDB 커넥터 소멸
		delete m_MatchDB_Connector;

		// DBThread 종료용 이벤트 반환
		CloseHandle(hDB_HBThread_ExitEvent);
	}
	
}