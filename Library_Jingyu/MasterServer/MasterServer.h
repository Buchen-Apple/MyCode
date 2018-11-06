#ifndef __MASTER_SERVER_H__
#define __MASTER_SERVER_H__

#include "NetworkLib/NetworkLib_LanServer.h"
#include "ObjectPool/Object_Pool_LockFreeVersion.h"
#include "Parser/Parser_Class.h"
#include "Protocol_Set/CommonProtocol_2.h"		// 궁극적으로는 CommonProtocol.h로 이름 변경 필요. 지금은 채팅서버에서 로그인 서버를 이용하는 프로토콜이 있어서 _2로 만듬.

#include <unordered_map>
#include <unordered_set>
#include <queue>

// -----------------------
//
// 마스터 Match 서버
// 
// -----------------------
namespace Library_Jingyu
{
	class CMatchServer_Lan :public CLanServer
	{
		friend class CBattleServer_Lan;


		// -------------------
		// 이너 구조체
		// -------------------

		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
			// 매치메이킹용 Lan 서버 정보
			TCHAR BindIP[20];
			int Port;
			int CreateWorker;
			int ActiveWorker;
			int CreateAccept;
			int Nodelay;
			int MaxJoinUser;
			int LogLevel;
			char EnterToken[32];


			// 배틀용 Lan 서버 정보
			TCHAR BattleBindIP[20];
			int BattlePort;
			int BattleCreateWorker;
			int BattleActiveWorker;
			int BattleCreateAccept;
			int BattleNodelay;
			int BattleMaxJoinUser;
			int BattleLogLevel;
			char BattleEnterToken[32];
		};

		// 매칭서버 구조체
		struct stMatching
		{
			ULONGLONG m_ullSessionID;
			int m_iServerNo;

			// 로그인 여부. true면 로그인 중
			bool m_bLoginCheck;

			stMatching()
			{
				m_bLoginCheck = false;
			}
		};
		


	private:
		// -----------------
		// 멤버 변수
		// -----------------

		// !! Battle 서버 !!
		CBattleServer_Lan* pBattleServer;

		// Config용 변수
		stConfigFile m_stConfig;



	private:
		// -----------------
		// 플레이어 관리용 자료구조
		// -----------------	

		// 매칭 랜 서버에서 관리 중인 Player 자료구조
		// umap 사용
		// ClientKey 이용
		//
		// Key : ClientKey, Value : AccountNo
		unordered_map<UINT64, UINT64> m_Player_ClientKey_Umap;

		// m_Player_ClientKey_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_Player_ClientKey_Umap;



	private:
		// -----------------
		// Match 서버 관리용 자료구조
		// -----------------	

		// 접속한 매치메이킹 서버 관리용 자료구조
		// uamp 사용
		//
		// Key : SessionID, Value : stMatching*
		unordered_map<ULONGLONG, stMatching*> m_MatchServer_Umap;

		// m_MatchServer_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_MatchServer_Umap;	

		// stMatching를 관리하는 TLSPool
		CMemoryPoolTLS<stMatching> *m_TLSPool_MatchServer;

		// ------------------------

		// Match 서버 중, 로그인 패킷까지 받은 서버 관리용 자료구조.
		// 존재 여부 판단용도.
		// uset 사용
		//
		// Key : ServerNo (매칭서버 No)
		unordered_set<int> m_LoginMatServer_Uset;

		// m_LoginMatServer_Uset관리용 SRWLOCK
		SRWLOCK m_srwl_LoginMatServer_Uset;

			  

	private:
		// -------------------------------------
		// 내부에서만 사용하는 함수
		// -------------------------------------

		// 파일에서 Config 정보 읽어오기
		// 
		// Parameter : config 구조체
		// return : 정상적으로 셋팅 시 true
		//		  : 그 외에는 false
		bool SetFile(stConfigFile* pConfig);



	private:
		// -----------------
		// 플레이어 관리 자료구조 함수
		// -----------------

		// 매칭에서 관리되는 플레이어 자료구조에 insert
		//
		// Parameter : ClinetKey, AccountNo
		// return : 정상적으로 삽입 시, true
		//		  : 실패 시 false;	
		bool InsertPlayerFunc(UINT64 ClientKey, UINT64 AccountNo);




	private:
		// -----------------
		// 접속한 매칭 서버 관리용 자료구조 함수 (umap)
		// -----------------

		// 접속자 관리 자료구조에 Insert
		// umap으로 관리중
		//
		// Parameter : SessionID, stMatching*
		// return : 실패 시 false 리턴
		bool InsertMatchServerFunc(ULONGLONG SessionID, stMatching* insertServer);
		
