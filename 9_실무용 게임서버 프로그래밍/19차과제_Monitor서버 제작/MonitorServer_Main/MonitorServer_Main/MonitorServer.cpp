#include "pch.h"
#include "MonitorServer.h"
#include "Log\Log.h"
#include "DB_Connector\DB_Connector.h"
#include "Parser\Parser_Class.h"

#include <process.h>
#include <strsafe.h>
#include <time.h>


namespace Library_Jingyu
{
	CCrashDump* g_MonitorDump = CCrashDump::GetInstance();

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cMonitorLibLog = CSystemLog::GetInstance();

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

		// ----------------- 파일에서 정보 읽어오기
		if (SetFile(&m_stConfig) == false)
			g_MonitorDump->Crash();

		// 락 초기화
		InitializeSRWLock(&srwl);
		InitializeSRWLock(&DBInfoSrwl);

		// ------------------- 로그 저장할 파일 셋팅
		// Net서버로 위치 변경 필요
		cMonitorLibLog->SetDirectory(L"MonitorServer");
		cMonitorLibLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LanLogLevel);

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
		CBConnectorTLS cConnector(g_This->m_stConfig.DB_IP, g_This->m_stConfig.DB_User,
			g_This->m_stConfig.DB_Password, g_This->m_stConfig.DB_Name, g_This->m_stConfig.DB_Port);

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
			StringCbPrintfA(DBTableName, 30, "monitorlog_%04d-%02d", NowTime.tm_year + 1900, NowTime.tm_mon + 1);


			// -----------------------------------------------------
			// DB에 쓰기
			// -----------------------------------------------------

			i = 0;
			while (i < TempCount)
			{
				// TempCount만큼 돌면서 DB에 Write한다.

				// 쿼리 만들기. -----------
				char query[1024] = "INSERT INTO `";
				StringCbCatA(query, 1024, DBTableName);
				StringCbCatA(query, 1024, "` (`logtime`, `serverno`, `servername`, `type`, `value`, `min`, `max`, `avg`) VALUE (now(), %d, '%s', %d, %d, %d, %d, %.2f)");
								
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
	bool CLan_Monitor_Server::ServerStart()
	{
		// 랜서버 오픈
		if (Start(m_stConfig.LanBindIP, m_stConfig.LanPort, m_stConfig.LanCreateWorker, m_stConfig.LanActiveWorker, m_stConfig.LanCreateAccept,
			m_stConfig.LanNodelay, m_stConfig.LanMaxJoinUser) == false)
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
	void CLan_Monitor_Server::ServerStop()
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

	// 파일에서 Config 정보 읽어오기
	// 
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CLan_Monitor_Server::SetFile(stConfigFile* pConfig)
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



	// 모니터링 클라이언트로부터 받은 정보 갱신
	// 정보 갱신 후, 각 뷰어들에게 전송
	//
	// Parameter : Lan직렬화 버퍼
	// return : 없음
	void CLan_Monitor_Server::DataUpdatePacket(CProtocolBuff_Lan* Payload)
	{
		// 마샬링
		BYTE Type;
		Payload->GetData((char*)&Type, 1);
		Type = Type - 1;	// 배열에 즉시 접근하기 때문에 -1을 해야한다. 배열은 0부터 시작.

		int Value;
		Payload->GetData((char*)&Value, 4);

		int TimeStamp;
		Payload->GetData((char*)&TimeStamp, 4);


		// 정보 갱신
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

		ReleaseSRWLockExclusive(&DBInfoSrwl);	// ------------- 언락


		// 모든 뷰어에게 데이터 샌드하기
		AcquireSRWLockShared(&srwl);			// ------------- 공유모드 락

		WORD SendType = en_PACKET_SS_MONITOR_DATA_UPDATE;

		int i = 0;
		while (i < m_iArrayCount)
		{
			CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

			SendBuff->PutData((char*)&SendType, 2);

			BYTE ServerNo = m_arrayJoinServer[i].m_bServerNo;
			SendBuff->PutData((char*)&ServerNo, 1);
			SendBuff->PutData((char*)&Type, 1);
			SendBuff->PutData((char*)&Value, 4);
			SendBuff->PutData((char*)&TimeStamp, 4);

			SendPacket(m_arrayJoinServer[i].m_ullSessionID, SendBuff);

			++i;
		}

		ReleaseSRWLockShared(&srwl);			// ------------- 공유모드 언락

	}


	// 로그인 요청 처리
	//
	// Parameter : Lan직렬화 버퍼
	// return : 없음
	void CLan_Monitor_Server::LoginPacket(CProtocolBuff_Lan* Payload)
	{
		// 마샬링
		int ServerNo;
		Payload->GetData((char*)&ServerNo, 4);

		// 유저 정보 갱신
		AcquireSRWLockExclusive(&srwl);		// ------- 락

		m_arrayJoinServer[m_iArrayCount].m_bServerNo = ServerNo;

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
				LoginPacket(Payload);
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