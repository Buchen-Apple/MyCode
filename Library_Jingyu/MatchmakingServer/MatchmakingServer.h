#ifndef __MATCH_MAKING_SERVER_H__
#define __MATCH_MAKING_SERVER_H__

#include "NetworkLib/NetworkLib_NetServer.h"
#include "NetworkLib/NetworkLib_LanClinet.h"
#include "DB_Connector/DB_Connector.h"
#include "Http_Exchange/HTTP_Exchange.h"

#include <unordered_map>

using namespace std;

// ---------------------------------------------
// 
// 매치메이킹 Net서버
//
// ---------------------------------------------
namespace Library_Jingyu
{
	class Matchmaking_Net_Server :public CNetServer
	{
		// Lan클라와 friend관계
		friend class Matchmaking_Lan_Client;
		friend class Monitor_Lan_Clinet;

		// ------------
		// 내부 구조체
		// ------------
		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
			// 매치메이킹 Net서버 정보
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
			int HeartBeat;

			// 매치메이킹 DB 정보
			TCHAR DB_IP[20];
			TCHAR DB_User[40];
			TCHAR DB_Password[40];
			TCHAR DB_Name[40];
			int  DB_Port;
			int MatchDBHeartbeat;	// 메치메이킹 DB에 몇 밀리세컨드마다 하트비트를 쏠 것인가.			

			// 매치메이킹 LanClient 정보 (마스터와 연결)
			TCHAR MasterServerIP[20];
			int MasterServerPort;
			int ClientCreateWorker;
			int ClientActiveWorker;
			int ClientNodelay;

			// 모니터링 LanClient 정보 (모니터링 서버와 연결)
			TCHAR MonitorServerIP[20];
			int MonitorServerPort;
			int MonitorClientCreateWorker;
			int MonitorClientActiveWorker;
			int MonitorClientNodelay;

		};

		// 유저 관리용 구조체
		struct stPlayer
		{
			// 토큰 아님. Net서버와 통신에 사용되는 키.
			ULONGLONG m_ullSessionID;

			// 해당 유저의 AccountNo
			INT64 m_i64AccountNo;

			// 접속한 클라이언트를 고유하게 체크하기 위함. 마스터 서버에서 사용
			UINT64	m_ui64ClientKey;					

			// 실제 로그인 패킷을 받았는지 체크. true면 받음. false면 안받음
			bool m_bLoginCheck;			

			// 배틀서버 방 접속 성공 패킷을 보낸 유저인지 체크. true면 보낸유저.
			bool m_bBattleRoomEnterCheck;

			// 마스터 서버로 방 정보 요청을 보낸 유저. true면 보낸 유저.
			bool m_bSendMaster_RoomInfo;

			// 마지막으로 패킷을 받은 시간 보관 변수
			DWORD m_dwLastPacketTime;

			stPlayer()
			{
				m_i64AccountNo = -1;	// 최초 생성시에는 -1로 시작
				m_bLoginCheck = false;	// 최초 생성시에는 flase로 시작
				m_bBattleRoomEnterCheck = false;	// 최초 생성시에는 flase로 시작
				m_bSendMaster_RoomInfo = false;
			}
		};
		

		// ------------
		// 멤버 변수
		// ------------

		// 파싱용 변수
		stConfigFile m_stConfig;	

		// HTTP로 통신하는 변수
		HTTP_Exchange* m_HTTP_Post;

		// 해당 매칭서버의 No.
		// 파싱으로 읽어온다.
		int m_iServerNo;

		// 마스터 서버로 들고갈 토큰
		// 파싱으로 읽어온다.
		char MasterToken[32];

		// ClinetKey를 만들기 위한 변수.
		// 유저 1명이 접속할 때 마다 ++한다.
		// 
		// 초기 값 : 0 (생성자에서 설정)
		UINT64 m_ClientKeyAdd;

		// 버전 코드
		// 클라가 들고온 것을 비교. 파싱으로 읽어옴.
		int m_uiVer_Code;

		// 해당 PC의 공인 IP
		char m_cServerIP[20];

		// 매치메이킹 DB에, 몇 명의 변화가 있을 때 마다 갱신할 것인가.
		int m_iMatchDBConnectUserChange; 

		// !! 마스터 lan서버와 통신하는 lan클라 !!
		Matchmaking_Lan_Client* m_pLanClient;

		// !! 모니터링 lan 서버와 통신하는 lan 클라 !!
		Monitor_Lan_Clinet* m_pMonitorLanClient;



		// -----------------------------------

