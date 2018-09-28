#include "pch.h"
#include "MonitorClient.h"
#include "CrashDump\CrashDump.h"
#include "Log\Log.h"
#include "Parser\Parser_Class.h"

#include "Protocol_Set\CommonProtocol.h"

#include <strsafe.h>

namespace Library_Jingyu
{

	// 서버 ID
#define SERVER_HARDWARE	2
#define SERVER_CHAT	3


	CCrashDump* g_MonitorClientDump = CCrashDump::GetInstance();
	CSystemLog* g_MonitorClientLog = CSystemLog::GetInstance();

	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	CMonitorClient::CMonitorClient()
	{
		// ------------------- 파일 읽어오기
		if (SetFile(&m_stConfig) == false)
			g_MonitorClientDump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		g_MonitorClientLog->SetDirectory(L"MonitorClient");
		g_MonitorClientLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.m_iLogLevel);

		// 일감 TLS 동적할당.
		m_WorkPool = new CMemoryPoolTLS<st_WorkNode>(0, false);

		// 일감 관리 큐 동적할당
		m_LFQueue = new CLF_Queue<st_WorkNode*>(0, false);

		// 로그인 중 아님으로 시작
		m_ullSessionID = 0xffffffffffffffff;
		m_bLoginCheck = false;


		// 뷰어를 위한 마지막 데이터 저장소의 서버No 셋팅

