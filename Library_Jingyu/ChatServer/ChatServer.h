#ifndef __CHAT_SERVER_H__
#define __CHAT_SERVER_H__

#include "NetworkLib\NetworkLib_NetServer.h"
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"

#include "LockFree_Queue\LockFree_Queue.h"

#include <vector>
#include <unordered_map>

using namespace std;


namespace Library_Jingyu
{

	// 1개 맵의 섹터 수
#define SECTOR_X_COUNT	50
#define SECTOR_Y_COUNT	50


	////////////////////////////////////////////
	// !! 채팅서버 !!
	////////////////////////////////////////////
	class CChatServer :public CNetServer
	{

		// -------------------------------------
		// inner 구조체
		// -------------------------------------
		// 미리 섹터 9개를 구해두는 구조체
		struct st_SecotrSaver
		{
			POINT m_Sector[9];
			LONG m_dwCount;
		};

		// 일감 구조체
		struct st_WorkNode
		{
			WORD m_wType;
			ULONGLONG m_ullSessionID;
			CProtocolBuff_Net* m_pPacket;
		};

		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
			TCHAR BindIP[20];
			int Port;
			int CreateWorker;
			int ActiveWorker;
			int CreateAccept;
			int HeadCode;
			int XORCode1;
			int XORCode2;
			int Nodelay;
			int MaxJoinUser;
			int LogLevel;
		};

		// 플레이어 구조체
		struct stPlayer
		{
			// 세션 ID (네트워크와 통신 시 사용하는 키값)
			ULONGLONG m_ullSessionID;

			// 섹터 좌표 X,Y
			WORD m_wSectorX;
			WORD m_wSectorY;

			// 유저 고유 값
			INT64 m_i64AccountNo;

			// 유저 ID (로그인 시 사용하는 ID)
			WCHAR m_tLoginID[20];

			// 닉네임
			WCHAR m_tNickName[20];

			// 토큰 (세션 키)
			char m_cToken[64];
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
		// 일감 구조체를 다룬다.
		CMemoryPoolTLS<st_WorkNode> *m_MessagePool;

		// 메시지를 받을 락프리 큐
		// 주소(8바이트)를 다룬다.
		CLF_Queue<st_WorkNode*> *m_LFQueue;

		// 플레이어 구조체를 다룰 TLS메모리풀
		CMemoryPoolTLS<stPlayer> *m_PlayerPool;

		// 플레이어 구조체를 다루는 map
		// Key : SessionID
		// Value : stPlayer*
		// 
		// 선택 이유 ---
		// 해당 자료구조를 이용해 [삽입, 삭제, 검색]을 주로 사용.
		// 이 중, 플레이어 접속, 접속해제(삽입, 삭제) 시에는 조금 느려도 괜찮다고 판단.
		// 핵심은 플레이 중인 유저의 정보를 얼마나 빠르게 처리하는가가 핵심. 
		// 추가로 딱히 데이터가 정렬되어 있을 필요도 없음
		// 때문에 umap을 선택
		unordered_map<ULONGLONG, stPlayer*> m_mapPlayer;

		// 섹터 vector
		// 요소 : SessionID
		//
		// 선택 이유 ---
		// 해당 자료구조를 이용해 [삽입, 삭제, 순회]를 주로 사용
		// 이 중, 주변 섹터에 채팅 메시지를 발송하는 "순회"를 핵심으로 판단.
		// 삽입, 삭제의 경우, push_pack, pop_pack 으로 하면 O(1)의 속도
		// 딱히 정렬될 필요가 없기 때문에, 삽입 시에는 그냥 push_back으로 넣으면 된다.
		// 문제는 pop_back인데, 이 경우, 빠른 순회를 통해, (배열의 경우, 처음 시작 위치에서 +하면서 위치를 이동하기때문에 순회가 굉장히 빠르다.)
		// 삭제하고자 하는 요소와 마지막 요소를 swap 후, 마지막 요소를 pop_back하는 형태로 해결.
		vector<ULONGLONG> m_vectorSecotr[SECTOR_Y_COUNT][SECTOR_X_COUNT];

		//  X,Y 기준, 9개의 섹터를 미리 구해서 저장해두는 배열
		st_SecotrSaver* m_stSectorSaver[SECTOR_Y_COUNT][SECTOR_X_COUNT];

		// 업데이트 스레드 핸들
		HANDLE hUpdateThraed;

