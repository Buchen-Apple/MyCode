#include "pch.h"
#include "BattleServer_Room_Version.h"
#include "Parser\Parser_Class.h"
#include "Protocol_Set\CommonProtocol_2.h"		// 차후 일반으로 변경 필요
#include "Log\Log.h"
#include "CPUUsage\CPUUsage.h"
#include "PDHClass\PDHCheck.h"

#include "shDB_Communicate.h"

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

#include <process.h>
#include <strsafe.h>

using namespace rapidjson;

extern ULONGLONG g_ullAcceptTotal_MMO;
extern LONG	  g_lAcceptTPS_MMO;
extern LONG	g_lSendPostTPS_MMO;
extern LONG	g_lRecvTPS_MMO;

extern LONG g_lAllocNodeCount;
extern LONG g_lAllocNodeCount_Lan;

extern LONG g_lAuthModeUserCount;
extern LONG g_lGameModeUserCount;

extern LONG g_lAuthFPS;
extern LONG g_lGameFPS;

LONG g_lShowAuthFPS;
LONG g_lShowGameFPS;

// 배틀서버 입장 토큰 에러
LONG g_lBattleEnterTokenError;

// auth의 로그인 패킷에서, DB 쿼리 시 결과로 -10(사용자 없음)이 올 경우
LONG g_lQuery_Result_Not_Find;

// auth의 로그인 패킷에서, DB 쿼리 시 결과로 -10도 아닌데 1이 아닌 에러일 경우 
LONG g_lTempError;

// auth의 로그인 패킷에서, 유저의 SessionKey(토큰)이 다를 경우
LONG g_lTokenError;

// auth의 로그인 패킷에서, 유저가 들고 온 버전이 다를 경우
LONG g_lVerError;

// auth의 로그인 패킷에서, 중복 로그인 시
LONG g_DuplicateCount;

// GQCS에서 세마포어 리턴 시 1 증가
extern LONG g_SemCount;



// !! 임시. 차후 강사님이 주신 파일에서 읽을 예정 !!
LONG g_Cartridge_Bullet = 10;





// ------------------
// CGameSession의 함수
// (CBattleServer_Room의 이너클래스)
// ------------------
namespace Library_Jingyu
{
	// 덤프용
	CCrashDump* g_BattleServer_Room_Dump = CCrashDump::GetInstance();

	// 로그용
	CSystemLog* g_BattleServer_RoomLog = CSystemLog::GetInstance();


	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CBattleServer_Room::CGameSession::CGameSession()
		:CMMOServer::cSession()
	{
		// ClientKey 초기값 셋팅
		m_int64ClientKey = -1;

		// 자료구조에 들어감 플래그
		m_bStructFlag = false;

		// 로그인 패킷 완료 처리
		m_bLoginFlag = false;

		// 곧 종료될 유저 체크 플래그
		m_bLogoutFlag = false;

		// 입장중인 방 초기 번호
		m_iRoomNo = -1;

		// 로그인 HTTP 요청 횟수
		m_lLoginHTTPCount = 0;

		// 생존 상태
		m_bAliveFlag = false;	
	}

	// 소멸자
	CBattleServer_Room::CGameSession::~CGameSession()
	{
		// 할거 없음
	}

	// -----------------
	// 가상함수
	// -----------------

	// --------------- AUTH 모드용 함수

	// 유저가 Auth 모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_ClientJoin()
	{
		// 할거 없음
	}

	// 유저가 Auth 모드에서 나감
	//
	// Parameter : Game모드로 변경된것인지 알려주는 Flag. 디폴트 false(변경되지 않음)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_ClientLeave(bool bGame)
	{
		// 모드 변경일 경우
		if (bGame == true)
		{
			
		}

		// 실제 게임 종료일 경우
		else
		{
			// ClientKey 초기값으로 셋팅.
			// !! 이걸 안하면, Release되는 중에 HTTP응답이 올 경우 !!
			// !! Auth_Update에서 이미 샌드큐가 모두 정리된 유저에게 또 SendPacket 가능성 !!
			// !! 이 경우, 전혀 엉뚱한 유저에게 Send가 될 수 있음 !!
			m_int64ClientKey = -1;

			// 해당 유저가 있는 룸 안에서 유저 제거
			// -1이 아니면 룸에 있는것.
			if (m_iRoomNo != -1)
			{
				AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 락 

				// 1. 룸 검색
				auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

				// 룸이 없으면 Crash
				if (FindRoom == m_pParent->m_Room_Umap.end())
				{
					ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 
					g_BattleServer_Room_Dump->Crash();
				}

				stRoom* NowRoom = FindRoom->second;

				// 룸 상태가 Play면 Crash. 말도 안됨
				// Auth모드에서 종료되는 유저가 있는 방은, 무조건 대기/준비 방이어야 함				
				if (NowRoom->m_iRoomState == eu_ROOM_STATE::PLAY_ROOM)
				{
					ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 
					g_BattleServer_Room_Dump->Crash();
				}

				// 여기까지 오면 방 모드가 WAIT_ROOM 혹은 READY_ROOM 확정.
				// 즉, Auth에서만 접근하는 방 확정. 락 푼다.
				// 혹시, 락 풀었는데 삭제될 가능성은 없음.
				// Auth에서 관리중인데 방이 삭제되는것은 말도 안됨. 
				// 방은 Play모드에서 삭제된다
				ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 


				// 2. 현재 룸 안에 있는 유저 수 감소
				--NowRoom->m_iJoinUserCount;

				// 감소 후, 룸 안의 유저 수가 0보다 적으면 말도 안됨.
				if (NowRoom->m_iJoinUserCount < 0)				
					g_BattleServer_Room_Dump->Crash();


				// 3. 룸 안의 자료구조에서 유저 제거
				if (NowRoom->Erase(this) == false)
					g_BattleServer_Room_Dump->Crash();

				m_iRoomNo = -1;


				// 4. 방 안의 모든 유저에게, 방에서 유저가 나갔다고 알려줌
				// BroadCast해도 된다. 어차피 방 안에 지금 나간 유저는 없음.
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_GAME_RES_REMOVE_USER;

				SendBuff->PutData((char*)&Type, 2);
				SendBuff->PutData((char*)&m_iRoomNo, 4);
				SendBuff->PutData((char*)&m_Int64AccountNo, 8);

				// 여기서, false가 리턴될 수 있음. (방 안의 자료구조에 유저가 0명)
				// 대기방인 상태에서 모든 유저가 나갈 수 있기 때문에 문제로 안본다.
				// 때문에 return 안받음
				NowRoom->SendPacket_BroadCast(SendBuff);


				// 5. 마스터 서버에게 해당 유저 나갔다고 패킷 보내줘야 함.
				m_pParent->m_Master_LanClient->Packet_RoomLeave_Req(m_iRoomNo, m_Int64AccountNo);
			}			
		}

	}

	// Auth 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_Packet(CProtocolBuff_Net* Packet)
	{
		// 패킷 처리
		try
		{
			// 1. 타입 빼기
			WORD Type;
			Packet->GetData((char*)&Type, 2);

			// 2. 타입에 따라 분기 처리
			switch (Type)
			{
				// 로그인 요청
			case en_PACKET_CS_GAME_REQ_LOGIN:
				Auth_LoginPacket(Packet);
				break;

				// 방 입장 요청
			case en_PACKET_CS_GAME_REQ_ENTER_ROOM:
				break;

				// 하트비트
			case en_PACKET_CS_GAME_REQ_HEARTBEAT:
				break;

				// 그 외에는 문제있음.
			default:
				throw CException(_T("OnAuth_Packet() --> Type Error!!"));
				break;
			}
		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// 덤프
			g_BattleServer_Room_Dump->Crash();
		}

	}



	// --------------- GAME 모드용 함수

	// 유저가 Game모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientJoin()
	{
		// 할거 없음
	}

