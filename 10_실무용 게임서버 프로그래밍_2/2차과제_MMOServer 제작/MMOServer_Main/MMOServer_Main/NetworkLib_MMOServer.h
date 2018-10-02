#ifndef __NETWORK_LIB_MMOSERVER_H__
#define __NETWORK_LIB_MMOSERVER_H__

#include <windows.h>
#include "ProtocolBuff\ProtocolBuff(Net)_ObjectPool.h"
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "LockFree_Stack\LockFree_Stack.h"
#include "LockFree_Queue\LockFree_Queue.h"

// ------------------------
// MMOServer
// ------------------------
namespace Library_Jingyu
{
	class CMMOServer
	{
	protected:
		// enum 전방선언
		enum class euError : int
		{
			NETWORK_LIB_ERROR__NORMAL = 0,					// 에러 없는 기본 상태
			NETWORK_LIB_ERROR__WINSTARTUP_FAIL,				// 윈속 초기화 하다가 에러남
			NETWORK_LIB_ERROR__CREATE_IOCP_PORT,			// IPCP 만들다가 에러남
			NETWORK_LIB_ERROR__W_THREAD_CREATE_FAIL,		// 워커스레드 만들다가 실패 
			NETWORK_LIB_ERROR__A_THREAD_CREATE_FAIL,		// 엑셉트 스레드 만들다가 실패 
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
			MODE_LOGOUT_IN_AUTH,	// 인증모드에서 종료 준비
			MODE_LOGOUT_IN_GAME,	// 게임모드에서 종료 준비
			MODE_WAIT_LOGOUT		//최종 종료
		};



		// ------------------
		// 이너 클래스 전방선언
		// ------------------
		struct stSession;

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

		// 워커, 엑셉트 스레드 핸들 배열
		HANDLE*	m_hAcceptHandle;
		HANDLE* m_hWorkerHandle;

		// IOCP 핸들
		HANDLE m_hIOCPHandle;

		// 워커스레드 수, 엑셉트 스레드 수
		int m_iA_ThreadCount;
		int m_iW_ThreadCount;


		// ----- 세션 관리용 -------
		// 세션 관리 배열
		stSession* m_stSessionArray;

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


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CMMOServer();
		virtual ~CMMOServer();


	private:
		// -----------------------
		// 내부에서만 사용하는 함수
		// -----------------------

		// Start에서 에러가 날 시 호출하는 함수.
		// 1. 입출력 완료포트 핸들 반환
		// 2. 워커스레드 종료, 핸들반환, 핸들 배열 동적해제
		// 3. 엑셉트 스레드 종료, 핸들 반환
		// 4. 리슨소켓 닫기
		// 5. 윈속 해제
		void ExitFunc(int w_ThreadCount);

		// 각종 변수들을 리셋시킨다.
		// Stop() 함수에서 사용.
		void Reset();

		// RecvPost함수
		//
		// return 0 : 성공적으로 WSARecv() 완료
		// return 1 : RecvQ가 꽉찬 유저
		// return 2 : I/O 카운트가 0이되어 삭제된 유저
		int RecvPost(stSession* NowSession);


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

	public:
		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 세션 셋팅
		//
		// Parameter : stSession* 포인터, Max 수(최대 접속 가능 유저 수)
		// return : 없음
		void SetSession(stSession* pSession, int Max);



	protected:
		// -----------------------
		// 상속에서만 호출 가능한 함수
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

		public:
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
