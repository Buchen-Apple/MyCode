#ifndef __MASTER_SERVER_H__
#define __MASTER_SERVER_H__

#include "NetworkLib/NetworkLib_LanServer.h"
#include "Log/Log.h"
#include "ObjectPool/Object_Pool_LockFreeVersion.h"
#include "Parser/Parser_Class.h"
#include "Protocol_Set/CommonProtocol_2.h"		// 궁극적으로는 CommonProtocol.h로 이름 변경 필요. 지금은 채팅서버에서 로그인 서버를 이용하는 프로토콜이 있어서 _2로 만듬.

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <queue>


using namespace std;


// -----------------------
//
// 마스터 Lan서버
// 외부에서는 해당 클래스에 접근하며, 해당 클래스가 각 Lan서버를 시작.
// 
// -----------------------
namespace Library_Jingyu
{
	class CMasterServer
	{

		friend class CMatchServer_Lan;
		friend class CBattleServer_Lan;

	private:
		// -----------------
		// 이너 구조체
		// -----------------	

		// CRoom 전방선언
		struct CRoom;

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
		
		// 배틀서버 구조체
		struct CBattleServer
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

		// 매칭서버 구조체
		struct CMatchingServer
		{
			ULONGLONG m_ullSessionID;
			int m_iServerNo;

			// 로그인 여부. true면 로그인 중
			bool m_bLoginCheck;

			CMatchingServer()
			{
				m_bLoginCheck = false;
			}
		};

		// Room_Priority_Queue에서 사용할 비교연산자
		struct RoomCMP
		{
			// m_iEmptyCount가 낮은 순으로 pop
			bool operator()(CRoom* t, CRoom* u)
			{
				return t->m_iEmptyCount > u->m_iEmptyCount;
			}
		};

		// Room 구조체
		struct CRoom
		{
			int m_iRoomNo;

			// 방에, 여유 공간이 얼마인지.
			// ex) 유저가 3/5상태라면, 해당 값은 2임.
			int m_iEmptyCount;

			// 룸이 존재하는 배틀 서버의 Key
			int m_iBattleServerNo;

			// 배틀서버 방 입장 토큰(배틀서버 발행)
			char	m_cEnterToken[32];

			// 해당 룸의 락
			SRWLOCK m_srwl_Room;

			CRoom()
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

	private:
		// -----------------
		// Battle 서버 관리용 자료구조
		// -----------------	

		// 접속한 배틀 서버 관리용 자료구조
		// uamp 사용
		//
		// Key : SessionID, Value : CBattleServer*
		unordered_map<ULONGLONG, CMasterServer::CBattleServer*> m_BattleServer_Umap;

		// m_BattleServer_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_BattleServer_Umap;

		// CBattleServer를 관리하는 TLSPool
		CMemoryPoolTLS<CMasterServer::CBattleServer> *m_TLSPool_BattleServer;
	

	private:
		// -----------------
		// Player 관리용 자료구조
		// -----------------			   

		// ClientKey를 이용해, 플레이어를 관리하는 자료구조
		// uset 사용
		//
		// Key : ClientKey, Value : CPlayer*
		unordered_set<UINT64> m_ClientKey_Uset;

		// m_ClientKey_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_ClientKey_Umap;

		// -----------------------

		// AccountNo를 이용해, 플레이어를 관리하는 자료구조
		// uset 사용
		//
		// Key : AccountNo
		unordered_set<UINT64>m_AccountNo_Uset;

		// m_AccountNo_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_m_AccountNo_Umap;

		// -----------------------

	private:
		// -----------------
		// Room 관리용 자료구조
		// -----------------

		// RoomNo를 이용해, 룸을 관리하는 자료구조
		// umap 사용
		//
		// Key : RoomNo, Value : CPlayer*
		unordered_map<int, CRoom*> m_Room_Umap;

		// m_Room_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_Room_Umap;

		// -----------------------			

		// 룸을 관리하는 자료구조2
		// Priority_Queue 사용
		//
		//  <자료형, 구현체, 비교연산자> 순서로 정의
		// 자료형 : CRoom*, 구현체 : Vector<CRoom*>. 비교 연산자 : 직접 지정(오름차순)
		priority_queue <CRoom*, vector<CRoom*>, RoomCMP> m_Room_pq;

		// m_Room_pq관리용 SRWLOCK
		SRWLOCK m_srwl_Room_pq;

		// -----------------------

		// CRoom를 관리하는 TLSPool
		CMemoryPoolTLS<CRoom> *m_TLSPool_Room;


	private:
		// -----------------
		// 멤버 변수
		// -----------------

		// 파일 읽어오기 용 구조체
		stConfigFile m_stConfig;

		// ----------------------------

		// !! 매치메이킹용 Lan서버 !!
		CMatchServer_Lan* m_pMatchServer;

		// !! 배틀용 Lan 서버 !!
		CBattleServer_Lan* m_pBattleServer;



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


	public:
		// -------------------------------------
		// 외부에서 사용 가능한 함수
		// -------------------------------------

		// 마스터 서버 시작
		// 각종 Lan서버를 시작한다.
		// 
		// Parameter : 없음
		// return : 실패 시 false 리턴
		bool ServerStart();

		// 마스터 서버 종료
		// 각종 Lan서버를 종료
		// 
		// Parameter : 없음
		// return : 없음
		void ServerStop();