	// 유저가 Game모드에서 나감
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientLeave()
	{
		// ClientKey 초기값으로 셋팅.
		// !! 이걸 안하면, Release되는 중에 HTTP응답이 올 경우 !!
		// !! Game_Update에서 이미 샌드큐가 모두 정리된 유저에게 또 SendPacket 가능성 !!
		// !! 이 경우, 전혀 엉뚱한 유저에게 Send가 될 수 있음 !!
		m_int64ClientKey = -1;


		// 해당 유저가 있는 룸 안에서 유저 제거
		// -1인 것은 말도 안됨. 무조건 방에 있어야 함
		if(m_iRoomNo == -1)
			g_BattleServer_Room_Dump->Crash();


		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 락 

		// 1. 룸 검색
		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 룸이 없으면 Crash
		if (FindRoom == m_pParent->m_Room_Umap.end())
		{
			ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 
			g_BattleServer_Room_Dump->Crash();
		}

		stRoom* NowRoom = FindRoom->second;

		// 룸 상태가 Play가 아니면 Crash. 말도 안됨
		// Game모드에서 종료되는 유저가 있는 방은, 무조건 플레이 방이어야 함
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
		{
			ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 
			g_BattleServer_Room_Dump->Crash();
		}

		// 여기까지 오면 방 모드가 PLAY_ROOM 확정.
		// 즉, Game에서만 접근하는 방 확정. 락 푼다.
		// 락 풀었는데 방이 삭제될 가능성은 전혀 없음. 
		// 방 삭제는 Game 스레드가 하고, 해당 함수도 Game 스레드에서 호출된다. 
		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 


		// 2. 현재 룸 안에 있는 유저 수 감소
		--NowRoom->m_iJoinUserCount;

		// 감소 후, 룸 안의 유저 수가 0보다 적으면 말도 안됨.
		if (NowRoom->m_iJoinUserCount < 0)
			g_BattleServer_Room_Dump->Crash();

		// 3. 생존한 유저 수 감소.
		// 생존한 유저 수로 승리여부를 판단해야 하기 때문에 
		// 유저가 게임에서 나가거나 HP가 0이되어 사망할 경우 감소시켜야 함
		--NowRoom->m_iAliveUserCount;

		// 감소 후, 0보다 적으면 말도 안됨.
		if (NowRoom->m_iAliveUserCount < 0)
			g_BattleServer_Room_Dump->Crash();


		// 4. 룸 안의 자료구조에서 유저 제거
		if (NowRoom->Erase(this) == false)
			g_BattleServer_Room_Dump->Crash();

		m_iRoomNo = -1;

	}

	// Game 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_Packet(CProtocolBuff_Net* Packet)
	{
		// 타입에 따라 패킷 처리
		try
		{
			// 1. 타입 빼기
			WORD Type;
			Packet->GetData((char*)&Type, 2);

			// 2. 타입에 따라 분기문 처리
			switch (Type)
			{	

				// 하트비트
			case en_PACKET_CS_GAME_REQ_HEARTBEAT:
				break;

				// 이 외에는 문제 있음
			default:
				throw CException(_T("OnGame_Packet() --> Type Error!!"));
				break;
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// 덤프
			g_BattleServer_Room_Dump->Crash();
		}

	}



	// --------------- Release 모드용 함수

	// Release된 유저.
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientRelease()
	{
		// m_bStructFlag가 true라면, 자료구조에 들어간 유저
		if (m_bStructFlag == true)
		{
			if (m_pParent->EraseAccountNoFunc(m_Int64AccountNo) == false)
				g_BattleServer_Room_Dump->Crash();

			m_bStructFlag = false;
		}
		
		m_bLoginFlag = false;	
		m_bLogoutFlag = false;
		m_bAliveFlag = false;
		m_lLoginHTTPCount = 0;		
	}




	// -----------------
	// Auth모드 패킷 처리 함수
	// -----------------

	// 로그인 요청 
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Auth_LoginPacket(CProtocolBuff_Net* Packet)
	{
		// 1. 현재 ClientKey가 초기값이 아니면 Crash
		if (m_int64ClientKey != -1)
			g_BattleServer_Room_Dump->Crash();


		// 2. AccountNo, ClientKey 마샬링
		INT64 AccountNo;
		INT64 ClinetKey;

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)&ClinetKey, 8);



		// 3. AccountNo, ClientKey 셋팅
		m_Int64AccountNo = AccountNo;
		m_int64ClientKey = ClinetKey;



		// 4. AccountNo 자료구조에 추가.
		// 이미 있으면(false 리턴) 중복 로그인으로 처리
		// 현재 유저에게는 실패 패킷, 접속 중인 유저는 DIsconnect.
		if (m_pParent->InsertAccountNoFunc(AccountNo, this) == false)
		{
			InterlockedIncrement(&g_DuplicateCount);			

			// 로그인 실패 패킷 보내고 끊기.
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 타입
			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			SendBuff->PutData((char*)&Type, 2);

			// Status
			BYTE Status = 6;		// 중복 로그인
			SendBuff->PutData((char*)&Status, 1);

			// AccountNo
			SendBuff->PutData((char*)&AccountNo, 8);			

			// SendPacket
			SendPacket(SendBuff);

			// 중복 로그인 처리된 유저 접속 종료 요청
			m_pParent->DisconnectAccountNoFunc(AccountNo);

			return;
		}

		// 자료구조에 들어감 플래그 변경
		m_bStructFlag = true;



		// 5. 로그인 인증처리를 위한 HTTP 통신
		// 통신 후, 해당 패킷에 대한 후처리는 Auth_Update에게 넘긴다.
		DB_WORK_LOGIN* Send_A = (DB_WORK_LOGIN*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();

		Send_A->m_wWorkType = eu_DB_READ_TYPE::eu_LOGIN_AUTH;
		Send_A->m_i64UniqueKey = ClinetKey;
		Send_A->pPointer = this;

		// 직렬화 버퍼 전달 전에, 레퍼런스 카운트 Add
		Packet->Add();
		Send_A->m_pBuff = Packet;

		Send_A->AccountNo = AccountNo;

		// Select_Account.php 요청
		m_pParent->m_shDB_Communicate.DBReadFunc((DB_WORK*)Send_A, en_PHP_TYPE::SELECT_ACCOUNT);


		// 6. 유저 전적 정보를 위한 HTTP 통신
		// 통신 후, 해당 패킷에 대한 후처리는 Auth_Update에게 넘긴다.
		DB_WORK_LOGIN_CONTENTS* Send_B = (DB_WORK_LOGIN_CONTENTS*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();
	
		Send_A->m_wWorkType = eu_DB_READ_TYPE::eu_LOGIN_INFO;
		Send_A->m_i64UniqueKey = ClinetKey;
		Send_A->pPointer = this;

		Send_A->AccountNo = AccountNo;

		// Select_Contents.php 요청
		m_pParent->m_shDB_Communicate.DBReadFunc((DB_WORK*)Send_A, en_PHP_TYPE::SELECT_CONTENTS);	
	}


	// 방 입장 요청
	// 
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Auth_RoomEnterPacket(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if(m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 1. 마샬링
		INT64 AccountNo;
		int RoomNo;
		char EnterToken[32];

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)&RoomNo, 4);
		Packet->GetData(EnterToken, 32);		

		// 2. 에러 체크
		if (AccountNo != m_Int64AccountNo)
			g_BattleServer_Room_Dump->Crash();
		
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);	// ----- Room umap Shared 락

		
		// 3. 입장하고자 하는 방 검색
		auto ret = m_pParent->m_Room_Umap.find(RoomNo);

		// 4. 방 존재여부 체크
		if (ret == m_pParent->m_Room_Umap.end())
		{
			ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);	// ----- Room umap Shared 언락

