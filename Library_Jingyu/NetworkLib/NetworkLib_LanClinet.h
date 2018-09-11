/*
락프리 적용된 LanServer
*/


#ifndef __NETWORK_LIB_LANCLIENT_H__
#define __NETWORK_LIB_LANCLIENT_H__

#include <windows.h>
#include "ProtocolBuff\ProtocolBuff_ObjectPool.h"
#include "LockFree_Queue\LockFree_Queue.h"
#include "RingBuff\RingBuff.h"

using namespace std;

namespace Library_Jingyu
{
	// --------------
	// CLanClient 클래스는, 내부 서버 간 통신에 사용된다.
	// 내부 서버간 통신은, 접속 받는쪽을 서버 / 접속하는 쪽을 클라로 나눠서 생각한다 (개념적으로)
	// 그 중 서버 부분이다.
	// --------------
	class CLanClient
	{
	protected:
		// ----------------------
		// 에러 enum값들
		// ----------------------
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
			// ------------ 여기까지는 LanServer 자체적으로 남기는 에러. 밖에서 신경 쓸 필요 없음

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


	private:
		// 한 번에 샌드할 수 있는 WSABUF의 카운트
#define dfSENDPOST_MAX_WSABUF			300

		// ----------------------
		// private 구조체 
		// ----------------------	
		
		// 세션 구조체
		struct stSession
		{
			// 세션과 연결된 소켓
			SOCKET m_Client_sock;

			// 해당 세션이 들어있는 배열 인덱스
			WORD m_lIndex;

			// Send overlapped구조체
			OVERLAPPED m_overSendOverlapped;

			// 세션 ID. 컨텐츠와 통신 시 사용.
			ULONGLONG m_ullSessionID;

			// 해당 인덱스 배열이 릴리즈 되었는지 체크
			// TRUE이면 릴리즈 되었음.(사용중 아님), FALSE이면 릴리즈 안됐음.(사용중)
			LONG m_lReleaseFlag;

			// I/O 카운트. WSASend, WSARecv시 1씩 증가.
			// 0이되면 접속 종료된 유저로 판단.
			// 사유 : 연결된 유저는 WSARecv를 무조건 걸기 때문에 0이 될 일이 없다.
			LONG	m_lIOCount;

			// 현재, WSASend에 몇 개의 데이터를 샌드했는가. (바이트 아님! 카운트. 주의!)
			int m_iWSASendCount;

			// Send한 직렬화 버퍼들 저장할 포인터 변수
			CProtocolBuff_Lan* m_PacketArray[dfSENDPOST_MAX_WSABUF];

			// Send가능 상태인지 체크. 1이면 Send중, 0이면 Send중 아님
			LONG	m_lSendFlag;

			// Send버퍼. 락프리큐 구조. 패킷버퍼(직렬화 버퍼)의 포인터를 다룬다.
			CLF_Queue<CProtocolBuff_Lan*>* m_SendQueue;

			// Recv overlapped구조체
			OVERLAPPED m_overRecvOverlapped;

			// Recv버퍼. 일반 링버퍼. 
			CRingBuff m_RecvQueue;

			// 생성자 
			stSession()
			{
				m_Client_sock = INVALID_SOCKET;
				m_SendQueue = new CLF_Queue<CProtocolBuff_Lan*>(1024, false);
				m_lIOCount = 0;
				m_lReleaseFlag = TRUE;
				m_lSendFlag = FALSE;
				m_iWSASendCount = 0;
			}

			// 소멸자
			~stSession()
			{
				delete m_SendQueue;
			}

		};

		// ----------------------
		// private 변수들
		// ----------------------

		// 리슨소켓
		SOCKET m_soListen_sock;

		// 워커  스레드 핸들 배열
		HANDLE* m_hWorkerHandle;

		// IOCP 핸들
		HANDLE m_hIOCPHandle;

		// 워커스레드 수
		int m_iW_ThreadCount;

		// 접속하고자 하는 서버의 IP와 Port
		TCHAR	m_tcServerIP[20];
		WORD	m_wPort;

		// 세션 ID
		ULONGLONG m_ullUniqueSessionID;

		// 세션 구조체
		stSession m_stSession;

		// 노딜레이 옵션 여부
		bool m_bNodelay;

		// --------------------------

		// 윈도우 에러 보관 변수, 내가 지정한 에러 보관 변수
		int m_iOSErrorCode;
		euError m_iMyErrorCode;

		// 미사용 인덱스 관리 스택의 세션 수.
		int m_iMaxStackCount;

		// 현재 서버에 연결중인 클라
		// 무조건 1명이겠지만, stop()함수에서 모두 종료되면 워커스레드를 종료시키기 때문에, 체크용도로 보유.
		ULONGLONG m_ullJoinUserCount;

		// 클라이언트 접속 여부. true면 접속중, false면 접속중 아님
		bool m_bClienetConnect;


	private:
		// ----------------------
		// private 함수들
		// ----------------------	

		// 지정한 서버에 connect하는 함수
		bool ConnectFunc();

		// SendPacket, Disconnect 등 외부에서 호출하는 함수에서, 락거는 함수.
		// 실제 락은 아니지만 락처럼 사용.
		//
		// Parameter : SessionID
		// return : 성공적으로 세션 찾았을 시, 해당 세션 포인터
		//			실패 시 nullptr
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
		// return true : 성공적으로 WSARecv() 완료 or 어쨋든 종료된 유저는 아님
		// return false : I/O카운트가 0이되어서 종료된 유저임
		bool RecvPost(stSession* NowSession);

		// SendPost함수
		//
		// return true : 성공적으로 WSASend() 완료 or 어쨋든 종료된 유저는 아님
		// return false : I/O카운트가 0이되어서 종료된 유저임
		bool SendPost(stSession* NowSession);

		// 내부에서 실제로 유저를 끊는 함수.
		void InDisconnect(stSession* NowSession);

		// 각종 변수들을 리셋시킨다.
		void Reset();


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CLanClient();
		virtual ~CLanClient();

	public:
		// -----------------------
		// 외부에서 호출 가능한 함수
		// -----------------------

		// ----------------------------- 기능 함수들 ---------------------------
		
		// 서버 시작
		// [서버 IP(커넥트 할 IP), 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)] 입력받음.
		//
		// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
		// return true : 성공
		bool Start(const TCHAR* ConnectIP, USHORT port, int WorkerThreadCount, int ActiveWThreadCount, bool Nodelay);

		// 서버 스탑.
		void Stop();

		// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
		// SendPacket은 그냥 아무때나 하면 된다.
		// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
		//
		// return true : SendQ에 성공적으로 데이터 넣음.
		// return true : SendQ에 데이터 넣기 실패.
		void SendPacket(ULONGLONG SessionID, CProtocolBuff_Lan* payloadBuff);


		// ----------------------------- 게터 함수들 ---------------------------
		// 윈도우 에러 얻기
		int WinGetLastError();

		// 내 에러 얻기 
		int NetLibGetLastError();

		// 현재 접속자 수 얻기
		ULONGLONG GetClientCount();

		// 클라 접속중인지 확인
		// return true : 접속중
		// return false : 접속중 아님
		bool GetClinetState();

	public:
		// -----------------------
		// 순수 가상함수
		// -----------------------

		// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnConnect(ULONGLONG ClinetID) = 0;

		// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnDisconnect(ULONGLONG ClinetID) = 0;

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload) = 0;

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG ClinetID, DWORD SendSize) = 0;

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






#endif // !__NETWORK_LIB_LANCLIENT_H__