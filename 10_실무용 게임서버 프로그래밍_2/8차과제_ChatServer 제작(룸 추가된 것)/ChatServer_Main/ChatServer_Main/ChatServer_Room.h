#ifndef __CHAT_SERVER_ROOM_H__
#define __CHAT_SERVER_ROOM_H__

#include "NetworkLib/NetworkLib_NetServer.h"
#include "NetworkLib/NetworkLib_LanClinet.h"
#include "ObjectPool/Object_Pool_LockFreeVersion.h"

#include <unordered_map>
#include <vector>
#include <process.h>

using namespace std;


// ------------------------
//
// Chat_Net서버
//
// ------------------------
namespace Library_Jingyu
{
	class CChatServer_Room	:public CNetServer
	{
		friend class CChat_LanClient;
		friend class CChat_MonitorClient;

		// --------------
		// 이너 클래스
		// --------------

		// 플레이어 구조체
		struct stPlayer
		{
			// 세션 ID
			// 엔진과 통신 시 사용
			ULONGLONG m_ullSessionID;

			// 회원 번호
			INT64 m_i64AccountNo;

			// 접속중인 룸 번호
			int m_iRoomNo;

			// ID
			TCHAR m_tcID[20];

			// Nickname
			TCHAR m_tcNick[20];

			// 로그인 여부 체크
			// true면 로그인 중.
			bool m_bLoginCheck;

			// 마지막으로 패킷을 받은 시간
			DWORD m_dwLastPacketTime;

			stPlayer()
			{
				// 시작 시, 룸 번호는 -1
				m_iRoomNo = -1;
			}
		};

		// 룸 구조체
		struct stRoom
		{
			// 배틀서버 번호
			int m_iBattleServerNo;

			// 룸 번호
			int m_iRoomNo;		

			// 입장 가능 유저 수
			int m_iMaxUser;

			// 방에 접속 중인 유저 수
			int m_iJoinUser;

			// 방 입장 토큰
			char m_cEnterToken[32];

			// 삭제될 방 플래그
			// true면 삭제될 방이다.
			// true면서 방에 유저 수가 0명이면 방이 삭제된다.
			bool m_bDeleteFlag;

			// 룸에 접속한 유저들 관리 자료구조
			//
			// SessionID 관리
			vector<ULONGLONG> m_JoinUser_vector;

			// 룸 락
			SRWLOCK m_Room_srwl;

			stRoom()
			{
				// 락 초기화
				InitializeSRWLock(&m_Room_srwl);
			}



			// --------------
			// 멤버 함수
			// --------------

			// 룸 락
			//
			// Parameter : 없음
			// return : 없음
			void ROOM_LOCK()
			{
				AcquireSRWLockExclusive(&m_Room_srwl);
			}

			// 룸 언락
			//
			// Parameter : 없음
			// return : 없음
			void ROOM_UNLOCK()
			{
				ReleaseSRWLockExclusive(&m_Room_srwl);
			}

			// 내부의 자료구조에 인자로 받은 ClientKey 추가
			//
			// Parameter : ClientKey
			// return : 없음
			void Insert(ULONGLONG ClientKey)
			{
				m_JoinUser_vector.push_back(ClientKey);
			}

			// 내부의 자료구조에서 인자로 받은 ClientKey 제거
			//
			// Parameter : ClientKey
			// return : 성공 시 true
			//		  : 해당 유저가 없을 시 false
			bool Erase(ULONGLONG ClientKey)
			{
				size_t Size = m_JoinUser_vector.size();
				bool Flag = false;

				// 1. 자료구조 안에 유저가 0이라면 return false
				if (Size == 0)
					return false;

				// 2. 자료구조 안에 유저가 1명이거나, 찾고자 하는 유저가 마지막에 있다면 바로 제거
				if (Size == 1 || m_JoinUser_vector[Size - 1] == ClientKey)
				{
					Flag = true;
					m_JoinUser_vector.pop_back();
				}

				// 3. 아니라면 Swap 한다
				else
				{
					size_t Index = 0;
					while (Index < Size)
					{
						// 내가 찾고자 하는 유저를 찾았다면
						if (m_JoinUser_vector[Index] == ClientKey)
						{
							Flag = true;

							ULONGLONG Temp = m_JoinUser_vector[Size - 1];
							m_JoinUser_vector[Size - 1] = m_JoinUser_vector[Index];
							m_JoinUser_vector[Index] = Temp;

							m_JoinUser_vector.pop_back();

							break;
						}

						++Index;
					}
				}

				// 4. 만약, 제거 못했다면 return false
				if (Flag == false)
					return false;

				return true;
			}
								
		};