			// 방이 없으면, 에러 리턴
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 4;
			BYTE MaxUser = 0;	// 방 없음 패킷에는 의미가 없을듯.. 그냥 아무거나 보냄.

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}

		stRoom* NowRoom = ret->second;

		// 5. 방이 대기방이 아니면 에러 3 리턴 (준비방 or 플레이 상태의 방일 경우)
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::WAIT_ROOM)
		{
			ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);	// ----- Room umap Shared 언락

			// 대기방이 아니면, 에러 리턴
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 3;
			BYTE MaxUser = 0;	// 대기방 아님 패킷에는 의미가 없을듯.. 그냥 아무거나 보냄.

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}
		
		// 여기까지 오면, [존재하는 대기방]이라는게 확정. (Auth모드에서 관리중인 방 확정)
		// 락 푼다. 어차피 Auth만 접근하는 방이다.
		// 락 풀고 중간에 삭제될 가능성도 없다. 
		// 삭제되기 위해서는 Play모드의 방이 되어야 하는데, 모드 변경은 Auth 스레드가 한다.
		// 그리고, 해당 함수는 Auth 스레드에서 호출된다.
		// 즉, 해당 함수가 끝나기 전에 방이 사라질 가능성은 없다.
		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);	// ----- Room umap Shared 언락


		// 6. 방 인원수 체크
		if (NowRoom->m_iJoinUserCount == NowRoom->m_iMaxJoinCount)
		{
			BYTE MaxUser = NowRoom->m_iJoinUserCount;		

			// 이미 최대 인원수라면, 에러 리턴			
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 5;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}

		// 7. 방 토큰 체크
		if (memcmp(EnterToken, NowRoom->m_cEnterToken, 32) != 0)
		{
			BYTE MaxUser = NowRoom->m_iJoinUserCount;		

			// 토큰이 다르면, 에러 리턴
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 2;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}


		// 8. 여기까지 오면 정상
		// 룸에 인원 추가
		NowRoom->m_iJoinUserCount++;
		NowRoom->Insert(this);

		int NowUser = NowRoom->m_iJoinUserCount;


		// 9. 정상적인 방 입장 응답 보내기
		CProtocolBuff_Net* SendBuff_A = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
		BYTE Result = 1;

		SendBuff_A->PutData((char*)&Type, 2);
		SendBuff_A->PutData((char*)&AccountNo, 8);
		SendBuff_A->PutData((char*)&RoomNo, 4);
		SendBuff_A->PutData((char*)&NowUser, 1);
		SendBuff_A->PutData((char*)&Result, 1);

		SendPacket(SendBuff_A);


		// 10. 방에 있는 모든 유저에게 "유저 추가됨" 패킷 보내기
		// 이번에 입장한 자기 자신 포함.
		Type = en_PACKET_CS_GAME_RES_ADD_USER;

		CProtocolBuff_Net* SendBuff_B = CProtocolBuff_Net::Alloc();

		SendBuff_B->PutData((char*)&Type, 2);
		SendBuff_B->PutData((char*)&RoomNo, 4);
		SendBuff_B->PutData((char*)&AccountNo, 8);
		SendBuff_B->PutData((char*)m_tcNickName, 40);

		SendBuff_B->PutData((char*)&m_iRecord_PlayCount, 4);
		SendBuff_B->PutData((char*)&m_iRecord_PlayTime, 4);
		SendBuff_B->PutData((char*)&m_iRecord_Kill, 4);
		SendBuff_B->PutData((char*)&m_iRecord_Die, 4);
		SendBuff_B->PutData((char*)&m_iRecord_Win, 4);

		if (NowRoom->SendPacket_BroadCast(SendBuff_B) == false)
			g_BattleServer_Room_Dump->Crash();



		// 11. 룸 인원수가 풀 방이 되었을 경우
		if (NowUser == NowRoom->m_iMaxJoinCount)
		{
			// 1) 마스터에게 방 닫힘 패킷 보내기
			m_pParent->m_Master_LanClient->Packet_RoomClose_Req(RoomNo);

			// 2) 모든 유저에게 카운트다운 패킷 보내기
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_PLAY_READY;
			BYTE ReadySec = m_pParent->m_stConst.m_bCountDownSec;		// 초 단위

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&ReadySec, 1);

			if (NowRoom->SendPacket_BroadCast(SendBuff) == false)
				g_BattleServer_Room_Dump->Crash();

			// 3) 방 카운트다운 변수 갱신
			// 이 시간 + 유저에게 보낸 카운트다운(10초)가 되면 방 모드를 Play로 변경시키고
			// 유저도 AUTH_TO_GAME으로 변경시킨다.
			NowRoom->m_dwCountDown = timeGetTime();

			// 4) 방의 상태를 Ready로 변경
			NowRoom->m_iRoomState = eu_ROOM_STATE::READY_ROOM;

			// 5) 대기방 수 감소
			// 해당 카운트는 Auth 모드에서만 접근하기 때문에 인터락 필요 없음.
			--m_pParent->m_lNowWaitRoomCount;
		}
	}



	// -----------------
	// Game모드 패킷 처리 함수
	// -----------------



}


// ------------------
// stRoom의 함수
// (CBattleServer_Room의 이너클래스)
// ------------------
namespace Library_Jingyu
{

	// ------------
	// 멤버 함수
	// ------------

	// 생성자
	CBattleServer_Room::stRoom::stRoom()
	{
		// 미리 메모리 공간 잡아두기
		m_JoinUser_Vector.reserve(10);

		// 게임이 종료되었는지 체크하는 플래그
		m_bGameEndFlag = false;

		// 연속 셧다운 방지용.
		m_bShutdownFlag = false;
	}

	// 자료구조 내의 모든 유저에게 인자로 받은 패킷 보내기
	//
	// Parameter : CProtocolBuff_Net*
	// return : 자료구조 내에 유저가 0명일 경우 false
	//		  : 그 외에는 true
	bool CBattleServer_Room::stRoom::SendPacket_BroadCast(CProtocolBuff_Net* SendBuff)
	{
		// 1. 자료구조 내의 유저 수 받기
		size_t Size = m_JoinUser_Vector.size();

		// 2. 유저 수가 0명이거나 적을 경우, return false
		if (Size <= 0)
			return false;

		// !! while문 돌기 전에 카운트를, 유저 수 만큼 증가 !!
		// 엔진쪽에서, 완료 통지가 오면 Free를 하기 때문에 Add해야 한다.
		SendBuff->Add((int)Size);

		// 3. 유저 수 만큼 돌면서 패킷 전송.
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->SendPacket(SendBuff);
			++Index;
		}

		// 4. 패킷 Free
		// !! 엔진쪽 완통에서 Free 하지만, 레퍼런스 카운트를 인원 수 만큼 Add 했기 때문에 1개가 더 증가한 상태. !!	
		CProtocolBuff_Net::Free(SendBuff);

		return true;
	}

	// 방 내의 모든 유저를 Auth_To_Game으로 변경
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::ModeChange()
	{
		// 1. 자료구조 내의 유저 수 받기
		size_t Size = m_JoinUser_Vector.size();

		// 2. 유저 수가 0명일 가능성 있음.
		// 카운트 다운 중에 모든 유저가 나갈 경우.
		// 문제로 안본다.
		if (Size == 0)
			return;

		// 3. 0명이 아니라면, 내부에 있는 모든 유저를 AUTH_TO_GAME으로 변경
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->SetMode_GAME();
			++Index;
		}
	}

	// 룸 안의 모든 유저를 생존상태로 변경
	//
	// Parameter : 없음
	// return : 자료구조 내에 유저가 0명일 경우 false
	//		  : 그 외에는 true
	bool CBattleServer_Room::stRoom::AliveFalg_True()
	{
		// 1. 자료구조 내의 유저 수 받기
		size_t Size = m_JoinUser_Vector.size();

		// 2. 유저 수가 0명이거나 0이하일 경우, return false
		if (Size <= 0)
			return false;

		// 3. 유저 수 만큼 돌면서 생존 플래그 변경
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->m_bAliveFlag = true;
			++Index;
		}

		return true;
	}

	// 방 안의 유저들에게 게임 종료 패킷 보내기
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::GameOver()
	{
		// 1. 방 안에 유저가 0명인 경우 Crash
		if (m_iJoinUserCount == 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 방 인원수 체크 변수와 vector 안의 사이즈가 달라도 Crash
		size_t Size = m_JoinUser_Vector.size();

		if(Size == 0 || Size != m_iJoinUserCount)
			g_BattleServer_Room_Dump->Crash();		
			   

		// 3. 승리자에게 보낼 승리패킷 만들기
		CProtocolBuff_Net* winPacket = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_WINNER;
		winPacket->PutData((char*)&Type, 2);


		// 4. 패배자에게 보낼 패배 패킷 만들기
		CProtocolBuff_Net* LosePacket = CProtocolBuff_Net::Alloc();

		Type = en_PACKET_CS_GAME_RES_GAMEOVER;
		LosePacket->PutData((char*)&Type, 2);


		// 5. 순회하면서 패킷 보내기
		size_t Index = 0;
		int WinUserCount = 0;

		while (Index < Size)
		{
			// 유저가 승리자일 경우 (생존자)
			if (m_JoinUser_Vector[Index]->m_bAliveFlag == true)
			{
				// 한 게임에 승리자는 1명.
				// WinUserCount가 1인데 여기 들어왔다면, 승리자가 1명 이상이 되었다는 의미.
				// 말도 안됨.
				if(WinUserCount == 1)
					g_BattleServer_Room_Dump->Crash();

				// 승리 패킷 보내기
				m_JoinUser_Vector[Index]->SendPacket(winPacket);

				// 승리패킷 보낸 수 증가
				++WinUserCount;
			}

			// 유저가 패배자일 경우 (사망자)
			else
			{
				// 래퍼런스 카운트 증가.
				LosePacket->Add();

				// 패배 패킷 보내기
				m_JoinUser_Vector[Index]->SendPacket(LosePacket);
			}

			++Index;
		}

		// 6. 패배 패킷의 래퍼런스 카운트 감소.
		CProtocolBuff_Net::Free(LosePacket);
	}

	// 방 안의 유저들에게 셧다운 날리기
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::Shutdown_All()
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안에 0명이면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if(Size <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 3. 백터 안의 유저와 변수가 다르면 크래시
		if(m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 4. 순회하며 Shutdown
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->Disconnect();

			++Index;
		}	
	}


	// ------------
	// 자료구조 함수
	// ------------

	// 자료구조에 Insert
	//
	// Parameter : 추가하고자 하는 CGameSession*
	// return : 없음
	void CBattleServer_Room::stRoom::Insert(CGameSession* InsertPlayer)
	{
		m_JoinUser_Vector.push_back(InsertPlayer);
	}

	// 자료구조에서 Erase
	//
	// Parameter : 제거하고자 하는 CGameSession*
	// return : 성공 시 true
	//		  : 실패 시  false
	bool CBattleServer_Room::stRoom::Erase(CGameSession* InsertPlayer)
	{
		size_t Size = m_JoinUser_Vector.size();
		bool Flag = false;

		// 1. 자료구조 안에 유저가 0보다 작거나 같으면 return false
		if (Size <= 0)
			return false;

		// 2. 자료구조 안에 유저가 1명이거나, 찾고자 하는 유저가 마지막에 있다면 바로 제거
		if (Size == 1 || m_JoinUser_Vector[Size - 1] == InsertPlayer)
		{
			Flag = true;
			m_JoinUser_Vector.pop_back();
		}

		// 3. 아니라면 Swap 한다
		else
		{
			size_t Index = 0;
			while (Index < Size)
			{
				// 내가 찾고자 하는 유저를 찾았다면
				if (m_JoinUser_Vector[Index] == InsertPlayer)
				{
					Flag = true;

					CGameSession* Temp = m_JoinUser_Vector[Size - 1];
					m_JoinUser_Vector[Size - 1] = m_JoinUser_Vector[Index];
					m_JoinUser_Vector[Index] = Temp;

					m_JoinUser_Vector.pop_back();

					break;
				}

				++Index;
			}
		}

		// 4. 만약, 제거 못했다면 return false
		if (Flag == false)
			return false;

		return true;
	}

}


