#ifndef __NETWORK_LIB_H__
#define __NETWORK_LIB_H__
#include <map>

using namespace std;


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

private:
	// ----------------------
	// private 변수들
	// ----------------------
	// 엑셉트 스레드 핸들
	HANDLE  m_hAcceptHandle;

	// 워커스레드 수, 엑셉트 스레드 수
	int m_iW_ThreadCount, m_iA_ThreadCount;

	// IOCP 핸들
	static HANDLE m_hIOCPHandle;

	// 워커스레드 핸들 배열, 
	static HANDLE* m_hWorkerHandle;

	// 리슨소켓
	static SOCKET m_soListen_sock;

	// 세션 관리하는 자료구조(map)
	// 키 : SessionID
	// 값 : stSession 구조체
	static map<ULONGLONG, stSession*> map_Session;

	// map에 사용하는 SRWLock
	static SRWLOCK m_srwSession_map_srwl;

#define	LockSession()	AcquireSRWLockExclusive(&m_srwSession_map_srwl)
#define UnlockSession()	ReleaseSRWLockExclusive(&m_srwSession_map_srwl)

private:
	// ----------------------
	// private 함수들
	// ----------------------
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

public:
	// 생성자와 소멸자
	CLanServer();
	~CLanServer();

	// [오픈 IP(바인딩 할 IP), 포트, 워커스레드 수, 네이글 옵션, 최대 접속자 수] 입력받음.
	// return false : 에러 발생 시. OnError()함수 호출 후 false 리턴
	// return true : 모두 성공
	bool Start(TCHAR* bindIP, USHORT port, int WorkerThreadCount, bool Nagle, int MaxConnect);

	// 서버 스탑.
	void Stop();

	// 외부에서, 지정한 유저를 끊을 때 호출하는 함수.
	bool Disconnect();

	// 외부에서, 어떤 데이터를 보내고 싶을때 호출하는 함수.
	// SendPacket은 그냥 아무때나 하면 된다.
	// 해당 유저의 SendQ에 넣어뒀다가 때가 되면 보낸다.
	bool SendPacket();

public:	

	// Accept 후 접속처리 완료 후 호출
	virtual void OnClientJoin() = 0;

	// Disconnect 후 호출된다.
	virtual void OnClientLeave() = 0;

	// Accept 직후, 호출된다.
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	virtual bool OnConnectioinRequest() = 0;

	// 패킷 수신 완료 후 호출되는 함수.
	virtual void OnRecv() = 0;

	// 패킷 송신 완료 후 호출되는 함수
	virtual void OnSend() = 0;

	// 워커 스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadBegin() = 0;

	// 워커스레드 1루푸 종료 후 호출되는 함수.
	virtual void OnWorkerThreadEnd() = 0;

	// 에러 발생 시 호출되는 함수.
	virtual void OnError(int error, const TCHAR* errorStr) = 0;

};
// static 변수들 선언
HANDLE CLanServer::m_hIOCPHandle;
HANDLE* CLanServer::m_hWorkerHandle;
SOCKET CLanServer::m_soListen_sock;
map<ULONGLONG, CLanServer::stSession*> CLanServer::map_Session;
SRWLOCK CLanServer::m_srwSession_map_srwl;







#endif // !__NETWORK_LIB_H__
