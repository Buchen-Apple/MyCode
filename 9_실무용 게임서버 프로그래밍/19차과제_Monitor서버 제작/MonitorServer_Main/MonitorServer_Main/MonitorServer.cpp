#include "pch.h"
#include "MonitorServer.h"
#include "Log\Log.h"
#include "DB_Connector\DB_Connector.h"
#include "Parser\Parser_Class.h"

#include <process.h>
#include <strsafe.h>
#include <time.h>


// ----------------------
// 모니터링 Net 서버
// ----------------------
namespace Library_Jingyu
{
	CCrashDump* g_MonitorDump = CCrashDump::GetInstance();

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cMonitorLibLog = CSystemLog::GetInstance();

	// -----------------
	// 생성자
	// -----------------
	CNet_Monitor_Server::CNet_Monitor_Server()
	{
		// ----------------- 파일에서 정보 읽어오기
		if (SetFile(&m_stConfig) == false)
			g_MonitorDump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		cMonitorLibLog->SetDirectory(L"MonitorServer");
		cMonitorLibLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LanLogLevel);

		// ----------------- 플레이어 보관 자료구조의 Capacity 할당 (최대 플레이어 수 만큼)
		m_vectorPlayer.reserve(1000);

		// 플레이어 TLS 메모리풀 동적할당
		// 하나도 안만들어둔다.
		m_PlayerTLS = new CMemoryPoolTLS<stPlayer>(0, false);

		// 모니터링 랜 서버 동적할당
		m_CLanServer = new CLan_Monitor_Server;
		m_CLanServer->ParentSet(this);