// ----------------------------------------
// 
// MMOServer를 이용한 배틀 서버. 룸 버전
//
// ----------------------------------------
namespace Library_Jingyu
{
	// Net 직렬화 버퍼 1개의 크기 (Byte)
	LONG g_lNET_BUFF_SIZE = 512;
		   	  


	// -----------------------
	// 내부에서만 사용하는 함수
	// -----------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CBattleServer_Room::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MMOGameServer_Config.ini"));
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
		// BATTLESERVER config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("BATTLESERVER")) == false)
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
		// 기본 config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CONFIG")) == false)
			return false;

		// VerCode
		if (Parser.GetValue_Int(_T("VerCode"), &m_uiVer_Code) == false)
			return false;

		// BattleNetServer의 IP
		if (Parser.GetValue_String(_T("BattleNetServerIP"), m_Master_LanClient->m_tcBattleNetServerIP) == false)
			return false;

		// MasterNetServer의 Port 셋팅.
		m_Master_LanClient->m_iBattleNetServerPort = pConfig->Port;

		// ChatNetServer의 IP
		if (Parser.GetValue_String(_T("ChatNetServerIP"), m_Master_LanClient->m_tcChatNetServerIP) == false)
			return false;

		// ChatNetServer의 Port
		if (Parser.GetValue_Int(_T("ChatNetServerPort"), &m_Master_LanClient->m_iChatNetServerPort) == false)
			return false;

		// 마스터 서버에 입장 시 들고갈 토큰
		TCHAR m_tcMasterToken[33];
		if (Parser.GetValue_String(_T("MasterEnterToken"), m_tcMasterToken) == false)
			return false;

		// 들고갈 때는 char형으로 들고가기 때문에 변환해서 가지고 있어야한다.
		int len = WideCharToMultiByte(CP_UTF8, 0, m_tcMasterToken, lstrlenW(m_tcMasterToken), 0, 0, 0, 0);
		WideCharToMultiByte(CP_UTF8, 0, m_tcMasterToken, lstrlenW(m_tcMasterToken), m_Master_LanClient->m_cMasterToken, len, 0, 0);


		





		////////////////////////////////////////////////////////
		// 마스터 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MASTERLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("MasterServerIP"), pConfig->MasterServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("MasterServerPort"), &pConfig->MasterServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("MasterClientCreateWorker"), &pConfig->MasterClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("MasterClientActiveWorker"), &pConfig->MasterClientActiveWorker) == false)
			return false;


		// Nodelay
		if (Parser.GetValue_Int(_T("MasterClientNodelay"), &pConfig->MasterClientNodelay) == false)
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

	// Start
	// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
	//
	// Parameter : 없음
	// return : 실패 시 false
	bool CBattleServer_Room::BattleServerStart()
	{
		// 1. 세션 셋팅
		m_cGameSession = new CGameSession[m_stConfig.MaxJoinUser];

		int i = 0;
		while (i < m_stConfig.MaxJoinUser)
		{
			// GameServer의 포인터 셋팅
			m_cGameSession[i].m_pParent = this;

			// 엔진에 세션 셋팅
			SetSession(&m_cGameSession[i], m_stConfig.MaxJoinUser);
			++i;
		}

		// 2. 모니터링 서버와 연결되는, 랜 클라이언트 가동
		if (m_Monitor_LanClient->ClientStart(m_stConfig.MonitorServerIP, m_stConfig.MonitorServerPort, m_stConfig.MonitorClientCreateWorker,
			m_stConfig.MonitorClientActiveWorker, m_stConfig.MonitorClientNodelay) == false)
			return false;

		// 3. 마스터 서버와 연결되는, 랜 클라이언트 가동
		if(m_Master_LanClient->ClientStart(m_stConfig.MasterServerIP, m_stConfig.MasterServerPort, m_stConfig.MasterClientCreateWorker,
			m_stConfig.MasterClientActiveWorker, m_stConfig.MasterClientNodelay) == false)
			return false;

		// 4. Battle 서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept,
			m_stConfig.Nodelay, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
			return false;		

		// 서버 오픈 로그 찍기		
		g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;

	}

	// Stop
	// 내부적으로 Stop 실행
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::BattleServerStop()
	{
		// 1. 모니터링 클라 종료
		if (m_Monitor_LanClient->GetClinetState() == true)
			m_Monitor_LanClient->ClientStop();

		// 2. 마스터 Lan 클라 종료
		if (m_Master_LanClient->GetClinetState() == true)
			m_Master_LanClient->ClientStop();

		// 3. 서버 종료
		if (GetServerState() == true)
			Stop();

		// 3. 세션 삭제
		delete[] m_cGameSession;
	}

	// 출력용 함수
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::ShowPrintf()
	{

	}



	// -----------------------
	// 패킷 후처리 함수
	// -----------------------

	// Login 패킷에 대한 인증 처리 (토큰 체크 등..)
	//
	// Parameter : DB_WORK_LOGIN*
	// return : 없음
	void CBattleServer_Room::Auth_LoginPacket_AUTH(DB_WORK_LOGIN* DBData)
	{
		// 1. ClientKey 체크
		CGameSession* NowPlayer = (CGameSession*)DBData->pPointer;

		// 다르면 이미 종료한 유저로 판단. 가능성 있는 상황
		if (NowPlayer->m_int64ClientKey != DBData->m_i64UniqueKey)
			return;


		// 2. Json데이터 파싱하기 (UTF-16)
		GenericDocument<UTF16<>> Doc;
		Doc.Parse(DBData->m_tcResponse);

		int iResult = Doc[_T("result")].GetInt();


		// 3. DB 요청 결과 확인
		// 결과가 1이 아니라면, 
		if (iResult != 1)
		{
			// 이미 종료 패킷이 간 유저인지 체크
			if (NowPlayer->m_bLogoutFlag == true)
				return;

			// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
			NowPlayer->m_bLogoutFlag = true;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, 
				L"Auth_LoginPacket_AUTH()--> DB Result Error!! AccoutnNo : %lld, Error : %d", NowPlayer->m_Int64AccountNo, iResult);

			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 2;			

			// 결과가 -10일 경우 (회원가입 자체가 안되어 있음)
			if (iResult == -10)
				InterlockedIncrement(&g_lQuery_Result_Not_Find);

			// 그 외 기타 에러일 경우
			else
				InterlockedIncrement(&g_lTempError);

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
			return;
		}


		// 4. SessionKey, ConnectToekn, Ver_Code 마샬링
		char SessionKey[64];
		char ConnectToken[32];
		UINT VerCode;

		DBData->m_pBuff->GetData(SessionKey, 64);
		DBData->m_pBuff->GetData(ConnectToken, 32);
		DBData->m_pBuff->GetData((char*)&VerCode, 4);


		// 5. 토큰키 비교
		const TCHAR* tDBToekn = Doc[_T("sessionkey")].GetString();

		char DBToken[64];
		int len = (int)_tcslen(tDBToekn);
		WideCharToMultiByte(CP_UTF8, 0, tDBToekn, (int)_tcslen(tDBToekn), DBToken, len, NULL, NULL);

		if (memcmp(DBToken, SessionKey, 64) != 0)
		{
			// 이미 종료 패킷이 간 유저인지 체크
			if (NowPlayer->m_bLogoutFlag == true)
				return;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_AUTH()--> SessionKey Error!! AccoutnNo : %lld", NowPlayer->m_Int64AccountNo);

			// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
			NowPlayer->m_bLogoutFlag = true;

			// 토큰이 다를경우 Result 3(세션키 오류)를 보낸다.
			InterlockedIncrement(&g_lTokenError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Result = 3;

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
			return;
		}


		// 6. 배틀서버 입장 토큰 비교
		// "현재" 토큰과 먼저 비교
		if (memcmp(ConnectToken, m_cConnectToken_Now, 32) != 0)
		{
			// 다르다면 "이전" 토큰과 비교
			if (memcpy(ConnectToken, m_cConnectToken_Before, 32) != 0)
			{
				// 이미 종료 패킷이 간 유저인지 체크
				if (NowPlayer->m_bLogoutFlag == true)
					return;

				// 에러 로그 찍기
				// 로그 찍기 (로그 레벨 : 에러)
				g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
					L"Auth_LoginPacket_AUTH()--> BattleEnterToken Error!! AccoutnNo : %lld", NowPlayer->m_Int64AccountNo);

				// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
				NowPlayer->m_bLogoutFlag = true;

				InterlockedIncrement(&g_lBattleEnterTokenError);

				// 그래도 다르다면 이상한 유저로 판단.
				// 배틀서버 입장 토큰이 다르다는 패킷 보낸다.
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
				BYTE Result = 3;	// 현재는 세션키 오류랑 같이 취급. 차후 수정될까?

				SendBuff->PutData((char*)&Type, 2);
				SendBuff->PutData((char*)&DBData->AccountNo, 8);
				SendBuff->PutData((char*)&Result, 1);

				NowPlayer->SendPacket(SendBuff);

				return;
			}
		}