		// 파싱 구조체
		struct stParser
		{
			// 넷 서버
			TCHAR BindIP[20];
			int Port;
			int CreateWorker;
			int ActiveWorker;
			int CreateAccept;
			int HeadCode;
			int XORCode;
			int Nodelay;
			int MaxJoinUser;
			int LogLevel;

			// 외부에서 접속할 IP
			TCHAR ChatIP[20];

			// 배틀과 연결되는 랜 클라
			TCHAR BattleServerIP[20];
			int BattleServerPort;
			int BattleClientCreateWorker;
			int BattleClientActiveWorker;
			int BattleClientNodelay;

			// 모니터링과 연결되는 랜 클라
			TCHAR MonitorServerIP[20];
			int MonitorServerPort;
			int MonitorClientCreateWorker;
			int MonitorClientActiveWorker;
			int MonitorClientNodelay;
		};



		// --------------
		// 멤버 변수
		// --------------

		// 파서 변수
		stParser m_Paser;

		// 모니터링 랜 클라 
		CChat_MonitorClient* m_pMonitor_Client;

		// 배틀 랜 클라
		CChat_LanClient* m_pBattle_Client;


		// ---- 카운트 용 ----

		// 실제 로그인 패킷까지 처리된 유저 카운트.
		LONG m_lChatLoginCount;

		// Player 구조체 할당량
		// Alloc시 1 증가, Free 시 1 감소
		LONG m_lUpdateStruct_PlayerCount;

		// 채팅서버 입장 토큰이 다를 시 1 증가
		LONG m_lEnterTokenMiss;

		// 룸 입장 토큰이 다를 시 1 증가
		LONG m_lRoom_EnterTokenMiss;

		// 채팅서버 내 방 수 (모드 상관 없이)
		LONG m_lRoomCount;

		// 중복 로그인 횟수
		LONG m_lLoginOverlap;





		// -----------------------
		// 토큰관리 변수
		// -----------------------

		// 입장 토큰 관리
		// 배틀서버는 이 2개 중 하나라도 일치한다면 맞다고 통과시킨다.

		// 배틀서버 입장 토큰 1번
		// "현재" 토큰을 보관한다.
		// 마스터와 연결된 랜 클라가 생성 및 갱신한다.
		char m_cConnectToken_Now[32];

		// 배틀서버 입장 토큰 2번 
		// "이전" 토큰을 보관한다.
		// 토큰이 재할당 될 경우, 이전 토큰을 여기다 저장한 후
		// 새로운 토큰을 "현재" 토큰에 넣는다.
		char m_cConnectToken_Before[32];





		// --------------------------------
		// 플레이어 관리 자료구조 변수
		// --------------------------------

		// 플레이어 관리 자료구조
		//
		// Key : 엔진에서 받은 SessionID, Value : stPlayer*
		unordered_map<ULONGLONG, stPlayer*> m_Player_Umap;

		//m_Player_Umap용 srw락
		SRWLOCK m_Player_Umap_srwl;

		// stPlayer 관리 메모리풀
		CMemoryPoolTLS<stPlayer> *m_pPlayer_Pool;






		// --------------------------------
		// 로그인 한 플레이어 관리 자료구조 변수
		// --------------------------------

		// 로그인 한 플레이어 관리 자료구조
		//
		// Key : AccountNo, Value : stPlayer*
		unordered_map<INT64, ULONGLONG> m_LoginPlayer_Umap;

		//m_Player_map용 srw락
		SRWLOCK m_LoginPlayer_Umap_srwl;




		// --------------------------
		// 룸 관리 자료구조 변수
		// --------------------------

		// 룸 관리 자료구조
		//
		// Key : RoomNo, Value : stRoom*
		unordered_map<int, stRoom*> m_Room_Umap;

		//m_Player_Umap용 srw락
		SRWLOCK m_Room_Umap_srwl;

		// stRoom 관리 메모리풀
		CMemoryPoolTLS<stRoom> *m_pRoom_Pool;



		// --------------------------
		// 하트비트 스레드 변수
		// --------------------------

		// 하트비트 스레드 핸들
		HANDLE m_hHBthreadHandle;

		// 하트비트 스레드 종료 이벤트
		HANDLE m_hHBThreadExitEvent;


	private:
		// -----------------------
		// 스레드
		// -----------------------

		// 하트비트 스레드
		static UINT WINAPI HeartBeatThread(LPVOID lParam);




	private:
		// ----------------------------
		// 플레이어 관리 자료구조 함수
		// ----------------------------

