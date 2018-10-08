#ifndef __NETWORK_LIB_MMOSERVER_H__
#define __NETWORK_LIB_MMOSERVER_H__

#include <windows.h>
#include "ProtocolBuff\ProtocolBuff(Net)_ObjectPool.h"
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "LockFree_Stack\LockFree_Stack.h"
#include "LockFree_Queue\LockFree_Queue.h"
#include "RingBuff\RingBuff.h"
#include "NormalTemplate_Queue\Normal_Queue_Template.h"

// ------------------------
// MMOServer
// ------------------------
namespace Library_Jingyu
{

	class CMMOServer
	{

		// 한 번에 샌드할 수 있는 WSABUF의 카운트
#define dfSENDPOST_MAX_WSABUF			300

	protected:
		// enum 전방선언
		enum class euError : int
		{
			NETWORK_LIB_ERROR__NORMAL = 0,					// 에러 없는 기본 상태
			NETWORK_LIB_ERROR__WINSTARTUP_FAIL,				// 윈속 초기화 하다가 에러남
			NETWORK_LIB_ERROR__CREATE_IOCP_PORT,			// IPCP 만들다가 에러남
			NETWORK_LIB_ERROR__THREAD_CREATE_FAIL,			// 스레드 만들다가 실패 
			NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL,			// 소켓 생성 실패 
			NETWORK_LIB_ERROR__BINDING_FAIL,				// 바인딩 실패
			NETWORK_LIB_ERROR__LISTEN_FAIL,					// 리슨 실패
			NETWORK_LIB_ERROR__SOCKOPT_FAIL,				// 소켓 옵션 변경 실패
			NETWORK_LIB_ERROR__WSAENOBUFS,					// WSASend, WSARecv시 버퍼사이즈 부족
			// ------------ 여기까지는 NetServer 자체적으로 남기는 에러. 밖에서 신경 쓸 필요 없음

			NETWORK_LIB_ERROR__IOCP_ERROR,					// IOCP 자체 에러
			NETWORK_LIB_ERROR__NOT_FIND_CLINET,				// map 검색 등을 할때 클라이언트를 못찾은경우.
			NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL,		// Enqueue사이즈가 꽉찬 유저
			NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY,			// Dequeue 시, 큐가 비어있는 유저. Peek을 시도하는데 큐가 비었을 상황은 없음
			NETWORK_LIB_ERROR__WSASEND_FAIL,				// SendPost에서 WSASend 실패			
			NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT,		// 엑셉트 스레드 비정상 종료. 보통 accept()함수에서 이상한 에러가 나온것.
			NETWORK_LIB_ERROR__A_THREAD_IOCPCONNECT_FAIL,	// 엑셉트 스레드에서 IOCP 연결 실패
			NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT,		// 워커 스레드 비정상 종료. 
			NETWORK_LIB_ERROR__WFSO_ERROR,					// WaitForSingleObject 에러.
			NETWORK_LIB_ERROR__IOCP_IO_FAIL,				// IOCP에서 I/O 실패 에러. 이 때는, 일정 횟수는 I/O를 재시도한다.
			NETWORK_LIB_ERROR__JOIN_USER_FULL,				// 유저가 풀이라서 더 이상 접속 못받음
			NETWORK_LIB_ERROR__RECV_CODE_ERROR,				// RecvPost에서 헤더 코드가 다른 데이터일 경우.
			NETWORK_LIB_ERROR__RECV_CHECKSUM_ERROR,			// RecvPost에서 Decode 중 체크섬이 다름
			NETWORK_LIB_ERROR__RECV_LENBIG_ERROR,			// RecvPost에서 헤더 안에 Len이 비정상적으로 큼.
		};

		// 세션 모드 enum
		enum class euSessionModeState
		{
			MODE_NONE = 0,			// 세션 미사용 상태
			MODE_AUTH,				// 연결 후 인증모드 상태
			MODE_AUTH_TO_GAME,		// 게임모드로 전환
			MODE_GAME,				// 인증 후 게임모드 상태
			MODE_WAIT_LOGOUT		// 최종 종료
		};