		// 7. 버전 비교 
		if (m_uiVer_Code != VerCode)
		{
			// 이미 종료 패킷이 간 유저인지 체크
			if (NowPlayer->m_bLogoutFlag == true)
				return;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_AUTH()--> VerCode Error!! AccoutnNo : %lld, Code : %d", NowPlayer->m_Int64AccountNo, VerCode);

			// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
			NowPlayer->m_bLogoutFlag = true;

			// 버전이 다를경우 Result 5(버전 오류)를 보낸다.
			InterlockedIncrement(&g_lVerError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Result = 5;

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
			return;
		}


		// 8. 닉네임 복사
		const TCHAR* TempNick = Doc[_T("nick")].GetString();
		StringCchCopy(NowPlayer->m_tcNickName, _Mycountof(NowPlayer->m_tcNickName), TempNick);


		// 9. Contents 정보도 셋팅됐다면 정상 패킷 보냄
		NowPlayer->m_lLoginHTTPCount++;

		if (NowPlayer->m_lLoginHTTPCount == 2)
		{
			// 로그인 패킷 처리 Flag 변경
			NowPlayer->m_bLoginFlag = true;

			// 정상 응답 패킷 보냄
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Result = 1;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
		}
	}

	// Login 패킷에 대한 Contents 정보 가져오기
	//
	// Parameter : DB_WORK_LOGIN*
	// return : 없음
	void CBattleServer_Room::Auth_LoginPacket_Info(DB_WORK_LOGIN_CONTENTS* DBData)
	{
		// 1. ClientKey 체크
		CGameSession* NowPlayer = (CGameSession*)DBData->pPointer;

		// 다르면 이미 종료한 유저로 판단. 가능성 있는 상황
		if (NowPlayer->m_int64ClientKey != DBData->m_i64UniqueKey)
			return;


		// 2. Json데이터 파싱하기 (UTF-16)
		GenericDocument<UTF16<>> Doc;
		Doc.Parse(DBData->m_tcResponse);

		int iResult = Doc[_T("result")].GetInt();


		// 3. DB 요청 결과 확인
		// 결과가 1이 아니라면, 
		if (iResult != 1)
		{
			// 이미 종료 패킷이 간 유저인지 체크
			if (NowPlayer->m_bLogoutFlag == true)
				return;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_Info()--> DB Result Error!! AccoutnNo : %lld, Error : %d", NowPlayer->m_Int64AccountNo, iResult);

			// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
			NowPlayer->m_bLogoutFlag = true;

			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 2;

			// 결과가 -10일 경우 (회원가입 자체가 안되어 있음)
			if (iResult == -10)
				InterlockedIncrement(&g_lQuery_Result_Not_Find);

			// 그 외 기타 에러일 경우
			else
				InterlockedIncrement(&g_lTempError);

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
			return;
		}

		// 4. 여기까지 오면 정상으로 패킷 찾아온 것.

		// 정보 복사
		NowPlayer->m_iRecord_PlayCount = Doc[_T("playcount")].GetInt();	// 플레이 횟수
		NowPlayer->m_iRecord_PlayTime = Doc[_T("playtime")].GetInt();	// 플레이 시간 초단위
		NowPlayer->m_iRecord_Kill = Doc[_T("kill")].GetInt();			// 죽인 횟수
		NowPlayer->m_iRecord_Die = Doc[_T("die")].GetInt();				// 죽은 횟수
		NowPlayer->m_iRecord_Win = Doc[_T("win")].GetInt();				// 최종승리 횟수

		// 5. 인증용 로그인 HTTP 요청이 완료되었다면 정상 패킷 보냄
		NowPlayer->m_lLoginHTTPCount++;

		if (NowPlayer->m_lLoginHTTPCount == 2)
		{
			// 로그인 패킷 처리 Flag 변경
			NowPlayer->m_bLoginFlag = true;

			// 정상 응답 패킷 보냄
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Result = 1;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
		}

	}




	// -----------------------
	// AccountNo 자료구조 관리 함수
	// -----------------------

	// AccountNo 자료구조에 유저를 추가하는 함수
	//
	// Parameter : AccountNo, CGameSession*
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CBattleServer_Room::InsertAccountNoFunc(INT64 AccountNo, CGameSession* InsertPlayer)
	{

		AcquireSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 락

		// 1. Insert
		auto ret = m_AccountNo_Umap.insert(make_pair(AccountNo, InsertPlayer));

		ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 언락

		// 2. 중복키일 시 false 리턴
		if (ret.second == false)
			return false;

		return true;
	}

	// AccountNo 자료구조에서 유저 검색 후, 해당 유저에게 Disconenct 하는 함수
	//
	// Parameter : AccountNo
	// return : 없음
	void CBattleServer_Room::DisconnectAccountNoFunc(INT64 AccountNo)
	{
		AcquireSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 락

		// 1. 검색
		auto ret = m_AccountNo_Umap.find(AccountNo);

		// 2. 없는 유저일 시 그냥 리턴.
		// 여기 락 걸고 들어오기 전에, 이미 유저가 종료했을 수도 있기 때문에
		// 정상적인 상황으로 판단.
		if (ret == m_AccountNo_Umap.end())
		{
			ReleaseSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락
			return;
		}

		// 3. 있는 유저라면 Disconnect
		ret->second->Disconnect();

		ReleaseSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락
	}


	// AccountNo 자료구조에서 유저를 제거하는 함수
	//
	// Parameter : AccountNo
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CBattleServer_Room::EraseAccountNoFunc(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 락

		// 1. 검색
		auto ret = m_AccountNo_Umap.find(AccountNo);

		// 2. 없는 유저일 시 false 리턴
		if (ret == m_AccountNo_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 언락
			return false;
		}

		// 3. 있는 유저라면 Erase
		m_AccountNo_Umap.erase(ret);

		ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락

		return true;
	}






	// ---------------------------------
	// Auth모드의 방 관리 자료구조 변수
	// ---------------------------------

	// 방을 Room 자료구조에 Insert하는 함수
	//
	// Parameter : RoomNo, stRoom*
	// return : 성공 시 true
	//		  : 실패(중복 키) 시 false	
	bool CBattleServer_Room::InsertRoomFunc(int RoomNo, stRoom* InsertRoom)
	{
		AcquireSRWLockExclusive(&m_Room_Umap_srwl);		// ----- Room 자료구조 Exclusive 락 

		// 1. insert
		auto ret = m_Room_Umap.insert(make_pair(RoomNo, InsertRoom));

		ReleaseSRWLockExclusive(&m_Room_Umap_srwl);		// ----- Room 자료구조 Exclusive 언락 

		// 2. 중복키라면 false 리턴
		if (ret.second == false)
			return false;

		return true;
	}






	// -----------------------
	// 가상함수
	// -----------------------

	// AuthThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnAuth_Update()
	{
		// ------------------- HTTP 통신 후, 후처리
		// 1. Q 사이즈 확인
		int iQSize = m_shDB_Communicate.m_pDB_ReadComplete_Queue->GetInNode();

		// 한 프레임에 최대 m_iHTTP_MAX개의 후처리.
		if (iQSize > m_stConst.m_iHTTP_MAX)
			iQSize = m_stConst.m_iHTTP_MAX;

		// 2. 있으면 로직 진행
		DB_WORK* DBData;
		while (iQSize > 0)
		{
			// 일감 디큐		
			if (m_shDB_Communicate.m_pDB_ReadComplete_Queue->Dequeue(DBData) == -1)
				g_BattleServer_Room_Dump->Crash();

			try
			{
				// 일감 타입에 따라 일감 처리
				switch (DBData->m_wWorkType)
				{
					// 로그인 패킷에 대한 인증처리
				case eu_DB_READ_TYPE::eu_LOGIN_AUTH:
					Auth_LoginPacket_AUTH((DB_WORK_LOGIN*)DBData);
					break;

					// 로그인 패킷에 대한 정보 얻어오기 처리
				case eu_DB_READ_TYPE::eu_LOGIN_INFO:
					Auth_LoginPacket_Info((DB_WORK_LOGIN_CONTENTS*)DBData);
					break;

					// 없는 일감 타입이면 에러.
				default:
					TCHAR str[200];
					StringCchPrintf(str, 200, _T("OnAuth_Update(). Type Error. Type : %d"), DBData->m_wWorkType);

					throw CException(str);
				}

			}
			catch (CException& exc)
			{
				// 로그 찍기 (로그 레벨 : 에러)
				g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
					(TCHAR*)exc.GetExceptionText());

				// 덤프
				g_BattleServer_Room_Dump->Crash();
			}

			// 모드에 따라 직렬화 버퍼 반환
			if (DBData->m_wWorkType == eu_DB_READ_TYPE::eu_LOGIN_AUTH)
			{
				// DB_WORK 안의 직렬화 버퍼 반환
				CProtocolBuff_Net::Free(DBData->m_pBuff);
			}

			// DB_WORK 반환
			m_shDB_Communicate.m_pDB_Work_Pool->Free(DBData);

			--iQSize;
		}


