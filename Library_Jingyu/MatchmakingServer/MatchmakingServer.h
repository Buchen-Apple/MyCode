#ifndef __MATCH_MAKING_SERVER_H__
#define __MATCH_MAKING_SERVER_H__

#include "NetworkLib/NetworkLib_NetServer.h"
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
			int XORCode1;
			int XORCode2;
			int Nodelay;
			int MaxJoinUser;
			int LogLevel;


			// 메치메이킹 DB 정보
			TCHAR DB_IP[20];
			TCHAR DB_User[40];
			TCHAR DB_Password[40];
			TCHAR DB_Name[40];
			int  DB_Port;
			int MatchDBHeartbeat;	// 메치메이킹 DB에 몇 밀리세컨드마다 하트비트를 쏠 것인가.
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

			stPlayer()
			{
				m_bLoginCheck = false;	// 최초 생성시에는 flase로 시작
			}
		};
		

		// ------------
		// 멤버 변수
		// ------------

		// 파싱용 변수
		stConfigFile m_stConfig;	

		// HTTP로 통신하는 변수
		HTTP_Exchange* m_HTTP_Post;

		// DB_Connector TLS 버전
		// MatchmakingDB와 연결
		CBConnectorTLS* m_MatchDBcon;

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
		


		// -----------------------------------

		// 접속한 유저들을 관리하는 자료구조
		// umap으로 결정 
		//
		// 사유 : 삽입과 삭제가 빈번하기 때문에, 일반 map(Red Black Tree)을 사용하면 삽입 삭제에 대한 오버헤드가 크다.
		// 때문에, O(1)의 해쉬를 사용하는 umap 사용
		//
		// 인자 : Net의 SessionKey, stPlayer*
		unordered_map<ULONGLONG, stPlayer*>	m_umapPlayer;

		// m_umapPlayer를 관리하는 LOCK
		SRWLOCK m_srwlPlayer;

		// stPlayer 구조체를 다루는 TLS
		CMemoryPoolTLS<stPlayer> *m_PlayerPool;

		// -----------------------------------
			   


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

		// 매치메이킹 DB에, 초기 정보를 Insert하는 함수.
		// 이미, 데이터가 존재하는 경우, 정보를 Update한다.
		// 
		// Parameter : 없음
		// return : 없음
		void ServerInfo_DBInsert();


	private:
		// -------------------------------------
		// 스레드
		// -------------------------------------

		// matchmakingDB에 일정 시간마다 하트비트를 쏘는 스레드.
		static UINT WINAPI DBHeartbeatThread(LPVOID lParam);


	private:
		// -------------------------------------
		// 자료구조 추가,삭제,검색용 함수
		// -------------------------------------

		// Player 관리 자료구조에, 유저 추가
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID, stPlayer*
		// return : 추가 성공 시, true
		//		  : SessionID가 중복될 시(이미 접속중인 유저) false
		bool InsertPlayerFunc(ULONGLONG SessionID, stPlayer* insertPlayer);

		// Player 관리 자료구조에서, 유저 검색
		// 현재 map으로 관리중
		// 
		// Parameter : SessionID
		// return : 검색 성공 시, stPalyer*
		//		  : 검색 실패 시 nullptr
		stPlayer* FindPlayerFunc(ULONGLONG SessionID);


		// Player 관리 자료구조에서, 유저 제거 (검색 후 제거)
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID
		// return : 성공 시, 제거된 유저 stPalyer*
		//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
		stPlayer* ErasePlayerFunc(ULONGLONG SessionID);


	private:
		// -------------------------------------
		// 패킷 처리용 함수
		// -------------------------------------

		// 매치메이킹 서버로 로그인 요청
		//
		// Parameter : SessionID, Payload
		// return : 없음. 문제가 생기면, 내부에서 throw 던짐
		void Packet_Match_Login(ULONGLONG SessionID, CProtocolBuff_Net* Payload);



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


#endif // !__MATCH_MAKING_SERVER_H__