		// DB 하트비트를 쏠 때, 해당 값 기준 +-100이면 쏜다.
		// 정기적으로 쿼리 전송도 있지만, 인원수 체크도 한다.
		size_t m_ChangeConnectUser;

		// m_ChangeConnectUser를 관리하는 LOCK
		SRWLOCK m_srwlChangeConnectUser;

		// -----------------------------------



		// -----------------------------------

		// DBHeartbeatThread의 핸들
		HANDLE hDB_HBThread;

		// DBHeartbeatThread 종료용 이벤트
		HANDLE hDB_HBThread_ExitEvent;

		// DBHeartbeatThread에게 일 시키기용 이벤트
		HANDLE hDB_HBThread_WorkerEvent;
		
		// -----------------------------------

		// HeartbeatThread의 핸들
		HANDLE m_hHBThread;

		// HeartbeatThread 종료용 이벤트
		HANDLE m_hHBThreadExitEvent;

		// -----------------------------------
		


		// -----------------------------------

		// 접속한 유저들을 관리하는 자료구조
		// umap으로 결정 
		//
		// 사유 : 삽입과 삭제가 빈번하기 때문에, 일반 map(Red Black Tree)을 사용하면 삽입 삭제에 대한 오버헤드가 크다.
		// 때문에, O(1)의 해쉬를 사용하는 umap 사용
		//
		// 인자 : Net의 SessionKey, stPlayer*
		unordered_map<ULONGLONG, stPlayer*>	m_umapPlayer;

		// ClientKey를 이용해 접속한 유저들을 관리하는 자료구조
		// umap으로 결정 
		//
		// 플레이어 관련 자료구조를 2개 사용하는 이유 : 마스터 서버와 Lan통신 시, ClientKey를 이용해 통신한다.
		// 마스터 LanServer에게 응답이 오면, ClinetKey를 이용해 유저를 찾아내야 한다. 그 용도.
		//
		// 인자 : ClientKey, stPlayer*
		unordered_map<UINT64, stPlayer*>	m_umapPlayer_ClientKey;

		// m_umapPlayer와 m_umapPlayer_ClientKey를 관리하는 LOCK
		SRWLOCK m_srwlPlayer;

		// stPlayer 구조체를 다루는 TLS
		CMemoryPoolTLS<stPlayer> *m_PlayerPool;

		// -----------------------------------


		// -----------------------------------

		// 로그인 한 유저를 관리하는 자료구조
		// umap
		//
		// Key : AccountNo / Value : SessionID
		unordered_map<UINT64, ULONGLONG> m_umapLoginPlayer;

		// m_umapLoginPlayer를 관리하는 LOCK
		SRWLOCK m_srwlLoginPlayer;

		// -----------------------------------
			  

		// ------------
		// 출력용
		// ------------

		// 로그인 패킷 받았을 시, 에러.
		LONG m_lTokenError;
		LONG m_lAccountError;
		LONG m_lTempError;
		LONG m_lVerError;
		LONG m_lOverlapError;

		// 로그인에 성공한 유저 수
		LONG m_lLoginUser;

		// 플레이어 구조체 할당 수
		LONG m_lstPlayer_AllocCount;

		// 배틀 방 입장 성공 패킷을 안보내고 끊은 클라이언트의 수
		LONG m_lNot_BattleRoom_Enter;

		// 방 배정에 성공한 유저 수(초당)
		LONG m_lRoomEnter_OK;

		// 하트비트로 끊긴 유저
		LONG m_lHeartBeatCount;



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

		// ClientKey 만드는 함수
		//
		// Parameter : 없음
		// return : ClientKey(UINT64)
		UINT64 CreateClientKey();

		// 접속한 모든 유저에게 shutdown 하는 함수
		//
		// Parameter : 없음
		// return : 없음
		void AllShutdown();



	private:
		// -------------------------------------
		// 스레드
		// -------------------------------------

		// matchmakingDB에 일정 시간마다 하트비트를 쏘는 스레드.
		static UINT WINAPI DBHeartbeatThread(LPVOID lParam);

		// 접속자 하트비트를 체크하는 스레드
		static UINT WINAPI HeartbeatThread(LPVOID lParam);



	private:
		// -------------------------------------
		// 자료구조 추가,삭제,검색용 함수
		// -------------------------------------

		// Player 관리 자료구조 "2개"에, 유저 추가
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID, ClientKey, stPlayer*
		// return : 추가 성공 시, true
		//		  : SessionID가 중복될 시(이미 접속중인 유저) false
		bool InsertPlayerFunc(ULONGLONG SessionID, UINT64 ClientKey, stPlayer* insertPlayer);