		// ------------------- 방 상태 처리

		// 한 프레임에 최대 m_iLoopRoomModeChange개의 방 모드 변경
		int ModeChangeCount = m_stConst.m_iLoopRoomModeChange;
		DWORD CmpTime = timeGetTime();

		AcquireSRWLockShared(&m_Room_Umap_srwl);	// ----- Room umap Shared 락

		auto itor_Now = m_Room_Umap.begin();
		auto itor_End = m_Room_Umap.end();

		// 방 자료구조를 처음부터 끝까지 순회
		while (itor_Now != itor_End)
		{
			// 만약, 이번 프레임에, 지정한 만큼의 방 모드 변경이 발생했다면 그만한다.
			if (ModeChangeCount == 0)
				break;

			stRoom* NowRoom = itor_Now->second;

			// 해당 방이 준비방일 경우
			if (NowRoom->m_iRoomState == eu_ROOM_STATE::READY_ROOM)
			{
				// 카운트 다운이 완료되었는지 체크
				if ( (NowRoom->m_dwCountDown + m_stConst.m_iCountDownMSec) <= CmpTime )
				{
					// 1. 생존한 유저 수 갱신
					if (NowRoom->m_iJoinUserCount != NowRoom->m_JoinUser_Vector.size())
						g_BattleServer_Room_Dump->Crash();

					NowRoom->m_iAliveUserCount = NowRoom->m_iJoinUserCount;


					// 2. 룸 내 모든 유저의 생존 플래그 변경
					// false가 리턴될 수 있음. 자료구조 내에 유저가 0명일 수 있기 때문에.
					// 때문에 리턴값 안받는다.
					NowRoom->AliveFalg_True();


					// 3. 방 안의 모든 유저에게, 배틀서버 대기방 플레이 시작 패킷 보내기
					CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

					WORD Type = en_PACKET_CS_GAME_RES_PLAY_START;

					SendBuff->PutData((char*)&Type, 2);
					SendBuff->PutData((char*)&NowRoom->m_iRoomNo, 4);

					// 여기서는 false가 리턴될 수 있음(자료구조 내에 유저가 0명일 수 있음)
					// 카운트다운이 끝나기 전에 모든 유저가 나갈 가능성.
					// 그래서 리턴값 안받는다.
					NowRoom->SendPacket_BroadCast(SendBuff);


					// 4. 방 상태를 Play로 변경
					NowRoom->m_iRoomState = eu_ROOM_STATE::PLAY_ROOM;


					// 5. 모든 유저를 AUTH_TO_GAME으로 변경
					NowRoom->ModeChange();


					// 6. 이번 프레임에서 방 모드 변경 남은 카운트 감소
					--ModeChangeCount;					
				}
			}

			++itor_Now;
		}

		ReleaseSRWLockShared(&m_Room_Umap_srwl);	// ----- Room umap Shared 언락





		// ------------------- 방생성
		// !! 대기방 생성 조건 !!
		// - [현재 게임 내에 만들어진 총 방 수 < 최대 존재할 수 있는 방 수] 만족
		// - [현재 대기방 수 < 최대 존재할 수 있는 대기방 수] 만족
			
		// 한 프레임에 최대 m_iLoopCreateRoomCount개의 방 생성
		int LoopCount = m_stConst.m_iLoopCreateRoomCount;

		while (LoopCount > 0)
		{
			// 1. 현재 만들어진 총 방수 체크
			if (m_lNowTotalRoomCount < m_stConst.m_lMaxTotalRoomCount)
			{
				// 2. 현재 대기방 수 체크
				if (m_lNowWaitRoomCount < m_stConst.m_lMaxWaitRoomCount)
				{
					// 여기까지 오면 방생성 조건 만족					

					// 1) 현재 총 방 수 증가
					InterlockedIncrement(&m_lNowTotalRoomCount);


					// 2) 대기방 수 증가.
					// Auth에서만 접근하는 변수이기 때문에 인터락 필요 없음.
					++m_lNowWaitRoomCount;


					// 3) 방 Alloc
					stRoom* NowRoom = m_Room_Pool->Alloc();

					// 만약, 방 안에 유저 수가 0명이 아니면 Crash
					if (NowRoom->m_iJoinUserCount != 0 || NowRoom->m_JoinUser_Vector.size() != 0)
						g_BattleServer_Room_Dump->Crash();


					// 4) 셋팅
					LONG RoomNo = InterlockedIncrement(&m_lGlobal_RoomNo);
					NowRoom->m_iRoomNo = RoomNo;
					NowRoom->m_iRoomState = eu_ROOM_STATE::WAIT_ROOM;
					NowRoom->m_iJoinUserCount = 0;
					NowRoom->m_iAliveUserCount = 0;
					NowRoom->m_dwCountDown = 0;		
					NowRoom->m_bGameEndFlag = false;
					NowRoom->m_dwGameEndMSec = 0;
					NowRoom->m_bShutdownFlag = false;

					// 토큰 셋팅				
					WORD Index = rand() % 64;	// 0 ~ 63 중 인덱스 골라내기				
					memcpy_s(NowRoom->m_cEnterToken, 32, m_cRoomEnterToken[Index], 32);


					// 5) 방 관리 자료구조에 추가
					if (InsertRoomFunc(RoomNo, NowRoom) == false)
						g_BattleServer_Room_Dump->Crash();


					// 6) 마스터 서버에게 [신규 대기방 생성 알림] 패킷 보냄
					// 랜 클라를 통해 보낸다.
					m_Master_LanClient->Packet_NewRoomCreate_Req(m_iServerNo, NowRoom);

					--LoopCount;
				}

				// 현재 대기방 수가 이미 max라면 방 안만들고 나간다.
				else
					break;
			}

			// 현재 만들어진 총 방수가 이미 max라면 방 안만들고 나간다.
			else
				break;			
		}

		