		int i = 0;
		int End = dfMONITOR_DATA_TYPE_END - 1;
		while (i < End)
		{
			// 하드웨어라면 
			if (i < dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY)
			{
				// 서버 No 셋팅
				m_LastData[i].m_ServerNo = dfMONITOR_ETC;
			}

			// 매치메이킹 서버라면
			else if (i < dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS)
			{
				// 서버 No 셋팅
				m_LastData[i].m_ServerNo = dfMONITOR_ETC;
			}

			// 마스터 서버라면
			else if (i < dfMONITOR_DATA_TYPE_MASTER_BATTLE_STANDBY_ROOM)
			{
				// 서버 No 셋팅
				m_LastData[i].m_ServerNo = dfMONITOR_ETC;
			}

			// 배틀 서버라면
			else if (i < dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY)
			{
				// 서버 No 셋팅
				m_LastData[i].m_ServerNo = dfMONITOR_ETC;
			}

			// 채팅 서버라면
			else
			{
				// 서버 No 셋팅
				m_LastData[i].m_ServerNo = dfMONITOR_CHATSERVER;
			}

			++i;

		}

	}

	CMonitorClient::~CMonitorClient()
	{
		// 아직 서버에 연결중이면, 연결 종료
		if (GetClinetState() == true)
			ClientStop();

		// 일감 TLS 동적해제
		delete m_WorkPool;

		// 일감 관리 큐 동적해제
		delete m_LFQueue;	
	}


	// 모니터 클래스 셋팅.
	// 외부의, WM_CREATE에서 호출
	//
	// Parameter : 인스턴스, 윈도우 핸들
	// retrun : 없음
	void CMonitorClient::SetMonitorClass(HINSTANCE hInst, HWND hWnd)
	{
		// -------------------------
		// 모니터링 정보 파싱
		// -------------------------
		stSetMonitorInfo FileInfo[dfMONITOR_DATA_TYPE_END - 1];

		int i = 0;
		int End = dfMONITOR_DATA_TYPE_END - 1;
		while (i < End)
		{
			// 구역 이름 만들기
			TCHAR Count[3] = { 0, };
			_itot_s(i+1, Count, 3, 10);

			TCHAR AreaName[20] = _T("MONITOR");

			StringCchCat(AreaName, 20, Count);

			// 구역 찾아보기.
			if (SetMonitorInfo(&FileInfo[i], AreaName) == true)
			{
				// 찾았으면, 사용중으로 바꾼다.
				FileInfo[i].m_bUseCheck = true;
			}
			else
			{
				// 못찾았을 경우, 사용중 아님으로 바꾸고 넘어간다.
				FileInfo[i].m_bUseCheck = false;
			}			

			++i;
		}


		// -------------------------
		// 모니터링 뷰어 생성
		// -------------------------
		i = 0;

		while (i < End)
		{
			if(FileInfo[i].m_bUseCheck == true)
			{
				// 생성자 호출
				m_pMonitor[i] = new CMonitorGraphUnit(hInst, hWnd, RGB(FileInfo[i].m_iRedColor, FileInfo[i].m_iGreenColor, FileInfo[i].m_iBlueColor), (CMonitorGraphUnit::TYPE)FileInfo[i].m_iGraphType,
					FileInfo[i].m_iPosX, FileInfo[i].m_iPosY, FileInfo[i].m_iWidth, FileInfo[i].m_iHeight, FileInfo[i].m_tcCaptionText);

				// 추가 정보 셋팅
				// 순서대로 [Max값, 알람 울리는 수치, 표시할 값의 단위]. 
				// - Max 값이 0이면, 현재 큐의 값을 가장 큰 값으로 사용
				// - 알람 값이 0이면 알람 울리지 않음.
				// - 표시할 값의 단위가 L"NULL" 이면, 단위 표시하지 않음.
				m_pMonitor[i]->AddData(FileInfo[i].m_iGraphMaxValue, FileInfo[i].m_iAlarmValue, FileInfo[i].m_iMinAlarmValue, FileInfo[i].m_tcUnit);

				// 컬럼 최초 정보 셋팅
				m_pMonitor[i]->SetColumnInfo(FileInfo[i].m_iDataTypeCount, FileInfo[i].m_iServerNo,
					FileInfo[i].m_iDataType, FileInfo[i].m_tcColumText);
			}

			++i;
		}		
	}

	// 1초에 1회 호출되는 업데이트 정보
	//
	// Parameter : 없음
	// return : 없음
	void CMonitorClient::Update()
	{
		st_WorkNode* NowNode;

		// 1. 큐 만큼 돌면서 처리
		while (m_LFQueue->GetInNode() != 0)
		{
			// 1. 큐에서 데이터 하나 뽑음
			if (m_LFQueue->Dequeue(NowNode) == -1)
				g_MonitorClientDump->Crash();

			try
			{
				// 2. 여기왔다는 것은 무조건 데이터라는것.
				// 데이터 처리
				Data_Packet(NowNode->SessionID, NowNode->m_pPacket, m_LastData);
			}
			catch (CException& exc)
			{
				g_MonitorClientDump->Crash();
			}		
			
			// 3. Packet다 썻으니 래퍼런스카운트 1 감소
			CProtocolBuff_Net::Free(NowNode->m_pPacket);

			// 4. 일감 Free
			m_WorkPool->Free(NowNode);		
		}

		// 2. 큐에 있는것 다 처리했으니, 이제 화면에 출력.
		int MonitorIndex = 0;
		int End = dfMONITOR_DATA_TYPE_END - 1;
		int DataIndex = 0;

		while (DataIndex < End)
		{			
			// 최초로, 한 번이라도 데이터를 받은적 있는 경우에만 아래 처리
			// 받은 적 없으면, 무엇도 체크할것 없음.
			if (m_LastData[DataIndex].m_bFirstCheck == true)
			{
				int Value;

				// value가 -1이 아니라면, 데이터가 온것. 데이터 셋팅
				if (m_LastData[DataIndex].m_Value != -1)
				{
					Value = m_LastData[DataIndex].m_Value;
				}

				// value가 -1이라면 데이터가 안온것.
				else
				{
					if(m_LastData[DataIndex].m_ZeroCount < 3)
						m_LastData[DataIndex].m_ZeroCount++;

					// 만약, 데이터 안온게, 서버 On/Off라면 ZeroCount를 올려준다.
					// 이걸로, 몇 번이나 안왔는지 체크 후 빨간화면 만들기를 한다.
					if (DataIndex + 1 == dfMONITOR_DATA_TYPE_MATCH_SERVER_ON ||
						DataIndex + 1 == dfMONITOR_DATA_TYPE_MASTER_SERVER_ON ||
						DataIndex + 1 == dfMONITOR_DATA_TYPE_CHAT_SERVER_ON)
					{
						// 증가 후, 값이 3이 되었다면, 3번동안 On 메시지가 안온것.
						// 이 때는 정말 서버가 꺼져있다고 판단한 후, 0을 넣는다. (서버 Off)
						if (m_LastData[DataIndex].m_ZeroCount >= 3)
							Value = 0;

						// 아직 3번까지는 안됐다면, 1을 넣는다. (서버 On 중)
						else
							Value = 1;
					}

					// 서버 On/Off가 아니라면, 바로 Value에 0을 넣는다.
					else
						Value = 0;

				}

				// 실제 데이터 넣기. 넣으면서 출력도 같이.
				while (MonitorIndex < End)
				{
					if (m_pMonitor[MonitorIndex] != nullptr)
					{
						m_pMonitor[MonitorIndex]->InsertData(Value, m_LastData[DataIndex].m_ServerNo, DataIndex + 1);
					}
					++MonitorIndex;
				}

				// 초기화
				MonitorIndex = 0;
				m_LastData[DataIndex].m_Value = -1;	
			}

			// 데이터 인덱스 ++
			DataIndex++;
		}
	}
	
	// 모니터링 클라이언트 스타트
	// 내부에서는 NetClient의 Start 함수 호출
	//
	// Parameter : 없음
	// return : 실패 시 false
	bool CMonitorClient::ClientStart()
	{
		if (Start(m_stConfig.m_tcConnectIP, m_stConfig.m_iPort, m_stConfig.m_iCreateWorker, m_stConfig.m_iActiveWorker, m_stConfig.m_iNodelay, 
			m_stConfig.m_bHeadCode, m_stConfig.m_bXORCode1, m_stConfig.m_bXORCode2) == false)
			return false;

		// 연결 성공 로그 찍기	
		g_MonitorClientLog->LogSave(L"MonitorClient", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"Connect Success...");

		return true;
	}

	// 모니터링 클라이언트 스탑
	// 내부에서는 NetClient의 Stop 함수 호출
	//
	// Parameter : 없음
	// return : 없음
	void CMonitorClient::ClientStop()
	{
		Stop();

		// 연결 종료 로그 찍기
		g_MonitorClientLog->LogSave(L"MonitorClient", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"Connect Close...");
	}






	// -----------------------
	// 내부에서만 사용 가능한 기능 함수
	// -----------------------

	// 로그인 요청에 대한 응답 처리
	//
	// Parameter : SessionID, Packet(Net)
	// return : 없음
	void CMonitorClient::Login_Packet(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1. 로그인 결과 알아오기
		BYTE Status = 0;

		Packet->GetData((char*)&Status, 1);

		// 2. 로그인 결과가 0이면 실패
		if (Status == 0)
			g_MonitorClientDump->Crash();

		// 3. 로그인 결과가 1이면 로그인 성공.
		m_bLoginCheck = true;
	}

	// 데이터 전송 패킷 받았을 시 하는 일
	//
	// Parameter : SessionID, Packet(Net), stLastData*
	// return : 없음
	void CMonitorClient::Data_Packet(ULONGLONG SessionID, CProtocolBuff_Net* Packet, stLastData* LastData)
	{
		// 로그인이 된 상태인지 체크
		if (m_bLoginCheck == false)
			g_MonitorClientDump->Crash();

		// 1. 마샬링
		BYTE ServerNo;
		BYTE DataType;
		int DataValue;
		int TimeStamp;

		Packet->GetData((char*)&ServerNo, 1);
		Packet->GetData((char*)&DataType, 1);
		Packet->GetData((char*)&DataValue, 4);
		Packet->GetData((char*)&TimeStamp, 4);

		DataType -= 1;

		// 2. 데이터를 받았으니, 데이터를 최초로 받음 표시를 true로 변경
		LastData[DataType].m_bFirstCheck = true;

		// 3. 데이터 보관
		// 만약, 이미 0으로 출력한 데이터가 뒤늦게 왔다면 무시한다.
		if (LastData[DataType].m_ZeroCount > 0)
		{
			// 타입이, 서버 On/Off라면, 받는 순간, 기존의 ZeroCount를 0으로 만들고, 받은 값을 바로 반영한다.
			if (DataType + 1 == dfMONITOR_DATA_TYPE_MATCH_SERVER_ON ||
				DataType + 1 == dfMONITOR_DATA_TYPE_MASTER_SERVER_ON ||
				DataType + 1 == dfMONITOR_DATA_TYPE_CHAT_SERVER_ON)
			{
				LastData[DataType].m_ZeroCount = 0;
				LastData[DataType].m_ServerNo = ServerNo;
				LastData[DataType].m_Value = DataValue;
			}

			// 만약, ZeroCount가 3이었다면, 오랫동안 안오다가 온것. 바로 반영한다.
			else if (LastData[DataType].m_ZeroCount == 3)
			{
				LastData[DataType].m_ZeroCount = 0;
				LastData[DataType].m_ServerNo = ServerNo;
				LastData[DataType].m_Value = DataValue;
			}

			// 그게 아니라면 카운트 하나 줄이고 끝.
			else
				LastData[DataType].m_ZeroCount--;			
		}

		// 0으로 출력한 데이터가 없다면, 정말 이번에 찍을 데이터가 온 것.
		else if (LastData[DataType].m_ZeroCount == 0)
		{
			LastData[DataType].m_ServerNo = ServerNo;
			LastData[DataType].m_Value = DataValue;			
		}
	}


	// 파싱 함수
	// 
	// Parameter : stInfo*
	// return : 실패 시 false
	bool CMonitorClient::SetFile(stInfo* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MonitorClient_Config.ini"));
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
		// Monitor NetClinet Config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("ConnectIP"), pConfig->m_tcConnectIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("Port"), &pConfig->m_iPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("CreateWorker"), &pConfig->m_iCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ActiveWorker"), &pConfig->m_iActiveWorker) == false)
			return false;		

		// 헤더 코드
		if (Parser.GetValue_Int(_T("HeadCode"), &pConfig->m_bHeadCode) == false)
			return false;

		// xorcode1
		if (Parser.GetValue_Int(_T("XorCode1"), &pConfig->m_bXORCode1) == false)
			return false;

		// xorcode2
		if (Parser.GetValue_Int(_T("XorCode2"), &pConfig->m_bXORCode2) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("Nodelay"), &pConfig->m_iNodelay) == false)
			return false;		

		// 로그 레벨
		if (Parser.GetValue_Int(_T("LogLevel"), &pConfig->m_iLogLevel) == false)
			return false;

		// 로그인 키
		TCHAR tcLoginKey[33];
		if (Parser.GetValue_String(_T("LoginKey"), tcLoginKey) == false)
			return false;

		// 클라가 들고올 때는 char형으로 들고오기 때문에 변환해서 가지고 있어야한다.
		int len = WideCharToMultiByte(CP_UTF8, 0, tcLoginKey, lstrlenW(tcLoginKey), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, tcLoginKey, lstrlenW(tcLoginKey), m_cLoginKey, len, NULL, NULL);

		return true;

	}

	// 모니터링 정보를 읽어오는 함수
	// 
	// Parameter : stSetMonitorInfo 구조체, 구역 이름
	// return : 구역 지정 실패 시 false (그 외 실패는 모두 Crash)
	bool CMonitorClient::SetMonitorInfo(stSetMonitorInfo* pConfig, TCHAR* AreaName)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MonitorClient_Config.ini"));
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
		// Monitor NetClinet Config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(AreaName) == false)
			return false;

		// DataTypeCount
		if (Parser.GetValue_Int(_T("DataTypeCount"), &pConfig->m_iDataTypeCount) == false)
			g_MonitorClientDump->Crash();
		
		// DataType
		// DataTypeCount만큼 돌면서 뽑아낸다.
		int i = 0;
		while (i < pConfig->m_iDataTypeCount)
		{
			// 컬럼 이름 만들기
			TCHAR Count[3] = { 0, };
			_itot_s(i + 1, Count, 3, 10);

			TCHAR TypeName[20] = _T("DataType");

			StringCchCat(TypeName, 20, Count);

			if (Parser.GetValue_Int(TypeName, &pConfig->m_iDataType[i]) == false)
				g_MonitorClientDump->Crash();

			++i;
		}
		

		

		// GraphType
		if (Parser.GetValue_Int(_T("GraphType"), &pConfig->m_iGraphType) == false)
			g_MonitorClientDump->Crash();

		// PosX
		if (Parser.GetValue_Int(_T("PosX"), &pConfig->m_iPosX) == false)
			g_MonitorClientDump->Crash();

		// PosY
		if (Parser.GetValue_Int(_T("PosY"), &pConfig->m_iPosY) == false)
			g_MonitorClientDump->Crash();

		// Width
		if (Parser.GetValue_Int(_T("Width"), &pConfig->m_iWidth) == false)
			g_MonitorClientDump->Crash();

		// Height
		if (Parser.GetValue_Int(_T("Height"), &pConfig->m_iHeight) == false)
			g_MonitorClientDump->Crash();



		// 캡션 텍스트
		if (Parser.GetValue_String(_T("CaptionText"), pConfig->m_tcCaptionText) == false)
			g_MonitorClientDump->Crash();

		// Unit텍스트
		if (Parser.GetValue_String(_T("Unit"), pConfig->m_tcUnit) == false)
			g_MonitorClientDump->Crash();




		// Red
		if (Parser.GetValue_Int(_T("Red"), &pConfig->m_iRedColor) == false)
			g_MonitorClientDump->Crash();

		// Green
		if (Parser.GetValue_Int(_T("Green"), &pConfig->m_iGreenColor) == false)
			g_MonitorClientDump->Crash();

		// Blue
		if (Parser.GetValue_Int(_T("Blue"), &pConfig->m_iBlueColor) == false)
			g_MonitorClientDump->Crash();




		// GraphMaxValue 
		if (Parser.GetValue_Int(_T("GraphMaxValue"), &pConfig->m_iGraphMaxValue) == false)
			g_MonitorClientDump->Crash();

		// AlarmValue 
		if (Parser.GetValue_Int(_T("AlarmValue"), &pConfig->m_iAlarmValue) == false)
			g_MonitorClientDump->Crash();

		// MinAlarmValue 
		if (Parser.GetValue_Int(_T("MinAlarmValue"), &pConfig->m_iMinAlarmValue) == false)
			g_MonitorClientDump->Crash();

			   		 

		

		// ServerNo 
		// DataTypeCount만큼 돌면서 뽑아낸다.
		i = 0;
		while (i < pConfig->m_iDataTypeCount)
		{
			// 컬럼 이름 만들기
			TCHAR Count[3] = { 0, };
			_itot_s(i + 1, Count, 3, 10);

			TCHAR ServerNoText[20] = _T("ServerNo");

			StringCchCat(ServerNoText, 20, Count);

			if (Parser.GetValue_Int(ServerNoText, &pConfig->m_iServerNo[i]) == false)
				g_MonitorClientDump->Crash();

			++i;
		}		

		// Colum 텍스트
		// DataTypeCount만큼 돌면서 뽑아낸다.
		i = 0;
		while (i < pConfig->m_iDataTypeCount)
		{
			// 컬럼 이름 만들기
			TCHAR Count[3] = { 0, };
			_itot_s(i + 1, Count, 3, 10);

			TCHAR ColumName[20] = _T("ColumText");

			StringCchCat(ColumName, 20, Count);

			if (Parser.GetValue_String(ColumName, pConfig->m_tcColumText[i]) == false)
				g_MonitorClientDump->Crash();

			++i;
		}
		

		return true;
	}


	// -----------------------
	// 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CMonitorClient::OnConnect(ULONGLONG ClinetID)
	{
		// 세션 ID 저장해두기
		m_ullSessionID = ClinetID;

		// 로그인 패킷 보내기
		WORD Type = en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN;
		char Key[] = "P09djiwl34jWJV%@oW@#o0d82jvk#cjz";

		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData(Key, 32);

		SendPacket(ClinetID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CMonitorClient::OnDisconnect(ULONGLONG ClinetID)
	{
		m_ullSessionID = 0xffffffffffffffff;
		m_bLoginCheck = false;
	}


	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CMonitorClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		try
		{
			// 1. Type을 빼본다.
			WORD Type;
			Payload->GetData((char*)&Type, 2);

			switch (Type)
			{
				// type이 로그인 요청이라면 로그인 패킷 처리
			case en_PACKET_CS_MONITOR_TOOL_RES_LOGIN:
				Login_Packet(SessionID, Payload);
				break;

				// type이 데이터 전송이라면
			case en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE:
			{
				// 일감 TLS에서 일감 구조체 Alloc
				st_WorkNode* NowWork = m_WorkPool->Alloc();

				// Payload 레퍼런스 카운트 증가 후 일감에 연결
				Payload->Add();
				NowWork->m_pPacket = Payload;
				NowWork->SessionID = SessionID;

				// 일감 큐에 넣기
				m_LFQueue->Enqueue(NowWork);
			}
			break;

			// 그 외 타입은 에러
			default:
				throw CException(_T("OnRecv() --> Type Error!!"));
				
				break;
			}

		}
		catch (CException& exc)
		{
			g_MonitorClientLog->LogSave(L"MonitorClient", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s", 
				(TCHAR*)exc.GetExceptionText());

			g_MonitorClientDump->Crash();
		}
		
		
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CMonitorClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CMonitorClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CMonitorClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CMonitorClient::OnError(int error, const TCHAR* errorStr)
	{
		g_MonitorClientLog->LogSave(L"MonitorClient", CSystemLog::en_LogLevel::LEVEL_ERROR, errorStr);
		g_MonitorClientDump->Crash();
	}
}