		// 플레이어 관리 자료구조에 Insert
		//
		// Parameter : SessionID, stPlayer*
		// return : 정상 추가 시 true
		//		  : 키 중복 시 false
		bool InsertPlayerFunc(ULONGLONG SessionID, stPlayer* InsertPlayer);

		// 플레이어 관리 자료구조에서 검색
		//
		// Parameter : SessionID
		// return :  잘 찾으면 stPlayer*
		//		  :  없는 유저일 시 nullptr	
		stPlayer* FindPlayerFunc(ULONGLONG SessionID);

		// 플레이어 관리 자료구조에서 제거
		//
		// Parameter : SessionID
		// return :  Erase 후 Second(stPlayer*)
		//		  : 없는 유저일 시 nullptr
		stPlayer* ErasePlayerFunc(ULONGLONG SessionID);
			   



	private:
		// ----------------------------
		// 로그인 한 플레이어 관리 자료구조 함수
		// ----------------------------

		// 로그인 한 플레이어 관리 자료구조에 Insert
		//
		// Parameter : AccountNo, SessionID
		// return : 정상 추가 시 true
		//		  : 키 중복 시 flase
		bool InsertLoginPlayerFunc(INT64 AccountNo, ULONGLONG SessionID);	

		// 로그인 한 플레이어 관리 자료구조에서 제거
		//
		// Parameter : AccountNo
		// return : 정상적으로 Erase 시 true
		//		  : 유저  검색 실패 시 false
		bool EraseLoginPlayerFunc(ULONGLONG AccountNo);



	private:
		// ----------------------------
		// 룸 관리 자료구조 함수
		// ----------------------------

		// 룸 자료구조에 Insert
		//
		// Parameter : RoonNo, stROom*
		// return : 정상 추가 시 true
		//		  : 키 중복 시 flase
		bool InsertRoomFunc(int RoomNo, stRoom* InsertRoom);

		// 룸 자료구조에서 Erase
		//
		// Parameter : RoonNo
		// return : 정상적으로 제거 시 stRoom*
		//		  : 검색 실패 시 nullptr
		stRoom* EraseRoomFunc(int RoomNo);

		// 룸 자료구조의 모든 방 삭제
		//
		// Parameter : 없음
		// return : 없음
		void RoomClearFunc();



	public:
		// --------------
		// 외부에서 호출 가능한 함수
		// --------------

		// 출력용 함수
		void ShowPrintf();


		// 서버 시작
		//
		// Parameter : 없음
		// return : 성공 시 true
		//		  : 실패 시 false
		bool ServerStart();

		// 서버 종료
		//
		// Parameter : 없음
		// return : 없음
		void ServerStop();



	private:
		// ------------------
		// 내부에서만 사용하는 함수
		// ------------------

		// 방 안의 모든 유저에게 인자로 받은 패킷 보내기.
		//
		// Parameter : ULONGLONG 배열, 배열의 수,  CProtocolBuff_Net*
		// return : 자료구조에 0명이면 false. 그 외에는 true
		bool Room_BroadCast(ULONGLONG Array[], int ArraySize, CProtocolBuff_Net* SendBuff);

		// Config 셋팅
		//
		// Parameter : stParser*
		// return : 실패 시 false
		bool SetFile(stParser* pConfig);		



	private:
		// ------------------
		// 패킷 처리 함수
		// ------------------

		// 로그인 패킷
		//
		// Parameter : SessionID, CProtocolBuff_Net*
		// return : 없음
		void Packet_Login(ULONGLONG SessionID, CProtocolBuff_Net* Packet);

		// 방 입장 
		//
		// Parameter : SessionID, CProtocolBuff_Net*
		// return : 없음
		void Packet_Room_Enter(ULONGLONG SessionID, CProtocolBuff_Net* Packet);

		// 채팅 보내기
		//
		// Parameter : SessionID, CProtocolBuff_Net*
		// return : 없음
		void Packet_Message(ULONGLONG SessionID, CProtocolBuff_Net* Packet);

		// 하트비트
		//
		// Parameter : SessionID
		// return : 없음
		void Packet_HeartBeat(ULONGLONG SessionID);



	private:
		// -----------------------
		// 가상함수
		// -----------------------

		// Accept 직후, 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

		// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
		//
		// parameter : 접속한 유저에게 할당된 세션키
		// return : 없음
		virtual void OnClientJoin(ULONGLONG SessionID);

		// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 유저 세션키
		// return : 없음
		virtual void OnClientLeave(ULONGLONG SessionID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);




	public:
		// --------------
		// 생성자와 소멸자
		// --------------

		// 생성자
		CChatServer_Room();

		// 소멸자
		virtual ~CChatServer_Room();
	};
}


