#ifndef __BATTLESERVER_ROOM_VERSION_H__
#define __BATTLESERVER_ROOM_VERSION_H__

#include "NetworkLib/NetworkLib_MMOServer.h"
#include "NetworkLib/NetworkLib_LanClinet.h"
#include "NetworkLib/NetworkLib_LanServer.h"

#include "Http_Exchange/HTTP_Exchange.h"
#include "shDB_Communicate.h"

#include <unordered_set>
#include <unordered_map>

using namespace std;

// ----------------------------------------
// 
// MMOServer를 이용한 배틀 서버. 룸 버전
//
// ----------------------------------------
namespace Library_Jingyu
{
	class CBattleServer_Room :public CMMOServer
	{
		friend class CGame_MinitorClient;
		friend class CBattle_Master_LanClient;
		friend class CBattle_Chat_LanServer;		



		// -----------------------
		// 이너 클래스
		// -----------------------

		// 아이템 타입
		enum eu_ITEM_TYPE
		{
			CARTRIDGE = 0,	// 탄창
			HELMET = 1,		// 헬멧
			MEDKIT = 2		// 메드킷
		};

		// 레드존 타입
		enum eu_REDZONE_TYPE
		{
			LEFT = 0,	// 왼쪽 레드존
			RIGHT,		// 오른쪽
			TOP,		// 위
			BOTTOM		// 아래
		};

		// CMMOServer의 cSession을 상속받는 세션 클래스
		class CGameSession :public CMMOServer::cSession
		{
			friend class CBattleServer_Room;	

			// -----------------------
			// 멤버 변수
			// -----------------------

			// 회원 번호
			INT64 m_Int64AccountNo;

			// 닉네임
			TCHAR m_tcNickName[20];

			// 세션키
			char m_cSessionKey[64];

			// 유저의 ClientKey
			// 매칭서버가 발급.
			// 유저 모두에게 고유
			INT64 m_int64ClientKey;

			// 해당 유저가 접속 중인 룸 번호
			int m_iRoomNo;

			// CBattleServer_Room의 포인터 (부모)
			CBattleServer_Room* m_pParent;		

			// AccountNo 자료구조에 들어감 체크 플래그
			// true면 들어간 상태
			bool m_bStructFlag;

			// 로그인 패킷 처리 플래그
			// true면 세션키, 컨텐츠 가져오기 까지 정상 처리됨.
			bool m_bLoginFlag;

			// 곧 종료될 유저 체크 플래그
			// true면 곧 종료될 유저.
			// 로그인 HTTP 처리 중, 이미 종료 패킷을 보낸 유저는 이 플래그를 true로 변경한다.
			bool m_bLogoutFlag;

			// 생존 플래그
			// true면 살아있는 유저.
			// Game모드에서만 유효하다.
			bool m_bAliveFlag;

			// 로그인 시 필요한 정보가 모두 셋팅되었나 체크하는 Flag
			// 총 2개의 HTTP가 호출. 이 값이 2라면 모두 셋팅된 것.
			LONG m_lLoginHTTPCount;


			// -----------------------
			// 전적 정보
			// -----------------------
			int		m_iRecord_PlayCount;	// 플레이 횟수
			int		m_iRecord_PlayTime;		// 플레이 시간 초단위
			int		m_iRecord_Kill;			// 죽인 횟수
			int		m_iRecord_Die;			// 죽은 횟수
			int		m_iRecord_Win;			// 최종승리 횟수

			// 실제 게임이 시작된 시간. 밀리세컨드 단위.
			// DB에 저장하거나 유저에게 보내줄 때는 초단위로 변환 후 보낸다.
			DWORD m_dwGameStartTime;		

			// 마지막 전적이 저장되었는지 체크하는 플래그
			// 강제 종료시에도 저장해야 하기 때문에, LastDBWriteFlag를 하나 둔다.
			bool m_bLastDBWriteFlag;
			


			// -----------------------
			// 컨텐츠 정보
			// -----------------------
			float	m_fPosX;
			float	m_fPosY;

			int		m_iHP;

			// 총알 수
			int		m_iBullet;

			// 탄창 수
			int		m_iCartridge;
					   
			// 헬멧 수
			int m_iHelmetCount;


			// -----------------------
			// 공격 타임 체크
			// -----------------------

			// Fire_1(총 발사)를 시작한 시간.
			// 이 시간부터 100m/s이내에 데미지 패킷이 와야 정상으로 처리해줌.
			// 패킷을 받은 순간 값이 들어가며, 데미지 패킷이 오면 다시 0으로 초기화
			// 즉, 해당 값이 0이면 Fire_1 공격이 안온 상태
			DWORD m_dwFire1_StartTime;

			
		private:
			// -----------------------
			// 생성자와 소멸자
			// -----------------------

			CGameSession();
			virtual ~CGameSession();


		private:
			// -----------------
			// 가상함수
			// -----------------

