#ifndef __MONITOR_SERVER_H__
#define __MONITOR_SERVER_H__

#include "NetworkLib\NetworkLib_NetServer.h"
#include "NetworkLib\NetworkLib_LanServer.h"
#include "Protocol_Set/CommonProtocol_2.h"
#include "CrashDump\CrashDump.h"

#include <vector>


// ----------------------
// 모니터링 Net 서버
// ----------------------
namespace Library_Jingyu
{
	class CNet_Monitor_Server :public CNetServer
	{
		// Config 정보 보관 구조체
		struct stConfigFile
		{
			// 모니터링 Net 서버
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

			// 모니터링 Lan 서버
			TCHAR LanBindIP[20];
			int LanPort;
			int LanCreateWorker;
			int LanActiveWorker;
			int LanCreateAccept;
			int LanNodelay;
			int LanMaxJoinUser;
			int LanLogLevel;

			// DB
			TCHAR DB_IP[20];
			TCHAR DB_User[40];
			TCHAR DB_Password[40];
			TCHAR DB_Name[40];
			int  DB_Port;

		};
		
		friend class CLan_Monitor_Server;

		// 플레이어 구조체
		struct stPlayer
		{
			ULONGLONG m_ullSessionID;
			bool m_bLoginCheck;		// true면 로그인 패킷 처리된 플레이어
		};

	private:
		// -----------------------
		// 멤버 변수
		// -----------------------

		// 로그인 툴이 들고오는 키		
		char m_cLoginKey[32];

		// Config 저장 변수
		stConfigFile m_stConfig;

		// 플레이어 구조체 관리 TLS
		// stPlayer를 다룬다.
		CMemoryPoolTLS<stPlayer> *m_PlayerTLS;

		// 플레이어 보관 자료구조 
		// Vector로 관리중
		// 요소 : stPlayer*
		//
		// 사유 : 해당 자료구조를 이용해 주로 [순회]를 한다.
		// 그리고, 접속하는 클라이언트는 모니터링 툴이기 때문에, 아무리 많이 들어와도 100 이상 안들어온다.
		// 때문에 순회에 적합한 백터 사용
		vector<stPlayer*> m_vectorPlayer;
		SRWLOCK	m_vectorSrwl;

		// !! 모니터링 랜 서버 !!
		CLan_Monitor_Server* m_CLanServer;


	public:
		// 생성자, 소멸자
		CNet_Monitor_Server();
		virtual ~CNet_Monitor_Server();

	private:
		// -----------------------
		// 내부에서만 사용 가능한 기능함수
		// -----------------------

		// 파일에서 Config 정보 읽어오기
		// 
		// Parameter : config 구조체
		// return : 정상적으로 셋팅 시 true
		//		  : 그 외에는 false
		bool SetFile(stConfigFile* pConfig);
			
		// 플레이어 자료구조에 플레이어 추가.
		//
		// Parameter : SessionID
		// return : 없음
		void InsertPlayer(ULONGLONG SessionID);

		// 플레이어 자료구조에서 플레이어 제거
		//
		// Parameter : SessionID
		// return : 없는 플레이어 일 시 false.
		bool ErasePlayer(ULONGLONG SessionID);
		
		// 접속한 클라들에게 정보 뿌리기 (브로드 캐스팅)
		//
		// Parameter : 서버No, 데이터 타입, 데이터 값, 타임스탬프
		// return : 없음
		void DataBroadCasting(BYTE ServerNo, BYTE DataType, int DataValue, int TimeStamp);
		

		// 로그인 요청 패킷 처리
		//
		// Parameter : 세션ID, Net 직렬화버퍼
		void LoginPakcet(ULONGLONG SessionID, CProtocolBuff_Net* Payload);



	public:
		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 모니터링 Net서버 시작
		// 내부에서는 CNetServer의 Start 함수 호출
		//
		// Parameter : 없음
		// return : 없음
		bool ServerStart();

		// 모니터링 Net서버 종료
		// 내부에서는 CNetServer의 Stop 함수 호출
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

		// 세마포어 발생 시 호출되는 함수
		//
		// parameter : SessionID
		// return : 없음
		virtual void OnSemaphore(ULONGLONG SessionID);

	};
}



// ----------------------
// 모니터링 Lan 서버
// ----------------------
namespace Library_Jingyu
{
	class CLan_Monitor_Server :public CLanServer
	{
		friend class CNet_Monitor_Server;

		// DB에 보낼 정보 보관 구조체
		struct stDBWriteInfo
		{
			int m_iType = 0;
			char m_cServerName[64] = { 0, };
			int m_iValue = 0;
			int m_iMin = 0x0fffffff;
			int m_iMax = 0;
			ULONGLONG m_iTotal = 0;
			int m_iTotalCount = 0;

			float m_iAvr = 0;
			int m_iServerNo = 0;

			// 초기화
			void init()
			{
				m_iValue = 0;
				m_iMin = 0x0fffffff;
				m_iMax = 0;
				m_iTotal = 0;
				m_iTotalCount = 0;
			}
		};

		// 접속한 랜 클라이언트 보관 구조체
		struct stServerInfo
		{
			ULONGLONG m_ullSessionID;
			int m_bServerNo;
		};


	private:
		// -----------------------
		// 멤버 변수
		// -----------------------

		// !! 모니터링 넷 서버(부모)의 this !!
		CNet_Monitor_Server* m_ParentThis;


		// 접속한 랜클라 관리할 구조체 배열
		// SessionID, ServerNo 관리
		stServerInfo m_arrayJoinServer[10];

		// 구조체 배열 안에 있는 유저 수 (접속한 랜 클라 수)
		int m_iArrayCount;

		// 구조체 배열 락
		SRWLOCK srwl;

		// DB 정보 보관 구조체와 락
		stDBWriteInfo m_stDBInfo[dfMONITOR_DATA_TYPE_END - 1];
		SRWLOCK DBInfoSrwl;

		// DBWriteThread 핸들
		HANDLE m_hDBWriteThread;

		// DBWriteThread 종료용 이벤트
		HANDLE m_hDBWriteThreadExitEvent;


	private:
		// -----------------------
		// 내부 기능 함수
		// -----------------------
		
		// 부모(NetServer)의 This 셋팅 함수
		//
		// Parameter : 모니터링 넷서버의 This
		void ParentSet(CNet_Monitor_Server* NetServer);


		// 모니터링 클라이언트로부터 받은 정보 갱신
		// 정보 갱신 후, 각 뷰어들에게 전송
		//
		// Parameter : Lan직렬화 버퍼
		// return : 없음
		void DataUpdatePacket(CProtocolBuff_Lan* Payload);
		
		// 로그인 요청 처리
		//
		// Parameter : SessionID, Lan직렬화 버퍼
		// return : 없음
		void LoginPacket(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// DB에 정보 저장 스레드
		// 1분에 1회 DB에 Insert
		static UINT	WINAPI DBWriteThread(LPVOID lParam);


	public:
		// -----------------------
		// 외부에서 접근 가능한 기능 함수
		// -----------------------

		// 모니터링 Lan 서버 시작
		// 내부적으로 CLanServer의 Start함수 호출
		//
		// Parameter : 없음
		// return :  없음
		bool LanServerStart();

		// 모니터링 Lan 서버 종료
		// 내부적으로 CLanServer의 stop함수 호출
		//
		// Parameter : 없음
		// return :  없음
		void LanServerStop();




	public:
		// 생성자
		CLan_Monitor_Server();

		// 소멸자
		virtual ~CLan_Monitor_Server();

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
	};
}


#endif // !__MONITOR_SERVER_H__