		// 접속자 관리 자료구조에서, 유저 검색
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID
		// return : 검색 성공 시, stMatchingr*
		//		  : 검색 실패 시 nullptr
		stMatching* FindMatchServerFunc(ULONGLONG SessionID);
		
		// 접속자 관리 자료구조에서 Erase
		// umap으로 관리중
		//
		// Parameter : SessionID
		// return : 성공 시, 제거된 stMatching*
		//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
		stMatching* EraseMatchServerFunc(ULONGLONG SessionID);



	
	private:
		// -----------------
		// 접속한 매칭 서버 관리용 자료구조 함수(로그인 한 유저 관리)
		// -----------------

		// 로그인 한 유저 관리 자료구조에 Insert
		// uset으로 관리중
		//
		// Parameter : ServerNo
		// return : 실패 시 false 리턴
		bool InsertLoginMatchServerFunc(int ServerNo);

		// 로그인 한 유저 관리 자료구조에서 Erase
		// uset으로 관리중
		//
		// Parameter : ServerNo
		// return : 실패 시 false 리턴
		bool EraseLoginMatchServerFunc(int ServerNo);
			   		 	  


	private:
		// -----------------
		// 패킷 처리용 함수
		// -----------------

		// Login패킷 처리
		//
		// Parameter : SessionID, Payload
		// return : 없음
		void Packet_Login(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 방 정보 요청
		// 정보 뽑은 후, Battle로 전달
		//
		// Parameter : SessionID, Payload
		// return : 없음
		void Relay_RoomInfo(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 방 입장 성공
		//
		// Parameter : SessionID, Payload
		// return : 없음
		void Packet_RoomEnter_OK(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);




	public:
		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 서버 시작
		//
		// Parameter : 없음
		// return : 실패 시 false.
		bool ServerStart();

		// 서버 종료
		//
		// Parameter : 없음
		// return : 없음
		void ServerStop();





	private:
		// -----------------------
		// 가상함수
		// -----------------------

		bool OnConnectionRequest(TCHAR* IP, USHORT port);

		void OnClientJoin(ULONGLONG SessionID);

		void OnClientLeave(ULONGLONG SessionID);

		void OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		void OnSend(ULONGLONG SessionID, DWORD SendSize);

		void OnWorkerThreadBegin();

		void OnWorkerThreadEnd();

		void OnError(int error, const TCHAR* errorStr);


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------

		// 생성자
		CMatchServer_Lan();

		// 소멸자
		virtual ~CMatchServer_Lan();


	};
}


// -----------------------
//
// 마스터 Battle 서버
// 
// -----------------------
namespace Library_Jingyu
{
	class CBattleServer_Lan :public CLanServer
	{
		friend class CMatchServer_Lan;

		// --------------
		// 이너 구조체
		// --------------

		// CRoom 전방선언
		struct stRoom;

		// Room_Priority_Queue에서 사용할 비교연산자
		struct RoomCMP
		{
			// m_iEmptyCount가 낮은 순으로 pop
			bool operator()(stRoom* t, stRoom* u)
			{
				return t->m_iEmptyCount > u->m_iEmptyCount;
			}
		};

		// 배틀서버 구조체
		struct stBattle
		{
			ULONGLONG m_ullSessionID;

			// 배틀서버 고유번호
			int m_iServerNo;

			// 배틀서버 접속 IP
			WCHAR	m_tcBattleIP[16];

			// 배틀서버 접속 Port
			WORD	m_wBattlePort;

			// 배틀서버 접속 토큰(배틀서버 발행)
			char	m_cConnectToken[32];

			// 해당 배틀서버와 연결된 채팅서버
			WCHAR	m_tcChatIP[16];
			WORD	m_wChatPort;
		};
		
		// Room 구조체
		struct stRoom
		{
			int m_iRoomNo;

			// 방에, 여유 공간이 얼마인지.
			// ex) 유저가 3/5상태라면, 해당 값은 2임.
			int m_iEmptyCount;

			// 룸이 존재하는 배틀 서버의 No
			int m_iBattleServerNo;

			// 배틀서버 방 입장 토큰(배틀서버 발행)
			char	m_cEnterToken[32];

			// 해당 룸의 락
			SRWLOCK m_srwl_Room;

			stRoom()
			{
				InitializeSRWLock(&m_srwl_Room);
			}

			// 해당 방의 락
			void RoomLOCK()
			{
				AcquireSRWLockExclusive(&m_srwl_Room);
			}

			// 해당 방의 언락
			void RoomUNLOCK()
			{
				ReleaseSRWLockExclusive(&m_srwl_Room);
			}
		};

		// 플레이어 구조체
		struct stPlayer
		{
			UINT64 m_ui64AccountNo;

			// 해당 유저가 존재한 방이 존재하는 배틀 서버의 No
			int m_iBattleServerNo;

			// 접속 중인 방 번호.
			int m_iJoinRoomNo;
		};