			// Auth 스레드에서 처리
			virtual void OnAuth_ClientJoin();
			virtual void OnAuth_ClientLeave(bool bGame = false);
			virtual void OnAuth_Packet(CProtocolBuff_Net* Packet);

			// Game 스레드에서 처리
			virtual void OnGame_ClientJoin();
			virtual void OnGame_ClientLeave();
			virtual void OnGame_Packet(CProtocolBuff_Net* Packet);

			// Release용
			virtual void OnGame_ClientRelease();


			// -----------------
			// Auth모드 패킷 처리 함수
			// -----------------

			// 로그인 요청 
			// 
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Auth_LoginPacket(CProtocolBuff_Net* Packet);	

			// 방 입장 요청
			// 
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Auth_RoomEnterPacket(CProtocolBuff_Net* Packet);



			// -----------------
			// Game모드 패킷 처리 함수
			// -----------------

			// 유저가 이동할 시 보내는 패킷.
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_MovePacket(CProtocolBuff_Net* Packet);

			// HitPoint 갱신
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_HitPointPacket(CProtocolBuff_Net* Packet);

			// Fire 1 패킷 (총 발사)
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_Frie_1_Packet(CProtocolBuff_Net* Packet);

			// HitDamage
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_HitDamage_Packet(CProtocolBuff_Net* Packet);
			
			// Frie 2 패킷 (발 차기)
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_Fire_2_Packet(CProtocolBuff_Net* Packet);

			// KickDamage
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_KickDamage_Packet(CProtocolBuff_Net* Packet);

			// Reload Request
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_Reload_Packet(CProtocolBuff_Net* Packet);

			// 아이템 획득 요청
			//
			// Parameter : CProtocolBuff_Net*, 아이템의 Type
			// return : 없음
			void Game_GetItem_Packet(CProtocolBuff_Net* Packet, int Type);	
		};

		friend class CGameSession;

		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
			// 넷 서버
			TCHAR BindIP[20];
			int Port;
			int CreateWorker;
			int ActiveWorker;
			int CreateAccept;
			int HeadCode;
			int XORCode;
			int Nodelay;
			int MaxJoinUser;
			int LogLevel;

			// 마스터 클라
			TCHAR MasterServerIP[20];
			int MasterServerPort;
			int MasterClientCreateWorker;
			int MasterClientActiveWorker;
			int MasterClientNodelay;


			// 모니터링 클라
			TCHAR MonitorServerIP[20];
			int MonitorServerPort;
			int MonitorClientCreateWorker;
			int MonitorClientActiveWorker;
			int MonitorClientNodelay;

			
			// 채팅 랜서버
			TCHAR ChatLanServerIP[20];
			int ChatPort;
			int ChatCreateWorker;
			int ChatActiveWorker;
			int ChatCreateAccept;
			int ChatNodelay;
			int ChatMaxJoinUser;
			int ChatLogLevel;
		};
				
		// 방 구조체
		struct stRoom
		{
			// 아이템 구조체
			struct stRoomItem
			{
				UINT m_uiID;
				eu_ITEM_TYPE m_euType;
				float m_fPosX;
				float m_fPosY;
			};			

			// 배틀서버 번호
			int m_iBattleServerNo;

			// 룸 번호
			int m_iRoomNo;

			// 룸의 상태
			// 0 : 대기방
			// 1 : 준비방
			// 2 : 플레이 방
			int m_iRoomState;

			// 현재 룸 안에 있는 유저 수
			int m_iJoinUserCount;			

			// 생존한 유저 수. 
			// HP가 0 이상인 유저.
			int m_iAliveUserCount;

			// 게임 모드로 전환된 유저 수.
			// 이 수가 m_iJoinUserCount와 동일해지면, 방 내의 모두에게
			// 캐릭터 생성 패킷을 보낸다 (나, 다른사람 포함)
			int m_iGameModeUser;

			// Play상태로 변경을 위한 카운트 다운. 밀리세컨드 단위
			// timeGetTime()의 값이 들어간다.
			DWORD m_dwCountDown;			

			// ------------

			// 게임이 종료되었는지 체크하는 Flag.
			// 승리자가 정해지면, Game 종료 패킷을 보낸 후 변경된다.
			// true면 종료된 게임.
			bool m_bGameEndFlag;	

			// 게임 종료가 체크된 시점의 밀리세컨드
			// m_bGameEndFlag를 true로 만들고 바로 
			DWORD m_dwGameEndMSec;

			// ------------

			// 게임이 종료되어, 방 내의 모든 유저들에게 Shutdown을 날린 상태.
			// 연속으로 Shutdown을 날리지 않도록 하기 위함.
			// true면 이미, 방 안의 모든 유저에게 셧다운을 함.
			bool m_bShutdownFlag;

			// 방 입장 토큰 (배틀서버 입장 토큰과는 다름)
			char m_cEnterToken[32];	

			// ------------

