#ifndef __CGAME_SERVER_H__
#define __CGAME_SERVER_H__

#include "NetworkLib\NetworkLib_MMOServer.h"
#include "NetworkLib\NetworkLib_LanClinet.h"
#include "CrashDump\CrashDump.h"


// ---------------
// CGameServer
// CMMOServer를 상속받는 게임 서버
// ---------------
namespace Library_Jingyu
{
	class CGameServer :public CMMOServer
	{
		friend class CGame_MinitorClient;

		// -----------------------
		// 이너 클래스
		// -----------------------

		// CMMOServer의 cSession을 상속받는 세션 클래스
		class CGameSession :public CMMOServer::cSession
		{
			friend class CGameServer;
			
			// -----------------------
			// 생성자와 소멸자
			// -----------------------
			CGameSession();
			virtual ~CGameSession();

			INT64 m_Int64AccountNo;


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

			// 테스트용 에코 요청
			//
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Game_EchoTest(CProtocolBuff_Net* Packet);

		};


		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
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

			// 모니터링 클라
			TCHAR MonitorServerIP[20];
			int MonitorServerPort;
			int MonitorClientCreateWorker;
			int MonitorClientActiveWorker;
			int MonitorClientNodelay;
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



	public:
		// -----------------
		// 생성자와 소멸자
		// -----------------
		CGameServer();
		virtual ~CGameServer();
		
		// Start
		// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
		//
		// Parameter : 없음
		// return : 실패 시 false
		bool GameServerStart();

		// Stop
		// 내부적으로 Stop 실행
		//
		// Parameter : 없음
		// return : 없음
		void GameServerStop();

		// 출력용 함수
		//
		// Parameter : 없음
		// return : 없음
		void ShowPrintf();

	
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


	};
}

// ---------------
// CGame_MonitorClient
// CLanClient를 상속받는 모니터링 클라. 모니터링 LanServer로 정보 전송
// ---------------
namespace Library_Jingyu
{
	class CGame_MinitorClient	:public CLanClient
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
		// !!Game 서버의 this !!
		// ----------------------
		CGameServer* m_GameServer_this;

	private:
		// -----------------------
		// 내부에서만 사용하는 기능 함수
		// -----------------------

		// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
		static UINT	WINAPI MonitorThread(LPVOID lParam);

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

		// 게임서버의 this를 입력받는 함수
		// 
		// Parameter : 게임 서버의 this
		// return : 없음
		void ParentSet(CGameServer* ChatThis);


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


#endif // !__CGAME_SERVER_H__
