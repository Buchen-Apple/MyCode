#include "pch.h"
#include "ChatServer_Room.h"
#include "Protocol_Set/CommonProtocol_2.h"
#include "CrashDump/CrashDump.h"
#include "Log/Log.h"

#include <strsafe.h>


namespace Library_Jingyu
{
	// 직렬화 버퍼 1개의 크기
	// 각 서버에 전역 변수로 존재해야 함.
	LONG g_lNET_BUFF_SIZE;

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



		// 4. 토큰 체크
		if (memcmp(m_cConnectToken, ConnectToken, 32) != 0)
		{
			// 다르면 실패 패킷
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_CHAT_RES_LOGIN;
			BYTE Status = 0;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&Status, 1);
			SendBuff->PutData((char*)&AccountNo, 8);

			SendPacket(SessionID, SendBuff);

			return;
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


		// 5. 접속중인 룸 검증
		if (RoomNo != NowPlayer->m_iRoomNo)
		{
			g_ChatDump->Crash();

			/*
			TCHAR str[100];
			StringCchPrintf(str, 100, _T("Packet_Room_Enter(). RoomNo Error. SessionID : %lld, AccountNo : %lld, RoomNo : %d"),
				SessionID, AccountNo, RoomNo);

			throw CException(str);
			*/
		}	



		// 6. 룸 검색
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


		// 7. 룸 토큰 검사 및 자료구조에 추가
		NowRoom->ROOM_LOCK();	// ----- 룸 락	

		// 룸 토큰 검사
		if (memcmp(EnterToken, NowRoom->m_cEnterToken, 32) != 0)
		{
			ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락
			NowRoom->ROOM_UNLOCK();							// ----- 룸 언락	

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

		NowRoom->ROOM_UNLOCK();	// ----- 룸 언락	

		ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락



		// 8. 성공 패킷 응답
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
			// 미리 초기화
			DeletePlayer->m_bLoginCheck = false;

			// 로그인 자료구조에서 제거
			if(EraseLoginPlayerFunc(DeletePlayer->m_i64AccountNo) == false)
				g_ChatDump->Crash();
		}


		// 3. stPlayer* Free
		m_pPlayer_Pool->Free(DeletePlayer);


		// 4. 룸에서 제거

		AcquireSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 락

		// 룸 검색
		auto FindRoom = m_Room_Umap.find(RoomNo);
		if(FindRoom == m_Room_Umap.end())
			g_ChatDump->Crash();

		stRoom* NowRoom = FindRoom->second;
		
		NowRoom->ROOM_LOCK();	// ----- 룸 락

		// 룸 내 자료구조에서 유저 제거
		if(NowRoom->Erase(SessionID) == false)
			g_ChatDump->Crash();
		
		NowRoom->ROOM_UNLOCK();	// ----- 룸 언락

		ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락
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
	{
		// config


		// 로그
		g_ChatLog->SetDirectory(L"ChatServer");
		g_ChatLog->SetLogLeve((CSystemLog::en_LogLevel)CSystemLog::en_LogLevel::LEVEL_DEBUG);
		//g_ChatLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);




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
	}
}