			// 방에 입장한 유저들 관리 자료구조.
			vector<CGameSession*> m_JoinUser_Vector;
					   
			// 입장 가능한 최대 인원 수. 고정 값
			const int m_iMaxJoinCount = 2;	

			// ------------

			// 방 별로, 고유하게 아이템ID를 부여하기 위한 변수.
			// 아이템 생성 시 ++한다.
			UINT m_uiItemID;
			
			// 아이템 구조체 관리 메모리풀 TLS
			CMemoryPoolTLS<stRoomItem> *m_Item_Pool;

			// 해당 방에 생성된 아이템 자료구조
			// Key : ItemID, Value : stRoomTiem*
			unordered_map<UINT, stRoomItem*> m_RoomItem_umap;

			// ------------

			CBattleServer_Room* m_pBattleServer;


			// ------------

			// 레드존 활성화 순서.
			// eu_REDZONE_TYPE와 비교해 체크한다.
			int m_arrayRedZone[4];

			// 레드존 활성화 체크를 위한 시간.
			// 이 시간으로부터 40초가 지났으면 레드존 활성화.
			DWORD m_dwReaZoneTime;

			// 레드존 데미지 Tick 체크를 위한 시간.
			// 1초단위로 갱신
			DWORD m_dwTick;

			// 현재 활성화된 레드존의 수.
			// 사실, bool 형태로 활성화 여부만 체크해도 되지만
			// 이왕 체크하는거 그냥 ++해봄. 
			// 혹시 나중에 현재 활성화된 레드존의 수가 필요할 수도 있으니..
			int m_iRedZoneCount;

			// 안전지대 X,Y 좌표.
			// 레드존이 활성화 될 때 마다 변경된다.
			float m_fSafePos[2][2];

			// 이번에 생성될, 레드존에 대한 경고 패킷 보냄 여부.
			// 레드존 활성화 후 다시 false로 되돌린다.
			bool m_bRedZoneWarningFlag;
		
			// ------------


			// ------------
			// 멤버 함수
			// ------------

			// 생성자
			stRoom();

			// 소멸자
			virtual ~stRoom();

			// 자료구조 내의 모든 유저에게 인자로 받은 패킷 보내기
			//
			// Parameter : CProtocolBuff_Net*
			// return : 자료구조 내에 유저가 0명일 경우 false
			//		  : 그 외에는 true
			bool SendPacket_BroadCast(CProtocolBuff_Net* SendBuff);

			// 자료구조 내의 모든 유저에게 인자로 받은 패킷 보내기
			// 인자로 받은 AccountNo를 제외하고 보낸다
			//
			// Parameter : CProtocolBuff_Net*, AccountNo
			// return : 자료구조 내에 유저가 0명일 경우 false
			//		  : 그 외에는 true
			bool SendPacket_BroadCast(CProtocolBuff_Net* SendBuff, INT64 AccountNo);
			
			// 방 내의 모든 유저를 Auth_To_Game으로 변경
			//
			// Parameter : 없음
			// return : 없음
			void ModeChange();			

			// 룸 안의 모든 유저를 생존상태로 변경
			//
			// Parameter : 없음
			// return : 자료구조 내에 유저가 0명일 경우 false
			//		  : 그 외에는 true
			bool AliveFlag_True();

			// 방 안의 유저들에게 게임 종료 패킷 보내기
			//
			// Parameter : 없음
			// return : 없음
			void GameOver();

			// 방 안의 유저들에게 셧다운 날리기
			//
			// Parameter : 없음
			// return : 없음
			void Shutdown_All();

			// 방 안의 모든 유저들에게 나 생성패킷과 다른 유저 생성 패킷 보내기
			// 
			// Parameter : 없음
			// return : 없음
			void CreateCharacter();

			// 방 안의 모든 유저들에게 전적 변경내용 보내기.
			//
			// Parameter : 없음
			// return : 없음
			void RecodeSend();

			// 해당 방에, 아이템 생성 (최초 게임 시작 시 생성)
			// 생성 후, 방 안의 유저에게 아이템 생성 패킷 보냄
			//
			// Parameter : 없음
			// return : 없음
			void StartCreateItem();

			// 해당 방에, 아이템 1개 생성 (유저 사망 시 생성)
			// 생성 후, 방 안의 유저에게 아이템 생성 패킷 보냄
			//
			// Parameter : CGameSession* (사망한 유저)
			// return : 없음
			void CreateItem(CGameSession* DiePlayer);

			// 방 안의 모든 유저에게 "유저 추가됨" 패킷 보내기
			//
			// Parameter : 이번에 입장한 유저 CGameSession*
			// return : 없음
			void Send_AddUser(CGameSession* NowPlayer);
			


			// ------------
			// 자료구조 함수
			// ------------

			// 자료구조에 Insert
			//
			// Parameter : 추가하고자 하는 CGameSession*
			// return : 없음
			void Insert(CGameSession* InsertPlayer);