// -----------------------
//
// 배틀서버와 연결되는 Lan 클라
//
// -----------------------
namespace Library_Jingyu
{
	class CChat_LanClient	:public CLanClient
	{
		friend class CChatServer_Room;


		// ----------------------
		// 멤버 변수
		// ----------------------

		// !! 채팅 넷서버 !!
		CChatServer_Room* m_pNetServer;

		// 배틀 랜서버와 접속한 세션아이디
		ULONGLONG m_ullSessionID;

		// 배틀 랜서버에 로그인 여부
		// true면 로그인 된 상태
		// 서버 켜짐을 보낸 후 응답을 받으면 true로 변경
		bool m_bLoginCheck;





	private:
		// ----------------------
		// 내부에서만 사용하는 함수
		// ----------------------

		// 채팅 넷서버 셋팅
		//
		// Parameter : CChatServer_Room*
		// return : 없음
		void SetNetServer(CChatServer_Room* NetServer);

		



	private:
		// ----------------------
		// 패킷 처리 함수
		// ----------------------

		// 신규 대기방 생성
		//
		// Parameter : ClientID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_RoomCreate(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload);

		// 연결 토큰 재발행
		//
		// Parameter : ClientID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_TokenChange(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload);

		// 로그인 패킷에 대한 응답
		//
		// Parameter : CProtocolBuff_Lan*
		// return : 없음
		void Packet_Login(CProtocolBuff_Lan* Payload);

		// 방 삭제 
		//
		// Parameter : ClientID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_RoomErase(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload);


	public:
		// ----------------------
		// 외부에서 사용 가능한 함수
		// ----------------------

		// 시작 함수
		// 내부적으로, 상속받은 CLanClient의 Start호출.
		//
		// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
		// return : 성공 시 true , 실패 시 falsel 
		bool ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay);

		// 클라이언트 종료
		//
		// Parameter : 없음
		// return : 없음
		void ClientStop();




	private:
		// -----------------------
		// 가상함수
		// -----------------------

		// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnConnect(ULONGLONG ClinetID);

		// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnDisconnect(ULONGLONG ClinetID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG ClinetID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);



	public:
		// -------------------
		// 생성자와 소멸자
		// -------------------

		// 생성자
		CChat_LanClient();

		// 소멸자
		virtual ~CChat_LanClient();

	};
}


// ---------------------------------------------
// 
// 모니터링 LanClient
// 
// ---------------------------------------------
namespace Library_Jingyu
{
	class CChat_MonitorClient :public CLanClient
	{
		friend class CChatServer_Room;

		// 디파인 정보들 모아두기
		enum en_MonitorClient
		{
			dfSERVER_NO = 3	// 채팅서버는 3번이다
		};

		// -----------------------
		// 멤버 변수
		// -----------------------

		// 모니터링 서버로 정보 전달할 스레드의 핸들.
		HANDLE m_hMonitorThread;

		// 모니터링 서버를 종료시킬 이벤트
		HANDLE m_hMonitorThreadExitEvent;

		// 현재 모니터링 서버와 연결된 세션 ID
		ULONGLONG m_ullSessionID;

		// ----------------------
		// !! 채팅 서버의 this !!
		// ----------------------
		CChatServer_Room* m_ChatServer_this;


	private:
		// -----------------------
		// 내부에서만 사용하는 기능 함수
		// -----------------------

		// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
		static UINT	WINAPI MonitorThread(LPVOID lParam);

		// 모니터링 서버로 데이터 전송
		//
		// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
		// return : 없음
		void InfoSend(BYTE DataType, int DataValue, int TimeStamp);




	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CChat_MonitorClient();
		virtual ~CChat_MonitorClient();


	public:

		// -----------------------
		// 외부에서 사용 가능한 함수
		// -----------------------

		// 시작 함수
		// 내부적으로, 상속받은 CLanClient의 Start호출.
		//
		// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
		// return : 성공 시 true , 실패 시 falsel 
		bool ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay);

		// 종료 함수
		// 내부적으로, 상속받은 CLanClient의 Stop호출.
		// 추가로, 리소스 해제 등
		//
		// Parameter : 없음
		// return : 없음
		void ClientStop();

		// 채팅서버의 this를 입력받는 함수
		// 
		// Parameter : 쳇 서버의 this
		// return : 없음
		void SetNetServer(CChatServer_Room* ChatThis);


	private:
		// -----------------------
		// 가상함수
		// -----------------------

		// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnConnect(ULONGLONG SessionID);

		// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnDisconnect(ULONGLONG SessionID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, CProtocolBuff_Lan*
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);

	};
}







#endif // !__CHAT_SERVER_ROOM_H__
