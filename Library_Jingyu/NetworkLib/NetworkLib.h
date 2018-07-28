#ifndef __NETWORK_LIB_H__
#define __NETWORK_LIB_H__
#include <windows.h>
#include <map>
#include "ProtocolBuff\ProtocolBuff.h"

using namespace std;


namespace Library_Jingyu
{
	// --------------
	// CLanServer 클래스는, 내부 서버 간 통신에 사용된다.
	// 내부 서버간 통신은, 접속 받는쪽을 서버 / 접속하는 쪽을 클라로 나눠서 생각한다 (개념적으로)
	// 그 중 서버 부분이다.
	// --------------
	class CLanServer
	{
	private:
		// ----------------------
		// private 구조체 or enum 전방선언
		// ----------------------
		// 에러 enum값들
		enum class euError : int;

		// Session구조체 전방선언
		struct stSession;


		// ----------------------
		// private 변수들
		// ----------------------
		// 엑셉트 스레드 핸들
		HANDLE  m_hAcceptHandle;

		// 워커스레드 핸들 배열, 
		HANDLE* m_hWorkerHandle;

		// 워커스레드 수, 엑셉트 스레드 수, 라이프 체크 스레드 수
		int m_iW_ThreadCount;
		int m_iA_ThreadCount;

		// IOCP 핸들
		HANDLE m_hIOCPHandle;

		// 리슨소켓
		SOCKET m_soListen_sock;

		// 세션 관리하는 자료구조(map)
		// 키 : SessionID
		// 값 : stSession 구조체
		map<ULONGLONG, stSession*> map_Session;

		// map에 사용하는 SRWLock
		SRWLOCK m_srwSession_map_srwl;

		// 윈도우 에러 보관 변수, 내가 지정한 에러 보관 변수
		int m_iOSErrorCode;
		euError m_iMyErrorCode;

		// 유저가 접속할 때 마다 1씩 증가하는 고유한 키.
		ULONGLONG m_ullSessionID;

		// 현재 접속중인 유저 수
		ULONGLONG m_ullJoinUserCount;

		// 서버 가동 여부. true면 작동중 / false면 작동중 아님
		bool m_bServerLife;

#define	Lock_Map()	LockMap_Func()
#define Unlock_Map()	UnlockMap_Func()

	private:
		// ----------------------
		// private 함수들
		// ----------------------
		// 락 걸기, 락 풀기
		void LockMap_Func();
		void UnlockMap_Func();

		// ClinetID로 stSession포인터 알아오는 함수
		stSession* FineSessionPtr(ULONGLONG ClinetID);

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
		bool RecvProc(stSession* NowSession);

		// Accept용 RecvPost함수
		bool RecvPost_Accept(stSession* NowSession);

		// RecvPost함수
		bool RecvPost(stSession* NowSession);

		// SendPost함수
		bool SendPost(stSession* NowSession);

		// 내부에서 실제로 유저를 끊는 함수.
		void InDisconnect(stSession* NowSession);

		// 각종 변수들을 초기값으로 초기화
		void Init();


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CLanServer();
		~CLanServer();

	public:
		// -----------------------
		// 외부에서 호출 가능한 함수
		// -----------------------

		// ----------------------------- 기능 함수들 ---------------------------
		// 서버 시작
		// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용), 최대 접속자 수] 입력받음.
		// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
		// return true : 성공
		bool Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, bool Nodelay, int MaxConnect);

		// 서버 스탑.
		void Stop();

		// 지정한 유저를 끊을 때 호출하는 함수. 외부 에서 사용.
		// 라이브러리한테 끊어줘!라고 요청하는 것 뿐
		bool Disconnect(ULONGLONG ClinetID);

		// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
		// SendPacket은 그냥 아무때나 하면 된다.
		// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
		bool SendPacket(ULONGLONG ClinetID, CProtocolBuff* payloadBuff);


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

	public:
		// -----------------------
		// 순수 가상함수
		// -----------------------

		// Accept 직후, 호출된다.
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port) = 0;

		// Accept 후, 접속처리까지 다 완료된 후 호출된다
		virtual void OnClientJoin(ULONGLONG ClinetID) = 0;

		// InDisconnect 후 호출된다. (실제 내부에서 디스커넥트 후 호출)
		virtual void OnClientLeave(ULONGLONG ClinetID) = 0;

		// 패킷 수신 완료 후 호출되는 함수.
		// 완성된 패킷 1개가 생기면 호출됨. PacketProc으로 생각하면 된다.
		virtual void OnRecv(ULONGLONG ClinetID, CProtocolBuff* Payload) = 0;

		// 패킷 송신 완료 후 호출되는 함수
		// 샌드 완료통지를 받았을 때, 링버퍼 이동 등 다 하고 호출된다.
		virtual void OnSend(ULONGLONG ClinetID, DWORD SendSize) = 0;

		// 워커 스레드 GQCS 바로 하단에서 호출
		virtual void OnWorkerThreadBegin() = 0;

		// 워커스레드 1루프 종료 후 호출되는 함수.
		virtual void OnWorkerThreadEnd() = 0;

		// 에러 발생 시 호출되는 함수.
		virtual void OnError(int error, const TCHAR* errorStr) = 0;

	};

}






#endif // !__NETWORK_LIB_H__