			// 자료구조에 유저가 있나 체크
			//
			// Parameter : AccountNo
			// return : 찾은 유저의 CGameSession*
			//		  : 유저가 없을 시 nullptr
			CGameSession* Find(INT64 AccountNo);

			// 자료구조에서 Erase
			//
			// Parameter : 제거하고자 하는 CGameSession*
			// return : 성공 시 true
			//		  : 실패 시  false
			bool Erase(CGameSession* InsertPlayer);


			// ------------
			// 아이템 자료구조 함수
			// ------------

			// 아이템 자료구조에 Insert
			//
			// Parameter : ItemID, stRoomItem*
			// return : 없음
			void Item_Insert(UINT ID, stRoomItem* InsertItem);

			// 아이템 자료구조에 아이템이 있나 체크
			//
			// Parameter : itemID
			// return : 찾은 아이템의 stRoomItem*
			//		  : 아이템이 없을 시 nullptr
			stRoomItem* Item_Find(UINT ID);

			// 아이템 자료구조에서 Erase
			//
			// Parameter : 제거하고자 하는 stRoomItem*
			// return : 성공 시 true
			//		  : 실패 시  false
			bool Item_Erase(stRoomItem* InsertPlayer);


			// ------------
			// 레드존 관련 함수
			// ------------
			
			// 레드존 경고
			// 이 함수는, 레드존 경고를 보내야 할 시점에 호출된다.
			// 이미 밖에서, 호출 시점 다 체크 후 이 함수 호출.
			//
			// Parameter : 경고 시간(BYTE). 
			// return : 없음
			void RedZone_Warning(BYTE AlertTimeSec);

			// 레드존 활성화
			// 이 함수는, 레드존이 활성화 될 시점에만 호출된다
			// 즉, 활성화 할지 말지는 밖에서 정한 다음에 이 함수 호출
			//
			// Parameter : 없음
			// return : 없음
			void RedZone_Active();

			// 레드존 데미지 체크
			// 이 함수는, 유저에게 데미지를 줘야 할 시점에 호출
			// 즉, 줄지 말지는 밖에서 장한 다음에 이 함수 호출
			//
			// Parameter : 없음
			// return : 없음
			void RedZone_Damage();

		};

		// 방 상태 state
		enum eu_ROOM_STATE
		{
			// 대기방
			WAIT_ROOM = 0,

			// 준비방
			READY_ROOM,

			// 플레이방
			PLAY_ROOM
		};
				
		// 절대 변경 불가능한 Config 변수 관리 구조체
		struct stCONFIG
		{
			// Auth_Update에서 한 프레임에 처리할 HTTP통신 후처리
			// 고정 값
			const int m_iHTTP_MAX = 100;

			// 최대 존재할 수 있는 방 수
			// 고정 값
			const LONG m_lMaxTotalRoomCount = 200;

			// 최대 존재할 수 있는 대기방 수
			// 고정 값
			const LONG m_lMaxWaitRoomCount = 200;

			// Auth_Update에서 한 프레임에 생성 가능한 방 수
			// 고정 값
			const int m_iLoopCreateRoomCount = 30;

			// Auth_Update에서 한 프레임에, Game모드로 넘기는 방 수
			// 즉, Ready 상태의 방을 Play로 변경하는 수
			// 고정 값
			const int m_iLoopRoomModeChange = 30;

			// 토큰 재발급 시간
			// 밀리세컨드 단위.
			// ex) 1000일 경우, 1초 단위로 토큰 재발급.
			const int m_iTokenChangeSlice = 120000;

			// 방이 Ready상태가 되었을 때 카운트다운.
			// 초 기준
			const BYTE m_bCountDownSec = 10;

			// 방이 Ready상태가 되었을 때 카운트다운.
			// 밀리세컨드 기준
			const int m_iCountDownMSec = 10000;

			// Play상태의 방이, 게임 종료 후 몇 초동안 대기하는지.
			// 밀리세컨드 단위
			const int m_iRoomCloseDelay = 5000;

			// 아이템 획득 좌표 오차
			float m_fGetItem_Correction = 2;

			// 레드존 활성화 시간.
			// 게임 시작 후, 아래 시간마다 하나씩 활성화
			DWORD m_dwReaZoneActiveTime = 40000; // (현재 40초)
		
			// 레드존 경고 보내는 시간.
			// 레드존 활성화 시간이, 이 변수만큼 남았을 경우, 경고 패킷을 보낸다.
			DWORD m_dwRedZoneWarningTime = 20000; // (현재 20초)
		};




		// -----------------------
		// 멤버 변수
		// -----------------------

		// 세션. CMMOServer와 공유.
		CGameSession* m_cGameSession;

		// Config 변수
		stConfigFile m_stConfig;

		// 고정값 변수들을 보관하는 구조체 변수
		stCONFIG m_stConst;

