#ifndef __CHAT_SERVER_ROOM_H__
#define __CHAT_SERVER_ROOM_H__

#include "NetworkLib/NetworkLib_NetServer.h"
#include "ObjectPool/Object_Pool_LockFreeVersion.h"

#include <unordered_map>
#include <vector>

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
		};

		// 룸 구조체
		struct stRoom
		{
			// 룸 번호
			int m_iRoomNo;

			// 방 입장 토큰
			char m_cEnterToken[32];

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




		// --------------
		// 멤버 변수
		// --------------

		// 채팅 Net서버 입장 토큰
		// 클라가 들고 온다.
		char m_cConnectToken[32];





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
		unordered_map<INT64, stPlayer*> m_LoginPlayer_Umap;

		//m_Player_Umap용 srw락
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
		// 플레이어 관리 자료구조 함수
		// ----------------------------

		// 로그인 한 플레이어 관리 자료구조에 Insert
		//
		// Parameter : AccountNo, stPlayer*
		// return : 정상 추가 시 true
		//		  : 키 중복 시 flase
		bool InsertLoginPlayerFunc(INT64 AccountNo, stPlayer* InsertPlayer);	

		// 로그인 한 플레이어 관리 자료구조에서 제거
		//
		// Parameter : AccountNo
		// return : 정상적으로 Erase 시 true
		//		  : 유저  검색 실패 시 false
		bool EraseLoginPlayerFunc(ULONGLONG AccountNo);




	public:
		// --------------
		// 외부에서 호출 가능한 함수
		// --------------

		// 서버 시작
		//
		// Parameter : 없음
		// return : 성공 시 true
		//		  : 실패 시 false



		// 서버 종료
		//
		// Parameter : 없음
		// return : 없음



	private:
		// ------------------
		// 내부에서만 사용하는 함수
		// ------------------

		// 방 안의 모든 유저에게 인자로 받은 패킷 보내기.
		//
		// Parameter : stRoom* CProtocolBuff_Net*
		// return : 자료구조에 0명이면 false. 그 외에는 true
		bool Room_BroadCast(stRoom* NowRoom, CProtocolBuff_Net* SendBuff)
		{
			// 자료구조 사이즈가 0이면 false
			size_t Size = NowRoom->m_JoinUser_vector.size();
			if (Size == 0)
				return false;

			// 직렬화 버퍼 레퍼런스 카운트 증가
			SendBuff->Add((int)Size);

			// 처음부터 순회하며 보낸다.
			size_t Index = 0;

			while (Index < Size)
			{
				SendPacket(NowRoom->m_JoinUser_vector[0], SendBuff);

				++Index;				
			}			

			return true;
		}



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


#endif // !__CHAT_SERVER_ROOM_H__