		// 각 스레드에서 처리하는 패킷의 수 enum
		enum class euDEFINE
		{
			// ******* AUTH 스레드용 *******
			eu_AUTH_PACKET_COUNT = 1,			// 1프레임동안, 1명의 유저당 몇 개의 패킷을 처리할 것인가
			eu_AUTH_SLEEP = 1,					// Sleep하는 시간(밀리세컨드)	
			eu_AUTH_NEWUSER_PACKET_COUNT = 50,   // 1프레임동안, Accept Socket Queue에서 빼는 패킷의 수. (즉, 1프레임에 None에서 Auth로 변경되는 유저 수)


			// ******* GAME 스레드용 *******
			eu_GAME_PACKET_COUNT = 200,			// 1프레임에, 1명의 유저당 몇 개의 패킷을 처리할 것인가
			eu_GAME_SLEEP = 1,					// Sleep하는 시간(밀리세컨드)	
			eu_GAME_NEWUSER_JOIN_COUNT = 50,	// 1프레임 동안, AUTH_IN_GAME에서 GAME으로 변경되는 유저의 수	

			// ******* RELEAE 스레드용 *******
			eu_RELEASE_SLEEP = 1					// Sleep하는 시간(밀리세컨드)	
		};


		// ------------------
		// 이너 클래스
		// ------------------

		// 세션 클래스
		class cSession
		{
			friend class CMMOServer;

			// -----------
			// 멤버 변수
			// -----------

			// Auth To Game Falg
			// true면 Game모드로 전환될 유저
			LONG m_lAuthToGameFlag;

			// 유저의 모드 상태
			euSessionModeState m_euMode;

			// 로그아웃 플래그
			// true면 로그아웃 될 유저.
			LONG m_lLogoutFlag;

			// CompleteRecvPacket(Queue)
			CNormalQueue<CProtocolBuff_Net*>* m_CRPacketQueue;

			// 세션과 연결된 소켓
			SOCKET m_Client_sock;

			// 해당 세션이 들어있는 배열 인덱스
			WORD m_lIndex;

			// 연결된 세션의 IP와 Port
			TCHAR m_IP[30];
			USHORT m_prot;

			// Send overlapped구조체
			OVERLAPPED m_overSendOverlapped;

			// 세션 ID. 컨텐츠와 통신 시 사용.
			ULONGLONG m_ullSessionID;

			// I/O 카운트. WSASend, WSARecv시 1씩 증가.
			// 0이되면 접속 종료된 유저로 판단.
			// 사유 : 연결된 유저는 WSARecv를 무조건 걸기 때문에 0이 될 일이 없다.
			LONG	m_lIOCount;

			// 현재, WSASend에 몇 개의 데이터를 샌드했는가. (바이트 아님! 카운트. 주의!)
			int m_iWSASendCount;

			// Send한 직렬화 버퍼들 저장할 포인터 변수
			CProtocolBuff_Net* m_PacketArray[dfSENDPOST_MAX_WSABUF];

			// Send가능 상태인지 체크. 1이면 Send중, 0이면 Send중 아님
			LONG	m_lSendFlag;

			// Send버퍼. 락프리큐 구조. 패킷버퍼(직렬화 버퍼)의 포인터를 다룬다.
			CLF_Queue<CProtocolBuff_Net*>* m_SendQueue;

			// Recv overlapped구조체
			OVERLAPPED m_overRecvOverlapped;

			// Recv버퍼. 일반 링버퍼. 
			CRingBuff m_RecvQueue;

			// 마지막 패킷 저장소
			void* m_LastPacket = nullptr;

			// 헤더 코드
			BYTE m_bCode;

			// XOR1 코드
			BYTE m_bXORCode_1;

			// XOR2 코드
			BYTE m_bXORCode_2;