		// 모니터링 클라
		CGame_MinitorClient* m_Monitor_LanClient;

		// 마스터와 연결된 클라
		CBattle_Master_LanClient* m_Master_LanClient;

		// 채팅서버에게 연결 받는 랜서버
		CBattle_Chat_LanServer* m_Chat_LanServer;

		// 버전 코드
		// 클라가 들고온 것을 비교. 파싱으로 읽어옴.
		int m_uiVer_Code;

		// shDB와 통신하는 변수
		shDB_Communicate m_shDB_Communicate;	

		// 나의 서버 No
		// 마스터 서버가 할당해 준다.
		int m_iServerNo;


		// -----------------------
		// 레드존 관련 변수
		// -----------------------

		// 레드존 생성 순서를 위한 순열. 생성자에서 next_permutation을 이용해 생성
		// 4!
		int m_arrayRedZoneCreate[24][4];

		// 레드존 타입에 따른 활성 위치
		float m_arrayRedZoneRange[4][2][2];



		// -----------------------
		// 출력용 변수
		// -----------------------
		
		// 배틀서버 입장 토큰 에러
		LONG m_lBattleEnterTokenError;

		// 방 입장 토큰 에러
		LONG m_lRoomEnterTokenError;

		// auth의 로그인 패킷에서, DB 쿼리 시 결과로 -10(사용자 없음)이 올 경우
		LONG m_lQuery_Result_Not_Find;

		// auth의 로그인 패킷에서, DB 쿼리 시 결과로 -10도 아닌데 1이 아닌 에러일 경우 
		LONG m_lTempError;

		// auth의 로그인 패킷에서, 유저의 SessionKey(토큰)이 다를 경우
		LONG m_lTokenError;

		// auth의 로그인 패킷에서, 유저가 들고 온 버전이 다를 경우
		LONG m_lVerError;

		// auth의 로그인 패킷에서, 중복 로그인 시
		LONG m_DuplicateCount;

		// auth의 로그인 패킷에서, DBWrite 중인데 들어올 경우
		LONG m_DBWrie_LoginCount;

		// Ready 방 수
		LONG m_lReadyRoomCount;

		// Play 방 수
		LONG m_lPlayRoomCount;

		// 총알 공격 시 유저에게 입혀야하는 데미지(거리 1당 데미지)
		float m_fFire1_Damage;


		

		// -----------------------
		// 토큰관리 변수
		// -----------------------

		// 입장 토큰 관리
		// 배틀서버는 이 2개 중 하나라도 일치한다면 맞다고 통과시킨다.

		// 배틀서버 입장 토큰 1번
		// "현재" 토큰을 보관한다.
		// 마스터와 연결된 랜 클라가 생성 및 갱신한다.
		char m_cConnectToken_Now[32];

		// 배틀서버 입장 토큰 2번 
		// "이전" 토큰을 보관한다.
		// 토큰이 재할당 될 경우, 이전 토큰을 여기다 저장한 후
		// 새로운 토큰을 "현재" 토큰에 넣는다.
		char m_cConnectToken_Before[32];




		// -----------------------
		// 방 관련 변수
		// -----------------------

		// 방 번호를 할당할 변수
		// 인터락으로 ++
		LONG m_lGlobal_RoomNo;
		
		// Wait Room 카운트
		LONG m_lNowWaitRoomCount;

		// 현재 게임 내에 만들어진 방 수 (모든 방 합쳐서)
		LONG m_lNowTotalRoomCount;

		// 방 입장 토큰
		// 미리 64개정도 만들어 두고, 방생성 시 랜덤으로 보낸다.
		char m_cRoomEnterToken[64][32];

		// stRoom 관리 Pool
		CMemoryPoolTLS<stRoom> *m_Room_Pool;

		

		


		// -----------------------
		// 방 관리 자료구조 변수
		// -----------------------

		// 방 관리 umap
		//
		// Key : RoomNo, Value : stRoom*
		unordered_map<int, stRoom*> m_Room_Umap;

		// m_Room_Umap SRW락
		SRWLOCK m_Room_Umap_srwl;






		// -----------------------
		// 유저 관리 자료구조 변수
		// -----------------------

		// AccountNo를 이용해 CGameSession* 관리
		//
		// 접속 중인 유저의 AccountNo를 기준으로 CGameSession* 관리
		// Key : AccountNo, Value : CGameSession*
		unordered_map<INT64, CGameSession*> m_AccountNo_Umap;

		// m_AccountNo_Umap용 SRW락
		SRWLOCK m_AccountNo_Umap_srwl;	



		// -----------------------
		// 아직 DB에 Write 중인 유저 관리 자료구조 변수
		// -----------------------

		// AccountNo를 이용해 DBWrite 중인 카운트 관리.
		//
		// Key : AccountNo, Value : WriteCount(int)
		unordered_map<INT64, int> m_DBWrite_Umap;