		// ------------------- 토큰 재발급
		// 마스터 랜 클라이언트의 함수 호출
		m_Master_LanClient->Packet_TokenChange_Req();	
	}

	// GameThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnGame_Update()
	{
		// ------------------- 방 체크

		// 이번 프레임에 삭제될 방 받아두기.
		// 한 프레임에 최대 100개의 방 삭제 가능
		int DeleteRoomNo[100];
		int Index = 0;

		AcquireSRWLockShared(&m_Room_Umap_srwl);	// ----- Room Umap Shared 락
		
		auto itor_Now = m_Room_Umap.begin();
		auto itor_End = m_Room_Umap.end();

		// Step 1. 삭제될 방 체크
		// Step 2. 아직 유저가 있다면, 게임 종료된 방 처리 	
		// Setp 3. 게임 종료된 방이 아니고 삭제될 방도 아니라면, 게임 종료 체크

		// 모든 방을 순회하며, PLAY 모드인 방에 한해 작업 진행
		while (itor_Now != itor_End)
		{
			// 방 모드 체크. 
			if (itor_Now->second->m_iRoomState == eu_ROOM_STATE::PLAY_ROOM)
			{
				// 여기까지 오면 나만 접근하는 방.
				stRoom* NowRoom = itor_Now->second;

				// Step 1. 삭제될 방 체크
				// 접속한 인원 수가 0명인 방. (생존 수 아님)
				// 게임 종료 후, 모든 유저를 쫒아내는 작업까지 완료된 방.
				if (NowRoom->m_iJoinUserCount == 0)
				{
					if (Index < 100)
					{
						DeleteRoomNo[Index] = NowRoom->m_iRoomNo;
						++Index;
					}
				}

				// Step 2. 아직 유저가 있다면, 게임 종료된 방 처리 
				// 셧다운을 날린 방은, 다음 프레임에 Step 1. 에서 걸리길 기대한다.
				else if (NowRoom->m_bGameEndFlag == true)
				{
					// 셧다운을 하지 않았을 경우에만 로직 진행.
					// 이미 이전 프레임에 셧다운을 날렸는데, 아직 모든 유저가 종료되지 않아
					// 또 이 로직을 탈 가능성이 있기 때문에.
					if (NowRoom->m_bShutdownFlag == false)
					{
						// 삭제 시간이 되었는지 체크
						if ((NowRoom->m_dwGameEndMSec + m_stConst.m_iRoomCloseDelay) <= timeGetTime())
						{		
							// 셧다운 날렸다는 플래그 변경.
							NowRoom->m_bShutdownFlag = true;

							// 아직 방에 있는 유저에게 Shutdown
							NowRoom->Shutdown_All();
						}
					}
				}				

				// Setp 3. 게임 종료된 방이 아니고 삭제될 방도 아니라면, 게임 종료 체크
				// 생존자가 1명이면 게임 종료.
				else if (NowRoom->m_iAliveUserCount == 1)
				{
					// 게임 종료 패킷 보내기
					// - 생존자 1명에겐 승리 패킷
					// - 나머지 접속자들에겐 패배 패킷
					NowRoom->GameOver();

					// 게임 방 종료 플래그 변경
					NowRoom->m_bGameEndFlag = true;

					// 현 시점의 시간 저장
					NowRoom->m_dwGameEndMSec = timeGetTime();

					// 각 유저마자 게임 시작 시간과 
				}
			}

			++itor_End;
		}

		ReleaseSRWLockShared(&m_Room_Umap_srwl);	// ----- Room Umap Shared 언락




		// ------------------- 방 삭제

		// Step 4. 방 삭제

		// Index가 0보다 크다면, 삭제할 방이 있는 것. 		
		if (Index > 0)
		{
			int Index_B = 0;

			AcquireSRWLockExclusive(&m_Room_Umap_srwl);	// ----- Room Umap Exclusive 락		

			while (Index_B < Index)
			{
				// 1. 검색
				auto FindRoom = m_Room_Umap.find(DeleteRoomNo[Index_B]);

				// 여기서 없으면 안됨. 방 삭제는 이 로직에서 밖에 안하기 때문에
				if (FindRoom == m_Room_Umap.end())
					g_BattleServer_Room_Dump->Crash();

				// 2. stRoom* 해제
				m_Room_Pool->Free(FindRoom->second);

				// 3. Erase
				m_Room_Umap.erase(FindRoom);

				++Index_B;
			}

			ReleaseSRWLockExclusive(&m_Room_Umap_srwl);	// ----- Room Umap Exclusive 언락
		}
	}

	// 새로운 유저 접속 시, Auth에서 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CBattleServer_Room::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CBattleServer_Room::OnError(int error, const TCHAR* errorStr)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		g_BattleServer_RoomLog->LogSave(L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
			errorStr, error);
	}

	   	  

	// -----------------
	// 생성자와 소멸자
	// -----------------
	CBattleServer_Room::CBattleServer_Room()
		:CMMOServer()
	{	
		srand((UINT)time(NULL));

		m_lNowWaitRoomCount = 0;
		m_lNowTotalRoomCount = 0;
		m_lGlobal_RoomNo = 0;
		m_iServerNo = -1;

		// 방 입장 토큰 만들어두기
		for (int i = 0; i < 64; ++i)
		{
			for (int h = 0; h < 32; ++h)
			{
				m_cRoomEnterToken[i][h] = (rand() % 128) + 1;
			}
		}	

		// 배틀서버 최초 입장 토큰 만들어 두기
		for (int i = 0; i < 32; ++i)
		{
			m_cConnectToken_Now[i] = (rand() % 128) + 1;
		}

		// 모니터링 서버와 통신하기 위한 LanClient 동적할당
		m_Monitor_LanClient = new CGame_MinitorClient;
		m_Monitor_LanClient->ParentSet(this);

		// 마스터 서버와 통신하기 위한 LanClient 동적할당
		m_Master_LanClient = new CBattle_Master_LanClient;
		m_Master_LanClient->ParentSet(this);


		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			g_BattleServer_Room_Dump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		g_BattleServer_RoomLog->SetDirectory(L"CBattleServer_Room");
		g_BattleServer_RoomLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);
				


		// TLS 동적할당
		m_Room_Pool = new CMemoryPoolTLS<stRoom>(0, false);		

		// reserve 셋팅.
		m_AccountNo_Umap.reserve(m_stConfig.MaxJoinUser);
		m_Room_Umap.reserve(m_stConst.m_lMaxTotalRoomCount);

		//SRW락 초기화
		InitializeSRWLock(&m_AccountNo_Umap_srwl);
		InitializeSRWLock(&m_Room_Umap_srwl);
	}

	CBattleServer_Room::~CBattleServer_Room()
	{
		// 모니터링 서버와 통신하는 LanClient 동적해제
		delete m_Monitor_LanClient;

		// 마스터 서버와 통신하는 LanClient 동적 해제
		delete m_Master_LanClient;

		// TLS 동적 해제
		delete m_Room_Pool;
	}

}


// ----------------------------------------
// 
// 마스터 서버와 연결되는 LanClient
//
// ----------------------------------------
namespace Library_Jingyu
{

	// -----------------------
	// 패킷 처리 함수
	// -----------------------

	// 마스터에게 보낸 로그인 요청 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_Login_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. Connect 상태인지 확인
		if (SessionID != m_ullSessionID)
			g_BattleServer_Room_Dump->Crash();

		// 2. 마샬링
		int ServerNo;
		Payload->GetData((char*)&ServerNo, 4);

		// 3. 서버 번호 셋팅
		// 만약, 배틀 Net 서버의 번호가 -1(초기 값)이 아니라면 2번 로그인 요청?을 보낸 것.
		if(m_BattleServer_this->m_iServerNo != -1)
			g_BattleServer_Room_Dump->Crash();

