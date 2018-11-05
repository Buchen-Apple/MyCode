#ifndef __MASTER_SERVER_H__
#define __MASTER_SERVER_H__

#include "NetworkLib/NetworkLib_LanServer.h"
#include "Log/Log.h"
#include "ObjectPool/Object_Pool_LockFreeVersion.h"
#include "Parser/Parser_Class.h"
#include "Protocol_Set/CommonProtocol_2.h"		// 궁극적으로는 CommonProtocol.h로 이름 변경 필요. 지금은 채팅서버에서 로그인 서버를 이용하는 프로토콜이 있어서 _2로 만듬.

#include <list>
#include <unordered_map>
#include <queue>


using namespace std;

// -----------------------
//
// 마스터 Lan 서버
// 
// -----------------------
namespace Library_Jingyu
{
	class CMasterServer_Lan :public CLanServer
	{

	private:
		// -----------------
		// 이너 클래스
		// -----------------	

		// CRoom 전방선언
		class CRoom;

		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
			// 마스터 Lan 서버 정보
			TCHAR BindIP[20];
			int Port;
			int CreateWorker;
			int ActiveWorker;
			int CreateAccept;
			int Nodelay;
			int MaxJoinUser;
			int LogLevel;
		};		

		// 배틀서버 클래스
		class CBattleServer
		{
			ULONGLONG m_ullSessionID;
			int m_iServerNo;
		};

		// 매칭서버 클래스
		class CMatchingServer
		{
			ULONGLONG m_ullSessionID;
			int m_iServerNo;
		};			

		// Room_Priority_Queue에서 비교연산자로 사용할 클래스
		class RoomCMP
		{
			// m_iEmptyCount가 낮은 순으로 pop
			bool operator()(CRoom* t, CRoom* u)
			{
				return t->m_iEmptyCount > u->m_iEmptyCount;
			}
		};

		// 유저 클래스
		class CPlayer
		{
			UINT64	m_ui64ClinetKey;
			UINT64	m_ui64AccountNo;

			// 유저가 접속하고 있는 Room
			CRoom* pJoinRoom = nullptr;
		};

		// Room 클래스
		class CRoom
		{
			friend class RoomCMP;

			int m_iRoomNo;

			// 방에, 여유 공간이 얼마인지.
			// ex) 유저가 3/5상태라면, 해당 값은 2임.
			int m_iEmptyCount;

			// 룸이 존재하는 배틀 서버
			CBattleServer* pBattle = nullptr;

			// 룸에 Join한 유저를 관리하는 list
			list<CPlayer*> m_JoinUser_List;
		};			


	private:
		// -----------------
		// Match 서버 관리용 자료구조
		// -----------------	

		// 접속한 매치메이킹 서버 관리용 자료구조
		// uamp 사용
		//
		// Key : SessionID, Value : CMatchingServer*
		unordered_map<ULONGLONG, CMatchingServer*> m_MatchServer_Umap;

		// m_MatchServer_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_MatchServer_Umap;

		// CMatchingServer를 관리하는 TLSPool
		CMemoryPoolTLS<CMatchingServer> *m_TLSPool_MatchServer;



	private:
		// -----------------
		// Battle 서버 관리용 자료구조
		// -----------------	

		// 접속한 배틀 서버 관리용 자료구조
		// uamp 사용
		//
		// Key : SessionID, Value : CBattleServer*
		unordered_map<ULONGLONG, CBattleServer*> m_BattleServer_Umap;

		// m_BattleServer_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_BattleServer_Umap;

		// CBattleServer를 관리하는 TLSPool
		CMemoryPoolTLS<CBattleServer> *m_TLSPool_BattleServer;



	private:
		// -----------------
		// Player 관리용 자료구조
		// -----------------			   

		// ClientKey를 이용해, 플레이어를 관리하는 자료구조
		// umap 사용
		//
		// Key : ClientKey, Value : CPlayer*
		unordered_map<UINT64, CPlayer*> m_ClientKey_Umap;

		// m_ClientKey_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_ClientKey_Umap;	

		// -----------------------

		// AccountNo를 이용해, 플레이어를 관리하는 자료구조
		// umap 사용
		//
		// Key : AccountNo, Value : CPlayer*
		unordered_map<UINT64, CPlayer*> m_AccountNo_Umap;

		// m_AccountNo_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_AccountNo_Umap;

		// -----------------------

		// CPlayer를 관리하는 TLSPool
		CMemoryPoolTLS<CPlayer> *m_TLSPool_Player;



	private:
		// -----------------
		// Room 관리용 자료구조
		// -----------------

		// RoomNo를 이용해, 룸을 관리하는 자료구조
		// umap 사용
		// !! 여유 유저 수가 0이 되어, 삭제될 예정인 Room만 관리한다 !!
		//
		// Key : RoomNo, Value : CPlayer*
		unordered_map<int, CRoom*> m_Room_Umap;

		// m_Room_Umap관리용 SRWLOCK
		SRWLOCK m_srwl_Room_Umap;

		// -----------------------			

		// 룸을 관리하는 자료구조2
		// Priority_Queue 사용
		// 방 정보 요청 등, 여유 유저 수가 0이 아닌 모든 룸을 관리.
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

		// 덤프용
		CCrashDump* m_CDump;

		// 로그 저장용
		CSystemLog* m_CLog;

		// 파일 읽어오기 용 구조체
		stConfigFile m_stConfig;


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
		CMasterServer_Lan();

		// 소멸자
		virtual ~CMasterServer_Lan();

	};
}


#endif // ! __MASTER_SERVER_H__
