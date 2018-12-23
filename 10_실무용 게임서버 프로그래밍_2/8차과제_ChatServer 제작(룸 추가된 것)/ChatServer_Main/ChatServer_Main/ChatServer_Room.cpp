#include "pch.h"
#include "ChatServer_Room.h"
#include "Protocol_Set/CommonProtocol_2.h"
#include "CrashDump/CrashDump.h"
#include "Log/Log.h"
#include "Parser/Parser_Class.h"
#include "CPUUsage/CPUUsage.h"
#include "PDHClass/PDHCheck.h"

#include <strsafe.h>


// ------------------------
//
// Chat_Net서버
//
// ------------------------
namespace Library_Jingyu
{

	// 직렬화 버퍼 1개의 크기
	// 각 서버에 전역 변수로 존재해야 함.
	LONG g_lNET_BUFF_SIZE = 512;

	// 덤프용 
	CCrashDump* g_ChatDump = CCrashDump::GetInstance();
	CSystemLog* g_ChatLog = CSystemLog::GetInstance();


	// ----------------------------
	// 플레이어 관리 자료구조 함수
	// ----------------------------

	// 플레이어 관리 자료구조에 Insert
	//
	// Parameter : SessionID, stPlayer*
	// return : 정상 추가 시 true
	//		  : 키 중복 시 false
	bool CChatServer_Room::InsertPlayerFunc(ULONGLONG SessionID, stPlayer* InsertPlayer)
	{
		AcquireSRWLockExclusive(&m_Player_Umap_srwl);	 // ----- Player Umap Exclusive 락

		// 1. 추가
		auto ret = m_Player_Umap.insert(make_pair(SessionID, InsertPlayer));

		// 중복키면 false 리턴
		if (ret.second == false)
		{
			ReleaseSRWLockExclusive(&m_Player_Umap_srwl);	 // ----- Player Umap Exclusive 언락
			return false;
		}


		ReleaseSRWLockExclusive(&m_Player_Umap_srwl);	 // ----- Player Umap Exclusive 언락

		return true;
	}

	// 플레이어 관리 자료구조에서 검색
	//
	// Parameter : SessionID
	// return :  잘 찾으면 stPlayer*
	//		  :  없는 유저일 시 nullptr	
	CChatServer_Room::stPlayer* CChatServer_Room::FindPlayerFunc(ULONGLONG SessionID)
	{
		AcquireSRWLockShared(&m_Player_Umap_srwl);	 // ----- Player Umap Shared 락

		// 1. 검색
		auto FindPlayer = m_Player_Umap.find(SessionID);

		// 없는 유저라면 false 리턴
		if (FindPlayer == m_Player_Umap.end())
		{
			ReleaseSRWLockShared(&m_Player_Umap_srwl);	 // ----- Player Umap Shared 언락
			return false;
		}

		// 2. 있다면 받아둔다.
		stPlayer* RetPlayer = FindPlayer->second;

		ReleaseSRWLockShared(&m_Player_Umap_srwl);	 // ----- Player Umap Shared 언락

		// 3. 리턴
		return RetPlayer;
	}

	// 플레이어 관리 자료구조에서 제거
	//
	// Parameter : SessionID
	// return :  Erase 후 Second(stPlayer*)
	//		  : 없는 유저일 시 nullptr
	CChatServer_Room::stPlayer* CChatServer_Room::ErasePlayerFunc(ULONGLONG SessionID)
	{
		AcquireSRWLockExclusive(&m_Player_Umap_srwl);	 // ----- Player Umap Exclusive 락

		// 1. 검색
		auto FindPlayer = m_Player_Umap.find(SessionID);

		// 못찾았으면 nullptr 리턴
		if (FindPlayer == m_Player_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_Player_Umap_srwl);	 // ----- Player Umap Exclusive 언락
			return nullptr;
		}

		// 2. 찾았으면 리턴 값 받아두고 Erase
		stPlayer* RetPlayer = FindPlayer->second;

		m_Player_Umap.erase(FindPlayer);


		ReleaseSRWLockExclusive(&m_Player_Umap_srwl);	 // ----- Player Umap Exclusive 언락