		// 락 초기화
		InitializeSRWLock(&m_vectorSrwl);
	}

	// -----------------
	// 소멸자
	// -----------------
	CNet_Monitor_Server::~CNet_Monitor_Server()
	{
		// 모니터링 랜 서버 동적해제
		delete m_CLanServer;

		// 넷 서버가 켜져있다면 서버 끈다.
		if (GetServerState() == true)
			ServerStop();

		// 플레이어 TLS 메모리풀 동적해제
		delete m_PlayerTLS;
	}




	// -----------------------
	// 내부에서만 사용 가능한 기능함수
	// -----------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CNet_Monitor_Server::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MonitorServer_Config.ini"));
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
		// Monitor NetServer Config 읽어오기
		////////////////////////////////////////////////////////
		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORSERVERNET")) == false)
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

		// 로그인 키
		TCHAR tcLoginKey[33];
		if (Parser.GetValue_String(_T("LoginKey"), tcLoginKey) == false)
			return false;

		// 클라가 들고올 때는 char형으로 들고오기 때문에 변환해서 가지고 있어야한다.
		int len = WideCharToMultiByte(CP_UTF8, 0, tcLoginKey, lstrlenW(tcLoginKey), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, tcLoginKey, lstrlenW(tcLoginKey), m_cLoginKey, len, NULL, NULL);



		////////////////////////////////////////////////////////
		// Monitor LanServer Config 읽어오기
		////////////////////////////////////////////////////////
		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORSERVERLAN")) == false)
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


		////////////////////////////////////////////////////////
		// DB config 읽어오기
		////////////////////////////////////////////////////////
		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORDBCONFIG")) == false)
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


		return true;
	}
	   	 	
	// 플레이어 자료구조에 플레이어 추가.
	//
	// Parameter : SessionID
	// return : 없음
	void CNet_Monitor_Server::InsertPlayer(ULONGLONG SessionID)
	{
		// 1. 플레이어 구조체 Alloc
		stPlayer* NowPlayer = m_PlayerTLS->Alloc();

		// 2. 유저 셋팅
		NowPlayer->m_ullSessionID = SessionID;
		NowPlayer->m_bLoginCheck = false;

		// 3. 유저 추가
		AcquireSRWLockExclusive(&m_vectorSrwl);		// 락 ------------------		
		m_vectorPlayer.push_back(NowPlayer);
		ReleaseSRWLockExclusive(&m_vectorSrwl);		// 언락 ------------------	
	}

	// 플레이어 자료구조에서 플레이어 제거
	//
	// Parameter : SessionID
	// return : 없는 플레이어 일 시 false.
	bool CNet_Monitor_Server::ErasePlayer(ULONGLONG SessionID)
	{
		AcquireSRWLockExclusive(&m_vectorSrwl);		// 락 ------------------

		// 1. 제거하고자 하는 플레이어 찾기
		int Size = (int)m_vectorPlayer.size();		

		// 만약, 접속중인 플레이어가 1명이거나 가장 뒤에 있다면
		if (Size == 1 || 
			m_vectorPlayer[Size - 1]->m_ullSessionID == SessionID)
		{			
			stPlayer* ErasePlayer = m_vectorPlayer[Size-1];

			// Pop 후, 미리 받아둔 Player를 Free 한다.
			m_vectorPlayer.pop_back();
			m_PlayerTLS->Free(ErasePlayer);

			ReleaseSRWLockExclusive(&m_vectorSrwl);		// 언 락 ------------------

			return true;
		}		

		// 그게 아니라면, 순회하면서 제거할 플레이어 찾음
		else
		{
			int i = 0;
			while (i < Size)
			{
				// 제거하고자 하는 플레이어를 찾았으면
				if (m_vectorPlayer[i]->m_ullSessionID == SessionID)
				{
					// 가장 뒤에 있는 플레이어의 정보를 제거할 플레이어의 위치에 대입
					m_vectorPlayer[i]->m_ullSessionID = m_vectorPlayer[Size - 1]->m_ullSessionID;
					m_vectorPlayer[i]->m_bLoginCheck = m_vectorPlayer[Size - 1]->m_bLoginCheck;
					
					stPlayer* ErasePlayer = m_vectorPlayer[Size - 1];

					// Pop 후, 미리 받아둔 Player를 Free 한다.
					m_vectorPlayer.pop_back();
					m_PlayerTLS->Free(ErasePlayer);
					
					ReleaseSRWLockExclusive(&m_vectorSrwl);		// 언 락 ------------------

					return true;
				}

				++i;
			}
		}

		ReleaseSRWLockExclusive(&m_vectorSrwl);		// 언 락 ------------------

		// 여기까지 왔으면 원하는 유저를 못찾은것. false 리턴
		return false;
	}

	// 접속한 클라들에게 정보 뿌리기 (브로드 캐스팅)
	//
	// Parameter : 서버No, 데이터 타입, 데이터 값, 타임스탬프
	// return : 없음
	void CNet_Monitor_Server::DataBroadCasting(BYTE ServerNo, BYTE DataType, int DataValue, int TimeStamp)
	{
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		// 1. 보낼 패킷 만들기
		WORD Type = en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerNo, 1);
		SendBuff->PutData((char*)&DataType, 1);
		SendBuff->PutData((char*)&DataValue, 4);
		SendBuff->PutData((char*)&TimeStamp, 4);

		// 2. 브로드 캐스팅
		AcquireSRWLockShared(&m_vectorSrwl);	// 공유 락 ----------------

		int Size = (int)m_vectorPlayer.size();

		// 접속한 툴이 1개 이상 있으면 보낸다
		if (Size > 0)
		{
			int i = 0;

			while (i < Size)
			{
				// 로그인 처리된 플레이어일 때만 패킷 보냄
				if (m_vectorPlayer[i]->m_bLoginCheck == true)
				{
					// ref 카운트 1 증가
					SendBuff->Add();

					// Send
					SendPacket(m_vectorPlayer[i]->m_ullSessionID, SendBuff);
				}

				++i;
			}
		}

		ReleaseSRWLockShared(&m_vectorSrwl);	// 공유 언락 ----------------

		// 다 썻으면 Free
		CProtocolBuff_Net::Free(SendBuff);
	}
	
	// 로그인 요청 패킷 처리
	//
	// Parameter : 세션ID, Net 직렬화버퍼
	void CNet_Monitor_Server::LoginPakcet(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		// 1. 마샬링
		char LoginKey[32];		
		Payload->GetData((char*)LoginKey, 32);

		// 2. 로그인 키 비교
		// 로그인 키가 같다면
		if (memcmp(m_cLoginKey, LoginKey, 32) == 0)
		{
			// 1. 락걸고, 로그인 상태로 변경
			AcquireSRWLockExclusive(&m_vectorSrwl);		// 락 -----------

			int Size = (int)m_vectorPlayer.size();

			int i = 0;
			bool bFlag = false;	// 원하는 유저를 찾아서 상태를 변경했는지 여부
			while (i < Size)
			{
				if (m_vectorPlayer[i]->m_ullSessionID == SessionID)
				{
					bFlag = true;
					m_vectorPlayer[i]->m_bLoginCheck = true;
					break;
				}
				++i;
			}

			ReleaseSRWLockExclusive(&m_vectorSrwl);		// 언락 -----------

			// 여기까지 왔는데 Flag가 false라면, 접속도 안한 유저?를 배열에 갖고 있던것?
			// 뭔가 이상함. Crash.
			if (bFlag == false)
				g_MonitorDump->Crash();

			// 2. 성공 패킷 보냄
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
			BYTE Status = dfMONITOR_TOOL_LOGIN_OK;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendBuff);
		}

		// 로그인 키가 다르다면 실패패킷 보냄
		else
		{
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
			BYTE Status = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendBuff);
		}

	}



	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 모니터링 Net서버 시작
	// 내부에서는 CNetServer의 Start 함수 호출
	//
	// Parameter : 없음
	// return : 없음
	bool CNet_Monitor_Server::ServerStart()
	{
		// 모니터링 랜 서버 시작
		if (m_CLanServer->LanServerStart() == false)
			g_MonitorDump->Crash();

		// 모니터링 넷 서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept,
			m_stConfig.Nodelay, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
		{
			g_MonitorDump->Crash();
		}		

		// 서버 오픈 로그 찍기		
		cMonitorLibLog->LogSave(L"MonitorServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// 모니터링 Net서버 종료
	// 내부에서는 CNetServer의 Stop 함수 호출
	//
	// Parameter : 없음
	// return : 없음
	void CNet_Monitor_Server::ServerStop()
	{
		// 랜서버 종료
		m_CLanServer->LanServerStop();			

		// 넷서버 종료
		Stop();			

		// 서버 종료 로그 찍기		
		cMonitorLibLog->LogSave(L"MonitorServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");
	}




	/////////////////////////////
	// 가상함수
	/////////////////////////////

	// Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CNet_Monitor_Server::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CNet_Monitor_Server::OnClientJoin(ULONGLONG SessionID)
	{
		// 자료구조에 플레이어 추가
		InsertPlayer(SessionID);
	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CNet_Monitor_Server::OnClientLeave(ULONGLONG SessionID)
	{
		// 원하는 유저를 못찾았다면 Crash()
		if (ErasePlayer(SessionID) == false)
			g_MonitorDump->Crash();
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CNet_Monitor_Server::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		try
		{
			// 1. 마샬링
			WORD Type;

			Payload->GetData((char*)&Type, 2);

			switch (Type)
			{
				// 로그인 요청
			case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
				LoginPakcet(SessionID, Payload);
				break;

				// 그 외에는 접속 끊음
			default:
				throw CException(_T("CNet_Monitor_Server. OnRecv() --> Type Error!!!"));
			}


		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			 cMonitorLibLog->LogSave(L"MonitorServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				 (TCHAR*)exc.GetExceptionText());

			Disconnect(SessionID);
		}

	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CNet_Monitor_Server::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CNet_Monitor_Server::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CNet_Monitor_Server::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CNet_Monitor_Server::OnError(int error, const TCHAR* errorStr)
	{}
}

// ----------------------
// 모니터링 Lan 서버
// ----------------------
namespace Library_Jingyu
{
	// 데이터 전송 시 서버 No
	enum en_Monitor_Type
	{
		dfMONITOR_ETC = 2,		// 채팅서버 외 모든 정보.
		dfMONITOR_CHATSERVER = 3		// 채팅 서버
	};

	// 생성자
	CLan_Monitor_Server::CLan_Monitor_Server()
	{
		// ----------------- DB 정보 보관 구조체에 타입과 서버 이름 셋팅해두기
		int i = 0;
		while (i < dfMONITOR_DATA_TYPE_END - 1)
		{
			m_stDBInfo[i].m_iType = i + 1;

			// 하드웨어라면 
			if (i < dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY)
			{
				// 이름과 서버 No 셋팅
				strcpy_s(m_stDBInfo[i].m_cServerName, 64, "HardWare");
				m_stDBInfo[i].m_iServerNo = dfMONITOR_ETC;
			}

			// 매치메이킹 서버라면
			else if (i < dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS)
			{
				// 이름과 서버 No 셋팅
				strcpy_s(m_stDBInfo[i].m_cServerName, 64, "MatchServer");
				m_stDBInfo[i].m_iServerNo = dfMONITOR_ETC;
			}

			// 마스터 서버라면
			else if (i < dfMONITOR_DATA_TYPE_MASTER_BATTLE_STANDBY_ROOM)
			{
				// 이름과 서버 No 셋팅
				strcpy_s(m_stDBInfo[i].m_cServerName, 64, "MasterServer");
				m_stDBInfo[i].m_iServerNo = dfMONITOR_ETC;
			}

			// 배틀 서버라면
			else if (i < dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY)
			{
				// 이름과 서버 No 셋팅
				strcpy_s(m_stDBInfo[i].m_cServerName, 64, "BattleServer");
				m_stDBInfo[i].m_iServerNo = dfMONITOR_ETC;
			}

			// 채팅 서버라면
			else
			{
				// 이름과 서버 No 셋팅
				strcpy_s(m_stDBInfo[i].m_cServerName, 64, "ChatServer");
				m_stDBInfo[i].m_iServerNo = dfMONITOR_CHATSERVER;
			}

			++i;
		}

		// 락 초기화
		InitializeSRWLock(&srwl);
		InitializeSRWLock(&DBInfoSrwl);

		// DBWirteThread 종료용 이벤트 생성
		// 
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		m_hDBWriteThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	}


	// 소멸자
	CLan_Monitor_Server::~CLan_Monitor_Server()
	{
		// 랜 서버가 켜져있다면 종료
		if (GetServerState() == true)
			LanServerStop();

		// DBWirteThread 종료용 이벤트 반환
		CloseHandle(m_hDBWriteThreadExitEvent);
	}






	// DB에 정보 저장 스레드
	// 1분에 1회 DB에 Insert
	UINT WINAPI CLan_Monitor_Server::DBWriteThread(LPVOID lParam)
	{
		CLan_Monitor_Server* g_This = (CLan_Monitor_Server*)lParam;

		// 종료 이벤트 받아두기
		HANDLE hEvent = g_This->m_hDBWriteThreadExitEvent;

		// DB에 Write하기전에 임시로 저장해둘 버퍼
		stDBWriteInfo TempDBInfo[dfMONITOR_DATA_TYPE_END - 1];
		int TempCount;

		// 총 몇 개의 배열이 만들어져 있는지 체크용.
		// 전체 순회 시 사용
		int Index = dfMONITOR_DATA_TYPE_END - 1;

		// DB와 연결
		CBConnectorTLS cConnector(g_This->m_ParentThis->m_stConfig.DB_IP, g_This->m_ParentThis->m_stConfig.DB_User,
			g_This->m_ParentThis->m_stConfig.DB_Password, g_This->m_ParentThis->m_stConfig.DB_Name, g_This->m_ParentThis->m_stConfig.DB_Port);

		// 템플릿 테이블 이름
		char TemplateName[30] = "monitorlog_template";

		while (1)
		{
			// 1분에 한번 깨어나서 일을 한다.
			DWORD ret = WaitForSingleObject(hEvent, 60000);

			// 이상한 신호라면
			if (ret == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("DBWriteThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 만약, 종료 신호가 왔다면 스레드 종료.
			else if (ret == WAIT_OBJECT_0)
				break;

			// -----------------------------------------------------
			// 그게 아니라면 1분이 되어 깨어난 것이니 일을 한다.
			// -----------------------------------------------------

			AcquireSRWLockShared(&g_This->DBInfoSrwl);		// 공유모드 락 -----------

			int i = 0;
			TempCount = 0;
			while (i < Index)
			{
				// 1. 해당 배열이 정보가 있는 배열인지 확인
				if (g_This->m_stDBInfo[i].m_iTotal == 0)
				{
					// 정보가 없으면, DB에 저장 안한다.
					// !! 정보가 없다는것은, 1분동안 정보가 하나도 안왔다는 것인데, 말도안됨. !!
					// !! 서버가 꺼져있는 경우 말고는 거~~~의 없음 !! 
					++i;
					continue;
				}

				// 2. 정보가 있다면, 정상적으로 값 채우기
				else
				{
					TempDBInfo[TempCount].m_iType = g_This->m_stDBInfo[i].m_iType;
					strcpy_s(TempDBInfo[TempCount].m_cServerName, 64, g_This->m_stDBInfo[i].m_cServerName);

					TempDBInfo[TempCount].m_iValue = g_This->m_stDBInfo[i].m_iValue;
					TempDBInfo[TempCount].m_iMin = g_This->m_stDBInfo[i].m_iMin;
					TempDBInfo[TempCount].m_iMax = g_This->m_stDBInfo[i].m_iMax;
					TempDBInfo[TempCount].m_iAvr = (float)(g_This->m_stDBInfo[i].m_iTotal / g_This->m_stDBInfo[i].m_iTotalCount);
					TempDBInfo[TempCount].m_iServerNo = g_This->m_stDBInfo[i].m_iServerNo;
				}

				// 3. 값 초기화
				g_This->m_stDBInfo[i].init();

				++TempCount;

				++i;
			}

			ReleaseSRWLockShared(&g_This->DBInfoSrwl);		// 공유모드 언락 -----------


			// -----------------------------------------------------
			// DB 이름 만들기. 년/월 별로 변경된다.
			// -----------------------------------------------------	
			char DBTableName[30] = { 0, };

			// 1. 현재 시각을 초 단위로 얻기
			time_t timer = time(NULL);

			// 2. 초 단위의 시간을 분리하여 구조체에 넣기 
			struct tm NowTime;
			if (localtime_s(&NowTime, &timer) != 0)
				g_MonitorDump->Crash();

			// 3. 이름 만들기. [DBName_Year-Mon]의 형태로 만들어진다.
			StringCchPrintfA(DBTableName, 30, "monitorlog_%04d-%02d", NowTime.tm_year + 1900, NowTime.tm_mon + 1);


			// -----------------------------------------------------
			// DB에 쓰기
			// -----------------------------------------------------

			i = 0;
			while (i < TempCount)
			{
				// TempCount만큼 돌면서 DB에 Write한다.

				// 쿼리 만들기. -----------
				char query[1024] = "INSERT INTO `";
				StringCchCatA(query, 1024, DBTableName);
				StringCchCatA(query, 1024, "` (`logtime`, `serverno`, `servername`, `type`, `value`, `min`, `max`, `avg`) VALUE (now(), %d, '%s', %d, %d, %d, %d, %.2f)");
								
				// DB로 날리기 -----------
				// 테이블이 없을 경우, 테이블 생성까지 하는 함수 호출.
				cConnector.Query_Save(TemplateName, DBTableName, query, TempDBInfo[i].m_iServerNo, TempDBInfo[i].m_cServerName, TempDBInfo[i].m_iType, TempDBInfo[i].m_iValue,
					TempDBInfo[i].m_iMin, TempDBInfo[i].m_iMax, TempDBInfo[i].m_iAvr);

				i++;
			}

			printf("AAAA!!!\n");
		}

		return 0;
	}


	// -----------------------
	// 외부에서 접근 가능한 기능 함수
	// -----------------------

	// 모니터링 Lan 서버 시작
	// 내부적으로 CLanServer의 Start함수 호출
	//
	// Parameter : 없음
	// return :  없음
	bool CLan_Monitor_Server::LanServerStart()
	{
		// 랜서버 오픈
		if (Start(m_ParentThis->m_stConfig.LanBindIP, m_ParentThis->m_stConfig.LanPort, m_ParentThis->m_stConfig.LanCreateWorker, m_ParentThis->m_stConfig.LanActiveWorker, 
			m_ParentThis->m_stConfig.LanCreateAccept, m_ParentThis->m_stConfig.LanNodelay, m_ParentThis->m_stConfig.LanMaxJoinUser) == false)
			return false;

		// DBWrite 스레드 생성
		m_hDBWriteThread = (HANDLE)_beginthreadex(NULL, 0, DBWriteThread, this, 0, 0);

		return true;
	}

	// 모니터링 Lan 서버 종료
	// 내부적으로 CLanServer의 stop함수 호출
	//
	// Parameter : 없음
	// return :  없음
	void CLan_Monitor_Server::LanServerStop()
	{
		// 1. 랜서버 종료
		Stop();

		// 2. DBWrite 스레드 종료
		SetEvent(m_hDBWriteThreadExitEvent);

		// 종료 대기
		DWORD ret = WaitForSingleObject(m_hDBWriteThreadExitEvent, INFINITE);

		// 정상종료가 아니면 덤프
		if (ret != WAIT_OBJECT_0)
		{
			g_MonitorDump->Crash();
		}

		// 4. DBWirte Thread 핸들 반환
		CloseHandle(m_hDBWriteThread);
	}




	// -----------------------
	// 내부 기능 함수
	// -----------------------
	   	  
	// 부모(NetServer)의 This 셋팅 함수
	//
	// Parameter : 모니터링 넷서버의 This
	void CLan_Monitor_Server::ParentSet(CNet_Monitor_Server* NetServer)
	{
		m_ParentThis = NetServer;
	}



	// 모니터링 클라이언트로부터 받은 정보 갱신
	// 정보 갱신 후, 각 뷰어들에게 전송
	//
	// Parameter : Lan직렬화 버퍼
	// return : 없음
	void CLan_Monitor_Server::DataUpdatePacket(CProtocolBuff_Lan* Payload)
	{
		// 1. 마샬링
		BYTE Type;
		Payload->GetData((char*)&Type, 1);
		Type = Type - 1;	// 배열에 즉시 접근하기 때문에 -1을 해야한다. 배열은 0부터 시작.

		int Value;
		Payload->GetData((char*)&Value, 4);

		int TimeStamp;
		Payload->GetData((char*)&TimeStamp, 4);

		

		// 2. 정보 갱신
		AcquireSRWLockExclusive(&DBInfoSrwl);	// ------------- 락

		// Value가 0이면 정보 갱신하지 않음.
		// 뷰어에게 Send는 한다.
		if (Value != 0)
		{
			// 1. Value 갱신
			m_stDBInfo[Type].m_iValue = Value;

			// 2. Min 갱신
			if (m_stDBInfo[Type].m_iMin > Value)
				m_stDBInfo[Type].m_iMin = Value;

			// 3. Max 갱신
			if (m_stDBInfo[Type].m_iMax < Value)
				m_stDBInfo[Type].m_iMax = Value;

			// 4. Total에 값 추가
			m_stDBInfo[Type].m_iTotal += Value;

			// 5. 카운트 증가
			m_stDBInfo[Type].m_iTotalCount++;			
		}		

		// 6. Send할 서버 No 받아두기
		BYTE ServerNo = m_stDBInfo[Type].m_iServerNo;

		ReleaseSRWLockExclusive(&DBInfoSrwl);	// ------------- 언락	

		// 3. 뷰어에게 정보 보내기
		// 위에서 Type을 1 감소시켰기 때문에 증가시켜야 타입 번호가 된다.
		m_ParentThis->DataBroadCasting(ServerNo, Type + 1, Value, TimeStamp);

	}


	// 로그인 요청 처리
	//
	// Parameter : Lan직렬화 버퍼
	// return : 없음
	void CLan_Monitor_Server::LoginPacket(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 마샬링
		int ServerNo;
		Payload->GetData((char*)&ServerNo, 4);

		int i = 0;

		// 유저 정보 갱신
		AcquireSRWLockExclusive(&srwl);		// ------- 락
				
		while (i < m_iArrayCount)
		{
			// 세션 ID 검사
			if (m_arrayJoinServer[i].m_ullSessionID == SessionID)
			{
				m_arrayJoinServer[i].m_bServerNo = ServerNo;
				break;
			}
			++i;
		}

		ReleaseSRWLockExclusive(&srwl);		// ------- 락 해제
	}



	// -----------------------
	// 가상함수
	// -----------------------

	// Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CLan_Monitor_Server::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CLan_Monitor_Server::OnClientJoin(ULONGLONG SessionID)
	{
		// 1. 배열에 유저 추가
		AcquireSRWLockExclusive(&srwl);		// ------- 락

		m_arrayJoinServer[m_iArrayCount].m_ullSessionID = SessionID;
		m_iArrayCount++;

		ReleaseSRWLockExclusive(&srwl);		// ------- 락 해제
	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CLan_Monitor_Server::OnClientLeave(ULONGLONG SessionID)
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
			if (m_arrayJoinServer[Tempindex].m_ullSessionID == SessionID)
			{
				// 만약, 해당 유저의 위치가 마지막이라면 카운트만 1 줄이고 끝
				if (Tempindex == (m_iArrayCount - 1))
				{
					m_iArrayCount--;
					break;
				}

				// 마지막 위치가 아니라면, 마지막 위치를 삭제할 위치에 넣은 후 카운트 감소
				m_arrayJoinServer[Tempindex].m_ullSessionID = m_arrayJoinServer[m_iArrayCount - 1].m_ullSessionID;
				m_arrayJoinServer[Tempindex].m_bServerNo = m_arrayJoinServer[m_iArrayCount - 1].m_bServerNo;

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
	void CLan_Monitor_Server::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		try
		{
			// 1. 패킷 Type알아오기
			WORD Type;
			Payload->GetData((char*)&Type, 2);

			// 2. Type에 따라 처리
			switch (Type)
			{
				// 로그인 요청 처리
			case en_PACKET_SS_MONITOR_LOGIN:
				LoginPacket(SessionID, Payload);
				break;

				// 데이터 전송 처리
			case en_PACKET_SS_MONITOR_DATA_UPDATE:
				DataUpdatePacket(Payload);
				break;

			default:
				throw CException(_T("OnRecv. Type Error."));
				break;
			}

		}
		catch (CException& exc)
		{
			char* pExc = exc.GetExceptionText();

			// 로그 찍기 (로그 레벨 : 에러)
			cMonitorLibLog->LogSave(L"MonitorServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)pExc);

			g_MonitorDump->Crash();
		}


	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CLan_Monitor_Server::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLan_Monitor_Server::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLan_Monitor_Server::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CLan_Monitor_Server::OnError(int error, const TCHAR* errorStr)
	{}
}