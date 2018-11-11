#ifndef __BATTLESERVER_ROOM_VERSION_H__
#define __BATTLESERVER_ROOM_VERSION_H__

#include "NetworkLib/NetworkLib_MMOServer.h"
#include "NetworkLib/NetworkLib_LanClinet.h"

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

		// -----------------------
		// 이너 클래스
		// -----------------------

		// CMMOServer의 cSession을 상속받는 세션 클래스
		class CGameSession :public CMMOServer::cSession
		{
			friend class CBattleServer_Room;			


			// -----------------------
			// 멤버 변수
			// -----------------------

			// 회원 번호
			INT64 m_Int64AccountNo;

			// 유저의 ClientKey
			// 매칭서버가 발급.
			// 유저 모두에게 고유
			INT64 m_int64ClientKey;

			// CBattleServer_Room의 포인터
			CBattleServer_Room* m_pParent;

			// ClientKey의 초기 값
			const INT64 m_i64Default_CK = -1;


			
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
			// 패킷 처리 함수
			// -----------------

			// 로그인 요청 
			// 
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Auth_LoginPacket(CProtocolBuff_Net* Packet);			

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
			int XORCode1;
			int XORCode2;
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
		};
				
		// 방 구조체
		struct stRoom
		{
			// 룸 번호
			int m_iRoomNo;

			// 룸에 입장한 인원 수
			int m_iJoinUserCount;

			// 방 최대 인원 수. 
			// 이 인원이 되면 꽉 찬것으로 판단
			const int m_iMaxJoinCount = 5;

			// 방 입장 토큰 (배틀서버 입장 토큰과는 다름)
			char m_cEnterToken[32];
		};

		enum eu_DB_READ_TYPE
		{
			// 로그인 패킷에 대한 처리
			eu_LOGIN	= 0			
		};





		// -----------------------
		// 멤버 변수
		// -----------------------

		// 세션. CMMOServer와 공유.
		CGameSession* m_cGameSession;

		// Config 변수
		stConfigFile m_stConfig;

		// 모니터링 클라
		CGame_MinitorClient* m_Monitor_LanClient;

		// 버전 코드
		// 클라가 들고온 것을 비교. 파싱으로 읽어옴.
		int m_uiVer_Code;

		// shDB와 통신하는 변수
		shDB_Communicate m_shDB_Communicate;	


		

		// -----------------------
		// 토큰관리 변수
		// -----------------------

		// 입장 토큰 관리
		// 배틀서버는 이 2개 중 하나라도 일치한다면 맞다고 통과시킨다.

		// 배틀서버 입장 토큰 1번
		// "현재" 토큰을 보관한다.
		// 마스터와 연결된 랜 클라가 생성 및 갱신한다.
		char cConnectToken_Now[32];

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

		// 최대 존재할 수 있는 방 수
		// 고정 값
		const int m_iMaxTotalRoomCount = 500;

		// 최대 존재할 수 있는 대기방 수
		// 고정 값
		const int m_iMaxWaitRoomCount = 200;


		// 현재 대기방 수 (5/5가 되지 않은 방)
		int m_iNowWaitRoomCount;

		// 현재 게임 내에 만들어진 방 수 (모든 방 합쳐서)
		int m_iNowTotalRoomCount;

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

		// Start
		// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
		//
		// Parameter : 없음
		// return : 실패 시 false
		bool BattleServerStart();

		// Stop
		// 내부적으로 Stop 실행
		//
		// Parameter : 없음
		// return : 없음
		void BattleServerStop();

		// 출력용 함수
		//
		// Parameter : 없음
		// return : 없음
		void ShowPrintf();



	private:
		// -----------------------
		// 패킷 후처리 함수
		// -----------------------

		// Login 패킷 후처리
		//
		// Parameter : DB_WORK_LOGIN*
		// return : 없음
		void Auth_LoginPacket_Last(DB_WORK_LOGIN* DBData);




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

		// AccountNo 자료구조에서 유저를 검색하는 함수
		//
		// Parameter : AccountNo
		// return : 성공 시 ClientKey
		//		  : 실패 시 nullptr
		CGameSession* FindAccountNoFunc(INT64 AccountNo);

		// AccountNo 자료구조에서 유저를 제거하는 함수
		//
		// Parameter : AccountNo
		// return : 성공 시 true
		//		  : 실패 시 false
		bool EraseAccountNoFunc(INT64 AccountNo);


	protected:
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



#endif // !__BATTLESERVER_ROOM_VERSION_H__
