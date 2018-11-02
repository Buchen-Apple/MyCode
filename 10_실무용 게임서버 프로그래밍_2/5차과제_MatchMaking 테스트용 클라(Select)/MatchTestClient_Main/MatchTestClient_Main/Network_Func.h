#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#include "ObjectPool/Object_Pool_LockFreeVersion.h"
#include "CrashDump/CrashDump.h"
#include "Http_Exchange/HTTP_Exchange.h"
#include "RingBuff/RingBuff.h"
#include "ProtocolBuff/ProtocolBuff(Net)_ObjectPool.h"
#include "LockFree_Queue/LockFree_Queue.h"

#include <unordered_map>
#include <unordered_set>

using namespace std;
using namespace Library_Jingyu;


class cMatchTestDummy
{
	// 더미 구조체
	struct stDummy
	{
		UINT64 m_ui64AccountNo;
		SOCKET m_sock;
		TCHAR m_tcToken[64];
		char m_cToekn[64];

		TCHAR m_tcServerIP[20];
		int m_iPort;

		// 매치메이킹 서버에 Connect했는지 체크. false면 Connect 안함.
		bool m_bMatchConnect;

		// 매치메이킹 서버에 Login 되었는지 체크 (로그인 패킷 보냈고 응답 받았는지)
		// false면 아직 로그인 안됨
		bool m_bMatchLogin;

		// 리시브 버퍼
		CRingBuff m_RecvBuff;

		// Send버퍼. 락프리큐 구조. 패킷버퍼(직렬화 버퍼)의 포인터를 다룬다.
		CLF_Queue<CProtocolBuff_Net*>* m_SendQueue;

		stDummy()
		{
			m_bMatchConnect = false;
			m_bMatchLogin = false;
			m_sock = INVALID_SOCKET;
		}
	};

	// ----------------
	// 멤버 변수
	// ----------------

	// 덤프
	// 생성자에서 얻어온다.
	CCrashDump* m_Dump;

	// HTTP 통신용
	HTTP_Exchange* HTTP_Post;

	// 더미 수
	int m_iDummyCount;

	// Config들.
	BYTE m_bCode;
	BYTE m_bXORCode_1;
	BYTE m_bXORCode_2;

	// ------------------------------

	// 더미 관리 umap
	// 
	// Key : AccountNo, Value : stDummy*
	unordered_map<UINT64, stDummy*> m_uamp_Dummy;

	// 더미 구조체 pool (TLS)
	CMemoryPoolTLS<stDummy> *m_DummyStructPool;

	// ------------------------------
	

private:
	// ------------------------------------
	// 더미 자료구조 관리용 함수
	// ------------------------------------

	// 더미 관리 자료구조에 더미 추가
	// 현재 umap으로 관리중
	// 
	// Parameter : AccountNo, stDummy*
	// return : 추가 성공 시, true
	//		  : AccountNo가 중복될 시(이미 접속중인 유저) false
	bool InsertDummyFunc(UINT64 AccountNo, stDummy* insertDummy);

	// 더미 관리 자료구조에서, 유저 검색
	// 현재 map으로 관리중
	// 
	// Parameter : AccountNo
	// return : 검색 성공 시, stDummy*
	//		  : 검색 실패 시 nullptr
	stDummy* FindDummyFunc(UINT64 AccountNo);


private:
	// ------------------------------------
	// 내부에서만 사용하는 함수
	// ------------------------------------

	// 매치메이킹 서버에 Connect
	//
	// Parameter : 없음
	// return : 없음
	void MatchConnect();

	// 모든 더미의 SendQ에 로그인 패킷 Enqueue
	//
	// Parameter : 없음
	// return : 없음
	void MatchLogin();

	// 해당 유저의 SendBuff에 로그인 패킷 넣기
	//
	// Parameter : stDummy*, CProtocolBuff_Net* (헤더 완성되지 않음)
	// return : 성공 시 true
	//		  : 실패 시 false
	bool SendPacket(stDummy* NowDummy, CProtocolBuff_Net* payload);


public:
	// ------------------------------------
	// 외부에서 사용 가능 함수
	// ------------------------------------

	// 더미 생성 함수
	//
	// Parameter : 생성할 더미 수
	// return : 없음
	void CreateDummpy(int Count);

	// 매치메이킹 서버의 IP와 Port 알아오는 함수
	// 각 더미가, 자신의 멤버로 Server의 IP와 Port를 들고있음.
	//
	// Parameter : 없음
	// return : 없음
	void MatchInfoGet();

	// 더미 Run 함수
	// 1. 매치메이킹 서버의 IP와 Port 알아오기 (Connect 안된 유저 대상)
	// 2. 매치메이킹 Connect (Connect 안된 유저 대상)
	// 3. 로그인 패킷 Enqueue (Connect 되었으며, Login 안된 유저 대상)
	// 4. Select 처리 (Connect 된 유저 대상)
	// 5. Disconnect. (Connect 후, Login 된 유저 대상. 확률로 Disconnect)
	//
	// Parameter : 없음
	// return : 없음
	void DummyRun();


public:
	// ------------------------------------
	// 생성자, 소멸자
	// ------------------------------------

	// 생성자
	cMatchTestDummy();

	// 소멸자
	~cMatchTestDummy();
};



#endif // !__NETWORK_FUNC_H__
