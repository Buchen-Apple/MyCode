#ifndef __BATTLESERVER_ROOM_VERSION_H__
#define __BATTLESERVER_ROOM_VERSION_H__

#include "NetworkLib/NetworkLib_MMOServer.h"
#include "NetworkLib/NetworkLib_LanClinet.h"

#include "Http_Exchange/HTTP_Exchange.h"
#include "shDB_Communicate.h"

#include <unordered_set>
#include <unordered_map>

using namespace std;

// ----------------------------------------
// 
// MMOServer를 이용한 배틀 서버. 룸 버전
//
// ----------------------------------------
namespace Library_Jingyu
{
	class CBattleServer_Room :public CMMOServer
	{
		friend class CGame_MinitorClient;
		friend class CBattle_Master_LanClient;



		// -----------------------
		// 이너 클래스
		// -----------------------

		// CMMOServer의 cSession을 상속받는 세션 클래스
		class CGameSession :public CMMOServer::cSession
		{
			friend class CBattleServer_Room;	

			// -----------------------
			// 멤버 변수
			// -----------------------

			// 회원 번호
			INT64 m_Int64AccountNo;

			// 유저의 ClientKey
			// 매칭서버가 발급.
			// 유저 모두에게 고유
			INT64 m_int64ClientKey;

			// 해당 유저가 접속 중인 룸 번호
			int m_iRoomNo;

			// CBattleServer_Room의 포인터 (부모)
			CBattleServer_Room* m_pParent;		

			// AccountNo 자료구조에 들어감 체크 플래그
			// true면 들어간 상태
			bool m_bStructFlag;

			// 로그인 패킷 처리 플래그
			// true면 세션키까지 정상적으로 처리됨
			bool m_bLoginFlag;


			
		private:
			// -----------------------
			// 생성자와 소멸자
			// -----------------------

			CGameSession();
			virtual ~CGameSession();


		private:
			// -----------------
			// 가상함수
			// -----------------

			// Auth 스레드에서 처리
			virtual void OnAuth_ClientJoin();
			virtual void OnAuth_ClientLeave(bool bGame = false);
			virtual void OnAuth_Packet(CProtocolBuff_Net* Packet);

			// Game 스레드에서 처리
			virtual void OnGame_ClientJoin();
			virtual void OnGame_ClientLeave();
			virtual void OnGame_Packet(CProtocolBuff_Net* Packet);

			// Release용
			virtual void OnGame_ClientRelease();


			// -----------------
			// 패킷 처리 함수
			// -----------------

			// 로그인 요청 
			// 
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Auth_LoginPacket(CProtocolBuff_Net* Packet);	

			// 방 입장 요청
			// 
			// Parameter : CProtocolBuff_Net*
			// return : 없음
			void Auth_RoomEnterPacket(CProtocolBuff_Net* Packet);

		};

		friend class CGameSession;

		// 파일에서 읽어오기 용 구조체
		struct stConfigFile
		{
			// 넷 서버
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

			// 마스터 클라
			TCHAR MasterServerIP[20];
			int MasterServerPort;
			int MasterClientCreateWorker;
			int MasterClientActiveWorker;
			int MasterClientNodelay;


			// 모니터링 클라
			TCHAR MonitorServerIP[20];
			int MonitorServerPort;
			int MonitorClientCreateWorker;
			int MonitorClientActiveWorker;
			int MonitorClientNodelay;
		};
				
		// 방 구조체
		struct stRoom
		{
			// 룸 번호
			int m_iRoomNo;

			// 룸의 상태
			// 0 : 대기방
			// 1 : 준비방
			// 2 : 플레이 방
			int m_iRoomState;

			// 현재 룸 안에 있는 유저 수
			int m_iJoinUserCount;			

			// Game모드 상태의 유저 수
			int m_iGameModeUserCount;

			// 해당 방이 정말로 Play중인지.
			// 정말 모든 유저가 Game모드가 되어 게임이 시작 된 경우
			// 방 모드와는 다름
			bool m_bGameStart;	

			// 카운트 다운. 밀리세컨드 단위
			// timeGetTime()의 값이 들어간다.
			DWORD m_dwCountDown;

			// 방 입장 토큰 (배틀서버 입장 토큰과는 다름)
			char m_cEnterToken[32];


			// ------------

			// 방에 입장한 유저들 관리 자료구조.
			vector<CGameSession*> m_JoinUser_Vector;
					   
			// 입장 가능한 최대 인원 수. 고정 값
			const int m_iMaxJoinCount = 5;

			// 생성자
			stRoom()
			{
				// 미리 메모리 공간 잡아두기
				m_JoinUser_Vector.reserve(10);
			}