		// m_DBWrite_Umap용 SRW락
		SRWLOCK m_DBWrite_Umap_srwl;


	public:
		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// Start
		// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
		//
		// Parameter : 없음
		// return : 실패 시 false
		bool ServerStart();

		// Stop
		// 내부적으로 Stop 실행
		//
		// Parameter : 없음
		// return : 없음
		void ServerStop();

		// 출력용 함수
		//
		// Parameter : 없음
		// return : 없음
		void ShowPrintf();


	private:
		// -----------------------
		// 내부에서만 사용하는 함수
		// -----------------------

		// 파일에서 Config 정보 읽어오기
		// 
		// Parameter : config 구조체
		// return : 정상적으로 셋팅 시 true
		//		  : 그 외에는 false
		bool SetFile(stConfigFile* pConfig);

		// 총알 데미지 계산 시 사용되는 함수 (발차기 계산시에는 사용되지 않음.)
		// 실제 감소시켜야 하는 HP를 계산한다.
		//
		// Parameter : 공격자와 피해자의 거리
		// return : 감소시켜야하는 HP
		int GetDamage(float Range);
		

	private:
		// -----------------------
		// 패킷 후처리 함수
		// -----------------------

		// Login 패킷에 대한 인증 처리 (토큰 체크 등..)
		//
		// Parameter : DB_WORK_LOGIN*
		// return : 없음
		void Auth_LoginPacket_AUTH(DB_WORK_LOGIN* DBData);

		// Login 패킷에 대한 Contents 정보 가져오기
		//
		// Parameter : DB_WORK_LOGIN*
		// return : 없음
		void Auth_LoginPacket_Info(DB_WORK_LOGIN_CONTENTS* DBData);
		
		// DB_Write에 대한 작업 후 처리
		//
		// Parameter : DB_WORK*
		// return : 없음
		void Game_DBWrite(DB_WORK* DBData);



	private:
		// -----------------------
		// AccountNo 자료구조 관리 함수
		// -----------------------

		// AccountNo 자료구조에 유저를 추가하는 함수
		//
		// Parameter : AccountNo, CGameSession*
		// return : 성공 시 true
		//		  : 실패 시 false
		bool InsertAccountNoFunc(INT64 AccountNo, CGameSession* InsertPlayer);

		// AccountNo 자료구조에서 유저 검색 후, 해당 유저에게 Disconenct 하는 함수
		//
		// Parameter : AccountNo
		// return : 없음
		void DisconnectAccountNoFunc(INT64 AccountNo);

		// AccountNo 자료구조에서 유저를 제거하는 함수
		//
		// Parameter : AccountNo
		// return : 성공 시 true
		//		  : 실패 시 false
		bool EraseAccountNoFunc(INT64 AccountNo);



	private:
		// -----------------------
		// DBWrite 카운트 자료구조 관리 함수
		// -----------------------

		// DBWrite 카운트 관리 자료구조에 유저를 추가하는 함수
		//
		// Parameter : AccountNo
		// return : 성공 시 true
		//		  : 실패(키 중복) 시 false
		bool InsertDBWriteCountFunc(INT64 AccountNo);

		// DBWrite 카운트 관리 자료구조에서 유저를 검색 후, 
		// 카운트(Value)를 1 증가하는 함수
		//
		// Parameter : AccountNo
		// return : 없음
		void AddDBWriteCountFunc(INT64 AccountNo);

		// DBWrite 카운트 관리 자료구조에서 유저를 검색 후, 
		// 카운트(Value)를 1 감소시키는 함수
		// 감소 후 0이되면 Erase한다.
		//
		// Parameter : AccountNo
		// return : 성공 시 true
		//		  : 검색 실패 시 false
		bool MinDBWriteCountFunc(INT64 AccountNo);
		


	private:
		// ---------------------------------
		// 방 관리 자료구조 변수
		// ---------------------------------

		// 방을 Room 자료구조에 Insert하는 함수
		//
		// Parameter : RoomNo, stRoom*
		// return : 성공 시 true
		//		  : 실패(중복 키) 시 false	
		bool InsertRoomFunc(int RoomNo, stRoom* InsertRoom);

		




	private:
		// -----------------------
		// 가상함수
		// -----------------------

		// AuthThread에서 1Loop마다 1회 호출.
		// 1루프마다 정기적으로 처리해야 하는 일을 한다.
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnAuth_Update();

		// GameThread에서 1Loop마다 1회 호출.
		// 1루프마다 정기적으로 처리해야 하는 일을 한다.
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnGame_Update();

		// 새로운 유저 접속 시, Auth에서 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------

		// 생성자
		CBattleServer_Room();

		// 소멸자
		virtual ~CBattleServer_Room();
	};

}


// ----------------------------------------
// 
// 마스터 서버와 연결되는 LanClient
//
// ----------------------------------------
namespace Library_Jingyu
{
	class CBattle_Master_LanClient :public CLanClient
	{
		friend class CBattleServer_Room;
		friend class CBattle_Chat_LanServer;


