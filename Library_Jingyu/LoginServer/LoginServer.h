#ifndef __LOGIN_SERVER_H__
#define __LOGIN_SERVER_H__

#include "NetworkLib\NetworkLib_NetServer.h"
#include "NetworkLib\NetworkLib_LanServer.h"
#include "NetworkLib\NetworkLib_LanClinet.h"
#include "CPUUsage\CPUUsage.h"
#include "PDHClass\PDHCheck.h"
#include "DB_Connector\DB_Connector.h"

#include <unordered_map>

using namespace std;

// ----------------------------------
// LoginServer (NetServer)
// ----------------------------------
namespace Library_Jingyu
{
	class CLogin_NetServer :public CNetServer
	{
		friend class CLogin_LanServer;
		friend class CMoniter_Clinet;

		///////////////////////
		// 내부 구조체
		///////////////////////

		// 접속한 유저 관리 구조체
		struct stPlayer
		{
			INT64 m_i64AccountNo;
			WCHAR m_wcID[20];
			WCHAR m_wcNickName[20];

			bool m_bLoginCheck;		// 로그인 여부. true면 로그인 된 유저.
		};

		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
			// 로그인 Net서버 정보
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

			// 게임서버와 채팅서버의 정보.
			TCHAR GameServerIP[16];
			int GameServerPort;

			TCHAR ChatServerIP[16];
			int ChatServerPort;

			// AccountDB에 연결 시 필요한 정보
			TCHAR DB_IP[20];
			TCHAR DB_User[40];
			TCHAR DB_Password[40];
			TCHAR DB_Name[40];
			int  DB_Port;

			// 로그인 Lan서버 정보
			TCHAR LanBindIP[20];
			int LanPort;
			int LanCreateWorker;
			int LanActiveWorker;
			int LanCreateAccept;
			int LanNodelay;
			int LanMaxJoinUser;
			int LanLogLevel;

			// 모니터링 클라 정보
			TCHAR MonitorServerIP[20];
			int	MonitorServerPort;
			int ClientCreateWorker;
			int ClientActiveWorker;
			int ClientNodelay;
		};

		
	private:

		/////////////////////////////
		// 멤버 변수
		/////////////////////////////

		stConfigFile m_stConfig;

		// Chat, Game의 LanClient와 통신할 LanServer
		CLogin_LanServer* m_cLanS;

		// 모니터링 서버에게 정보를 보내줄 LanClient
		CMoniter_Clinet* m_LanMonitorC;

		// 유저 구조체 관리 TLS
		CMemoryPoolTLS< stPlayer >  *m_MPlayerTLS;

		// 접속한 유저 관리 자료구조
		// umap 사용
		//
		// Key : Net서버의 SessionID
		// Value : stPlayer*
		unordered_map<ULONGLONG, stPlayer*> m_umapJoinUser;

		// DB_Connector TLS 버전
		//
		// AccountDB와 연결
		CBConnectorTLS* m_AcountDB_Connector;

		// 유저 관리 자료구조에 사용되는 락
		SRWLOCK srwl;

	private:	

		/////////////////////////////
		// 기능 처리 함수
		/////////////////////////////

		// 파일에서 Config 정보 읽어오기
		// 
		// 
		// Parameter : config 구조체
		// return : 정상적으로 셋팅 시 true
		//		  : 그 외에는 false
		bool SetFile(stConfigFile* pConfig);



		/////////////////////////////
		// 자료구조 관리 함수
		/////////////////////////////

		// 유저 관리 자료구조에서 유저 검색
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID
		// return : 검색 성공 시 stPlayer*
		//		  : 실패 시 nullptr
		stPlayer* FindPlayerFunc(ULONGLONG SessionID);

		// 유저 관리 자료구조에, 새로 접속한 유저 추가
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID, stPlayer*
		// return : 추가 성공 시, true
		//		  : SessionID가 중복될 시 false
		bool InsertPlayerFunc(ULONGLONG SessionID, stPlayer* InsertPlayer);

		// 유저 관리 자료구조에 있는, 유저의 정보 갱신
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID, AccountNo, UserID(cahr*), NickName(char*)
		// return : 없음
		void SetPlayerFunc(ULONGLONG SessionID, INT64 AccountNo, char* UserID, char* NickName);