		// Player 관리 자료구조에서, 유저 검색
		// !!SessionID!! 를 이용해 검색
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID
		// return : 검색 성공 시, stPalyer*
		//		  : 검색 실패 시 nullptr
		stPlayer* FindPlayerFunc(ULONGLONG SessionID);

		// Player 관리 자료구조에서, 유저 검색
		// !!ClientKey!! 를 이용해 검색
		// 현재 umap으로 관리중
		// 
		// Parameter : ClientKey
		// return : 검색 성공 시, stPalyer*
		//		  : 검색 실패 시 nullptr
		stPlayer* FindPlayerFunc_ClientKey(UINT64 ClientKey);

		// Player 관리 자료구조 "2개"에서, 유저 제거 (검색 후 제거)
		// 현재 uumap으로 관리중
		// 
		// Parameter : SessionID
		// return : 성공 시, 제거된 유저 stPalyer*
		//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
		stPlayer* ErasePlayerFunc(ULONGLONG SessionID);



	private:
		// -------------------------------------
		// 로그인한 유저 관리 자료구조
		// -------------------------------------

		// 로그인 유저 관리 자료구조에 추가
		//
		// Parameter : AccountNo, SessionID
		// return : 성공 시 true / 실패 시 false
		bool InsertLoginPlayerFunc(INT64 AccountNo, ULONGLONG SessionID);

		// 로그인 유저 관리 자료구조에서 검색
		//
		// Parameter : AccountNo, (out)SessionID
		// return : 성공 시 true와 함께 인자로 던진 SessionID를 채워줌
		//		  : 실패 시 false와 함께 인자로 던진 SessionID 채우지 않음
		bool FindLoginPlayerFunc(INT64 AccountNo, ULONGLONG* SessionID);

		// 로그인 한 유저 관리 자료구조에서 유저 삭제
		//
		// Parameter : AccountNo
		// return : 제거 성공 시 true / 실패 시 false
		bool EraseLoginPlayerFunc(INT64 AccountNo);



	private:
		// -------------------------------------
		// 패킷 처리용 함수
		// -------------------------------------

		// 클라의 로그인 요청 받음
		//
		// Parameter : SessionID, Payload
		// return : 없음. 문제가 생기면, 내부에서 throw 던짐
		void Packet_Match_Login(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

		// 방 입장 성공
		// 마스터에게 방 입장 성공 패킷 보냄
		//
		// Parameter : SessionID, Payload
		// return : 없음. 문제가 생기면, 내부에서 throw 던짐
		void Packet_Battle_EnterOK(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

		// 방 입장 실패
		// 마스터에게 방 입장 실패 패킷 보냄
		//
		// Parameter : ClinetKey
		// return : 없음
		void Packet_Battle_EnterFail(UINT64 ClientKey);

	public:
		// -------------------------------------
		// 외부에서 사용 가능한 함수
		// -------------------------------------

		// 매치메이킹 서버 Start 함수
		//
		// Parameter : 없음
		// return : 실패 시 false 리턴
		bool ServerStart();

		// 매치메이킹 서버 종료 함수
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
		// -------------------------------------
		// NetServer의 가상함수
		// -------------------------------------

		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

		virtual void OnClientJoin(ULONGLONG SessionID);

		virtual void OnClientLeave(ULONGLONG SessionID);

		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		virtual void OnWorkerThreadBegin();

		virtual void OnWorkerThreadEnd();

		virtual void OnError(int error, const TCHAR* errorStr);

		virtual void OnSemaphore(ULONGLONG SessionID);



	public:	
		// -------------------------------------
		// 생성자와 소멸자
		// -------------------------------------

		// 생성자
		Matchmaking_Net_Server();

		// 소멸자
		virtual ~Matchmaking_Net_Server();

	};
}


// ---------------------------------------------
// 
// 매치메이킹 LanClient(Master서버의 LanServer와 통신)
//
// ---------------------------------------------
namespace Library_Jingyu
{
	class Matchmaking_Lan_Client :public CLanClient
	{
		// Net서버와 friend관계
		friend class Matchmaking_Net_Server;


		// ------------
		// 멤버 변수
		// ------------

		// !! 매칭 Net 서버 !!
		Matchmaking_Net_Server* m_pParent;

		// 마스터 서버와 연결된 SessionID
		ULONGLONG m_ullClientID;

		// 로그인 여부. 로그인 패킷을 정상적으로 받으면 true로 변경
		// false면 로그인 안된 상태
		bool m_bLoginCheck;

	private:
		// -------------------------------------
		// Net 서버가 호출하는 함수
		// -------------------------------------