		// ------------- 
		// 멤버 변수
		// ------------- 

		// 현재 마스터 서버와 연결된 세션 ID
		ULONGLONG m_ullSessionID;

		// 마스터 넷 서버의 IP와 Port
		TCHAR m_tcBattleNetServerIP[30];
		int m_iBattleNetServerPort;

		// 채팅 넷 서버의 IP와 Port
		// 채팅 Lan 클라에게 받는다.
		TCHAR m_tcChatNetServerIP[30];
		int m_iChatNetServerPort;

		// 마스터 서버에 들고갈 입장 토큰
		char m_cMasterToken[32];

		// 마스터 서버에게 패킷을 하나 보낼 때 1씩 증가되는 값.
		UINT uiReqSequence;

		// 로그인 체크
		// true면 로그인 패킷을 보낸 상태
		bool m_bLoginCheck;




		// ----------------------
		// !!Battle 서버의 this !!
		// ----------------------
		CBattleServer_Room* m_BattleServer_this;


		



	private:
		// -----------------------
		// 패킷 처리 함수
		// -----------------------

		// 마스터에게 보낸 로그인 요청 응답
		//
		// Parameter : SessionID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_Login_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 신규 대기방 생성 응답
		//
		// Parameter : SessionID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_NewRoomCreate_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);
		
		// 토큰 재발행 응답
		//
		// Parameter : SessionID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_TokenChange_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 방 닫힘 응답
		//
		// Parameter : SessionID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_RoomClose_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 방 퇴장 응답
		//
		// Parameter : RoomNo, AccountNo
		// return : 없음
		void Packet_RoomLeave_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);


	private:
		// -----------------------
		// 채팅 Lan 서버가 호출하는 함수
		// -----------------------

		// 마스터에게, 신규 대기방 생성 패킷 보내기
		//
		// Parameter : RoomNo
		// return : 없음
		void Packet_NewRoomCreate_Req(int RoomNo);

		// 토큰 재발급 함수
		//
		// Parameter : 없음
		// return : 없음
		void Packet_TokenChange_Req();


	private:	
		// -----------------------
		// Battle Net 서버가 호출하는 함수
		// -----------------------		

		// 마스터에게, 방 닫힘 패킷 보내기
		//
		// Parameter : RoomNo
		// return : 없음
		void Packet_RoomClose_Req(int RoomNo);

		// 마스터에게, 방 퇴장 패킷 보내기
		//
		// Parameter : RoomNo, ClientKey
		// return : 없음
		void Packet_RoomLeave_Req(int RoomNo, INT64 ClientKey);



	public:
		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 시작 함수
		// 내부적으로, 상속받은 CLanClient의 Start호출.
		//
		// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
		// return : 성공 시 true , 실패 시 falsel 
		bool ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay);

		// 종료 함수
		// 내부적으로, 상속받은 CLanClient의 Stop호출.
		// 추가로, 리소스 해제 등
		//
		// Parameter : 없음
		// return : 없음
		void ClientStop();

		// 배틀서버의 this를 입력받는 함수
		// 
		// Parameter : 배틀 서버의 this
		// return : 없음
		void ParentSet(CBattleServer_Room* ChatThis);



	private:
		// -----------------------
		// 순수 가상함수
		// -----------------------

		// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnConnect(ULONGLONG SessionID);

		// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnDisconnect(ULONGLONG SessionID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, CProtocolBuff_Lan*
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);

		

	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CBattle_Master_LanClient();
		virtual ~CBattle_Master_LanClient();

	};
}



// ---------------
// CGame_MonitorClient
// CLanClient를 상속받는 모니터링 클라. 모니터링 LanServer로 정보 전송
// ---------------
namespace Library_Jingyu
{
	class CGame_MinitorClient :public CLanClient
	{
		friend class CGameServer;

		// 디파인 정보들 모아두기
		enum en_MonitorClient
		{
			dfSERVER_NO = 2	// 배틀서버는 2번이다
		};

		// 모니터링 서버로 정보 전달할 스레드의 핸들.
		HANDLE m_hMonitorThread;

		// 모니터링 스레드를 종료시킬 이벤트
		HANDLE m_hMonitorThreadExitEvent;

		// 현재 모니터링 서버와 연결된 세션 ID
		ULONGLONG m_ullSessionID;

		// ----------------------
		// !!Battle 서버의 this !!
		// ----------------------
		CBattleServer_Room* m_BattleServer_this;



	private:
		// -----------------------
		// 스레드
		// -----------------------

		// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
		static UINT	WINAPI MonitorThread(LPVOID lParam);




	private:
		// -----------------------
		// 내부에서만 사용하는 기능 함수
		// -----------------------	

		// 모니터링 서버로 데이터 전송
		//
		// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
		// return : 없음
		void InfoSend(BYTE DataType, int DataValue, int TimeStamp);
			   		 

	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CGame_MinitorClient();
		virtual ~CGame_MinitorClient();