			// 자료구조에 Insert
			//
			// Parameter : 추가하고자 하는 CGameSession*
			// return : 없음
			void Insert(CGameSession* InsertPlayer)
			{
				m_JoinUser_Vector.push_back(InsertPlayer);
			}

			// 자료구조에서 Erase
			//
			// Parameter : 제거하고자 하는 CGameSession*
			// return : 성공 시 true
			//		  : 실패 시  false
			bool Erase(CGameSession* InsertPlayer)
			{
				size_t Size = m_JoinUser_Vector.size();
				bool Flag = false;

				// 1. 자료구조 안에 유저가 0보다 작거나 같으면 return false
				if (Size <= 0)
					return false;	

				// 2. 자료구조 안에 유저가 1명이거나, 찾고자 하는 유저가 마지막에 있다면 바로 제거
				if (Size == 1 || m_JoinUser_Vector[Size - 1] == InsertPlayer)
				{
					Flag = true;
					m_JoinUser_Vector.pop_back();
				}

				// 3. 아니라면 Swap 한다
				else
				{
					size_t Index = 0;
					while (Index < Size)
					{
						// 내가 찾고자 하는 유저를 찾았다면
						if (m_JoinUser_Vector[Index] == InsertPlayer)
						{
							Flag = true;

							CGameSession* Temp = m_JoinUser_Vector[Size - 1];
							m_JoinUser_Vector[Size - 1] = m_JoinUser_Vector[Index];
							m_JoinUser_Vector[Index] = Temp;

							m_JoinUser_Vector.pop_back();							

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

			// 자료구조 내의 모든 유저에게 인자로 받은 패킷 보내기
			//
			// Parameter : CProtocolBuff_Net*
			// return : 성공 시 true
			//		  : 유저가 0명일 시 false.
			bool SendPacket_BroadCast(CProtocolBuff_Net* SendBuff)
			{
				// 1. 자료구조 내의 유저 수 받기
				size_t Size = m_JoinUser_Vector.size();

				// 2. 유저 수가 0명이거나 적을 경우, 문제있음 return false
				if (Size <= 0)
					return false;

				// !! while문 돌기 전에 카운트를, 유저 수 만큼 증가 !!
				// 엔진쪽에서, 완료 통지가 오면 Free를 하기 때문에 Add해야 한다.
				SendBuff->Add((int)Size);

				// 3. 유저 수 만큼 돌면서 패킷 전송.
				size_t Index = 0;

				while (Index < Size)
				{
					m_JoinUser_Vector[Index]->SendPacket(SendBuff);
					++Index;
				}

				// 4. 패킷 Free
				// !! 엔진쪽 완통에서 Free 하지만, 레퍼런스 카운트를 인원 수 만큼 Add 했기 때문에 1개가 더 증가한 상태. !!	
				CProtocolBuff_Net::Free(SendBuff);

				return true;
			}
		};

		// 방 상태 state
		enum eu_ROOM_STATE
		{
			// 대기방
			WAIT_ROOM = 0,

			// 준비방
			READY_ROOM,

			// 플레이방
			PLAY_ROOM
		};

		// DB 요청에 대한 후처리를 위한, Type
		enum eu_DB_READ_TYPE
		{
			// 로그인 패킷에 대한 처리
			eu_LOGIN	= 0			
		};
		
		// 절대 변경 불가능한 Config 변수 관리 구조체
		struct stCONFIG
		{
			// Auth_Update에서 한 프레임에 처리할 HTTP통신 후처리
			// 고정 값
			const int m_iHTTP_MAX = 50;

			// 최대 존재할 수 있는 방 수
			// 고정 값
			const LONG m_lMaxTotalRoomCount = 500;

			// 최대 존재할 수 있는 대기방 수
			// 고정 값
			const LONG m_lMaxWaitRoomCount = 200;

			// Auth_Update에서 한 프레임에 생성 가능한 방 수
			// 고정 값
			const int m_iLoopCreateRoomCount = 30;

			// 토큰 재발급 시간
			// 밀리세컨드 단위.
			// ex) 1000일 경우, 1초 단위로 토큰 재발급.
			const int m_iTokenChangeSlice = 12000;
		};




		// -----------------------
		// 멤버 변수
		// -----------------------

		// 세션. CMMOServer와 공유.
		CGameSession* m_cGameSession;

		// Config 변수
		stConfigFile m_stConfig;

		// 고정값 변수들을 보관하는 구조체 변수
		stCONFIG m_stConst;

		// 모니터링 클라
		CGame_MinitorClient* m_Monitor_LanClient;

		// 마스터와 연결된 클라
		CBattle_Master_LanClient* m_Master_LanClient;

		// 버전 코드
		// 클라가 들고온 것을 비교. 파싱으로 읽어옴.
		int m_uiVer_Code;

		// shDB와 통신하는 변수
		shDB_Communicate m_shDB_Communicate;	

		// 나의 서버 No
		// 마스터 서버가 할당해 준다.
		int m_iServerNo;

		


		

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




		// -----------------------
		// 방 관련 변수
		// -----------------------

		// 방 번호를 할당할 변수
		// 인터락으로 ++
		LONG m_lGlobal_RoomNo;
		
		// 현재 대기방 수 (5/5가 되지 않은 방)
		LONG m_lNowWaitRoomCount;

		// 현재 게임 내에 만들어진 방 수 (모든 방 합쳐서)
		LONG m_lNowTotalRoomCount;

		// 방 입장 토큰
		// 미리 64개정도 만들어 두고, 방생성 시 랜덤으로 보낸다.
		char m_cRoomEnterToken[64][32];

		// stRoom 관리 Pool
		CMemoryPoolTLS<stRoom> *m_Room_Pool;

		

		


		// -----------------------
		// 방 관리 자료구조 변수
		// -----------------------

		// Auth 모드에 있는 방 관리 umap
		//
		// Key : RoomNo, Value : stRoom*
		unordered_map<int, stRoom*> m_Room_Umap;

		// m_AuthRoom_Umap SRW락
		SRWLOCK m_Room_Umap_srwl;






		// -----------------------
		// 유저 관리 자료구조 변수
		// -----------------------

		// AccountNo를 이용해 CGameSession* 관리
		//
		// 접속 중인 유저의 AccountNo를 기준으로 CGameSession* 관리
		// Key : AccountNo, Value : CGameSession*
		unordered_map<INT64, CGameSession*> m_AccountNo_Umap;

		// m_AccountNo_Umap용 SRW락
		SRWLOCK m_AccountNo_Umap_srwl;			  



	private:
		// -----------------------
		// 내부에서만 사용하는 함수
		// -----------------------

		// 파일에서 Config 정보 읽어오기
		// 
		// Parameter : config 구조체
		// return : 정상적으로 셋팅 시 true
		//		  : 그 외에는 false
		bool SetFile(stConfigFile* pConfig);

		// Start
		// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
		//
		// Parameter : 없음
		// return : 실패 시 false
		bool BattleServerStart();

		// Stop
		// 내부적으로 Stop 실행
		//
		// Parameter : 없음
		// return : 없음
		void BattleServerStop();

		// 출력용 함수
		//
		// Parameter : 없음
		// return : 없음
		void ShowPrintf();


		

	private:
		// -----------------------
		// 패킷 후처리 함수
		// -----------------------

		// Login 패킷 후처리
		//
		// Parameter : DB_WORK_LOGIN*
		// return : 없음
		void Auth_LoginPacket_Last(DB_WORK_LOGIN* DBData);




	private:
		// -----------------------
		// AccountNo 자료구조 관리 함수
		// -----------------------

		// AccountNo 자료구조에 유저를 추가하는 함수
		//
		// Parameter : AccountNo, CGameSession*
		// return : 성공 시 true
		//		  : 실패 시 false
		bool InsertAccountNoFunc(INT64 AccountNo, CGameSession* InsertPlayer);

		// AccountNo 자료구조에서 유저 검색 후, 해당 유저에게 Disconenct 하는 함수
		//
		// Parameter : AccountNo
		// return : 없음
		void DisconnectAccountNoFunc(INT64 AccountNo);

		// AccountNo 자료구조에서 유저를 제거하는 함수
		//
		// Parameter : AccountNo
		// return : 성공 시 true
		//		  : 실패 시 false
		bool EraseAccountNoFunc(INT64 AccountNo);

		


	private:
		// ---------------------------------
		// Auth모드의 방 관리 자료구조 변수
		// ---------------------------------

		// 방을 Room 자료구조에 Insert하는 함수
		//
		// Parameter : RoomNo, stRoom*
		// return : 성공 시 true
		//		  : 실패(중복 키) 시 false	
		bool InsertRoomFunc(int RoomNo, stRoom* InsertRoom);

		




	private:
		// -----------------------
		// 가상함수
		// -----------------------

		// AuthThread에서 1Loop마다 1회 호출.
		// 1루프마다 정기적으로 처리해야 하는 일을 한다.
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnAuth_Update();

		// GameThread에서 1Loop마다 1회 호출.
		// 1루프마다 정기적으로 처리해야 하는 일을 한다.
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnGame_Update();

		// 새로운 유저 접속 시, Auth에서 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

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
		// -----------------------
		// 생성자와 소멸자
		// -----------------------

		// 생성자
		CBattleServer_Room();

		// 소멸자
		virtual ~CBattleServer_Room();
	};

}


// ----------------------------------------
// 
// 마스터 서버와 연결되는 LanClient
//
// ----------------------------------------
namespace Library_Jingyu
{
	class CBattle_Master_LanClient :public CLanClient
	{
		friend class CBattleServer_Room;


		// ------------- 
		// 멤버 변수
		// ------------- 

		// 현재 마스터 서버와 연결된 세션 ID
		ULONGLONG m_ullSessionID;

		// 마스터 넷 서버의 IP와 Port
		TCHAR m_tcBattleNetServerIP[30];
		int m_iBattleNetServerPort;

		// 채팅 넷 서버의 IP와 Port
		TCHAR m_tcChatNetServerIP[30];
		int m_iChatNetServerPort;

		// 마스터 서버에 들고갈 입장 토큰
		char m_cMasterToken[32];

		// 마스터 서버에게 패킷을 하나 보낼 때 1씩 증가되는 값.
		UINT uiReqSequence;

		// 토큰 발급 시간 갱신하기
		// timeGetTime() 함수의 리턴값.
		// 밀리 세컨드 단위
		DWORD m_dwTokenSendTime;



		// ----------------------
		// !!Battle 서버의 this !!
		// ----------------------
		CBattleServer_Room* m_BattleServer_this;


		



	private:
		// -----------------------
		// 패킷 처리 함수
		// -----------------------

		// 마스터에게 보낸 로그인 요청 응답
		//
		// Parameter : SessionID, CProtocolBuff_Lan*
		// return : 없음
		void Packet_Login_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);




	private:
		// -----------------------
		// Battle Net 서버가 호출하는 함수
		// -----------------------

		// 마스터에게, 신규 대기방 생성 패킷 보내기
		//
		// Parameter : 배틀서버 No, stRoom*
		// return : 없음
		void Packet_NewRoomCreate_Req(int BattleServerNo, CBattleServer_Room::stRoom* NewRoom);

		// 토큰 재발급 함수
		//
		// Parameter : 없음
		// return : 없음
		void Packet_TokenChange_Req();

		// 마스터에게, 방 닫힘 패킷 보내기
		//
		// Parameter : RoomNo
		// return : 없음
		void Packet_RoomClose(int RoomNo);


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

		// 배틀서버의 this를 입력받는 함수
		// 
		// Parameter : 배틀 서버의 this
		// return : 없음
		void ParentSet(CBattleServer_Room* ChatThis);



	private:
		// -----------------------
		// 순수 가상함수
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

		

	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CBattle_Master_LanClient();
		virtual ~CBattle_Master_LanClient();

	};
}



// ---------------
// CGame_MonitorClient
// CLanClient를 상속받는 모니터링 클라. 모니터링 LanServer로 정보 전송
// ---------------
namespace Library_Jingyu
{
	class CGame_MinitorClient :public CLanClient
	{
		friend class CGameServer;

