#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#include "ObjectPool/Object_Pool_LockFreeVersion.h"
#include "CrashDump/CrashDump.h"
#include "Log/Log.h"
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

		// 로그인 패킷 보냄 여부
		// false면 안보냄
		bool m_bLoginPacketSend;

		// 리시브 버퍼
		CRingBuff m_RecvBuff;
		
		// 샌드 버퍼
		CRingBuff m_SendBuff;

		stDummy()
		{
			m_bMatchConnect = false;
			m_bMatchLogin = false;
			m_bLoginPacketSend = false;
			m_sock = INVALID_SOCKET;
		}

		// Disconenct 시 초기화하는 함수
		void Reset()
		{
			m_sock = INVALID_SOCKET;
			m_bMatchLogin = false;
			m_bMatchConnect = false;
			m_bLoginPacketSend = false;
			m_RecvBuff.ClearBuffer();
			m_SendBuff.ClearBuffer();
		}
	};
	
	// 헤더 구조체
#pragma pack(push, 1)
	struct stProtocolHead
	{
		BYTE	m_Code;
		WORD	m_Len;
		BYTE	m_RandXORCode;
		BYTE	m_Checksum;
	};
#pragma pack(pop)

	// ----------------
	// 멤버 변수
	// ----------------

	// 덤프
	// 생성자에서 얻어온다.
	CCrashDump* m_Dump;

	// 로그
	// 생성자에서 얻어온다.
	CSystemLog* m_Log;

	// HTTP 통신용
	HTTP_Exchange* HTTP_Post;

	// 더미 수
	int m_iDummyCount;

	// 시작 어카운트 넘버
	UINT64 m_ui64StartAccountNo;

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

	// 더미의 Socket 기준, 더미를 검색한다.
	//
	// Key : socket, Value : stDummy*
	unordered_map<SOCKET, stDummy*> m_uamp_Socket_and_AccountNo;

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
	// SOCKET 기준, 더미 자료구조 관리용 함수
	// ------------------------------------

	// 더미 관리 자료구조에 더미 추가
	// 현재 umap으로 관리중
	// 
	// Parameter : SOCKET, stDummy*
	// return : 추가 성공 시, true
	//		  : SOCKET이 중복될 시(이미 접속중인 유저) false
	bool InsertDummyFunc_SOCKET(SOCKET sock, stDummy* insertDummy);

	// 더미 관리 자료구조에서, 유저 검색
	// 현재 map으로 관리중
	// 
	// Parameter : SOCKET
	// return : 검색 성공 시, stDummy*
	//		  : 검색 실패 시 nullptr
	stDummy* FindDummyFunc_SOCKET(SOCKET sock);

	//더미 관리 자료구조에서, 더미 제거 (검색 후 제거)
	// 현재 umap으로 관리중
	// 
	// Parameter : SOCKET
	// return : 성공 시, 제거된 더미의 stDummy*
	//		  : 검색 실패 시(접속중이지 않은 더미) nullptr
	stDummy* EraseDummyFunc_SOCKET(SOCKET socket);






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

	// Select 처리
	//
	// Parameter : 없음
	// return : 없음
	void SelectFunc();

	// Match 서버에 Disconnect. (확률에 따라)
	// 
	// Parameter : 없음
	// return : 없음
	void MatchDisconenct();

	// 해당 유저의 SendBuff에 로그인 패킷 넣기
	//
	// Parameter : stDummy*, CProtocolBuff_Net* (헤더 완성되지 않음)
	// return : 성공 시 true
	//		  : 실패 시 false
	bool SendPacket(stDummy* NowDummy, CProtocolBuff_Net* payload);
	
	// 서버와 인자로 받은 더미 1개체와 연결을 끊는 함수.
	//
	// Parameter : stDummy*
	// returb : 없음
	void Disconnect(stDummy* NowDummy);


private:
	// ------------------------------------
	// 네트워크 처리 함수
	// ------------------------------------

	// Recv() 처리
	//
	// Parameter : stDummy*
	// return : 접속이 끊겨야 하는 유저는, false 리턴
	bool RecvProc(stDummy* NowDummy);

	// SendBuff의 데이터를 Send하기
	//
	// Parameter : stDummy*
	// return : 접속이 끊겨야 하는 유저는, false 리턴
	bool SendProc(stDummy* NowDummy);

	// Recv 한 데이터 처리 부분
	//
	// Parameter : stDummy*, 페이로드(CProtocolBuff_Net*)
	// return : 없음
	void Network_Packet(stDummy* NowDummy, CProtocolBuff_Net* payload);


private:
	// ------------------------------------
	// 패킷 처리 함수
	// ------------------------------------

	// Match서버로 보낸 로그인 패킷의 응답 처리
	//
	// Parameter : stDummy*, 페이로드(CProtocolBuff_Net*)
	// return : 없음
	void PACKET_Login_Match(stDummy* NowDummy, CProtocolBuff_Net* payload);



public:
	// ------------------------------------
	// 외부에서 사용 가능 함수
	// ------------------------------------

	// 매치메이킹 서버의 IP와 Port 알아오는 함수
	// 각 더미가, 자신의 멤버로 Server의 IP와 Port를 들고있음.
	//
	// Parameter : 없음
	// return : 없음
	void MatchInfoGet();

	// 더미 생성 함수
	//
	// Parameter : 생성할 더미 수, 시작 AccountNo
	// return : 없음
	void CreateDummpy(int Count, int StartAccountNo);

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
