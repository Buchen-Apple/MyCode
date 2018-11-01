#ifndef __MATCH_MAKING_SERVER_H__
#define __MATCH_MAKING_SERVER_H__

#include "NetworkLib/NetworkLib_NetServer.h"
#include "DB_Connector/DB_Connector.h"

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


			// 메치메이킹 DB의 정보
			TCHAR DB_IP[20];
			TCHAR DB_User[40];
			TCHAR DB_Password[40];
			TCHAR DB_Name[40];
			int  DB_Port;
			int MatchDBHeartbeat;	// 메치메이킹 DB에 몇 밀리세컨드마다 하트비트를 쏠 것인가.

			// 매치메이킹 Lan 클라 정보.
			int ServerNo;	// 해당 정보는, Net서버에서도 쓰지만(DB 하트비트), Lan클라에게 더 중요한 정보이기 때문에 Lan클라 정보로 분류
			char MasterToken[32];
		};
		

		// ------------
		// 멤버 변수
		// ------------

		// 파싱용 변수
		stConfigFile m_stConfig;

		// DBHeartbeatThread의 핸들
		HANDLE hDB_HBThread;

		// DBHeartbeatThread 종료용 이벤트
		HANDLE hDB_HBThread_ExitEvent;

		// DB_Connector TLS 버전
		//
		// MatchmakingDB와 연결
		CBConnectorTLS* m_MatchDB_Connector;




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


		// -------------------------------------
		// 스레드
		// -------------------------------------

		// matchmakingDB에 일정 시간마다 하트비트를 쏘는 스레드.
		static UINT WINAPI DBHeartbeatThread(LPVOID lParam);



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