		// 방 정보 요청
		// 마스터에게 패킷 보냄
		//
		// Parameter : SessionID
		// return : 없음. 문제가 생기면, 내부에서 throw 던짐
		void Packet_Battle_Info(ULONGLONG SessionID);


	private:
		// -------------------------------------
		// 마스터에게 받은 패킷 처리용 함수
		// -------------------------------------

		// 방 정보 요청에 대한 응답
		// 내부에서, Net서버의 SendPacket()까지 호출한다.
		// 
		// Parameter : CProtocolBuff_Lan*
		// return : 없음. 문제가 생기면, 내부에서 throw 던짐
		void Response_Battle_Info(CProtocolBuff_Lan* payload);

		// 로그인 요청에 대한 응답
		// 
		// Parameter : CProtocolBuff_Lan*
		// return : 없음. 문제가 생기면, 내부에서 throw 던짐
		void Response_Login(CProtocolBuff_Lan* payload);


	public:
		// -------------------------------------
		// 외부에서 사용 가능한 함수
		// -------------------------------------

		// 매치메이킹 LanClient 시작 함수
		//
		// Parameter : 없음
		// return : 실패 시 false 리턴
		bool ClientStart();

		// 매치메이킹 LanClient 종료 함수
		//
		// Parameter : 없음
		// return : 없음
		void ClientStop();


	protected:
		// -------------------------------------
		// 상속 관계에서만 사용 가능한 기능 함수
		// -------------------------------------
		
		// 내 부모를 채워주는 함수
		// 
		// Parameter : Net서버의 this
		// return : 없음
		void SetParent(Matchmaking_Net_Server* NetServer);


	private:
		// -----------------------
		// Lan 클라의 가상 함수
		// -----------------------

		void OnConnect(ULONGLONG ClinetID);

		void OnDisconnect(ULONGLONG ClinetID);

		void OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload);

		void OnSend(ULONGLONG ClinetID, DWORD SendSize);

		void OnWorkerThreadBegin();

		void OnWorkerThreadEnd();

		void OnError(int error, const TCHAR* errorStr);



	public:
		// -------------------------------------
		// 생성자와 소멸자
		// -------------------------------------

		// 생성자
		Matchmaking_Lan_Client();

		// 소멸자
		virtual ~Matchmaking_Lan_Client();	
	};
}


// ---------------------------------------------
// 
// 모니터링 LanClient
//
// ---------------------------------------------
namespace Library_Jingyu
{
	class Monitor_Lan_Clinet	:public CLanClient
	{
		// Net서버와 Friend 관계
		friend class Matchmaking_Net_Server;

		// 디파인 정보들 모아두기
		enum en_MonitorClient
		{
			dfSERVER_NO = 4	// 매치메이킹 서버는 4번이다
		};

		// -------------
		// 멤버 변수
		// -------------

		// 모니터링 서버로 정보 전달할 스레드의 핸들.
		HANDLE m_hMonitorThread;

		// 모니터링 서버를 종료시킬 이벤트
		HANDLE m_hMonitorThreadExitEvent;

		// 현재 모니터링 서버와 연결된 세션 ID
		ULONGLONG m_ullSessionID;


		// -------------
		// !! 매칭서버의 this !!
		// -------------
		Matchmaking_Net_Server* m_MatchServer_this;

	private:
		// ----------------------------
		// 내부에서만 사용하는 기능 함수
		// ----------------------------

		// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
		static UINT	WINAPI MonitorThread(LPVOID lParam);

		// 모니터링 서버로 데이터 전송
		//
		// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
		// return : 없음
		void InfoSend(BYTE DataType, int DataValue, int TimeStamp);

	public:

		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 시작 함수
		// 내부적으로, 상속받은 CLanClient의 Start호출.
		//
		// Parameter : 없음
		// return : 성공 시 true , 실패 시 falsel 
		bool ClientStart();

		// 종료 함수
		// 내부적으로, 상속받은 CLanClient의 Stop호출.
		// 추가로, 리소스 해제 등
		//
		// Parameter : 없음
		// return : 없음
		void ClientStop();

		// 매칭서버의 this를 입력받는 함수
		// 
		// Parameter : 쳇 서버의 this
		// return : 없음
		void SetParent(Matchmaking_Net_Server* ChatThis);


	private:
		// -----------
		// 가상 함수
		// -----------

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
		// 생성자와 소멸자
		Monitor_Lan_Clinet();
		virtual ~Monitor_Lan_Clinet();
	};
}

#endif // !__MATCH_MAKING_SERVER_H__