		// 유저 관리 자료구조에 있는, 유저 삭제
		// 현재 umap으로 관리중
		// 
		// Parameter : SessionID
		// return : 성공 시, stPlayer*
		//		  : 실패 시, nullptr
		stPlayer* ErasePlayerFunc(ULONGLONG SessionID);


		/////////////////////////////
		// Lan의 패킷 처리 함수
		/////////////////////////////		

		// LanClient에게 받은 패킷 처리 함수 (메인)
		//
		// Parameter : 패킷 타입, AccountNo, SessionID(LanClient에게 보냈던 Parameter)
		// return : 없음
		void LanClientPacketFunc(WORD Type, INT64 AccountNo, ULONGLONG SessionID);


		/////////////////////////////
		// Net의 패킷 처리 함수
		/////////////////////////////

		// 로그인 요청
		// 
		// Parameter : SessionID, Packet*
		// return : 없음
		void LoginPacketFunc(ULONGLONG SessionID, CProtocolBuff_Net* Packet);		
		
		// "성공" 패킷 만들고 보내기
		//
		// Parameter : SessionID, AccountNo, Status, UserID, NickName
		// return : 없음
		void Success_Packet(ULONGLONG SessionID, INT64 AccountNo, BYTE Status);

		// "실패" 패킷 만들고 보내기
		//
		// Parameter : SessionID, AccountNo, Status, UserID, NickName
		// return : 없음
		void Fail_Packet(ULONGLONG SessionID, INT64 AccountNo, BYTE Status, char* UserID = nullptr, char* NickName = nullptr);
			   

	public:
		// 출력용 함수
		void ShowPrintf();
	

	public:
		// 생성자
		CLogin_NetServer();

		// 소멸자
		virtual ~CLogin_NetServer();


	public:
		/////////////////////////////
		// 외부에서 호출 가능한 기능 함수
		/////////////////////////////

		// 로그인 서버 시작 함수
		// 내부적으로 NetServer의 Start 호출
		// 
		// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
		// return true : 성공
		bool ServerStart();

		// 로그인 서버 종료 함수
		//
		// Parameter : 없음
		// return : 없음
		void ServerStop();


	private:
		/////////////////////////////
		// 가상함수
		/////////////////////////////

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
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

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


// ----------------------------------
// LoginServer (LanServer)
// ----------------------------------
namespace Library_Jingyu
{	
	class CLogin_LanServer :public CLanServer
	{
		friend class CLogin_NetServer;

		// 내 부모(CLogin_NetServer)의 포인터
		CLogin_NetServer* m_cParentS;

		// 접속한 서버 관리할 배열
		// SessionID를 들고 있다.
		ULONGLONG m_arrayJoinServer[10];

		// 배열 안에 있는 유저 수
		int m_iArrayCount;

		// 배열의 락
		SRWLOCK srwl;

	private:
		// 부모 셋팅
		//
		// Parameter : CLogin_NetServer*
		// return : 없음
		void ParentSet(CLogin_NetServer* parent);

		// GameServer, ChatServer의 LanClient에 새로운 유저 접속 알림
		// --- CLogin_NetServer의 "OnRecv"에서 호출 ---
		// 
		// Parameter : AccountNo, Token(64Byte), ULONGLONG Parameter
		// return : 없음
		void UserJoinSend(INT64 AccountNo, char* Token, ULONGLONG Parameter);		

	public:
		// 생성자
		CLogin_LanServer();

		// 소멸자
		virtual ~CLogin_LanServer();


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

		
	};
}


// ----------------------------------
// 모니터링 클라이언트 (LanClient)
// ----------------------------------
namespace Library_Jingyu
{
	// 정보를 수집해서 모니터링 서버(LanServer로 전달한다)
	class CMoniter_Clinet :public CLanClient
	{
		friend class CLogin_NetServer;

		// 디파인 정보들 모아두기
		enum en_MonitorClient
		{
			dfSERVER_NO = 2	// 하드웨어를 전송하는 서버의 No는 2번이다.
		};

		// 모니터링 서버로 정보 전달할 스레드의 핸들.
		HANDLE m_hMonitorThread;

		// 모니터링 서버를 종료시킬 이벤트
		HANDLE m_hMonitorThreadExitEvent;

		// 현재 모니터링 서버와 연결된 세션 ID
		ULONGLONG m_ullSessionID;	


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
		CMoniter_Clinet();
		virtual ~CMoniter_Clinet();

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


	public:
		// -----------------------
		// 가상함수
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


#endif // !__LOGIN_SERVER_H__
