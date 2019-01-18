/*
락프리 적용된 NetServer
*/

#ifndef __NETWORK_LIB_NETSERVER_H__
#define __NETWORK_LIB_NETSERVER_H__

#include <windows.h>
#include "ProtocolBuff\ProtocolBuff(Net)_ObjectPool.h"
#include "LockFree_Stack\LockFree_Stack.h"



namespace Library_Jingyu
{
	// --------------
	// CNetServer 클래스는, 내부 서버 간 통신에 사용된다.
	// 내부 서버간 통신은, 접속 받는쪽을 서버 / 접속하는 쪽을 클라로 나눠서 생각한다 (개념적으로)
	// 그 중 서버 부분이다.
	// --------------
	class CNetServer
	{	
	protected:
		// Error enum 전방선언
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
			NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL,		// SendQ 사이즈가 꽉찬 유저
			NETWORK_LIB_ERROR__RECV_QUEUE_SIZE_FULL,		// RecvQ 사이즈가 꽉찬 유저
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

			
	private:
		// ----------------------
		// private 구조체
		// ----------------------
		// Session구조체 전방선언
		struct stSession;

		// 헤더 구조체
		struct stProtocolHead;




		// ----------------------
		// private 변수들
		// ----------------------

		// Net서버의 Config들.
		BYTE m_bCode;
		BYTE m_bXORCode;

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

		// PQCS overlapped 구조체
		OVERLAPPED m_overPQCSOverlapped;


		// ------ 카운트용 ------

		ULONGLONG m_ullAcceptTotal;
		LONG	  m_lAcceptTPS;
		LONG	m_lSendPostTPS;
		LONG	m_lRecvTPS;
		LONG	m_lSemCount;



	private:
		// ----------------------
		// private 함수들
		// ----------------------	
		// SendPacket, Disconnect 등 외부에서 호출하는 함수에서, 락거는 함수.
		// 실제 락은 아니지만 락처럼 사용.
		//
		// Parameter : SessionID
		// return : 성공적으로 세션 찾았을 시, 해당 세션 포인터
		//		  : I/O카운트가 0이되어 삭제된 유저는, nullptr
		stSession* GetSessionLOCK(ULONGLONG SessionID);

		// SendPacket, Disconnect 등 외부에서 호출하는 함수에서, 락 해제하는 함수
		// 실제 락은 아니지만 락처럼 사용.
		//
		// parameter : 세션 포인터
		// return : 성공적으로 해제 시, true
		//		  : I/O카운트가 0이되어 삭제된 유저는, false
		bool GetSessionUnLOCK(stSession* NowSession);

		// 워커 스레드
		static UINT	WINAPI	WorkerThread(LPVOID lParam);

		// Accept 스레드
		static UINT	WINAPI	AcceptThread(LPVOID lParam);

		// 중간에 무언가 에러났을때 호출하는 함수
		// 1. 윈속 해제
		// 2. 입출력 완료포트 핸들 반환
		// 3. 워커스레드 종료, 핸들반환, 핸들 배열 동적해제
		// 4. 리슨소켓 닫기
		void ExitFunc(int w_ThreadCount);

		// RecvProc 함수. 큐의 내용 체크 후 PacketProc으로 넘긴다.
		void RecvProc(stSession* NowSession);

		// RecvPost함수
		//
		// return 0 : 성공적으로 WSARecv() 완료
		// return 1 : RecvQ가 꽉찬 유저
		// return 2 : I/O 카운트가 0이되어 삭제된 유저
		// return 3 : 삭제는 안됐는데 WSARecv 실패
		int RecvPost(stSession* NowSession);

		// SendPost함수
		//
		// return 0 : 성공적으로 WSASend() 완료 or WSASend가 실패했지만 종료된 유저는 아님.
		// return 1 : SendFlag가 TRUE(누가 이미 샌드중)임.
		// return 2 : I/O카운트가 0이되어서 종료된 유저
		int SendPost(stSession* NowSession);

		// 내부에서 실제로 유저를 끊는 함수.
		void InDisconnect(stSession* NowSession);

		// 각종 변수들을 리셋시킨다.
		void Reset();


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CNetServer();
		virtual ~CNetServer();


	protected:
		// -----------------------
		// 상속에서만 호출 가능한 함수
		// -----------------------

		// 서버 시작
		// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 활성화시킬 워커스레드 수, 엑셉트 스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수, 패킷 Code, XOR 코드] 입력받음.
		//
		// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
		// return true : 성공
		bool Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int ActiveWThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect, BYTE Code, BYTE XORCode);

		// 서버 스탑.
		void Stop();

	public:
		// -----------------------
		// 외부에서 호출 가능한 함수
		// -----------------------

		// ----------------------------- 기능 함수들 ---------------------------		

		// 지정한 유저를 끊을 때 호출하는 함수. 외부 에서 사용.
		// 라이브러리한테 끊어줘!라고 요청하는 것 뿐
		//
		// Parameter : SessionID
		// return : 없음
		void Disconnect(ULONGLONG SessionID);

		// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
		// SendPacket은 그냥 아무때나 하면 된다.
		// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
		//
		// Parameter : SessionID, SendBuff, LastFlag(디폴트 FALSE)
		// return : 없음
		void SendPacket(ULONGLONG SessionID, CProtocolBuff_Net* payloadBuff, LONG LastFlag = FALSE);


		// ----------------------------- 게터 함수들 ---------------------------
		// 윈도우 에러 얻기
		int WinGetLastError();

		// 내 에러 얻기 
		int NetLibGetLastError();

		// 현재 접속자 수 얻기
		ULONGLONG GetClientCount();

		// 서버 가동상태 얻기
		// return true : 가동중
		// return false : 가동중 아님
		bool GetServerState();

		// 미사용 세션 관리 스택의 노드 얻기
		LONG GetStackNodeCount();

		// Accept Total 얻기.
		ULONGLONG GetAcceptTotal();

		// AcceptTPS 얻기.
		// 반환과 동시에 기존 값은 0으로 초기화
		LONG GetAccpetTPS();

		// SendTPS 얻기
		// 반환과 동시에 기존 값은 0으로 초기화
		LONG GetSendTPS();

		// RecvTPS 얻기
		// 반환과 동시에 기존 값은 0으로 초기화
		LONG GetRecvTPS();

		// 세마포어 카운트 얻기
		LONG GetSemCount();



	protected:
		// -----------------------
		// 순수 가상함수
		// -----------------------

		// Accept 직후, 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port) = 0;

		// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
		//
		// parameter : 접속한 유저에게 할당된 세션키
		// return : 없음
		virtual void OnClientJoin(ULONGLONG SessionID) = 0;

		// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 유저 세션키
		// return : 없음
		virtual void OnClientLeave(ULONGLONG SessionID) = 0;

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload) = 0;

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize) = 0;

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

		// 세마포어 발생 시 호출되는 함수
		//
		// parameter : SessionID
		// return : 없음
		virtual void OnSemaphore(ULONGLONG SessionID) = 0;

	};

}






#endif // !__NETWORK_LIB_NETSERVER_H__