		m_BattleServer_this->m_iServerNo = ServerNo;
	}

	// 신규 대기방 생성 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_NewRoomCreate_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}

	// 토큰 재발행 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_TokenChange_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}

	// 방 닫힘 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomClose_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}

	// 방 퇴장 응답
	//
	// Parameter : RoomNo, AccountNo
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomLeave_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}




	// -----------------------
	// Battle Net 서버가 호출하는 함수
	// -----------------------

	// 마스터에게, 신규 대기방 생성 패킷 보내기
	//
	// Parameter : 배틀서버 No, stRoom*
	// return : 없음
	void CBattle_Master_LanClient::Packet_NewRoomCreate_Req(int BattleServerNo, CBattleServer_Room::stRoom* NewRoom)
	{
		// 1. 패킷 만들기
		WORD Type = en_PACKET_BAT_MAS_REQ_CREATED_ROOM;
		UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&BattleServerNo, 4);
		SendBuff->PutData((char*)&NewRoom->m_iRoomNo, 4);
		SendBuff->PutData((char*)&NewRoom->m_iMaxJoinCount, 4);
		SendBuff->PutData(NewRoom->m_cEnterToken, 32);

		SendBuff->PutData((char*)&ReqSequence, 4);

		// 2. 토큰 발급 시간 갱신하기
		m_dwTokenSendTime = timeGetTime();

		// 3. 마스터에게 Send하기
		SendPacket(m_ullSessionID, SendBuff);
	}
	
	// 토큰 재발급 함수
	//
	// Parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::Packet_TokenChange_Req()
	{
		// 마스터와 연결이 끊긴 상태라면 패킷 안보낸다.
		if (m_ullSessionID == 0xffffffffffffffff)
			return;


		// 토큰 재발급 시점이 되었는지 확인하기.
		DWORD NowTime = timeGetTime();

		if ((m_dwTokenSendTime + m_BattleServer_this->m_stConst.m_iTokenChangeSlice) <= NowTime)
		{
			// 1. 토큰 발급 시간 갱신
			m_dwTokenSendTime = NowTime;

			// 2. "현재" 토큰을 "이전" 토큰에 복사
			memcpy_s(m_BattleServer_this->m_cConnectToken_Now, 32, m_BattleServer_this->m_cConnectToken_Before, 32);

			// 3. "현재" 토큰 다시 만들기
			int i = 0;
			while (i < 32)
			{
				m_BattleServer_this->m_cConnectToken_Now[i] = (rand() % 128) + 1;

				++i;
			}

			// 4. 토큰 재발급 패킷 만들기
			WORD Type = en_PACKET_BAT_MAS_REQ_CONNECT_TOKEN;
			UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

			CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData(m_BattleServer_this->m_cConnectToken_Now, 32);
			SendBuff->PutData((char*)&ReqSequence, 4);


			// 5. 마스터에게 패킷 보내기
			SendPacket(m_ullSessionID, SendBuff);		
		}
	}

	// 마스터에게, 방 닫힘 패킷 보내기
	//
	// Parameter : RoomNo
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomClose_Req(int RoomNo)
	{
		// 마스터와 연결이 끊긴 상태라면 패킷 안보낸다.
		if (m_ullSessionID == 0xffffffffffffffff)
			return;

		// 1. 패킷 만들기
		WORD Type = en_PACKET_BAT_MAS_REQ_CLOSED_ROOM;
		UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&ReqSequence, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}

	// 마스터에게, 방 퇴장 패킷 보내기
	//
	// Parameter : RoomNo, AccountNo
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomLeave_Req(int RoomNo, INT64 AccountNo)
	{
		// 마스터와 연결이 끊긴 상태라면 패킷 안보낸다.
		if (m_ullSessionID == 0xffffffffffffffff)
			return;

		// 1. 패킷 만들기
		WORD Type = en_PACKET_BAT_MAS_REQ_LEFT_USER;
		UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&ReqSequence, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}



	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 시작 함수
	// 내부적으로, 상속받은 CLanClient의 Start호출.
	//
	// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
	// return : 성공 시 true , 실패 시 falsel 
	bool CBattle_Master_LanClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 마스터 서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
			return false;

		return true;
	}

	// 종료 함수
	// 내부적으로, 상속받은 CLanClient의 Stop호출.
	// 추가로, 리소스 해제 등
	//
	// Parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::ClientStop()
	{
		// 마스터 서버와 연결 종료
		Stop();
	}

	// 배틀서버의 this를 입력받는 함수
	// 
	// Parameter : 배틀 서버의 this
	// return : 없음
	void CBattle_Master_LanClient::ParentSet(CBattleServer_Room* ChatThis)
	{
		m_BattleServer_this = ChatThis;
	}




	// -----------------------
	// 순수 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CBattle_Master_LanClient::OnConnect(ULONGLONG SessionID)
	{
		m_ullSessionID = SessionID;

		// 마스터 서버(Lan)로 로그인 패킷 보냄
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_BAT_MAS_REQ_SERVER_ON;

		SendBuff->PutData((char*)&Type, 2);

		SendBuff->PutData((char*)m_tcBattleNetServerIP, 32);
		SendBuff->PutData((char*)&m_iBattleNetServerPort, 2);
		SendBuff->PutData(m_BattleServer_this->m_cConnectToken_Now, 32);
		SendBuff->PutData(m_cMasterToken, 32);

		SendBuff->PutData((char*)m_tcChatNetServerIP, 32);
		SendBuff->PutData((char*)&m_iChatNetServerPort, 2);


		SendPacket(SessionID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CBattle_Master_LanClient::OnDisconnect(ULONGLONG SessionID)
	{
		// SessionID를 초기값으로 변경
		m_ullSessionID = 0xffffffffffffffff;

		// 배틀 Net 서버의 ServerNo를 초기값으로 변경
		m_BattleServer_this->m_iServerNo = -1;
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// Type 분리
		WORD Type;
		Payload->GetData((char*)&Type, 2);


		// 타입에 따라 분기 처리
		try
		{
			switch (Type)
			{
				// 로그인 요청에 대한 응답
			case en_PACKET_BAT_MAS_RES_SERVER_ON:
				Packet_Login_Res(SessionID, Payload);

				// 신규 대기방 생성 응답
			case en_PACKET_BAT_MAS_RES_CREATED_ROOM:
				Packet_NewRoomCreate_Res(SessionID, Payload);

				// 토큰 재발행 응답
			case en_PACKET_BAT_MAS_RES_CONNECT_TOKEN:
				Packet_TokenChange_Res(SessionID, Payload);

				// 방 닫힘 응답
			case en_PACKET_BAT_MAS_RES_CLOSED_ROOM:
				Packet_RoomClose_Res(SessionID, Payload);

				// 방 퇴장 응답
			case en_PACKET_BAT_MAS_RES_LEFT_USER:
				Packet_RoomLeave_Res(SessionID, Payload);

				break;
			default:
				break;
			}

		}
		catch (CException& exc)
		{

		}
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CBattle_Master_LanClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CBattle_Master_LanClient::OnError(int error, const TCHAR* errorStr)
	{}






	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CBattle_Master_LanClient::CBattle_Master_LanClient()
	{
		// SessionID 초기화
		m_ullSessionID = 0xffffffffffffffff;

		// Req시퀀스 초기화
		uiReqSequence = 0;
	}

	// 소멸자
	CBattle_Master_LanClient::~CBattle_Master_LanClient()
	{
		// 아직 연결이 되어있으면, 연결 해제
		if (GetClinetState() == true)
			ClientStop();
	}
}


// ---------------
// CGame_MinitorClient
// CLanClienet를 상속받는 모니터링 클라
// ---------------
namespace Library_Jingyu
{
	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	CGame_MinitorClient::CGame_MinitorClient()
		:CLanClient()
	{
		// 모니터링 서버 정보전송 스레드를 종료시킬 이벤트 생성
		//
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		m_hMonitorThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	CGame_MinitorClient::~CGame_MinitorClient()
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
	bool CGame_MinitorClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 모니터링 서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
			return false;

		// 모니터링 서버로 정보 전송할 스레드 생성
		m_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);

		return true;
	}

	// 종료 함수
	// 내부적으로, 상속받은 CLanClient의 Stop호출.
	// 추가로, 리소스 해제 등
	//
	// Parameter : 없음
	// return : 없음
	void CGame_MinitorClient::ClientStop()
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

	// 배틀서버의 this를 입력받는 함수
	// 
	// Parameter : 배틀 서버의 this
	// return : 없음
	void CGame_MinitorClient::ParentSet(CBattleServer_Room* ChatThis)
	{
		m_BattleServer_this = ChatThis;
	}




	// -----------------------
	// 내부에서만 사용하는 기능 함수
	// -----------------------

	// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
	UINT	WINAPI CGame_MinitorClient::MonitorThread(LPVOID lParam)
	{
		// this 받아두기
		CGame_MinitorClient* g_This = (CGame_MinitorClient*)lParam;

		// 종료 신호 이벤트 받아두기
		HANDLE* hEvent = &g_This->m_hMonitorThreadExitEvent;

		// CPU 사용율 체크 클래스 (채팅서버 소프트웨어)
		CCpuUsage_Process CProcessCPU;

		// CPU 사용율 체크 클래스 (하드웨어)
		CCpuUsage_Processor CProcessorCPU;

		// PDH용 클래스
		CPDH	CPdh;

		while (1)
		{
			// 1초에 한번 깨어나서 정보를 보낸다.
			DWORD Check = WaitForSingleObject(*hEvent, 1000);

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

			// 프로세서, 프로세스 CPU 사용율, PDH 정보 갱신
			CProcessorCPU.UpdateCpuTime();
			CProcessCPU.UpdateCpuTime();
			CPdh.SetInfo();

			// ----------------------------------
			// 하드웨어 정보 보내기 (프로세서)
			// ----------------------------------
			int TimeStamp = (int)(time(NULL));

			// 1. 하드웨어 CPU 사용률 전체
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL, (int)CProcessorCPU.ProcessorTotal(), TimeStamp);

			// 2. 하드웨어 사용가능 메모리 (MByte)
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY, (int)CPdh.Get_AVA_Mem(), TimeStamp);

			// 3. 하드웨어 이더넷 수신 바이트 (KByte)
			int iData = (int)(CPdh.Get_Net_Recv() / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV, iData, TimeStamp);

			// 4. 하드웨어 이더넷 송신 바이트 (KByte)
			iData = (int)(CPdh.Get_Net_Send() / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND, iData, TimeStamp);

			// 5. 하드웨어 논페이지 메모리 사용량 (MByte)
			iData = (int)(CPdh.Get_NonPaged_Mem() / 1024 / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY, iData, TimeStamp);



			// ----------------------------------
			// 배틀서버 정보 보내기
			// ----------------------------------

			// 배틀서버가 On일 경우, 게임서버 정보 보낸다.
			if (g_This->m_BattleServer_this->GetServerState() == true)
			{
				// 1. 배틀서버 ON		
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, TRUE, TimeStamp);

				// 2. 배틀서버 CPU 사용률 (커널 + 유저)
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_CPU, (int)CProcessCPU.ProcessTotal(), TimeStamp);

				// 3. 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
				int Data = (int)(CPdh.Get_UserCommit() / 1024 / 1024);
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, Data, TimeStamp);

				// 4. 배틀서버 패킷풀 사용량
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, g_lAllocNodeCount + g_lAllocNodeCount_Lan, TimeStamp);

				// 5. Auth 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, g_lShowAuthFPS, TimeStamp);

				// 6. Game 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, g_lShowGameFPS, TimeStamp);

				// 7. 배틀서버 접속 세션전체
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, (int)g_This->m_BattleServer_this->GetClientCount(), TimeStamp);

				// 8. Auth 스레드 모드 인원
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH, g_lAuthModeUserCount, TimeStamp);

				// 9. Game 스레드 모드 인원
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME, g_lGameModeUserCount, TimeStamp);

				// 10. 대기방 수

				// 11. 플레이 방 수
			}

		}

		return 0;
	}

	// 모니터링 서버로 데이터 전송
	//
	// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
	// return : 없음
	void CGame_MinitorClient::InfoSend(BYTE DataType, int DataValue, int TimeStamp)
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
	void CGame_MinitorClient::OnConnect(ULONGLONG SessionID)
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
	void CGame_MinitorClient::OnDisconnect(ULONGLONG SessionID)
	{}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, CProtocolBuff_Lan*
	// return : 없음
	void CGame_MinitorClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CGame_MinitorClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGame_MinitorClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGame_MinitorClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CGame_MinitorClient::OnError(int error, const TCHAR* errorStr)
	{}
}