	private:
		// -------------------------------------
		// Player 관리용 자료구조 함수
		// -------------------------------------

		// Player 관리 자료구조 "2개"에 Isnert
		//
		// Parameter : ClinetKey, AccountNo
		// return : 중복될 시 false.
		bool InsertPlayerFunc(UINT64 ClinetKey, UINT64 AccountNo);


		// ClinetKey 관리용 자료구조에서 Erase
		// umap으로 관리중
		//
		// Parameter : ClinetKey
		// return : 없는 유저일 시 false
		//		  : 있는 유저일 시 true
		bool ErasePlayerFunc(UINT64 ClinetKey);
		


	public:
		// -------------------------------------
		// 생성자와 소멸자
		// -------------------------------------

		// 생성자
		CMasterServer();

		// 소멸자
		virtual ~CMasterServer();

	};
}



// -----------------------
//
// 매치메이킹과 연결되는 Lan 서버
// 
// -----------------------
namespace Library_Jingyu
{
	class CMatchServer_Lan :public CLanServer
	{
		friend class CMasterServer;			


	private:
		// -----------------
		// Match 서버 관리용 자료구조
		// -----------------	

		// 접속한 매치메이킹 서버 관리용 자료구조
		// uamp 사용
		//
		// Key : SessionID, Value : CMatchingServer*
		unordered_map<ULONGLONG, CMasterServer::CMatchingServer*> m_MatchServer_Umap;

		// m_MatchServer_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_MatchServer_Umap;

		// CMatchingServer를 관리하는 TLSPool
		CMemoryPoolTLS<CMasterServer::CMatchingServer> *m_TLSPool_MatchServer;
		
		// ------------------------
		
		// Match 서버 중, 로그인 패킷까지 받은 서버 관리용 자료구조.
		// 존재 여부 판단용도.
		// uset 사용
		//
		// Key : ServerNo (매칭서버 No)
		unordered_set<int> m_LoginMatServer_Uset;

		// m_LoginMatServer_Uset관리용 SRWLOCK
		SRWLOCK m_srwl_LoginMatServer_Umap;



	private:
		// -----------------
		// 멤버 변수
		// -----------------

		// !! 부모 느낌?의 마스터 서버 !!
		CMasterServer* pMasterServer;
			   
	private:
		// -----------------
		// umap용 자료구조 관리 함수
		// -----------------

		// 접속자 관리 자료구조에 Insert
		// umap으로 관리중
		//
		// Parameter : SessionID, CMatchingServer*
		// return : 실패 시 false 리턴
		bool InsertPlayerFunc(ULONGLONG SessionID, CMasterServer::CMatchingServer* insertServer);

		// 접속자 관리 자료구조에서, 유저 검색
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID
		// return : 검색 성공 시, CMatchingServer*
		//		  : 검색 실패 시 nullptr
		CMasterServer::CMatchingServer* FindPlayerFunc(ULONGLONG SessionID);
			   
		// 접속자 관리 자료구조에서 Erase
		// umap으로 관리중
		//
		// Parameter : SessionID
		// return : 성공 시, 제거된 CMatchingServer*
		//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
		CMasterServer::CMatchingServer* ErasePlayerFunc(ULONGLONG SessionID);


	private:
		// -----------------
		// uset용 자료구조 관리 함수
		// -----------------

		// 로그인 한 유저 관리 자료구조에 Insert
		// uset으로 관리중
		//
		// Parameter : ServerNo
		// return : 실패 시 false 리턴
		bool InsertLoginPlayerFunc(int ServerNo);

		// 로그인 한 유저 관리 자료구조에서 Erase
		// uset으로 관리중
		//
		// Parameter : ServerNo
		// return : 실패 시 false 리턴
		bool EraseLoginPlayerFunc(int ServerNo);




	private:
		// -----------------
		// 내부에서만 사용하는 함수
		// -----------------

		// 마스터 서버의 정보 셋팅
		//
		// Parameter : CMasterServer* 
		// return : 없음
		void SetParent(CMasterServer* Parent);


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
		//
		// Parameter : SessionID, Payload
		// return : 없음
		void Packet_RoomInfo(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);
		
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


	private:
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
// 배틀과 연결되는 Lan 서버
// 
// -----------------------
namespace Library_Jingyu
{
	class CBattleServer_Lan :public CLanServer
	{
		friend class CMasterServer;

	private:
		// -----------------
		// 멤버 변수
		// -----------------

		// !! 부모 느낌?의 마스터 서버 !!
		CMasterServer* pMasterServer; 

	private:
		// -----------------
		// 내부에서만 사용하는 함수
		// -----------------

		// 마스터 서버의 정보 셋팅
		//
		// Parameter : CMasterServer* 
		// return : 없음
		void SetParent(CMasterServer* Parent);


	public:
		// -------------------------------------
		// 외부에서 사용 가능한 함수
		// -------------------------------------

		// 서버 시작
		// 각종 Lan서버를 시작한다.
		// 
		// Parameter : 없음
		// return : 실패 시 false 리턴
		bool ServerStart();

		// 서버 종료
		// 각종 Lan서버를 종료
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
		// -------------------------
		// 생성자와 소멸자
		// -------------------------

		// 생성자
		CBattleServer_Lan();

		// 소멸자
		virtual ~CBattleServer_Lan();

	};
}

#endif // ! __MASTER_SERVER_H__