		return RetPlayer;
	}





	// ----------------------------
	// 플레이어 관리 자료구조 함수
	// ----------------------------

	// 로그인 한 플레이어 관리 자료구조에 Insert
	//
	// Parameter : AccountNo, stPlayer*
	// return : 정상 추가 시 true
	//		  : 키 중복 시 flase
	bool CChatServer_Room::InsertLoginPlayerFunc(INT64 AccountNo, stPlayer* InsertPlayer)
	{
		AcquireSRWLockExclusive(&m_LoginPlayer_Umap_srwl);	 // ----- 로그인 Player Umap Exclusive 락

		// 1. 추가
		auto ret = m_LoginPlayer_Umap.insert(make_pair(AccountNo, InsertPlayer));

		// 중복키면 false 리턴
		if (ret.second == false)
		{
			ReleaseSRWLockExclusive(&m_LoginPlayer_Umap_srwl);	 // ----- 로그인 Player Umap Exclusive 언락
			return false;
		}

		ReleaseSRWLockExclusive(&m_LoginPlayer_Umap_srwl);	 // ----- 로그인 Player Umap Exclusive 언락

		return true;

	}

	// 로그인 한 플레이어 관리 자료구조에서 제거
	//
	// Parameter : AccountNo
	// return : 정상적으로 Erase 시 true
	//		  : 유저  검색 실패 시 false
	bool CChatServer_Room::EraseLoginPlayerFunc(ULONGLONG AccountNo)
	{
		AcquireSRWLockExclusive(&m_LoginPlayer_Umap_srwl);	 // ----- 로그인 Player Umap Exclusive 락

		// 1. 검색
		auto FindPlayer = m_LoginPlayer_Umap.find(AccountNo);

		// 못찾았으면 nullptr 리턴
		if (FindPlayer == m_LoginPlayer_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_LoginPlayer_Umap_srwl);	 // ----- 로그인 Player Umap Exclusive 언락
			return false;
		}

		// 2. 찾았으면 Erase
		m_LoginPlayer_Umap.erase(FindPlayer);

		ReleaseSRWLockExclusive(&m_LoginPlayer_Umap_srwl);	 // ----- 로그인 Player Umap Exclusive 언락

		return true;
	}




	// ----------------------------
	// 룸 관리 자료구조 함수
	// ----------------------------

	// 룸 자료구조에 Insert
	//
	// Parameter : RoonNo, stROom*
	// return : 정상 추가 시 true
	//		  : 키 중복 시 flase
	bool CChatServer_Room::InsertRoomFunc(int RoomNo, stRoom* InsertRoom)
	{
		AcquireSRWLockExclusive(&m_Room_Umap_srwl);	// ----- 룸 Exclusive 락

		// 1. 추가
		auto ret = m_Room_Umap.insert(make_pair(RoomNo, InsertRoom));

		if (ret.second == false)
		{
			ReleaseSRWLockExclusive(&m_Room_Umap_srwl);	// ----- 룸 Exclusive 언락
			return false;
		}

		ReleaseSRWLockExclusive(&m_Room_Umap_srwl);	// ----- 룸 Exclusive 언락

		return true;
	}
	
	// 룸 자료구조에서 Erase
	//
	// Parameter : RoonNo
	// return : 정상적으로 제거 시 stRoom*
	//		  : 검색 실패 시 nullptr
	CChatServer_Room::stRoom* CChatServer_Room::EraseRoomFunc(int RoomNo)
	{
		AcquireSRWLockExclusive(&m_Room_Umap_srwl);		// ----- 룸 Exclusive 락

		// 1. 검색
		auto FindRoom = m_Room_Umap.find(RoomNo);

		if (FindRoom == m_Room_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_Room_Umap_srwl);		// ----- 룸 Exclusive 언락
			return nullptr;
		}

		// 2. 잘 찾았으면 Erase
		stRoom* RetRoom = FindRoom->second;

		m_Room_Umap.erase(FindRoom);

		ReleaseSRWLockExclusive(&m_Room_Umap_srwl);		// ----- 룸 Exclusive 언락

		return RetRoom;
	}

	// 룸 자료구조의 모든 방 삭제
	//
	// Parameter : 없음
	// return : 없음
	void CChatServer_Room::RoomClearFunc()
	{
		AcquireSRWLockExclusive(&m_Room_Umap_srwl);		// ----- 룸 Exclusive 락		

		// 룸이 있을 경우 룸 클리어 진행
		if (m_Room_Umap.size() > 0)
		{
			auto itor_Now = m_Room_Umap.begin();
			auto itor_End = m_Room_Umap.end();

			while (itor_Now != itor_End)
			{
				// 해당 방에 유저가 있는 경우, 방 안의 모든 유저에게 셧다운 날림.
				// 이 방은, OnClientLeave에서 방 파괴
				if (itor_Now->second->m_iJoinUser > 0)
				{
					stRoom* NowRoom = itor_Now->second;

					NowRoom->m_bDeleteFlag = true;

					size_t Size = NowRoom->m_JoinUser_vector.size();

					if (Size != NowRoom->m_iJoinUser)
						g_ChatDump->Crash();

					size_t Index = 0;
					while (Index < Size)
					{
						// 셧다운
						Disconnect(NowRoom->m_JoinUser_vector[Index]);
						++Index;
					}

					++itor_Now;
				}

				// 방에 유저가 없는 경우, 즉시 삭제
				else
				{
					m_pRoom_Pool->Free(itor_Now->second);

					InterlockedDecrement(&m_lRoomCount);

					itor_Now = m_Room_Umap.erase(itor_Now);
				}
			}
		}

		ReleaseSRWLockExclusive(&m_Room_Umap_srwl);		// ----- 룸 Exclusive 언락
	}





	// --------------
	// 외부에서 호출 가능한 함수
	// --------------

	// 출력용 함수
	void  CChatServer_Room::ShowPrintf()
	{
		// 해당 프로세스의 사용량 체크할 클래스
		static CCpuUsage_Process ProcessUsage;

		// 화면 출력할 것 셋팅
		/*
		MonitorConnect : %d, BattleConnect : %d	- 모니터링 서버에 접속 여부, 배틀 랜 서버에 접속 여부. 1이면 접속함.
		SessionNum : 	- NetServer 의 세션수
		PacketPool_Net : 	- 외부에서 사용 중인 Net 직렬화 버퍼의 수

		PlayerData_Pool :	- Player 구조체 할당량
		Player Count : 		- Contents 파트 Player 개수

		Accept Total :		- Accept 전체 카운트 (accept 리턴시 +1)
		Accept TPS :		- Accept 처리 횟수
		Send TPS			- 초당 Send완료 횟수. (완료통지에서 증가)		

		Net_BuffChunkAlloc_Count : - Net 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		Chat_PlayerChunkAlloc_Count : - 플레이어 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)

		TotalRoom_Pool :	- 방 Umap에 Size
		TotalRoom :			- 방 수
		Room_ChunkAlloc_Count : - 룸 총 Alloc한 청크 수(외부에서 사용 청크 수)

		Server_EnterToken_Miss : 		- 채팅서버 입장 토큰 미스
		Room_EnterTokenNot_Miss : 	- 방 입장토큰 미스

		----------------------------------------------------
		PacketPool_Lan : 	- 외부에서 사용 중인 Lan 직렬화 버퍼의 수

		Lan_BuffChunkAlloc_Count : - Lan 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)

		----------------------------------------------------
		CPU usage [ChatServer:%.1f%% U:%.1f%% K:%.1f%%] - 프로세스 사용량.

		*/

		// 출력 전에, 프로세스 사용량 갱신
		ProcessUsage.UpdateCpuTime();

		printf("==================== Chat Server =====================\n"
			"MonitorConnect : %d, BattleConnect : %d\n"
			"SessionNum : %lld\n"
			"PacketPool_Net : %d\n\n"

			"PlayerData_Pool : %d\n"
			"Player Count : %lld\n\n"

			"Accept Total : %lld\n"
			"Accept TPS : %d\n"
			"Send TPS : %d\n\n"

			"Net_BuffChunkAlloc_Count : %d (Out : %d)\n"
			"Chat_PlayerChunkAlloc_Count : %d (Out : %d)\n\n"

			"Server_EnterToken_Miss : %d\n"
			"Room_EnterTokenNot_Miss : %d\n\n"

			"------------------------------------------------\n"
			"TotalRoom_Pool : %lld\n"
			"TotalRoom : %d\n"
			"Room_ChunkAlloc_Count : %d (Out : %d)\n\n"

			"------------------------------------------------\n"
			"PacketPool_Lan : %d\n\n"

			"Lan_BuffChunkAlloc_Count : %d (Out : %d)\n\n"

			"========================================================\n\n"
			"CPU usage [ChatServer:%.1f%% U:%.1f%% K:%.1f%%]\n",

			// ----------- 채팅 서버용
			m_pMonitor_Client->GetClinetState(), m_pBattle_Client->GetClinetState(),
			GetClientCount(),
			CProtocolBuff_Net::GetNodeCount(),

			m_lUpdateStruct_PlayerCount,
			m_Player_Umap.size(),

			GetAcceptTotal(),
			GetAccpetTPS(),
			GetSendTPS(),

			CProtocolBuff_Net::GetChunkCount(), CProtocolBuff_Net::GetOutChunkCount(),
			m_pPlayer_Pool->GetAllocChunkCount(), m_pPlayer_Pool->GetOutChunkCount(),

			m_lEnterTokenMiss,
			m_lRoom_EnterTokenMiss,

			m_Room_Umap.size(),
			m_lRoomCount,
			m_pRoom_Pool->GetAllocChunkCount(), m_pRoom_Pool->GetOutChunkCount(),

			// ----------- 배틀 랜 클라이언트용
			CProtocolBuff_Lan::GetNodeCount(),
			CProtocolBuff_Lan::GetChunkCount(), CProtocolBuff_Lan::GetOutChunkCount(),

			// ----------- 프로세스 사용량 
			ProcessUsage.ProcessTotal(), ProcessUsage.ProcessUser(), ProcessUsage.ProcessKernel());

	}
	   	  
	// 서버 시작
	//
	// Parameter : 없음
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CChatServer_Room::ServerStart()
	{
		// 변수 초기화
		m_lChatLoginCount = 0;
		m_lUpdateStruct_PlayerCount = 0;
		m_lEnterTokenMiss = 0;
		m_lRoom_EnterTokenMiss = 0;
		m_lRoomCount = 0;


		// 모니터링 서버와 연결되는 랜 클라 시작
		if (m_pMonitor_Client->ClientStart(m_Paser.MonitorServerIP, m_Paser.MonitorServerPort, m_Paser.MonitorClientCreateWorker,
			m_Paser.MonitorClientActiveWorker, m_Paser.MonitorClientNodelay) == false)
		{
			return false;
		}

		// 배틀과 연결되는 랜 클라 시작
		if (m_pBattle_Client->ClientStart(m_Paser.BattleServerIP, m_Paser.BattleServerPort, m_Paser.BattleClientCreateWorker,
			m_Paser.BattleClientActiveWorker, m_Paser.BattleClientNodelay) == false)
		{
			return false;
		}

		// 채팅 Net 서버 시작
		if (Start(m_Paser.BindIP, m_Paser.Port, m_Paser.CreateWorker, m_Paser.ActiveWorker, m_Paser.ActiveWorker,
			m_Paser.Nodelay, m_Paser.MaxJoinUser, m_Paser.HeadCode, m_Paser.XORCode) == false)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"Net ServerOpenError");
			
			return false;
		}	
		
		// 서버 오픈 로그 찍기 (로그 레벨 : 에러)
		g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// 서버 종료
	//
	// Parameter : 없음
	// return : 없음
	void CChatServer_Room::ServerStop()
	{
		// 모니터링 서버와 연결되는 랜 클라 종료

		// 배틀과 연결되는 랜 클라 종료
		m_pBattle_Client->Stop();

		// 채팅 Net 서버 종료
		Stop();

		// 서버 닫힘 로그 찍기 (로그 레벨 : 에러)
		g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"All Server Stop...");
	}






	// ------------------
	// 내부에서만 사용하는 함수
	// ------------------

	// 방 안의 모든 유저에게 인자로 받은 패킷 보내기.
	//
	// Parameter : stRoom* CProtocolBuff_Net*
	// return : 자료구조에 0명이면 false. 그 외에는 true
	bool CChatServer_Room::Room_BroadCast(stRoom* NowRoom, CProtocolBuff_Net* SendBuff)
	{
		// 자료구조 사이즈가 0이면 false
		size_t Size = NowRoom->m_JoinUser_vector.size();
		if (Size == 0)
			return false;

		// 직렬화 버퍼 레퍼런스 카운트 증가
		SendBuff->Add((int)Size);

		// 처음부터 순회하며 보낸다.
		size_t Index = 0;

		while (Index < Size)
		{
			SendPacket(NowRoom->m_JoinUser_vector[Index], SendBuff);

			++Index;
		}

		return true;
	}

	// Config 셋팅
	//
	// Parameter : stParser*
	// return : 실패 시 false
	bool CChatServer_Room::SetFile(stParser* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("ChatServer_Config.ini"));
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
		// 채팅 넷 서버의 config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CHATSERVER")) == false)
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

		// xorcode
		if (Parser.GetValue_Int(_T("XorCode"), &pConfig->XORCode) == false)
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
		// config 읽어오기
		////////////////////////////////////////////////////////
			   		 
		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CONFIG")) == false)
			return false;

		// 외부 IP
		if (Parser.GetValue_String(_T("ChatIP"), pConfig->ChatIP) == false)
			return false;




		////////////////////////////////////////////////////////
		// 채팅 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CHATLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("BattleServerIP"), pConfig->BattleServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("BattleServerPort"), &pConfig->BattleServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("ClientCreateWorker"), &pConfig->BattleClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ClientActiveWorker"), &pConfig->BattleClientActiveWorker) == false)
			return false;


		// Nodelay
		if (Parser.GetValue_Int(_T("ClientNodelay"), &pConfig->BattleClientNodelay) == false)
			return false;




		////////////////////////////////////////////////////////
		// 모니터링 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("MonitorServerIP"), pConfig->MonitorServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("MonitorServerPort"), &pConfig->MonitorServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientCreateWorker"), &pConfig->MonitorClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientActiveWorker"), &pConfig->MonitorClientActiveWorker) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("MonitorClientNodelay"), &pConfig->MonitorClientNodelay) == false)
			return false;


		return true;
	}





	// ------------------
	// 패킷 처리 함수
	// ------------------

	// 로그인 패킷
	//
	// Parameter : SessionID, CProtocolBuff_Net*
	// return : 없음
	void CChatServer_Room::Packet_Login(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1. 플레이어 검색
		stPlayer* NowPlayer = FindPlayerFunc(SessionID);

		// 없으면 크래시
		if (NowPlayer == nullptr)
			g_ChatDump->Crash();

		// 2. 마샬링
		INT64	AccountNo;
		char	ConnectToken[32];

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)NowPlayer->m_tcID, 40);
		Packet->GetData((char*)NowPlayer->m_tcNick, 40);
		Packet->GetData(ConnectToken, 32);


		// 3. 로그인 상태 체크. 
		// 이미 로그인중이면 로그인 패킷을 또보낸것? 혹은 로직 실수..
		if (NowPlayer->m_bLoginCheck == true)
		{
			g_ChatDump->Crash();

			/*
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_Login(). LoginFlag True. SessionID : %lld, AccountNo : %lld"), 
				SessionID, AccountNo);

			throw CException(str);	
			*/
		}		



		// 4. "현재" 토큰 체크
		if (memcmp(m_cConnectToken_Now, ConnectToken, 32) != 0)
		{
			// 다르면 "이전" 패킷과 비교
			if (memcmp(m_cConnectToken_Before, ConnectToken, 32) != 0)
			{
				// 그래도 다르면, 실패 패킷
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_CHAT_RES_LOGIN;
				BYTE Status = 0;

				SendBuff->PutData((char*)&Type, 2);
				SendBuff->PutData((char*)&Status, 1);
				SendBuff->PutData((char*)&AccountNo, 8);

				SendPacket(SessionID, SendBuff);

				return;
			}
		}
		
			

		// 5. AccountNo 복사
		NowPlayer->m_i64AccountNo = AccountNo;



		// 6. 로그인 플레이어 관리용 자료구조에 추가 (중복 로그인 체크)
		if (InsertLoginPlayerFunc(AccountNo, NowPlayer) == false)
		{
			// 실패 패킷 -------
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_CHAT_RES_LOGIN;
			BYTE Status = 0;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&Status, 1);
			SendBuff->PutData((char*)&AccountNo, 8);

			SendPacket(SessionID, SendBuff);


			// 기존에 접속중이던 유저 접속 종료	-------

			AcquireSRWLockShared(&m_LoginPlayer_Umap_srwl);	// ----- 로그인 Player Shared 락

			// 1) 검색
			auto FindPlayer = m_LoginPlayer_Umap.find(SessionID);

			// 2) 없을 수 있음. 이 로직을 처리하는데 종료됐을 가능성
			if (FindPlayer == m_LoginPlayer_Umap.end())
			{
				ReleaseSRWLockShared(&m_LoginPlayer_Umap_srwl);	// ----- 로그인 Player Shared 언락
				return;
			}

			// 3) 있으면 접속 종료 요청
			Disconnect(FindPlayer->second->m_ullSessionID);

			ReleaseSRWLockShared(&m_LoginPlayer_Umap_srwl);	// ----- 로그인 Player Shared 언락

			return;
		}

		// 7. 잘 추가됐으면 플래그 변경.
		NowPlayer->m_bLoginCheck = true;

		// 로그인 패킷까지 처리된 유저 카운트 1 증가
		InterlockedIncrement(&m_lChatLoginCount);


		// 8. 정상 응답 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_CHAT_RES_LOGIN;
		BYTE Status = 1;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&Status, 1);
		SendBuff->PutData((char*)&AccountNo, 8);

		SendPacket(SessionID, SendBuff);
	}

	// 방 입장 
	//
	// Parameter : SessionID, CProtocolBuff_Net*
	// return : 없음
	void CChatServer_Room::Packet_Room_Enter(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1. 플레이어 검색
		stPlayer* NowPlayer = FindPlayerFunc(SessionID);

		// 없으면 크래시
		if (NowPlayer == nullptr)
			g_ChatDump->Crash();


		// 2. 마샬링
		INT64	AccountNo;
		int		RoomNo;
		char	EnterToken[32];

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)&RoomNo, 4);
		Packet->GetData(EnterToken, 32);


		// 3. 로그인 상태 체크. 
		// 로그인 중이 아니면 Crash
		if (NowPlayer->m_bLoginCheck == false)
		{
			g_ChatDump->Crash();

			/*
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_Room_Enter(). LoginFlag True. SessionID : %lld, AccountNo : %lld"),
				SessionID, AccountNo);

			throw CException(str);
			*/
		}
		


		// 4. AccountNo 검증
		if (AccountNo != NowPlayer->m_i64AccountNo)
		{
			g_ChatDump->Crash();

			/*
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_Room_Enter(). AccountNo Error. SessionID : %lld, AccountNo : %lld"), 
				SessionID, AccountNo);

			throw CException(str);
			*/
		}

		// 5. 룸 검색
		AcquireSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 락

		auto FindRoom = m_Room_Umap.find(RoomNo);

		if (FindRoom == m_Room_Umap.end())
		{
			ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락

			// 방이 없을 수 있음. 실패 패킷 보냄
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_CHAT_RES_ENTER_ROOM;
			BYTE Status = 3;	// 방 없음

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendBuff);
			return;
		}

		stRoom* NowRoom = FindRoom->second;


		// 6. 룸 토큰 검사 및 자료구조에 추가
		NowRoom->ROOM_LOCK();	// ----- 룸 락	

		// 룸 토큰 검사
		if (memcmp(EnterToken, NowRoom->m_cEnterToken, 32) != 0)
		{
			NowRoom->ROOM_UNLOCK();							// ----- 룸 언락	
			ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락
			

			// 토큰	불일치 패킷
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_CHAT_RES_ENTER_ROOM;
			BYTE Status = 2;	// 토큰 다름

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&Status, 1);

			SendPacket(SessionID, SendBuff);
			return;
		}

		// 룸 내 자료구조에 유저 추가
		NowRoom->Insert(SessionID);

		// 룸 내 유저 수 증가
		++NowRoom->m_iJoinUser;

		NowRoom->ROOM_UNLOCK();	// ----- 룸 언락	

		ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락

		// 유저에게 룸 번호 할당
		NowPlayer->m_iRoomNo = RoomNo;

		// 7. 성공 패킷 응답
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_CHAT_RES_ENTER_ROOM;
		BYTE Status = 1;	// 성공

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&Status, 1);

		SendPacket(SessionID, SendBuff);
	}

	// 채팅 보내기
	//
	// Parameter : SessionID, CProtocolBuff_Net*
	// return : 없음
	void CChatServer_Room::Packet_Message(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1. 플레이어 검색
		stPlayer* NowPlayer = FindPlayerFunc(SessionID);

		// 없으면 크래시
		if (NowPlayer == nullptr)
			g_ChatDump->Crash();



		// 2. 마샬링
		INT64	AccountNo;
		WORD	MessageLen;
		WCHAR	Message[512];		// null 미포함

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)&MessageLen, 2);
		Packet->GetData((char*)Message, MessageLen);
		


		// 3. 로그인 상태 체크. 
		// 로그인 중이 아니면 Crash
		if (NowPlayer->m_bLoginCheck == false)
		{
			g_ChatDump->Crash();

			/*
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_Message(). LoginFlag True. SessionID : %lld, AccountNo : %lld"),
				SessionID, AccountNo);

			throw CException(str);
			*/
		}


		// 4. AccountNo 검증
		if (AccountNo != NowPlayer->m_i64AccountNo)
		{
			g_ChatDump->Crash();

			/*
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_Message(). AccountNo Error. SessionID : %lld, AccountNo : %lld"),
				SessionID, AccountNo);

			throw CException(str);
			*/
		}		


		// 5.  채팅 응답 패킷 만들기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_CHAT_RES_MESSAGE;

		SendBuff->PutData((char*)&Type, 2);

		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)NowPlayer->m_tcID, 40);
		SendBuff->PutData((char*)NowPlayer->m_tcNick, 40);

		SendBuff->PutData((char*)&MessageLen, 2);
		SendBuff->PutData((char*)Message, MessageLen);
			   	


		// 6. 룸 검색
		int RoomNo = NowPlayer->m_iRoomNo;
		AcquireSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 락

		auto FindRoom = m_Room_Umap.find(RoomNo);

		// 방 없는건 말도 안됨. Crash
		if (FindRoom == m_Room_Umap.end())
			g_ChatDump->Crash();

		stRoom* NowRoom = FindRoom->second;


		// 7. 룸 안의 모든 유저에게 채팅 응답 패킷 BroadCast	
		// 자기 자신 포함

		NowRoom->ROOM_LOCK();	// ----- 룸 락			

		// BoradCast		
		if(Room_BroadCast(NowRoom, SendBuff) == false)
			g_ChatDump->Crash();

		NowRoom->ROOM_UNLOCK();	// ----- 룸 언락	

		ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락

		// BroadCast했던 패킷 Free
		// Room_BroadCast() 함수에서, 유저 수 만큼 래퍼런스 카운트를 증가시켰기 때문에
		// 여기서 1 감소시켜줘야 한다 
		CProtocolBuff_Net::Free(SendBuff);
	}




	// -----------------------
	// 가상함수
	// -----------------------

	//Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CChatServer_Room::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CChatServer_Room::OnClientJoin(ULONGLONG SessionID)
	{
		// 1. stPlayer Alloc
		stPlayer* NewPlayer = m_pPlayer_Pool->Alloc();
		InterlockedIncrement(&m_lUpdateStruct_PlayerCount);

		// 2. 초기 셋팅
		NewPlayer->m_ullSessionID = SessionID;

		// 3. 자료구조에 추가
		InsertPlayerFunc(SessionID, NewPlayer);
	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CChatServer_Room::OnClientLeave(ULONGLONG SessionID)
	{
		// 1. 자료구조에서 제거
		stPlayer* DeletePlayer = ErasePlayerFunc(SessionID);

		// 없으면 Crash
		if (DeletePlayer == nullptr)
			g_ChatDump->Crash();

		int RoomNo = DeletePlayer->m_iRoomNo;


		// 2. 로그인 상태일 경우
		if (DeletePlayer->m_bLoginCheck == true)
		{
			// 로그인 유저 카운트 감소
			InterlockedDecrement(&m_lChatLoginCount);

			// 미리 초기화
			DeletePlayer->m_bLoginCheck = false;

			// 로그인 자료구조에서 제거
			if(EraseLoginPlayerFunc(DeletePlayer->m_i64AccountNo) == false)
				g_ChatDump->Crash();
		}		


		// 3. 룸에 들어가 있다면, 룸에서 제거
		if (RoomNo != -1)
		{
			int DeleteRoomNo = -1;

			// 룸 번호 초기화
			DeletePlayer->m_iRoomNo = -1;

			AcquireSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 락

			// 룸 검색
			auto FindRoom = m_Room_Umap.find(RoomNo);
			if (FindRoom == m_Room_Umap.end())
				g_ChatDump->Crash();

			stRoom* NowRoom = FindRoom->second;

			NowRoom->ROOM_LOCK();	// ----- 룸 락

			// 룸 내 자료구조에서 유저 제거
			if (NowRoom->Erase(SessionID) == false)
				g_ChatDump->Crash();

			// 룸의 유저 수 감소
			--NowRoom->m_iJoinUser;

			// 룸의 유저 수가 0명이고, 삭제플래그가 true일 경우, 삭제될 방
			if (NowRoom->m_iJoinUser == 0 && NowRoom->m_bDeleteFlag == true)
				DeleteRoomNo = RoomNo;

			NowRoom->ROOM_UNLOCK();	// ----- 룸 언락

			ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락


			// 이번에 유저가 빠진 방이, 삭제될 방이라면 Erase
			if (DeleteRoomNo != -1)
			{
				// Erase
				stRoom* DeleteRoom = EraseRoomFunc(DeleteRoomNo);

				if (DeleteRoom == nullptr)
					g_ChatDump->Crash();

				InterlockedDecrement(&m_lRoomCount);

				// 룸 Free
				m_pRoom_Pool->Free(DeleteRoom);				
			}
		}

		// 4. stPlayer* Free
		m_pPlayer_Pool->Free(DeletePlayer);

		InterlockedDecrement(&m_lUpdateStruct_PlayerCount);
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CChatServer_Room::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		// 타입 마샬링
		WORD Type;
		Payload->GetData((char*)&Type, 2);


		// 타입에 따라 분기 처리
		try
		{
			switch (Type)
			{
				// 채팅 보내기
			case en_PACKET_CS_CHAT_REQ_MESSAGE:
				Packet_Message(SessionID, Payload);
				break;


				// 로그인 요청
			case en_PACKET_CS_CHAT_REQ_LOGIN:
				Packet_Login(SessionID, Payload);
				break;


				// 방 입장 요청
			case en_PACKET_CS_CHAT_REQ_ENTER_ROOM:
				Packet_Room_Enter(SessionID, Payload);
				break;


				// 하트비트
			case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
				break;


				// 없는 타입이면 크래시
			default:
				TCHAR str[100];
				StringCchPrintf(str, 100, _T("OnRecv(). TypeError. SessionID : %lld, Type : %d"), SessionID, Type);

				throw CException(str);
			}
		}
		catch (CException& exc) 
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			g_ChatDump->Crash();

			// 접속 끊기 요청
			//Disconnect(SessionID);
		}

	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CChatServer_Room::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{

	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CChatServer_Room::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CChatServer_Room::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CChatServer_Room::OnError(int error, const TCHAR* errorStr)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
			errorStr, error);

		g_ChatDump->Crash();
	}






	// --------------
	// 생성자와 소멸자
	// --------------

	// 생성자
	CChatServer_Room::CChatServer_Room()
		: CNetServer()
	{
		// config
		if (SetFile(&m_Paser) == false)
			g_ChatDump->Crash();

		// 로그
		g_ChatLog->SetDirectory(L"ChatServer");
		g_ChatLog->SetLogLeve((CSystemLog::en_LogLevel)m_Paser.LogLevel);


		// 모니터 랜 클라 동적할당
		m_pMonitor_Client = new CChat_MonitorClient;
		m_pMonitor_Client->SetNetServer(this);

		// 배틀 랜클라 동적할당
		m_pBattle_Client = new CChat_LanClient;
		m_pBattle_Client->SetNetServer(this);
		
		// 자료구조 공간 미리 잡아두기
		m_Player_Umap.reserve(5000);
		m_LoginPlayer_Umap.reserve(5000);

		// 락 초기화
		InitializeSRWLock(&m_Player_Umap_srwl);
		InitializeSRWLock(&m_LoginPlayer_Umap_srwl);

		// TLS Pool 동적할당
		m_pPlayer_Pool = new CMemoryPoolTLS<stPlayer>(0, false);
		m_pRoom_Pool = new CMemoryPoolTLS<stRoom>(0, false);
	}

	// 소멸자
	CChatServer_Room::~CChatServer_Room()
	{
		// TLS Pool 동적해제
		delete m_pPlayer_Pool;
		delete m_pRoom_Pool;

		// 랜 클라 동적해제
		delete m_pMonitor_Client;
		delete m_pBattle_Client;
	}
}


