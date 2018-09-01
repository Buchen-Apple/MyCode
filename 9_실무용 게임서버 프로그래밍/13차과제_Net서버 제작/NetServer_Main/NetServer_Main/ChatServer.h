#ifndef __CHAT_SERVER_H__
#define __CHAT_SERVER_H__

#include "NetworkLib\NetworkLib_NetServer.h"
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"

#include "LockFree_Queue\LockFree_Queue.h"
#include "ProtocolStruct.h"

#include <map>
#include <list>


// 1개 맵의 섹터 수
#define SECTOR_X_COUNT	50
#define SECTOR_Y_COUNT	50

using namespace Library_Jingyu;

////////////////////////////////////////////
// !! 채팅서버 !!
////////////////////////////////////////////
class CChatServer :public CNetServer
{
	// -------------------------------------
	// inner 구조체
	// -------------------------------------

	// 섹터 정보를 담는 구조체
	struct stSectorCheck
	{
		// 몇 개의 Sector Index가 저장되어 있는지
		DWORD m_dwCount;

		// 섹터의 X,Y 인덱스 좌표 저장.
		POINT m_Sector[9];
	};

	// 플레이어 구조체
	struct stPlayer
	{
		// 유저 고유 값
		INT64 m_i64AccountNo;

		// 유저 ID (로그인 시 사용하는 ID)
		TCHAR m_tLoginID[20];

		// 닉네임
		TCHAR m_tNickName[20];

		// 섹터 좌표 X,Y
		WORD m_wSectorX;
		WORD m_wSectorY;

		// 토큰 (세션 키)
		char m_cToken[64];

		// 세션 ID (네트워크와 통신 시 사용하는 키값)
		ULONGLONG m_ullSessionID;
	};

private:
	// -------------------------------------
	// 멤버 변수
	// -------------------------------------
	// !! 캐시 히트를 고려해 자주 사용되는 것들끼리 근접시켜서 묶음
	// !! 대부분, 접근 후 수정하는(Write)하는 변수들이기 때문에 읽기전용, 쓰기전용 기준으로 나누지 않음.

	// 덤프
	CCrashDump* m_ChatDump = CCrashDump::GetInstance();

	// 메시지 구조체를 다룰 TLS메모리풀
	// 가장 큰 메시지 구조체를 다룬다.
	CMemoryPoolTLS<st_Protocol_CS_CHAT_RES_MESSAGE>* m_MessagePool;	

	// 메시지를 받을 락프리 큐
	// 주소(8바이트)를 다룬다.
	CLF_Queue<st_Protocol_CS_CHAT_RES_MESSAGE*>* m_LFQueue;

	// 업데이트 스레드 깨우기 용도 이벤트
	// !!!! 외부에서 전달받는다 (생성자에서) !!!!
	HANDLE* m_UpdateThreadEvent;

	// 플레이어 구조체를 다룰 TLS메모리풀
	CMemoryPoolTLS<stPlayer>* m_PlayerPool;

	// 플레이어 구조체를 다루는 map
	// Key : SessionID
	// Value : stPlayer*
	map<ULONGLONG, stPlayer*> m_mapPlayer;

	// 섹터 리스트
	list< stPlayer*> m_listSecotr[SECTOR_Y_COUNT][SECTOR_X_COUNT];	
	

private:
	// -------------------------------------
	// 클래스 내부에서만 사용하는 기능 함수
	// -------------------------------------
	
	// Player 관리 자료구조에, 유저 추가
	// 현재 map으로 관리중
	// 
	// Parameter : SessionID, stPlayer*
	// return : 추가 성공 시, true
	//		  : SessionID가 중복될 시(이미 접속중인 유저) false
	bool InsertPlayerFunc(ULONGLONG SessionID, stPlayer* insertPlayer);

	// Player 관리 자료구조에서, 유저 검색
	// 현재 map으로 관리중
	// 
	// Parameter : SessionID
	// return : 검색 성공 시, stPalyer*
	//		  : 검색 실패 시 nullptr
	stPlayer* FindPlayerFunc(ULONGLONG SessionID);

	// Player 관리 자료구조에서, 유저 제거 (검색 후 제거)
	// 현재 map으로 관리중
	// 
	// Parameter : SessionID
	// return : 성공 시, 제거된 유저 stPalyer*
	//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
	stPlayer* ErasePlayerFunc(ULONGLONG SessionID);

	// 섹터 기준 9방향 구하기
	// 인자로 받은 X,Y를 기준으로, 인자로 받은 구조체에 9방향을 넣어준다.
	//
	// Parameter : 기준 SectorX, 기준 SectorY, (out)&stSectorCheck
	// return : 없음
	void GetSector(int SectorX, int SectorY, stSectorCheck* Sector);

	// 인자로 받은 9개 섹터의 모든 유저(서버에 패킷을 보낸 클라 포함)에게 SendPacket 호출
	//
	// parameter : SessionID, 보낼 버퍼, 섹터 9개
	// return : 없음
	void SendPacket_Sector(ULONGLONG SessionID, CProtocolBuff_Net* SendBuff, stSectorCheck* Sector);


	// -------------------------------------
	// 클래스 내부에서만 사용하는 패킷 처리 함수
	// -------------------------------------

	// 섹터 이동요청 패킷 처리
	//
	// Parameter : SessionID, Packet
	// return : 성공 시 true
	//		  : 접속중이지 않은 유저알 시 false
	bool Packet_Sector_Move(ULONGLONG SessionID, st_Protocol_CS_CHAT_RES_MESSAGE* Packet);

	// 채팅 보내기 요청
	//
	// Parameter : SessionID, Packet
	// return : 성공 시 true
	//		  : 접속중이지 않은 유저일 시 false
	bool Packet_Chat_Message(ULONGLONG SessionID, st_Protocol_CS_CHAT_RES_MESSAGE* Packet);
	
	// Accept 성공 (채팅 서버에 입장)
	// 네트워크 통신은 아니며, Net서버에서 Chat 서버로 알려준다.
	// 
	// Parameter : SessionID, Packet
	// return : 성공 시 true
	//		  : 이미 접속중인 유저라면 false
	bool Packet_Chat_Join(ULONGLONG SessionID, st_Protocol_CS_CHAT_RES_MESSAGE* Packet);

	// 유저 종료 (채팅 서버에서 나감)
	// 네트워크 통신은 아니며, Net서버에서 Chat 서버로 알려준다.
	// 
	// Parameter : SessionID, Packet
	// return : 이미 나간유저일 시 false
	bool Packet_Chat_Leave(ULONGLONG SessionID, st_Protocol_CS_CHAT_RES_MESSAGE* Packet);

private:
	// -------------------------------------
	// NetServer의 가상함수
	// -------------------------------------

	virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

	virtual void OnClientJoin(ULONGLONG SessionID);

	virtual void OnClientLeave(ULONGLONG SessionID);

	virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

	virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

	virtual void OnWorkerThreadBegin();

	virtual void OnWorkerThreadEnd();

	virtual void OnError(int error, const TCHAR* errorStr);

public:
	// -------------------------------------
	// 클래스 외부에서 사용 가능한 함수
	// -------------------------------------

	// 생성자
	//
	// Parameter : 업데이트 스레드를 깨울 때 사용할 이벤트
	CChatServer(HANDLE* UpdateThreadEvent);

	//소멸자
	~CChatServer();

	// 패킷 처리 함수
	void PacketHandling();

	// 처리할 일감이 있는지 체크하는 함수
	// 내부적으로는 QueueSize를 체크하는 것.
	LONG WorkCheck();

};



#endif // !__CHAT_SERVER_H__