	private:
		// --------------
		// 멤버 변수
		// --------------

		// !! Matching 서버 !!
		CMatchServer_Lan* pMatchServer;
		

	private:
		// -----------------
		// 플레이어 관리용 자료구조
		// -----------------
		
		// 배틀 랜 서버에서 관리 중인 Player 자료구조
		// umap 사용
		// AccountNo 이용
		//
		// Key : AccountNo, Value : stPlayer*
		unordered_map<UINT64, stPlayer*> m_Player_AccountNo_Umap;

		// m_Player_AccountNo_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_Player_AccountNo_Umap;

		// stPlayer를 관리하는 TLSPool
		CMemoryPoolTLS<stPlayer> *m_TLSPool_Player;



	private:
		// -----------------
		// 접속한 Battle 서버 관리용 자료구조
		// -----------------	

		// 접속한 배틀 서버 관리용 자료구조
		// uamp 사용
		//
		// Key : SessionID, Value : stBattle*
		unordered_map<ULONGLONG, stBattle*> m_BattleServer_Umap;

		// m_BattleServer_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_BattleServer_Umap;

		// stBattle을 관리하는 TLSPool
		CMemoryPoolTLS<stBattle> *m_TLSPool_BattleServer;




	private:
		// -----------------
		// Room 관리용 자료구조
		// -----------------

		// RoomNo를 이용해, 룸을 관리하는 자료구조
		// umap 사용
		//
		// Key : RoomNo, Value : CPlayer*
		unordered_map<int, stRoom*> m_Room_Umap;

		// m_Room_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_Room_Umap;

		// -----------------------			

		// 룸을 관리하는 자료구조2
		// Priority_Queue 사용
		//
		//  <자료형, 구현체, 비교연산자> 순서로 정의
		// 자료형 : CRoom*, 구현체 : Vector<CRoom*>. 비교 연산자 : 직접 지정(오름차순)
		priority_queue <stRoom*, vector<stRoom*>, RoomCMP> m_Room_pq;

		// m_Room_pq관리용 SRWLOCK
		SRWLOCK m_srwl_Room_pq;

		// -----------------------

		// stRoom을 관리하는 TLSPool
		CMemoryPoolTLS<stRoom> *m_TLSPool_Room;



	private:
		// -----------------
		// 플레이어 관리용 자료구조 함수
		// -----------------

		// 플레이어 관리 자료구조에 플레이어 Insert
		//
		// Parameter : AccountNo, stPlayer*
		// return : 정상적으로 Insert 시 true
		//		  : 실패 시 false
		bool InsertPlayerFunc(UINT64 AccountNo, stPlayer* InsertPlayer);

		// 인자로 받은 AccountNo를 이용해 유저 검색.
		// 1. RoomNo, BattleServerNo가 정말 일치하는지 확인
		// 2. 자료구조에서 Erase
		// 3. stPlayer*를 메모리풀에 Free
		//
		// !! 매칭 랜 서버에서 호출 !!
		//
		// Parameter : AccountNo, RoomNo, BattleServerNo
		//
		// return Code
		// 0 : 정상적으로 삭제됨
		// 1 : 존재하지 않는 유저 (AccountNo로 유저 검색 실패
		// 2 : RoomNo 불일치
		// 3 : BattleServerNo 불일치
		int ErasePlayerFunc(UINT64 AccountNo, int RoomNo, int BattleServerNo);



	public:
		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 서버 시작
		//
		// Parameter : 없음
		// return : 실패 시 false.
		bool ServerStart();

		// 서버 종료
		//
		// Parameter : 없음
		// return : 없음
		void ServerStop();





	private:
		// -------------------------------
		// 패킷 처리 함수
		// -------------------------------

		// 방 정보 요청 패킷 처리
		// !! 매칭 랜 서버에서 호출 !!
		//
		// Parameter : SessionID(매칭 쪽의 SessionID), ClientKey, AccountNo
		// return : 없음
		void Relay_Battle_Room_Info(ULONGLONG SessionID, UINT64 ClientKey, UINT64 AccountNo);
			
		


	private:
		// -----------------------
		// 가상함수
		// -----------------------

		bool OnConnectionRequest(TCHAR* IP, USHORT port);

		void OnClientJoin(ULONGLONG SessionID);

		void OnClientLeave(ULONGLONG SessionID);

		void OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		void OnSend(ULONGLONG SessionID, DWORD SendSize);

		void OnWorkerThreadBegin();

		void OnWorkerThreadEnd();

		void OnError(int error, const TCHAR* errorStr);


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------

		// 생성자
		CBattleServer_Lan();

		// 소멸자
		virtual ~CBattleServer_Lan();
	};
}


#endif // !__MASTER_SERVER_H