// -----------------------
//
// 배틀서버와 연결되는 Lan 클라
//
// -----------------------
namespace Library_Jingyu
{
	// ----------------------
	// 내부에서만 사용하는 함수
	// ----------------------

	// 채팅 넷서버 셋팅
	//
	// Parameter : CChatServer_Room*
	// return : 없음
	void CChat_LanClient::SetNetServer(CChatServer_Room* NetServer)
	{
		m_pNetServer = NetServer;
	}





	// ----------------------
	// 패킷 처리 함수
	// ----------------------

	// 신규 대기방 생성
	//
	// Parameter : ClientID, CProtocolBuff_Lan*
	// return : 없음
	void CChat_LanClient::Packet_RoomCreate(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload)
	{
		// 로그인 패킷 처리됐는지 확인
		if (m_bLoginCheck == false)
		{
			// 밖으로 예외 던짐
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_RoomCreate(). Not LoginUser. SessionID : %lld"), ClinetID);

			throw CException(str);
		}

		// 1. 방 Alloc
		CChatServer_Room::stRoom* NewRoom = m_pNetServer->m_pRoom_Pool->Alloc();


		// 2. 마샬링 및 방 셋팅
		UINT	ReqSequence;	
		int RoomNo;

		Payload->GetData((char*)&NewRoom->m_iBattleServerNo, 4);
		Payload->GetData((char*)&RoomNo, 4);
		Payload->GetData((char*)&NewRoom->m_iMaxUser, 4);
		Payload->GetData(NewRoom->m_cEnterToken, 32);

		Payload->GetData((char*)&ReqSequence, 4);

		NewRoom->m_iRoomNo = RoomNo;
		NewRoom->m_iJoinUser = 0;
		NewRoom->m_bDeleteFlag = false;

		if(NewRoom->m_JoinUser_vector.size() != 0)
			g_ChatDump->Crash();

		InterlockedIncrement(&m_pNetServer->m_lRoomCount);

		// 3. 방 자료구조에 추가
		if (m_pNetServer->InsertRoomFunc(RoomNo, NewRoom) == false)
			g_ChatDump->Crash();


		// 4. 응답패킷 보내기
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_CHAT_BAT_RES_CREATED_ROOM;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&ReqSequence, 4);

