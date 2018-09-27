#include "pch.h"
#include "MonitorClient.h"
#include "CrashDump\CrashDump.h"
#include "Log\Log.h"

#include "Protocol_Set\CommonProtocol.h"

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

		// ------------------- 로그 저장할 파일 셋팅
		g_MonitorClientLog->SetDirectory(L"MonitorClient");
		g_MonitorClientLog->SetLogLeve((CSystemLog::en_LogLevel)CSystemLog::en_LogLevel::LEVEL_DEBUG);

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
		// -------------------- 
		// 하드웨어
		// -------------------- 
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL-1] = new CMonitorGraphUnit(hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_SINGLE, 10, 10, 500, 130, L"서버 CPU");
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY-1] = new CMonitorGraphUnit(hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_SINGLE, 520, 10, 400, 130, L"서버 사용 가능 메모리");
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV-1] = new CMonitorGraphUnit(hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_MULTI, 930, 10, 510, 130, L"Network");
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY -1] = new CMonitorGraphUnit(hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_SINGLE, 1450, 10, 460, 130, L"Nonpaged Mem");

		// 추가 정보 세팅
		// 순서대로 [Max값, 알람 울리는 수치, 표시할 값의 단위]. 
		// Max 값이 0이면, 현재 큐의 값을 가장 큰 값으로 사용
		// 알람 값이 0이면 알람 울리지 않음.
		// 표시할 값의 단위가 L"NULL" 이면, 단위 표시하지 않음.
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL - 1]->AddData(100, 90, L"%");
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY - 1]->AddData(0, 0, L"MB");
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV - 1]->AddData(0, 0, L"KByte");
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY - 1]->AddData(0, 0, L"MB");

		// 컬럼 최초 정보 세팅
		int p1_ColumnCount = 1;
		int p1_ServerID[1] = { SERVER_HARDWARE };
		int p1_DataType[1] = { dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL };
		TCHAR p1_DataName[1][20] = { L"CPU 사용량" };

		int p2_ColumnCount = 1;
		int p2_ServerID[1] = { SERVER_HARDWARE };
		int p2_DataType[1] = { dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY };
		TCHAR p2_DataName[1][20] = { L"서버 사용 가능 메모리" };

		int p3_ColumnCount = 2;
		int p3_ServerID[2] = { SERVER_HARDWARE, SERVER_HARDWARE };
		int p3_DataType[2] = { dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV, dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND };
		TCHAR p3_DataName[2][20] = { L"Recv", L"Send" };

		int p4_ColumnCount = 1;
		int p4_ServerID[1] = { SERVER_HARDWARE };
		int p4_DataType[1] = { dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY };
		TCHAR p4_DataName[1][20] = { L"Nonpaged Mem" };

		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL - 1]->SetColumnInfo(1, p1_ServerID, p1_DataType, p1_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY - 1]->SetColumnInfo(1, p2_ServerID, p2_DataType, p2_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV - 1]->SetColumnInfo(2, p3_ServerID, p3_DataType, p3_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY - 1]->SetColumnInfo(1, p4_ServerID, p4_DataType, p4_DataName);


		// -------------------- 
		// 채팅서버
		// -------------------- 
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_SERVER_ON - 1] = new CMonitorGraphUnit(hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::ONOFF, 10, 150, 150, 130, L"채팅 서버 On/Off");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_CPU - 1] = new CMonitorGraphUnit(hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 170, 150, 200, 130, L"채팅서버 CPU");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT - 1] = new CMonitorGraphUnit(hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 380, 150, 200, 130, L"채팅서버 메모리");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL - 1] = new CMonitorGraphUnit(hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 590, 150, 200, 130, L"PacketPool");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_SESSION - 1] = new CMonitorGraphUnit(hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 1110, 150, 200, 130, L"SessionAll");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_PLAYER - 1] = new CMonitorGraphUnit(hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 1320, 150, 300, 130, L"Login Player");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_ROOM - 1] = new CMonitorGraphUnit(hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 1630, 150, 280, 130, L"Room");
				
		// 추가 정보 세팅		
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_SERVER_ON - 1]->AddData(0, 0, L"NULL");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_CPU - 1]->AddData(100, 90, L"%");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT - 1]->AddData(0, 0, L"MB");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL - 1]->AddData(0, 0, L"NULL");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_SESSION - 1]->AddData(0, 0, L"NULL");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_PLAYER - 1]->AddData(0, 0, L"NULL");
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_ROOM - 1]->AddData(0, 0, L"NULL");

		// 컬럼 최초 정보 세팅
		int p5_ColumnCount = 1;
		int p5_ServerID[1] = { SERVER_CHAT };
		int p5_DataType[1] = { dfMONITOR_DATA_TYPE_CHAT_SERVER_ON };
		TCHAR p5_DataName[1][20] = { L"채팅 서버 On/Off" };

		int p6_ColumnCount = 1;
		int p6_ServerID[1] = { SERVER_CHAT };
		int p6_DataType[1] = { dfMONITOR_DATA_TYPE_CHAT_CPU };
		TCHAR p6_DataName[1][20] = { L"채팅서버 CPU" };

		int p7_ColumnCount = 1;
		int p7_ServerID[1] = { SERVER_CHAT };
		int p7_DataType[1] = { dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT };
		TCHAR p7_DataName[1][20] = { L"채팅서버 메모리" };

		int p8_ColumnCount = 1;
		int p8_ServerID[1] = { SERVER_CHAT };
		int p8_DataType[1] = { dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL };
		TCHAR p8_DataName[1][20] = { L"PacketPool" };

		int p9_ColumnCount = 1;
		int p9_ServerID[1] = { SERVER_CHAT };
		int p9_DataType[1] = { dfMONITOR_DATA_TYPE_CHAT_SESSION };
		TCHAR p9_DataName[1][20] = { L"SessionAll" };

		int p10_ColumnCount = 1;
		int p10_ServerID[1] = { SERVER_CHAT };
		int p10_DataType[1] = { dfMONITOR_DATA_TYPE_CHAT_PLAYER };
		TCHAR p10_DataName[1][20] = { L"Login Player" };

		int p11_ColumnCount = 1;
		int p11_ServerID[1] = { SERVER_CHAT };
		int p11_DataType[1] = { dfMONITOR_DATA_TYPE_CHAT_ROOM };
		TCHAR p11_DataName[1][20] = { L"Room" };
		
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_SERVER_ON - 1]->SetColumnInfo(1, p5_ServerID, p5_DataType, p5_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_CPU - 1]->SetColumnInfo(1, p6_ServerID, p6_DataType, p6_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT - 1]->SetColumnInfo(1, p7_ServerID, p7_DataType, p7_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL - 1]->SetColumnInfo(1, p8_ServerID, p8_DataType, p8_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_SESSION - 1]->SetColumnInfo(1, p9_ServerID, p9_DataType, p9_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_PLAYER - 1]->SetColumnInfo(1, p10_ServerID, p10_DataType, p10_DataName);
		m_pMonitor[dfMONITOR_DATA_TYPE_CHAT_ROOM - 1]->SetColumnInfo(1, p11_ServerID, p11_DataType, p11_DataName);
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
			int Value = 0;

			// value가 -1이 아니라면, 데이터가 온것. 데이터 셋팅
			if (m_LastData[DataIndex].m_Value != -1)
			{
				Value = m_LastData[DataIndex].m_Value;						
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

			// 인덱스 ++
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
		TCHAR IP[20] = L"127.0.0.1";
		WORD Port = 18245;
		int CreateWorker = 1;
		int ActiveWorker = 1;
		int Nodelay = 0;
		BYTE HeadCode = 119;
		BYTE XORCode1 = 50;
		BYTE XORCode2 = 132;

		if (Start(IP, Port, CreateWorker, ActiveWorker, Nodelay, HeadCode, XORCode1, XORCode2) == false)
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

		// 2. 데이터 보관
		// 만약, 이미 0으로 출력한 데이터가 뒤늦게 왔다면 무시한다.
		if (LastData[DataType].m_ZeroCount > 0)
		{
			// 카운트 하나 줄이고 끝.
			LastData[DataType].m_ZeroCount--;
		}

		// 0으로 출력한 데이터가 없다면, 정말 이번에 찍을 데이터가 온 것.
		else if (LastData[DataType].m_ZeroCount == 0)
		{
			LastData[DataType].m_ServerNo = ServerNo;
			LastData[DataType].m_Value = DataValue;			
		}




		///////////////////////////////////////////////////////////
		//// 전달할 데이터 세팅
		///////////////////////////////////////////////////////////
		//int SendVal[12] = { 0, };

		//int a[12];

		//// 하드웨어의 CPU 사용량
		//a[0] = rand() % 5;

		//// 하드웨어의 사용 가능 메모리
		//a[1] = rand() % 80;

		//// 하드웨어의 Network Recv
		//a[2] = rand() % 40;

		//// 하드웨어의 Network Send
		//a[3] = rand() % 40;

		//// 하드웨어의 Nonpaged_Mem
		//a[4] = rand() % 38;

		//// 채팅서버 On/Off
		//a[5] = rand() % 2;

		//// 채팅서버 CPU
		//a[6] = rand() % 5;

		//// 채팅서버 메모리
		//a[7] = rand() % 10;

		//// PacketPool
		//a[8] = rand() % 100;

		//// SessionAll
		//a[9] = rand() % 100;

		//// Login Player
		//a[10] = rand() % 50;

		//// Room
		//a[11] = rand() % 3;


		///////////////////////////////////////////////////////////
		//// 전달할 데이터 가공
		///////////////////////////////////////////////////////////
		//// 첫 번째 그래프는 %로 표시하기 때문에(임시로) 100 이상을 넘지 않도록 강제한다.
		//// 전달할 값을, +할지 -할지 결정한다.
		//int randCheck = rand() % 2;

		//if (randCheck == 0)	// 0이면 뺸다.
		//{
		//	if (0 > (SendVal[0] - a[0]))	// 빼야하는데 뺸 후의 값이 -라면 더한다.
		//		SendVal[0] += a[0];
		//	else
		//		SendVal[0] -= a[0];
		//}
		//else   // 1이면 더한다.
		//{
		//	if (100 < (SendVal[0] + a[0]))	// 더해야 하는데, 더한 후의 값이 100을 넘어가면 뺀다.
		//		SendVal[0] -= a[0];
		//	else
		//		SendVal[0] += a[0];
		//}

		//int MonitorIndex = 0;
		//int End = dfMONITOR_DATA_TYPE_END - 1;
		//int DataType = 1;

		//while (MonitorIndex < End)
		//{
		//	if (m_pMonitor[MonitorIndex] != nullptr)
		//	{
		//		m_pMonitor[MonitorIndex]->InsertData(SendVal[0], SERVER_1, DataType);
		//	}
		//	++MonitorIndex;
		//}

		//DataType++;

		//// 2~10번 정보들 처리
		//for (int i = 1; i < 12; ++i)
		//{
		//	// 전달할 값을, +할지 -할지 결정한다.
		//	randCheck = rand() % 2;

		//	if (randCheck == 0)	// 0이면 뺸다.
		//	{
		//		if (0 > (SendVal[i] - a[i]))	// 빼야하는데 뺸 값이 -라면 더한다.
		//			SendVal[i] += a[i];
		//		else
		//			SendVal[i] -= a[i];
		//	}
		//	else   // 1이면 더한다.
		//	{
		//		SendVal[i] += a[i];
		//	}

		//	MonitorIndex = 0;

		//	while (MonitorIndex < End)
		//	{
		//		if (m_pMonitor[MonitorIndex] != nullptr)
		//		{
		//			m_pMonitor[MonitorIndex]->InsertData(SendVal[i], SERVER_1, DataType);
		//		}
		//		++MonitorIndex;
		//	}

		//	DataType++;
		//	if (DataType == 6)
		//		DataType += 30;
		//}

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

			// 그 외 타입은 크래시
		default:
			g_MonitorClientDump->Crash();
			break;
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
	{}
}