	public:
		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 시작 함수
		// 내부적으로, 상속받은 CLanClient의 Start호출.
		//
		// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
		// return : 성공 시 true , 실패 시 falsel 
		bool ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay);

		// 종료 함수
		// 내부적으로, 상속받은 CLanClient의 Stop호출.
		// 추가로, 리소스 해제 등
		//
		// Parameter : 없음
		// return : 없음
		void ClientStop();

		// 배틀서버의 this를 입력받는 함수
		// 
		// Parameter : 배틀 서버의 this
		// return : 없음
		void ParentSet(CBattleServer_Room* ChatThis);




	private:
		// -----------------------
		// 순수 가상함수
		// -----------------------

		// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnConnect(ULONGLONG SessionID);

		// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnDisconnect(ULONGLONG SessionID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, CProtocolBuff_Lan*
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);

	};
}



// ---------------------------
//
// 채팅서버의 랜클라와 연결되는 Lan 서버
//
// ---------------------------
namespace Library_Jingyu
{
	class CBattle_Chat_LanServer	:public CLanServer
	{
		friend class CBattle_Master_LanClient;
		friend class CBattleServer_Room;



		// ------------
		// 멤버 변수
		// ------------

		// 마스터 랜서버와 연결되는 랜클라
		CBattle_Master_LanClient* m_pMasterClient;

		// 배틀 Net서버
		CBattleServer_Room* m_BattleServer;

		// 접속한 세션.
		// 초기값은 0xffffffffffffffff
		ULONGLONG m_ullSessionID;
		
		// 로그인 여부.
		// 로그인 패킷을 받으면 true로 변경
		bool m_bLoginCheck;

		// 채팅 랜 클라에게 패킷을 하나 보낼 때 1씩 증가되는 값.
		UINT m_uiReqSequence;

		// 토큰 발급 시간 갱신하기
		// timeGetTime() 함수의 리턴값.
		// 밀리 세컨드 단위
		DWORD m_dwTokenSendTime;



	private:
		// -----------------------
		// Battle Net 서버가 호출하는 함수
		// -----------------------

		// 채팅 랜클라에게, 신규 대기방 생성 패킷 보내기
		//
		// Parameter : stRoom*
		// return : 없음
		void Packet_NewRoomCreate_Req(CBattleServer_Room::stRoom* NewRoom);

		// 채팅 랜클라에게, 토큰 발급
		//
		// Parameter : 없음
		// return : 없음
		void Packet_TokenChange_Req();


	public:
		// ------------------------
		// 외부에서 호출 가능한 함수
		// ------------------------

		// 서버 시작
		//
		// Parameter : IP, Port, 생성 워커, 활성화 워커, 생성 엑셉트 스레드, 노딜레이, 최대 접속자 수
		// return : 실패 시 false
		bool ServerStart(TCHAR* IP, int Port, int CreateWorker, int ActiveWorker, int CreateAccept, int Nodelay, int MaxUser);

		// 서버 종료
		//
		// Parameter : 없음
		// return : 없음
		void ServerStop();


	private:
		// -----------------------
		// 패킷 처리 함수
		// -----------------------

	
		// 신규 대기방 생성 패킷 응답.
		// 이 안에서 마스터에게도 보내준다.
		//
		// Parameter : SessinID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_NewRoomCreate_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Packet);

		// 방 삭제에 대한 응답
		//
		// Parameter : SessinID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_RoomClose_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Packet);


		// 토큰 재발행에 대한 응답
		// 이 안에서 마스터에게도 보내준다.
		//
		// Parameter : SessionID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_TokenChange_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Packet);

		// 로그인 요청
		//
		// Parameter : SessinID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_Login(ULONGLONG SessionID, CProtocolBuff_Lan* Packet);



	private:
		// --------------------------
		// 내부에서만 사용하는 함수
		// --------------------------

		// 마스터 랜 클라, 배틀 Net 서버 셋팅
		//
		// Parameter : CBattle_Master_LanClient*
		// return : 없음
		void SetMasterClient(CBattle_Master_LanClient* SetPoint, CBattleServer_Room* SetPoint2);



	private:
		// -----------------------
		// 가상함수
		// -----------------------

		// Accept 직후, 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

		// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
		//
		// parameter : 접속한 유저에게 할당된 세션키
		// return : 없음
		virtual void OnClientJoin(ULONGLONG SessionID);

		// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 유저 세션키
		// return : 없음
		virtual void OnClientLeave(ULONGLONG SessionID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);


	public:
		// ---------------
		// 생성자와 소멸자
		// ----------------

		// 생성자
		CBattle_Chat_LanServer();

		// 소멸자
		virtual ~CBattle_Chat_LanServer();

	};
}

#endif // !__BATTLESERVER_ROOM_VERSION_H__