		public:
			// -----------------
			// 생성자와 소멸자
			// -----------------
			cSession();
			virtual ~cSession();

		protected:
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


		protected:
			// -----------------
			// 네트워크용 기능함수
			// -----------------

			// 유저를 끊을 때 호출하는 함수. 외부 에서 사용.
			// 라이브러리한테 끊어줘!라고 요청하는 것 뿐
			//
			// Parameter : 없음
			// return : 없음
			void Disconnect();

			// 외부에서, 유저에게 어떤 데이터를 보내고 싶을때 호출하는 함수.
			// SendPacket은 그냥 아무때나 하면 된다.
			// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
			//
			// Parameter : SendBuff, LastFlag(디폴트 FALSE)
			// return : 없음
			void SendPacket(CProtocolBuff_Net* payloadBuff, LONG LastFlag = FALSE);


			// -----------------
			// 일반 기능 함수
			// -----------------
			// 해당 유저의 모드를 GAME모드로 변경하는 함수
			// 내부에서는 m_lAuthToGameFlag 변수를 TRUE로 만든다.
			//
			// Parameter : 없음
			// return : 없음
			void SetMode_GAME();

		};

		// 헤더 구조체
		struct stProtocolHead;

	private:

		// Accept Socket Queue에서 관리할 일감 구조체
		struct stAuthWork
		{
			SOCKET m_clienet_Socket;
			TCHAR m_tcIP[30];
			USHORT m_usPort;
		};


	private:
		// ------------------
		// 멤버 변수
		// ------------------

		// Accept Socket Queue의 Pool
		CMemoryPoolTLS<stAuthWork>* m_pAcceptPool;

		// Accept Socket Queue
		CLF_Queue<stAuthWork*>* m_pASQ;


		// Net서버의 Config들.
		BYTE m_bCode;
		BYTE m_bXORCode_1;
		BYTE m_bXORCode_2;

		// 리슨소켓
		SOCKET m_soListen_sock;

		// 워커, 엑셉트, 샌드, 어스, 게임 스레드 핸들 배열
		HANDLE*	m_hAcceptHandle;
		HANDLE* m_hWorkerHandle;
		HANDLE* m_hSendHandle;
		HANDLE* m_hAuthHandle;
		HANDLE* m_hGameHandle;

		// 릴리즈 스레드
		HANDLE m_hReleaseHandle;

		// IOCP 핸들
		HANDLE m_hIOCPHandle;

		// 워커스레드 수, 엑셉트 스레드 수, 샌드 스레드 수, 어스 스레드 수, 게임 스레드 수
		int m_iA_ThreadCount;
		int m_iW_ThreadCount;
		int m_iS_ThreadCount;
		int m_iAuth_ThreadCount;
		int m_iGame_ThreadCount;


		// ----- 세션 관리용 -------
		// 세션 관리 배열
		cSession** m_stSessionArray;

		// 미사용 인덱스 관리 스택
		CLF_Stack<WORD>* m_stEmptyIndexStack;
		// --------------------------

		// 윈도우 에러 보관 변수, 내가 지정한 에러 보관 변수
		int m_iOSErrorCode;
		euError m_iMyErrorCode;

		// 최대 접속 가능 유저 수
		int m_iMaxJoinUser;

		// 현재 접속중인 유저 수
		ULONGLONG m_ullJoinUserCount;

		// 서버 가동 여부. true면 작동중 / false면 작동중 아님
		bool m_bServerLife;

		// Auth, Game, Send 스레드 종료 이벤트
		HANDLE m_hAuthExitEvent;
		HANDLE m_hGameExitEvent;
		HANDLE m_hSendExitEvent;
		HANDLE m_hReleaseExitEvent;


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CMMOServer();
		virtual ~CMMOServer();


	public:
		// -----------------------
		// 게터 함수
		// -----------------------

		// 서버 작동중인지 확인
		//
		// Parameter : 없음
		// return : 작동중일 시 true.
		bool GetServerState();