		// 업데이트 스레드 깨우기 용도 Event
		HANDLE UpdateThreadEvent;

		// 업데이트 스레드 종료 용도 Event
		HANDLE UpdateThreadEXITEvent;

	private:
		// -------------------------------------
		// 클래스 내부에서만 사용하는 기능 함수
		// -------------------------------------	

		void SecotrSave(int SectorX, int SectorY, st_SecotrSaver* Sector);

		// 파일에서 Config 정보 읽어오기
		// 
		// 
		// Parameter : config 구조체
		// return : 정상적으로 셋팅 시 true
		//		  : 그 외에는 false
		bool SetFile(stConfigFile* pConfig);

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

		// 인자로 받은 섹터 X,Y 주변 9개 섹터의 유저들(서버에 패킷을 보낸 클라 포함)에게 SendPacket 호출
		//
		// parameter : 섹터 x,y, 보낼 버퍼
		// return : 없음
		void SendPacket_Sector(int SectorX, int SectorY, CProtocolBuff_Net* SendBuff);

		// 업데이트 스레드
		static UINT	WINAPI	UpdateThread(LPVOID lParam);



		// -------------------------------------
		// 클래스 내부에서만 사용하는 패킷 처리 함수
		// -------------------------------------

		// 접속 패킷처리 함수
		// OnClientJoin에서 호출
		// 
		// Parameter : SessionID
		// return : 없음
		void Packet_Join(ULONGLONG SessionID);

		// 종료 패킷처리 함수
		// OnClientLeave에서 호출
		// 
		// Parameter : SessionID
		// return : 없음
		void Packet_Leave(ULONGLONG SessionID);

		// 일반 패킷처리 함수
		// 
		// Parameter : SessionID, CProtocolBuff_Net*
		// return : 없음
		void Packet_Normal(ULONGLONG SessionID, CProtocolBuff_Net* Packet);



		// -------------------------------------
		// '일반 패킷 처리 함수'에서 처리되는 일반 패킷들
		// -------------------------------------

		// 섹터 이동요청 패킷 처리
		//
		// Parameter : SessionID, CProtocolBuff_Net*
		// return : 없음
		void Packet_Sector_Move(ULONGLONG SessionID, CProtocolBuff_Net* Packet);

		// 채팅 보내기 요청
		//
		// Parameter : SessionID, CProtocolBuff_Net*
		// return : 없음
		void Packet_Chat_Message(ULONGLONG SessionID, CProtocolBuff_Net* Packet);

		// 로그인 요청
		//
		// Parameter : SessionID, CProtocolBuff_Net*
		// return : 없음
		void Packet_Chat_Login(ULONGLONG SessionID, CProtocolBuff_Net* Packet);


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
		CChatServer();

		//소멸자
		virtual ~CChatServer();

		// 채팅 서버 시작 함수
		// 내부적으로 NetServer의 Start도 같이 호출
		//
		// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
		// return true : 성공
		bool ServerStart();

		// 채팅 서버 종료 함수
		//
		// Parameter : 없음
		// return : 없음
		void ServerStop();

		// 테스트용. 큐 노드 수 얻기
		LONG GetQueueInNode()
		{
			return m_LFQueue->GetInNode();
		}

		// 테스트용. 맵 안의 플레이어 수 얻기
		LONG JoinPlayerCount()
		{
			return (LONG)m_mapPlayer.size();
		}

		// !! 테스트용 !!
		// 일감 TLS의 총 할당된 청크 수 반환
		LONG GetWorkChunkCount()
		{
			return m_MessagePool->GetAllocChunkCount();
		}

		// !! 테스트용 !!
		// 일감 TLS의 현재 밖에서 사용중인 청크 수 반환
		LONG GetWorkOutChunkCount()
		{
			return m_MessagePool->GetOutChunkCount();
		}


		// !! 테스트용 !!
		// 플레이어 TLS의 총 할당된 청크 수 반환
		LONG GetPlayerChunkCount()
		{
			return m_PlayerPool->GetAllocChunkCount();
		}

		// !! 테스트용 !!
		// 플레이어 TLS의 현재 밖에서 사용중인 청크 수 반환
		LONG GetPlayerOutChunkCount()
		{
			return m_PlayerPool->GetOutChunkCount();
		}
	};
}



#endif // !__CHAT_SERVER_H__