		// 디파인 정보들 모아두기
		enum en_MonitorClient
		{
			dfSERVER_NO = 2	// 배틀서버는 2번이다
		};

		// 모니터링 서버로 정보 전달할 스레드의 핸들.
		HANDLE m_hMonitorThread;

		// 모니터링 스레드를 종료시킬 이벤트
		HANDLE m_hMonitorThreadExitEvent;

		// 현재 모니터링 서버와 연결된 세션 ID
		ULONGLONG m_ullSessionID;

		// ----------------------
		// !!Battle 서버의 this !!
		// ----------------------
		CBattleServer_Room* m_BattleServer_this;



	private:
		// -----------------------
		// 스레드
		// -----------------------

		// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
		static UINT	WINAPI MonitorThread(LPVOID lParam);




	private:
		// -----------------------
		// 내부에서만 사용하는 기능 함수
		// -----------------------	

		// 모니터링 서버로 데이터 전송
		//
		// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
		// return : 없음
		void InfoSend(BYTE DataType, int DataValue, int TimeStamp);
			   		 

	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CGame_MinitorClient();
		virtual ~CGame_MinitorClient();


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

		// 배틀서버의 this를 입력받는 함수
		// 
		// Parameter : 배틀 서버의 this
		// return : 없음
		void ParentSet(CBattleServer_Room* ChatThis);




	private:
		// -----------------------
		// 순수 가상함수
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



#endif // !__BATTLESERVER_ROOM_VERSION_H__