		SendPacket(ClinetID, SendBuff);		   		 	  
	}

	// 연결 토큰 재발행
	//
	// Parameter : ClientID, CProtocolBuff_Lan*
	// return : 없음
	void CChat_LanClient::Packet_TokenChange(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload)
	{
		// 로그인 패킷 처리됐는지 확인
		if (m_bLoginCheck == false)
		{
			// 밖으로 예외 던짐
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_TokenChange(). Not LoginUser. SessionID : %lld"), ClinetID);

			throw CException(str);
		}

		// 1. 마샬링
		char Token[32];
		UINT ReqSequence;

		Payload->GetData((char*)Token, 32);
		Payload->GetData((char*)&ReqSequence, 4);


		// 2. "현재" 토큰을 "이전" 토큰에 복사
		// 최초로 해당 패킷을 받았을 경우에도, 그냥 memcpy 한다. 
		memcpy_s(m_pNetServer->m_cConnectToken_Before, 32, m_pNetServer->m_cConnectToken_Now, 32);

		// 패킷에서 받은 토큰을 현재 토큰에 복사
		memcpy_s(m_pNetServer->m_cConnectToken_Now, 32, Token, 32);


		// 3. 응답 보내기
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_CHAT_BAT_RES_CONNECT_TOKEN;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ReqSequence, 8);


		SendPacket(ClinetID, SendBuff);
	}

	// 로그인 패킷에 대한 응답
	//
	// Parameter : ClientID, CProtocolBuff_Lan*
	// return : 없음
	void CChat_LanClient::Packet_Login(CProtocolBuff_Lan* Payload)
	{
		// 로그인 패킷 처리됐는지 확인
		// 됐으면 안됨.
		if (m_bLoginCheck == true)
		{
			// 밖으로 예외 던짐
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_Login(). LoginUser"));

			throw CException(str);
		}

		// 이 플래그로 로그인 처리가 되었음을 확인.
		m_bLoginCheck = true;		
	}
	
	// 방 삭제 
	//
	// Parameter : ClientID, CProtocolBuff_Lan*
	// return : 없음
	void CChat_LanClient::Packet_RoomErase(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload)
	{
		// 로그인 패킷 처리됐는지 확인
		if (m_bLoginCheck == false)
		{
			// 밖으로 예외 던짐
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_RoomErase(). Not LoginUser. SessionID : %lld"), ClinetID);

			throw CException(str);
		}


		// 1. 마샬링
		int		BattleServerNo;
		int		RoomNo;
		UINT	ReqSequence;			// 메시지 시퀀스 번호 (REQ / RES 짝맞춤 용도)

		Payload->GetData((char*)&BattleServerNo, 4);
		Payload->GetData((char*)&RoomNo, 4);
		Payload->GetData((char*)&ReqSequence, 4);


		// 2. 룸 검색

		AcquireSRWLockExclusive(&m_pNetServer->m_Room_Umap_srwl);	 // ----- 룸 Exclusive 락

		auto FindRoom = m_pNetServer->m_Room_Umap.find(RoomNo);
		
		// 방 검색 시, 방이 없을 수도 있음.
		// 채팅이 꺼질 경우, 배틀서버는 보유하고 있는 Wait 방을 모두 파괴시킨다. Play방은 남아있다.
		// 다시 채팅이 붙으면, 배틀은 Play방이 있고 채팅은 방이 하나도 없다.
		// 이 상태에서 Play방이 종료되면, 채팅에게 방 삭제 패킷을 보내는데 
		// 채팅은 다시 켜졌기 때문에 이전의 플레이 방이 없다.
		if (FindRoom == m_pNetServer->m_Room_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_pNetServer->m_Room_Umap_srwl);	 // ----- 룸 Exclusive 언락
			return;
		}

		CChatServer_Room::stRoom* NowRoom = FindRoom->second;

		NowRoom->ROOM_LOCK();	// ----- 룸 락


		// 3. 오류 체크
		
		// 배틀서버 번호 
		if(NowRoom->m_iBattleServerNo != BattleServerNo)
			g_ChatDump->Crash();

		// 룸 번호
		if(NowRoom->m_iRoomNo != RoomNo)
			g_ChatDump->Crash();


		// 4. 셧다운 날리기
		// 현재 접속자 수가 1명 이상이면, 방에 있는 모든 유저에게 Shutdown 호출
		// 그리고 삭제될 방 플래그 체크
		if(NowRoom->m_iJoinUser < 0)
			g_ChatDump->Crash();

		if (NowRoom->m_iJoinUser > 0)
		{
			NowRoom->m_bDeleteFlag = true;

			size_t Size = NowRoom->m_JoinUser_vector.size();

			if (Size != NowRoom->m_iJoinUser)
				g_ChatDump->Crash();

			size_t Index = 0;
			while (Size > Index)
			{
				// 셧다운
				m_pNetServer->Disconnect(NowRoom->m_JoinUser_vector[Index]);

				++Index;
			}

			NowRoom->ROOM_UNLOCK();	// ----- 룸 언락

			ReleaseSRWLockExclusive(&m_pNetServer->m_Room_Umap_srwl);	 // ----- 룸 Exclusive 언락
		}

		// 5. 접속자 수가 0명이면 방 즉시 제거
		else
		{
			NowRoom->ROOM_UNLOCK();	// ----- 룸 언락

			InterlockedDecrement(&m_pNetServer->m_lRoomCount);

			m_pNetServer->m_Room_Umap.erase(FindRoom);

			ReleaseSRWLockExclusive(&m_pNetServer->m_Room_Umap_srwl);	 // ----- 룸 Exclusive 언락
		}		


		// 6. 응답 패킷
		// 이 때, 방이 아직 삭제되지 않았을 수도 있지만, 곧 삭제될 예정이기 때문에 상관없다.
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_CHAT_BAT_RES_DESTROY_ROOM;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&ReqSequence, 4);

		SendPacket(ClinetID, SendBuff);
	}





	// ----------------------
	// 외부에서 사용 가능한 함수
	// ----------------------

	// 시작 함수
	// 내부적으로, 상속받은 CLanClient의 Start호출.
	//
	// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
	// return : 성공 시 true , 실패 시 falsel 
	bool CChat_LanClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 배틀 랜서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
		{
			// 배틀 랜클라 시작 에러
			g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Battle LanClient Start Error");

			return false;
		}
		

		// 배틀 랜클라 시작 로그
		g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"Battle LanClient Start...");

		return true;
	}

	// 클라이언트 종료
	//
	// Parameter : 없음
	// return : 없음
	void CChat_LanClient::ClientStop()
	{
		Stop();

		// 배틀 랜클라 종료 로그
		g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"Battle LanClient Stop...");
	}






	// -----------------------
	// 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CChat_LanClient::OnConnect(ULONGLONG ClinetID)
	{
		// 접속 종료, 혹은 생성자에서 초기화했기 때문에,
		// 무조건 0xffffffffffffffff이여야 한다.
		if (m_ullSessionID != 0xffffffffffffffff)
			g_ChatDump->Crash();

		m_ullSessionID = ClinetID;


		// 배틀서버에게 응답 패킷 보냄
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_CHAT_BAT_REQ_SERVER_ON;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)m_pNetServer->m_Paser.ChatIP, 32);
		SendBuff->PutData((char*)&m_pNetServer->m_Paser.Port, 2);

		SendPacket(ClinetID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CChat_LanClient::OnDisconnect(ULONGLONG ClinetID)
	{
		m_ullSessionID = 0xffffffffffffffff;
		m_bLoginCheck = false;

		// 배틀서버와 연결이 끊기면 모든 방 삭제
		m_pNetServer->RoomClearFunc();
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 세션키, 받은 패킷
	// return : 없음
	void CChat_LanClient::OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload)
	{
		// 접속 상태 확인
		if (m_ullSessionID == 0xffffffffffffffff ||
			m_ullSessionID != ClinetID)
		{
			g_ChatDump->Crash();
		}

		// 타입 확인
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		try
		{
			// 타입에 따라 분기처리
			switch (Type)
			{
				// 신규 대기방 생성
			case en_PACKET_CHAT_BAT_REQ_CREATED_ROOM:
				Packet_RoomCreate(ClinetID, Payload);
				break;

				// 방 삭제
			case en_PACKET_CHAT_BAT_REQ_DESTROY_ROOM:
				Packet_RoomErase(ClinetID, Payload);
				break;

				// 입장 토큰 재발행
			case en_PACKET_CHAT_BAT_REQ_CONNECT_TOKEN:
				Packet_TokenChange(ClinetID, Payload);
				break;
			
				// 서버 켜짐에 대한 응답(이미 보냈던 로그인 요청에 대한 응답)
			case en_PACKET_CHAT_BAT_RES_SERVER_ON:
				Packet_Login(Payload);
			break;

				// 없는 타입이면 크래시
			default:
				
				TCHAR str[100];
				StringCchPrintf(str, 100, _T("BattleLanClient. OnRecv(). TypeError. SessionID : %lld, Type : %d"), ClinetID, Type);

				throw CException(str);
				break;
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			g_ChatDump->Crash();

			// 접속 끊기 요청
			//Disconnect(SessionID);
		}
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 세션키, Send 한 사이즈
	// return : 없음
	void CChat_LanClient::OnSend(ULONGLONG ClinetID, DWORD SendSize)
	{

	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CChat_LanClient::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CChat_LanClient::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CChat_LanClient::OnError(int error, const TCHAR* errorStr)
	{

	}



	// -------------------
	// 생성자와 소멸자
	// -------------------

	// 생성자
	CChat_LanClient::CChat_LanClient()
		:CLanClient()
	{
		m_ullSessionID = 0xffffffffffffffff;
		m_bLoginCheck = false;
	}

	// 소멸자
	CChat_LanClient::~CChat_LanClient()
	{

	}

}


// ---------------------------------------------
// 
// 모니터링 LanClient
// 
// ---------------------------------------------
namespace Library_Jingyu
{

	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	CChat_MonitorClient::CChat_MonitorClient()
		:CLanClient()
	{
		// 모니터링 서버 정보전송 스레드를 종료시킬 이벤트 생성
		//
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		m_hMonitorThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		m_ullSessionID = 0xffffffffffffffff;
	}

	CChat_MonitorClient::~CChat_MonitorClient()
	{
		// 아직 연결이 되어있으면, 연결 해제
		if (GetClinetState() == true)
			ClientStop();

		// 이벤트 삭제
		CloseHandle(m_hMonitorThreadExitEvent);
	}


	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 시작 함수
	// 내부적으로, 상속받은 CLanClient의 Start호출.
	//
	// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
	// return : 성공 시 true , 실패 시 falsel 
	bool CChat_MonitorClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 모니터링 서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
		{
			// 모니터 랜클라 시작 에러
			g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Monitor LanClient Start Error");

			return false;
		}

		// 모니터링 서버로 정보 전송할 스레드 생성
		m_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);

		// 모니터 랜클라 시작 로그
		g_ChatLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"Monitor LanClient Star");

		return true;
	}

	// 종료 함수
	// 내부적으로, 상속받은 CLanClient의 Stop호출.
	// 추가로, 리소스 해제 등
	//
	// Parameter : 없음
	// return : 없음
	void CChat_MonitorClient::ClientStop()
	{
		// 1. 모니터링 서버 정보전송 스레드 종료
		SetEvent(m_hMonitorThreadExitEvent);

		// 종료 대기
		if (WaitForSingleObject(m_hMonitorThread, INFINITE) == WAIT_FAILED)
		{
			DWORD Error = GetLastError();
			printf("MonitorThread Exit Error!!! (%d) \n", Error);
		}

		// 2. 스레드 핸들 반환
		CloseHandle(m_hMonitorThread);

		// 3. 모니터링 서버와 연결 종료
		Stop();
	}

	// 채팅서버의 this를 입력받는 함수
	// 
	// Parameter : 쳇 서버의 this
	// return : 없음
	void CChat_MonitorClient::SetNetServer(CChatServer_Room* ChatThis)
	{
		m_ChatServer_this = ChatThis;
	}




	// -----------------------
	// 내부에서만 사용하는 기능 함수
	// -----------------------

	// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
	UINT	WINAPI CChat_MonitorClient::MonitorThread(LPVOID lParam)
	{
		// this 받아두기
		CChat_MonitorClient* g_This = (CChat_MonitorClient*)lParam;

		// 종료 신호 이벤트 받아두기
		HANDLE hEvent = g_This->m_hMonitorThreadExitEvent;

		// CPU 사용율 체크 클래스 (채팅서버 소프트웨어)
		CCpuUsage_Process CProcessCPU;

		// PDH용 클래스
		CPDH	CPdh;

		while (1)
		{
			// 1초에 한번 깨어나서 정보를 보낸다.
			DWORD Check = WaitForSingleObject(hEvent, 1000);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("MoniterThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 만약, 종료 신호가 왔다면 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;

			// 그게 아니라면, 일을 한다.
			// 모니터링 서버와 연결중일 때만!
			if (g_This->GetClinetState() == false)
				continue;

			// 프로세서 CPU 사용율, PDH 정보 갱신
			CProcessCPU.UpdateCpuTime();
			CPdh.SetInfo();

			// 채팅서버가 On일 경우, 패킷을 보낸다.
			if (g_This->m_ChatServer_this->GetServerState() == true)
			{
				// 타임스탬프 구하기
				int TimeStamp = (int)(time(NULL));

				// 1. 채팅서버 ON		
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_SERVER_ON, TRUE, TimeStamp);

				// 2. 채팅서버 CPU 사용률 (커널 + 유저)
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_CPU, (int)CProcessCPU.ProcessTotal(), TimeStamp);

				// 3. 채팅서버 메모리 유저 커밋 사용량 (Private) MByte
				int Data = (int)(CPdh.Get_UserCommit() / 1024 / 1024);
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT, Data, TimeStamp);

				// 4. 채팅서버 패킷풀 사용량
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, CProtocolBuff_Net::GetNodeCount() + CProtocolBuff_Lan::GetNodeCount(), TimeStamp);

				// 5. 채팅서버 접속 세션전체
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_SESSION, (int)g_This->m_ChatServer_this->GetClientCount(), TimeStamp);

				// 6. 채팅서버 로그인을 성공한 전체 인원				
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_PLAYER, g_This->m_ChatServer_this->m_lChatLoginCount, TimeStamp);

				// 7. 배틀서버 방 수				
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_ROOM, g_This->m_ChatServer_this->m_lRoomCount, TimeStamp);
			}

		}

		return 0;
	}

	// 모니터링 서버로 데이터 전송
	//
	// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
	// return : 없음
	void CChat_MonitorClient::InfoSend(BYTE DataType, int DataValue, int TimeStamp)
	{
		WORD Type = en_PACKET_SS_MONITOR_DATA_UPDATE;

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&DataType, 1);
		SendBuff->PutData((char*)&DataValue, 4);
		SendBuff->PutData((char*)&TimeStamp, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}



	// -----------------------
	// 순수 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CChat_MonitorClient::OnConnect(ULONGLONG SessionID)
	{
		m_ullSessionID = SessionID;

		// 모니터링 서버(Lan)로 로그인 패킷 보냄
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_SS_MONITOR_LOGIN;
		int ServerNo = dfSERVER_NO;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerNo, 4);

		SendPacket(SessionID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CChat_MonitorClient::OnDisconnect(ULONGLONG SessionID)
	{
		m_ullSessionID = 0xffffffffffffffff;
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, CProtocolBuff_Lan*
	// return : 없음
	void CChat_MonitorClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CChat_MonitorClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CChat_MonitorClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CChat_MonitorClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CChat_MonitorClient::OnError(int error, const TCHAR* errorStr)
	{}
}