		// 접속 중인 세션 수 얻기
		//
		// Parameter : 없음
		// return : 접속자 수 (ULONGLONG)
		ULONGLONG GetClientCount();

		// Accept Socket Queue 안의 일감 수 얻기
		//
		// Parameter : 없음
		// return : 일감 수(LONG)
		LONG GetASQ_Count();

		// Accept Socket Queue Pool의 총 청크 수 얻기
		// 
		// Parameter : 없음
		// return : 총 청크 수 (LONG)
		LONG GetChunkCount();

		// Accept Socket Queue Pool의 밖에서 사용중인 청크 수 얻기
		// 
		// Parameter : 없음
		// return : 밖에서 사용중인 청크 수 (LONG)
		LONG GetOutChunkCount();


	private:
		// -----------------------
		// 내부에서만 사용하는 함수
		// -----------------------

		// Start에서 에러가 날 시 호출하는 함수.
		//
		// Parameter : 워커스레드의 수
		// return : 없음
		void ExitFunc(int w_ThreadCount);

		// 각종 변수들을 리셋시킨다.
		// Stop() 함수에서 사용.
		void Reset();

		// 내부에서 실제로 유저를 끊는 함수.
		//
		// Parameter : 끊을 세션 포인터
		// return : 없음
		void InDisconnect(cSession* DeleteSession);

		// RecvProc 함수.
		// 완성된 패킷을 인자로받은 Session의 CompletionRecvPacekt (Queue)에 넣는다.
		//
		// Parameter : 세션 포인터
		// return : 없음
		void RecvProc(cSession* NowSession);

		// RecvPost함수
		//
		// return 0 : 성공적으로 WSARecv() 완료
		// return 1 : RecvQ가 꽉찬 유저
		// return 2 : I/O 카운트가 0이되어 삭제된 유저
		int RecvPost(cSession* NowSession);	




	private:
		// -----------------------
		// 스레드들
		// -----------------------

		// 워커 스레드
		static UINT	WINAPI	WorkerThread(LPVOID lParam);

		// Accept 스레드
		static UINT	WINAPI	AcceptThread(LPVOID lParam);

		// Auth 스레드
		static UINT	WINAPI	AuthThread(LPVOID lParam);

		// Game 스레드
		static UINT	WINAPI	GameThread(LPVOID lParam);

		// Send 스레드
		static UINT WINAPI	SendThread(LPVOID lParam);		

		// Release 스레드
		static UINT WINAPI	ReleaseThread(LPVOID lParam);


	protected:
		// -----------------------
		// 상속 관계에서만 호출 가능한 함수
		// -----------------------

		// 서버 시작
		// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 활성화시킬 워커스레드 수, 엑셉트 스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수, 패킷 Code, XOR 1번코드, XOR 2번코드] 입력받음.
		//
		// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
		// return true : 성공
		bool Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int ActiveWThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect,
			BYTE Code, BYTE XORCode1, BYTE XORCode2);

		// 서버 스탑.
		void Stop();

		// 세션 셋팅
		//
		// Parameter : cSession* 포인터, Max 수(최대 접속 가능 유저 수), HeadCode, XORCode1, XORCode2
		// return : 없음
		void SetSession(cSession* pSession, int Max, BYTE HeadCode, BYTE XORCode1, BYTE XORCode2);


	private:
		// -----------------------
		// 순수 가상함수
		// -----------------------

		// AuthThread에서 1Loop마다 1회 호출.
		// 1루프마다 정기적으로 처리해야 하는 일을 한다.
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnAuth_Update() = 0;

		// GameThread에서 1Loop마다 1회 호출.
		// 1루프마다 정기적으로 처리해야 하는 일을 한다.
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnGame_Update() = 0;

		// 새로운 유저 접속 시, Auth에서 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port) = 0;

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin() = 0;

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd() = 0;

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr) = 0;

	



	};
}

#endif // !__NETWORK_LIB_MMOSERVER_H__
