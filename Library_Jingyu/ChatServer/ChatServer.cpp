#include "pch.h"
#include "ChatServer.h"

#include "Protocol_Set\CommonProtocol.h"
#include "Log\Log.h"
#include "Parser\Parser_Class.h"
#include "CPUUsage\CPUUsage.h"
#include "PDHClass\PDHCheck.h"

#include <strsafe.h>
#include <process.h>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

extern LONG g_lAllocNodeCount;
extern LONG g_lAllocNodeCount_Lan;

extern ULONGLONG g_ullAcceptTotal;
extern LONG	  g_lAcceptTPS;
extern LONG	g_lSendPostTPS;

LONG g_lUpdateStructCount;
LONG g_lUpdateStruct_PlayerCount;
LONG		 g_lUpdateTPS;
LONG g_lTokenNodeCount;

LONG g_lTokenMiss;
LONG g_lTokenNotFound;

LONG ChatLoginCount;	// 실제 로그인 패킷까지 처리된 유저 카운트.

LONG g_lJobThreadTokenEraseCount;

// ------------- 공격 테스트용
int m_SectorPosError;

int m_SectorNoError;
int m_ChatNoError;

int m_TypeError;

int m_HeadCodeError;

int m_ChackSumError;
int m_HeaderLenBig;



// ---------------------------------------------
// 
// ChatServer
// 
// ---------------------------------------------
namespace Library_Jingyu
{

	// 패킷의 타입
#define TYPE_JOIN	0
#define TYPE_LEAVE	1
#define TYPE_PACKET	2

// 최초 입장 시, 임의로 셋팅해두는 섹터 X,Y 값
#define TEMP_SECTOR_POS	12345	

	// 넷 직렬화 버퍼 1개의 크기
	// 각 서버에 전역 변수로 존재해야 함.
	LONG g_lNET_BUFF_SIZE = 512;

	// 로그 찍을 전역변수 하나 받기.
	CSystemLog* cChatLibLog = CSystemLog::GetInstance();


	// -------------------------------------
	// 클래스 내부에서 사용하는 함수
	// -------------------------------------

	// 인자로 받은 섹터 X,Y 기준, 브로드 캐스트 시, 보내야 할 섹터 9방향 구해둔다.
	// 
	// Parameter : 기준 섹터 X, Y, 섹터구조체(out)
	// return : 없음
	void CChatServer::SecotrSave(int SectorX, int SectorY, st_SecotrSaver* Sector)
	{
		int iCurX, iCurY;

		SectorX--;
		SectorY--;

		Sector->m_dwCount = 0;

		for (iCurY = 0; iCurY < 3; iCurY++)
		{
			if (SectorY + iCurY < 0 || SectorY + iCurY >= SECTOR_Y_COUNT)
				continue;

			for (iCurX = 0; iCurX < 3; iCurX++)
			{
				if (SectorX + iCurX < 0 || SectorX + iCurX >= SECTOR_X_COUNT)
					continue;

				Sector->m_Sector[Sector->m_dwCount].x = SectorX + iCurX;
				Sector->m_Sector[Sector->m_dwCount].y = SectorY + iCurY;
				Sector->m_dwCount++;

			}
		}
	}

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CChatServer::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("ChatServer_Config.ini"));
		}
		catch (int expn)
		{
			if (expn == 1)
			{
				printf("File Open Fail...\n");
				return false;
			}
			else if (expn == 2)
			{
				printf("FileR Read Fail...\n");
				return false;
			}
		}

		////////////////////////////////////////////////////////
		// ChatServer config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CHATSERVER")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("BindIP"), pConfig->BindIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("Port"), &pConfig->Port) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("CreateWorker"), &pConfig->CreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ActiveWorker"), &pConfig->ActiveWorker) == false)
			return false;

		// 생성 엑셉트
		if (Parser.GetValue_Int(_T("CreateAccept"), &pConfig->CreateAccept) == false)
			return false;

		// 헤더 코드
		if (Parser.GetValue_Int(_T("HeadCode"), &pConfig->HeadCode) == false)
			return false;

		// xorcode1
		if (Parser.GetValue_Int(_T("XorCode1"), &pConfig->XORCode1) == false)
			return false;

		// xorcode2
		if (Parser.GetValue_Int(_T("XorCode2"), &pConfig->XORCode2) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("Nodelay"), &pConfig->Nodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("MaxJoinUser"), &pConfig->MaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("LogLevel"), &pConfig->LogLevel) == false)
			return false;


		////////////////////////////////////////////////////////
		// ChatServer의 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CHATLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("LoginServerIP"), pConfig->LoginServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("LoginServerPort"), &pConfig->LoginServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("ClientCreateWorker"), &pConfig->Client_CreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ClientActiveWorker"), &pConfig->Client_ActiveWorker) == false)
			return false;


		// Nodelay
		if (Parser.GetValue_Int(_T("ClientNodelay"), &pConfig->Client_Nodelay) == false)
			return false;


		////////////////////////////////////////////////////////
		// 모니터링 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("MonitorServerIP"), pConfig->MonitorServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("MonitorServerPort"), &pConfig->MonitorServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientCreateWorker"), &pConfig->MonitorClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientActiveWorker"), &pConfig->MonitorClientActiveWorker) == false)
			return false;


		// Nodelay
		if (Parser.GetValue_Int(_T("MonitorClientNodelay"), &pConfig->MonitorClientNodelay) == false)
			return false;


		return true;
	}
	
	// Player 관리 자료구조에, 유저 추가
	// 현재 map으로 관리중
	// 
	// Parameter : SessionID, stPlayer*
	// return : 추가 성공 시, true
	//		  : SessionID가 중복될 시(이미 접속중인 유저) false
	bool CChatServer::InsertPlayerFunc(ULONGLONG SessionID, stPlayer* insertPlayer)
	{
		// map에 추가
		auto ret = m_mapPlayer.insert(make_pair(SessionID, insertPlayer));

		// 중복된 키일 시 false 리턴.
		if (ret.second == false)
			return false;

		return true;
	}

	// Player 관리 자료구조에서, 유저 검색
	// 현재 map으로 관리중
	// 
	// Parameter : SessionID
	// return : 검색 성공 시, stPalyer*
	//		  : 검색 실패 시 nullptr
	CChatServer::stPlayer* CChatServer::FindPlayerFunc(ULONGLONG SessionID)
	{
		auto FindPlayer = m_mapPlayer.find(SessionID);
		if (FindPlayer == m_mapPlayer.end())
			return nullptr;

		return FindPlayer->second;
	}

	// Player 관리 자료구조에서, 유저 제거 (검색 후 제거)
	// 현재 map으로 관리중
	// 
	// Parameter : SessionID
	// return : 성공 시, 제거된 유저 stPalyer*
	//		  : 검색 실패 시(접속중이지 않은 유저) nullptr
	CChatServer::stPlayer* CChatServer::ErasePlayerFunc(ULONGLONG SessionID)
	{
		// 1) map에서 유저 검색
		auto FindPlayer = m_mapPlayer.find(SessionID);
		if (FindPlayer == m_mapPlayer.end())
			return nullptr;

		stPlayer* ret = FindPlayer->second;

		// 2) 맵에서 제거
		m_mapPlayer.erase(FindPlayer);

		return ret;
	}

	// 마지막 패킷 받은 시간 관리 자료구조에, 유저와 시간 추가
	// 이미 있는 유저라면 기존의 정보를 갱신한다.
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID
	// return : 없음
	void CChatServer::InsertLastTime(ULONGLONG SessionID)
	{
		// 유저가 마지막으로 패킷을 받은 시간 갱신
		ULONGLONG LastTime = GetTickCount64();

		AcquireSRWLockExclusive(&m_LastPacketsrwl);		// ------------- 락
		auto ret = m_mapLastPacketTime.insert(make_pair(SessionID, LastTime));

		// 이미 있는 유저라면, 시간 갱신
		if (ret.second == false)
		{
			auto Find = m_mapLastPacketTime.find(SessionID);
			Find->second = LastTime;		
		}

		ReleaseSRWLockExclusive(&m_LastPacketsrwl);		// ------------- 언락
	}

	// 마지막 패킷 받은 시간 관리 자료구조에서, 유저 제거
	// 현재 umap으로 관리중
	// 
	// Parameter : SessionID
	// return : 없음
	void CChatServer::EraseLastTime(ULONGLONG SessionID)
	{
		// 1) map에서 유저 검색

		// erase까지가 한 작업이기 때문에, Exclusive 락 사용.
		AcquireSRWLockExclusive(&m_LastPacketsrwl);		// ---------------- Lock

		auto FindToken = m_mapLastPacketTime.find(SessionID);
		if (FindToken == m_mapLastPacketTime.end())
		{
			ReleaseSRWLockExclusive(&m_LastPacketsrwl);	// ---------------- Unlock

			// 말이 안됨. 크래시
			m_ChatDump->Crash();
		}

		// 2) 맵에서 제거	
		m_mapLastPacketTime.erase(FindToken);

		ReleaseSRWLockExclusive(&m_LastPacketsrwl);	// ---------------- Unlock
	}


	// 인자로 받은 섹터 X,Y 주변 9개 섹터의 유저들(서버에 패킷을 보낸 클라 포함)에게 SendPacket 호출
	//
	// parameter : 섹터 x,y, 보낼 버퍼
	// return : 없음
	void CChatServer::SendPacket_Sector(int SectorX, int SectorY, CProtocolBuff_Net* SendBuff)
	{
		// 1. 해당 섹터 기준, 몇 개의 섹터에 브로드캐스트 되어야하는지 카운트를 받아옴.
		DWORD dwCount = m_stSectorSaver[SectorY][SectorX]->m_dwCount;

		// 2. 카운트만큼 돌면서 보낸다.
		DWORD i = 0;
		while (i < dwCount)
		{
			auto NowSector = &m_vectorSecotr[m_stSectorSaver[SectorY][SectorX]->m_Sector[i].y][m_stSectorSaver[SectorY][SectorX]->m_Sector[i].x];

			size_t Size = NowSector->size();
			if (Size > 0)
			{
				// !! for문 돌기 전에 카운트를, 해당 섹터의 유저 수 만큼 증가 !!
				// NetServer쪽에서, 완료 통지가 오면 Free를 하기 때문에 Add해야 한다.
				SendBuff->Add((int)Size);

				size_t Index = 0;
				while (Index < Size)
				{
					SendPacket((*NowSector)[Index], SendBuff);
					Index++;
				}
			}

			i++;
		}

		// 3. 패킷 Free
		// !! Net서버쪽 완통에서 Free 하지만, 하나씩 증가시키면서 보냈기 때문에 1개가 더 증가한 상태. !!	
		CProtocolBuff_Net::Free(SendBuff);
	}

	// 업데이트 스레드
	//
	// static 함수.
	// Parameter : 채팅서버 this
	UINT WINAPI	CChatServer::UpdateThread(LPVOID lParam)
	{
		CChatServer* g_this = (CChatServer*)lParam;

		// [종료 신호, 일하기 신호] 순서대로
		HANDLE hEvent[2] = { g_this->UpdateThreadEXITEvent , g_this->UpdateThreadEvent };

		st_WorkNode* NowWork;

		// 업데이트 스레드
		while (1)
		{
			// 신호가 있으면 깨어난다.
			DWORD Check = WaitForMultipleObjects(2, hEvent, FALSE, INFINITE);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("UpdateThread Exit Error!!! (%d) \n", Error); 
				break;
			}

			// 만약, 종료 신호가 왔다면업데이트 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;

			// ----------------- 일감 있으면 일한다.
			// 일감 수를 스냅샷으로 받아둔다.
			// 해당 일감만큼만 처리하고 자러간다.
			// 처리 못한 일감은, 다음에 깨서 처리한다.
			int Size = g_this->m_LFQueue->GetInNode();

			while (Size > 0)
			{
				// 1. 큐에서 일감 1개 빼오기	
				if (g_this->m_LFQueue->Dequeue(NowWork) == -1)
					g_this->m_ChatDump->Crash();				

				// 2. Type에 따라 로직 처리				
				switch (NowWork->m_wType)
				{
					// 패킷 처리
				// 해당 타입 내부에서, 따로 try ~ catch로 처리.
				case TYPE_PACKET:
					g_this->Packet_Normal(NowWork->m_ullSessionID, NowWork->m_pPacket);

					// 패킷 Free
					CProtocolBuff_Net::Free(NowWork->m_pPacket);
					break;

					// 종료 처리
				case TYPE_LEAVE:
					g_this->Packet_Leave(NowWork->m_ullSessionID);
					break;

					// 접속 처리
				case TYPE_JOIN:
					g_this->Packet_Join(NowWork->m_ullSessionID);
					break;

				default:
					break;
				}

				--Size;

				// 3. 일감 Free
				g_this->m_MessagePool->Free(NowWork);
				InterlockedDecrement(&g_lUpdateStructCount);
				InterlockedIncrement(&g_lUpdateTPS);
			}

		}

		printf("UpdateThread Exit!!\n");

		return 0;
	}

	// 일감 스레드
	//
	// 유저 하트비트 체크, 토큰 삭제 체크
	// 토큰 자료구조 일정시간마다 비우기
	UINT	WINAPI	CChatServer::JobThread(LPVOID lParam)
	{
		CChatServer* g_this = (CChatServer*)lParam;

		// [종료 신호] 인자로 받아두기
		HANDLE hEvent = g_this->JobThreadEXITEvent;

		while (1)
		{
			// 대기 (60초에 1회 깨어난다)
			DWORD Check = WaitForSingleObject(hEvent, 60000);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("JobAddThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 만약, 종료 신호가 왔다면업데이트 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;

			// 그 무엇도 아니라면, 일을 한다.

			// 1) 하트 비트 체크	-----------------------------------
			// 하트비트는, 지정된 시간동안 유저에게 패킷이 1개도 오지 않았을 경우.
			// 즉, 새로 메시지를 받을 때 마다 유저가 마지막으로 패킷을 받은 시간은 계속 갱신된다.		

			/*
			ULONGLONG DisconnectArray[10000];
			int ArrayIndex = 0;

			AcquireSRWLockShared(&g_this->m_LastPacketsrwl);	// 락 ----------------

			// 유저 자료구조에 사이즈가 0 이상인지 체크 후 로직 진행
			if (g_this->m_mapLastPacketTime.size() > 0)
			{
				ULONGLONG ullCheckTime = GetTickCount64();

				auto LastPacketbegin = g_this->m_mapLastPacketTime.begin();
				auto LastPacketend = g_this->m_mapLastPacketTime.end();

				while (LastPacketbegin != LastPacketend)
				{
					// 마지막 패킷을 받은지 40초 이상이 되었다면
					if ((ullCheckTime - LastPacketbegin->second) >= 40000)
					{
						// 접속 끊을 유저를 배열에 보관.
						// !! m_mapLastPacketTime에 락걸고 하고 있기 때문에, 경합현상을 최대한 줄이기 위해 Disconnect 호출을 위한, SessionID만 배열에 보관해 둔다. !!
						DisconnectArray[ArrayIndex] = LastPacketbegin->first;
						ArrayIndex++;

						// 배열이 꽉찼으면 break. 나머지는 다음에 깨어났을 때 지운다.
						if (ArrayIndex == 10000)
						{
							ArrayIndex--;
							break;
						}
					}

					++LastPacketbegin;
				}
			}

			ReleaseSRWLockShared(&g_this->m_LastPacketsrwl);	// 언 락 ----------------


			// 배열에 보관한 유저를, 카운트만큼 돌면서 Disconnect 한다.
			while (ArrayIndex >= 0)
			{
				// 접속 끊기
				g_this->Disconnect(DisconnectArray[ArrayIndex]);
				--ArrayIndex;
			}
			*/


			// 2) 토큰 체크	-----------------------------------
			// 3분 이상 유지되고 있는 토큰은 erase한다.		

			AcquireSRWLockExclusive(&g_this->m_Logn_LanClient->srwl);	// 락 ----------------

			// 토큰 자료구조에 사이즈가 0 이상인지 체크 후 로직 진행
			if (g_this->m_Logn_LanClient->m_umapTokenCheck.size() > 0)
			{
				ULONGLONG ullCheckTime = GetTickCount64();

				auto Tokenbegin = g_this->m_Logn_LanClient->m_umapTokenCheck.begin();
				auto Tokenend = g_this->m_Logn_LanClient->m_umapTokenCheck.end();

				while (Tokenbegin != Tokenend)
				{
					// 인서트 된 지 3분 이상이 되었다면
					if ((ullCheckTime - Tokenbegin->second->m_ullInsertTime) >= 180000)
					{
						g_lJobThreadTokenEraseCount++;

						auto EraseToken = Tokenbegin->second;

						// 맵에서 제거
						Tokenbegin = g_this->m_Logn_LanClient->m_umapTokenCheck.erase(Tokenbegin);

						// Free
						g_this->m_Logn_LanClient->m_MTokenTLS->Free(EraseToken);
						InterlockedDecrement(&g_lTokenNodeCount);

					}

					else
						++Tokenbegin;
				}
			}

			ReleaseSRWLockExclusive(&g_this->m_Logn_LanClient->srwl);	// 언 락 ----------------
		}

		printf("JobAddThread Exit!!\n");

		return 0;
	}






	// -------------------------------------
	// 클래스 내부에서만 사용하는 패킷 처리 함수
	// -------------------------------------

	// 접속 패킷처리 함수
	// OnClientJoin에서 호출
	// 
	// Parameter : SessionID
	// return : 없음
	void CChatServer::Packet_Join(ULONGLONG SessionID)
	{
		// 1) Player Alloc()
		stPlayer* JoinPlayer = m_PlayerPool->Alloc();

		InterlockedIncrement(&g_lUpdateStruct_PlayerCount);

		// 2) 셋팅
		// SessionID
		JoinPlayer->m_ullSessionID = SessionID;

		// 섹터 좌표
		JoinPlayer->m_wSectorY = TEMP_SECTOR_POS;
		JoinPlayer->m_wSectorX = TEMP_SECTOR_POS;

		// 로그인 상태 아님으로 변경
		JoinPlayer->m_bLoginCheck = false;

		// 3) Player 관리 자료구조에 유저 추가
		if (InsertPlayerFunc(SessionID, JoinPlayer) == false)
		{
			printf("duplication SessionID!!\n");
			m_ChatDump->Crash();
		}
	}

	// 종료 패킷처리 함수
	// OnClientLeave에서 호출
	// 
	// Parameter : SessionID
	// return : 없음
	void CChatServer::Packet_Leave(ULONGLONG SessionID)
	{
		ULONGLONG TempID = SessionID;

		// 1) Player 자료구조에서 제거
		stPlayer* ErasePlayer = ErasePlayerFunc(SessionID);
		if (ErasePlayer == nullptr)
			m_ChatDump->Crash();


		// 2) 최초 할당이 아닐 경우, 섹터에서 제거
		if (ErasePlayer->m_wSectorY != TEMP_SECTOR_POS &&
			ErasePlayer->m_wSectorX != TEMP_SECTOR_POS)
		{
			auto NowSector = &m_vectorSecotr[ErasePlayer->m_wSectorY][ErasePlayer->m_wSectorX];
			size_t Size = NowSector->size();

			// -- 마지막 요소가 내가 찾고자 하는 유저이거나, 유저가 1명뿐이라면 사이즈를 1 줄인다.
			if (Size == 1 || (*NowSector)[Size - 1] == SessionID)
				NowSector->pop_back();

			// 아니라면 swap 한다
			else
			{
				size_t Index = 0;
				while (Index < Size)
				{
					if ((*NowSector)[Index] == SessionID)
					{
						ULONGLONG Temp = (*NowSector)[Size - 1];
						(*NowSector)[Size - 1] = (*NowSector)[Index];
						(*NowSector)[Index] = Temp;
						NowSector->pop_back();


						break;
					}

					++Index;

				}
			}
		}

		// 3) 초기화 ------------------
		// SectorPos를 초기 위치로 설정
		ErasePlayer->m_wSectorY = TEMP_SECTOR_POS;
		ErasePlayer->m_wSectorX = TEMP_SECTOR_POS;
		
		// 로그인 패킷까지 처리된 유저였다면, 로그인 유저 카운트 1 감소
		if (ErasePlayer->m_bLoginCheck == true)
			InterlockedDecrement(&ChatLoginCount);
		
		// 4) Player Free()
		m_PlayerPool->Free(ErasePlayer);

		// 5) 마지막 패킷 시간관리 자료구조에서 삭제.
		EraseLastTime(SessionID);

		InterlockedDecrement(&g_lUpdateStruct_PlayerCount);

	}

	// 일반 패킷처리 함수
	// 
	// Parameter : SessionID, CProtocolBuff_Net*
	// return : 없음
	void CChatServer::Packet_Normal(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{		
		try
		{
			// 1. 패킷 타입 확인
			WORD Type;
			Packet->GetData((char*)&Type, 2);

			// 2. 타입에 따라 switch case
			switch (Type)
			{
				// 채팅서버 섹터 이동 요청
			case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
				Packet_Sector_Move(SessionID, Packet);

				break;

				// 채팅서버 채팅보내기 요청
			case en_PACKET_CS_CHAT_REQ_MESSAGE:
				Packet_Chat_Message(SessionID, Packet);

				break;

				// 하트비트
			case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
				// 하는거 없음
				break;

				// 로그인 요청
			case en_PACKET_CS_CHAT_REQ_LOGIN:
				Packet_Chat_Login(SessionID, Packet);
				break;

			default:
				// 이상한 타입의 패킷이 오면 끊는다.
				m_TypeError++;

				throw CException(_T("Packet_Normal(). TypeError"));
				break;
			}

		}
		catch (CException& exc)
		{
			//// 로그 찍기 (로그 레벨 : 에러)
			//cChatLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
			//	(TCHAR*)exc.GetExceptionText());	

			// 접속 끊기 요청
			Disconnect(SessionID);
		}

	}





	// -------------------------------------
	// '일반 패킷 처리 함수'에서 처리되는 일반 패킷들
	// -------------------------------------

	// 섹터 이동요청 패킷 처리
	//
	// Parameter : SessionID, Packet
	// return : 성공 시 true
	//		  : 접속중이지 않은 유저거나 비정상 섹터 이동 시 false
	void CChatServer::Packet_Sector_Move(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1) map에서 유저 검색
		stPlayer* FindPlayer = FindPlayerFunc(SessionID);
		if (FindPlayer == nullptr)
		{
			printf("Not Find Player!!\n");
			return;
		}

		// 2) 로그인 중인 유저인지 체크
		if(FindPlayer->m_bLoginCheck == false)
			throw CException(_T("Packet_Sector_Move(). Not Login User"));

		// 3) 마샬링
		INT64 AccountNo;
		Packet->GetData((char*)&AccountNo, 8);

		WORD wSectorX, wSectorY;
		Packet->GetData((char*)&wSectorX, 2);
		Packet->GetData((char*)&wSectorY, 2);

		// 패킷 검증 -----------------------------
		// 4) 이동하고자 하는 섹터가 정상인지 체크
		if (wSectorX < 0 || wSectorX > SECTOR_X_COUNT ||
			wSectorY < 0 || wSectorY > SECTOR_Y_COUNT)
		{
			m_SectorPosError++;

			// 비정상이면 밖으로 예외 던진다
			throw CException(_T("Packet_Sector_Move(). Sector pos Error"));
		}

		// 5) AccountNo 체크
		if (AccountNo != FindPlayer->m_i64AccountNo)
		{
			m_SectorNoError++;

			// 비정상이면 밖으로 예외 던진다
			throw CException(_T("Packet_Sector_Move(). AccountNo Error"));
		}		

		// 6) 유저의 섹터 정보 갱신
		// 최초 섹터 패킷이 아니라면, 기존 섹터에서 뺀다.
		// 똑같은 값이기 때문에, 하나만 체크해도 된다.
		if (FindPlayer->m_wSectorY != TEMP_SECTOR_POS &&
			FindPlayer->m_wSectorX != TEMP_SECTOR_POS)
		{
			auto NowSector = &m_vectorSecotr[FindPlayer->m_wSectorY][FindPlayer->m_wSectorX];
			size_t Size = NowSector->size();

			// -- 마지막 요소가 내가 찾고자 하는 유저이거나, 유저가 1명뿐이라면 사이즈를 1 줄인다.
			if (Size == 1 || (*NowSector)[Size - 1] == SessionID)
				NowSector->pop_back();

			// 아니라면 swap 한다
			else
			{
				size_t Index = 0;
				while (Index < Size)
				{
					if ((*NowSector)[Index] == SessionID)
					{
						ULONGLONG Temp = (*NowSector)[Size - 1];
						(*NowSector)[Size - 1] = (*NowSector)[Index];
						(*NowSector)[Index] = Temp;
						NowSector->pop_back();


						break;
					}

					++Index;
				}
			}
		}

		// 색터 갱신
		FindPlayer->m_wSectorX = wSectorX;
		FindPlayer->m_wSectorY = wSectorY;

		// 새로운 색터에 유저 추가
		m_vectorSecotr[wSectorY][wSectorX].push_back(SessionID);

		// 7) 클라이언트에게 보낼 패킷 조립 (섹터 이동 결과)
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		// 타입, AccountNo, SecotrX, SecotrY
		WORD SendType = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
		SendBuff->PutData((char*)&SendType, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&wSectorX, 2);
		SendBuff->PutData((char*)&wSectorY, 2);

		// 8) 클라에게 패킷 보내기(정확히는 NetServer의 샌드버퍼에 넣기)
		SendPacket(SessionID, SendBuff);
	}

	// 채팅 보내기 요청
	//
	// Parameter : SessionID, Packet
	// return : 성공 시 true
	//		  : 접속중이지 않은 유저일 시 false
	void CChatServer::Packet_Chat_Message(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1) map에서 유저 검색
		stPlayer* FindPlayer = FindPlayerFunc(SessionID);
		if (FindPlayer == nullptr)
			m_ChatDump->Crash();

		// 2) 로그인 중인 유저인지 체크
		if (FindPlayer->m_bLoginCheck == false)
			throw CException(_T("Packet_Chat_Message(). Not Login User"));

		// 3) 마샬링
		INT64 AccountNo;
		Packet->GetData((char*)&AccountNo, 8);

		WORD MessageLen;
		Packet->GetData((char*)&MessageLen, 2);

		WCHAR Message[512];
		Packet->GetData((char*)Message, MessageLen);

		// ------------------- 검증
		// 4) AccountNo 체크
		if (AccountNo != FindPlayer->m_i64AccountNo)
		{
			m_ChatNoError++;

			// 비정상이면 밖으로 예외 던진다
			throw CException(_T("Packet_Chat_Message(). AccountNo Error"));
		}

		// 5) 클라이언트에게 보낼 패킷 조립 (채팅 보내기 응답)
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_CHAT_RES_MESSAGE;
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)FindPlayer->m_tLoginID, 40);
		SendBuff->PutData((char*)FindPlayer->m_tNickName, 40);
		SendBuff->PutData((char*)&MessageLen, 2);
		SendBuff->PutData((char*)Message, MessageLen);

		// 6) 주변 유저에게 채팅 메시지 보냄
		// 모든 유저에게 보낸다 (채팅을 보낸 유저 포함)
		SendPacket_Sector(FindPlayer->m_wSectorX, FindPlayer->m_wSectorY, SendBuff);
	}


	// 로그인 요청
	//
	// Parameter : SessionID, CProtocolBuff_Net*
	// return : 없음
	void CChatServer::Packet_Chat_Login(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
	{
		// 1) 플레이어 알아오기
		stPlayer* FindPlayer = FindPlayerFunc(SessionID);
		if (FindPlayer == nullptr)
			m_ChatDump->Crash();

		// 2) 이미 로그인 중인 유저인지 체크
		if (FindPlayer->m_bLoginCheck == true)
			throw CException(_T("Packet_Chat_Message(). Login User!!! Login Fail..."));

		// 3) 마샬링하며 셋팅
		INT64 AccountNo;
		Packet->GetData((char*)&AccountNo, 8);
		FindPlayer->m_i64AccountNo = AccountNo;

		Packet->GetData((char*)FindPlayer->m_tLoginID, 40);
		Packet->GetData((char*)FindPlayer->m_tNickName, 40);

		char Token[64];
		Packet->GetData(Token, 64);

		// 4) 이번 통신에 받은 패킷이 유효한지 체크		
		AcquireSRWLockExclusive(&m_Logn_LanClient->srwl);		// 락 -----

		// 토큰 검색
		auto FindToken = m_Logn_LanClient->m_umapTokenCheck.find(AccountNo);

		// 토큰을 못찾았으면 g_lTokenNotFound 카운트 증가
		if (FindToken == m_Logn_LanClient->m_umapTokenCheck.end())
		{
			InterlockedIncrement(&g_lTokenNotFound);
			ReleaseSRWLockExclusive(&m_Logn_LanClient->srwl);		// 언락 -----

			// 해당 유저 접속 끊기
			Disconnect(SessionID);

			return;
		}

		// 토큰이 같다면, 정상적으로 로그인 처리 됨			
		if (memcmp(FindToken->second->m_cToken, Token, 64) == 0)
		{
			// 기존 토큰 erase	
			Chat_LanClient::stToken* retToken = FindToken->second;
			m_Logn_LanClient->m_umapTokenCheck.erase(FindToken);

			ReleaseSRWLockExclusive(&m_Logn_LanClient->srwl);		// 언락 -----

			// 유저 로그인 상태로 변경
			FindPlayer->m_bLoginCheck = true;

			// stToken* Free
			m_Logn_LanClient->m_MTokenTLS->Free(retToken);
			InterlockedDecrement(&g_lTokenNodeCount);

			// 클라이언트에게 보낼 패킷 조립 (로그인 요청 응답) 
			// 성공패킷
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD SendType = en_PACKET_CS_CHAT_RES_LOGIN;
			SendBuff->PutData((char*)&SendType, 2);

			BYTE Status = 1;
			SendBuff->PutData((char*)&Status, 1);

			SendBuff->PutData((char*)&AccountNo, 8);

			// 로그인 패킷까지 처리된 유저 카운트 1 증가
			InterlockedIncrement(&ChatLoginCount);

			// 클라에게 패킷 보내기(정확히는 NetServer의 샌드버퍼에 넣기)
			SendPacket(SessionID, SendBuff);
		}

		// 토큰이 다르다면 실패패킷 "보내고 끊기".
		// 토큰 자료구조에서 삭제하지 않음. 곧 접속할 유저일 가능성이 높기 때문에.
		else
		{		
			ReleaseSRWLockExclusive(&m_Logn_LanClient->srwl);		// 언락 -----
			
			// g_lTokenMiss 카운트 증가
			InterlockedIncrement(&g_lTokenMiss);

			// 클라이언트에게 보낼 패킷 조립 (로그인 요청 응답) 
			// 실패패킷
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD SendType = en_PACKET_CS_CHAT_RES_LOGIN;
			SendBuff->PutData((char*)&SendType, 2);

			BYTE Status = 0;
			SendBuff->PutData((char*)&Status, 1);

			SendBuff->PutData((char*)&AccountNo, 8);

			// 클라에게 패킷 보내기(정확히는 NetServer의 샌드버퍼에 넣기)
			// 보내고 끊기
			SendPacket(SessionID, SendBuff, TRUE);			
		}

	}



	// ------------------------------------------------
	// 생성자와 소멸자
	// ------------------------------------------------

	// 생성자
	CChatServer::CChatServer()
		:CNetServer()
	{
		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			m_ChatDump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		cChatLibLog->SetDirectory(L"ChatServer");
		cChatLibLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);


		// ------------------- 각종 리소스 할당
		// 로그인 서버와 통신하기 위한 LanClient 동적할당
		m_Logn_LanClient = new Chat_LanClient;

		// 모니터링 서버와 통신하기 위한 LanClient 동적할당
		m_Monitor_LanClient = new Chat_MonitorClient;
		m_Monitor_LanClient->ParentSet(this);		

		// 플레이어를 관리하는 umap의 용량을 할당해둔다.
		m_mapPlayer.reserve(m_stConfig.MaxJoinUser);

		// 플레이어 최근 패킷 시간을 관리하는 umap의 용량을 할당해둔다.
		m_mapLastPacketTime.reserve(m_stConfig.MaxJoinUser);

		// 일감 TLS 메모리풀 동적할당
		m_MessagePool = new CMemoryPoolTLS<st_WorkNode>(0, false);

		// 플레이어 구조체 TLS 메모리풀 동적할당	
		m_PlayerPool = new CMemoryPoolTLS<stPlayer>(0, false);

		// 락프리 큐 동적할당 (네트워크가 컨텐츠에게 일감 던지는 큐)
		// 사이즈가 0인 이유는, UpdateThread에서 큐가 비었는지 체크하고 쉬러 가야하기 때문에.
		m_LFQueue = new CLF_Queue<st_WorkNode*>(0);

		// 업데이트 스레드 깨우기 용도 Event, 업데이트 스레드 종료 용도 Event
		// 
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		UpdateThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		UpdateThreadEXITEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		JobThreadEXITEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		// 브로드 캐스트 시, 어느 섹터에 보내야하는지 미리 다 만들어둔다.
		for (int Y = 0; Y < SECTOR_Y_COUNT; ++Y)
		{
			for (int X = 0; X < SECTOR_X_COUNT; ++X)
			{
				m_stSectorSaver[Y][X] = new st_SecotrSaver;
				SecotrSave(X, Y, m_stSectorSaver[Y][X]);
			}
		}

		// 섹터 vector의 capacity를 미리 할당, 차후 요소 추가로 인한 복사생성자 호출을 막는다.
		// 각 vector마다 100의 capaticy 할당
		for (int Y = 0; Y < SECTOR_Y_COUNT; ++Y)
		{
			for (int X = 0; X < SECTOR_X_COUNT; ++X)
			{
				m_vectorSecotr[Y][X].reserve(100);
			}
		}

		// 락 초기화
		InitializeSRWLock(&m_LastPacketsrwl);

		// 시간
		timeBeginPeriod(1);
	}

	// 소멸자
	CChatServer::~CChatServer()
	{
		// 로그인 서버와 통신하는 LanClient 동적해제
		delete m_Logn_LanClient;

		// 모니터링 서버와 통신하는 LanClient 동적해제
		delete m_Monitor_LanClient;

		// 일감 큐 동적해제.
		delete m_LFQueue;

		// 구조체 메시지 TLS 메모리 풀 동적해제
		delete m_MessagePool;

		// 플레이어 구조체 TLS 메모리풀 동적해제
		delete m_PlayerPool;

		// 업데이트 스레드 깨우기용 이벤트 해제
		CloseHandle(UpdateThreadEvent);

		// 업데이트 스레드 종료용 이벤트 해제
		CloseHandle(UpdateThreadEXITEvent);

		// 잡 스레드 종료용 이벤트 해제
		CloseHandle(JobThreadEXITEvent);


		// 브로드캐스트용 섹터 모두 해제
		for (int Y = 0; Y < SECTOR_Y_COUNT; ++Y)
		{
			for (int X = 0; X < SECTOR_X_COUNT; ++X)
			{
				delete m_stSectorSaver[Y][X];
			}
		}

		// 시간 
		timeEndPeriod(1);
	}



	// -------------------------------------
	// 외부에서 사용 가능한 함수
	// -------------------------------------
	
	// 출력용 함수
	void  CChatServer::ShowPrintf()
	{
		// 해당 프로세스의 사용량 체크할 클래스
		static CCpuUsage_Process ProcessUsage;

		// 화면 출력할 것 셋팅
		/*
		MonitorConnect : %d, LoginConnect : %d	- 모니터링 서버에 접속 여부, 로그인 서버에 접속 여부. 1이면 접속함.
		SessionNum : 	- NetServer 의 세션수
		PacketPool_Net : 	- 외부에서 사용 중인 Net 직렬화 버퍼의 수

		UpdateMessage_Pool :	- UpdateThread 용 구조체 할당량 (일감)
		UpdateMessage_Queue : 	- UpdateThread 큐 남은 개수

		PlayerData_Pool :	- Player 구조체 할당량
		Player Count : 		- Contents 파트 Player 개수

		Accept Total :		- Accept 전체 카운트 (accept 리턴시 +1)
		Accept TPS :		- Accept 처리 횟수
		Update TPS :		- Update 처리 초당 횟수
		Send TPS			- 초당 Send완료 횟수. (완료통지에서 증가)

		SecotrPosError :	- 섹터 위치 에러
		SectorAccountError : - 섹터 패킷의 AccountNo 에러
		ChatAccountError :	- 채팅 패킷의 AccountNo 에러
		TypeError :			- 페이로드의 메시지 타입 에러
		HeadCodeError :		- (네트워크) 헤더 코드 에러
		CheckSumError :		- (네트워크) 체크썸 에러
		HeaderLenBig :		- (네트워크) 헤더의 Len사이즈가 비정상적으로 큼.

		Net_BuffChunkAlloc_Count : - Net 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		Chat_MessageChunkAlloc_Count : - 일감 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		Chat_MessageChunkAlloc_Count : - 플레이어 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)

		TokenMiss : 		- 토큰키가 다른 유저가 채팅서버로 들어옴
		TokenNotFound : 	- 토큰키를 찾지 못함
		TokenEraseCount :	- JobThread에서 토큰을 삭제할 시 1씩 증가하는 카운트.

		----------------------------------------------------
		PacketPool_Lan : 	- 외부에서 사용 중인 Lan 직렬화 버퍼의 수

		Token_umap_Count - 토큰 umap 안의 카운트
		Token_UseNode_Count - 토큰 TLS의 사용 중인 Node 카운트

		Lan_BuffChunkAlloc_Count : - Lan 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		Token_ChunkAlloc_Count : - 토큰 TLS의 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)

		----------------------------------------------------
		CPU usage [ChatServer:%.1f%% U:%.1f%% K:%.1f%%] - 프로세스 사용량.

		*/

		LONG UpdateTPS = InterlockedExchange(&g_lUpdateTPS, 0);
		LONG AccpetTPS = InterlockedExchange(&g_lAcceptTPS, 0);
		LONG SendTPS = InterlockedExchange(&g_lSendPostTPS, 0);

		// 출력 전에, 프로세스 사용량 갱신
		ProcessUsage.UpdateCpuTime();

		printf("========================================================\n"
			"MonitorConnect : %d, LoginConnect : %d\n"
			"SessionNum : %lld\n"
			"PacketPool_Net : %d\n\n"

			"UpdateMessage_Pool : %d\n"
			"UpdateMessage_Queue : %d\n\n"

			"PlayerData_Pool : %d\n"
			"Player Count : %lld\n\n"

			"Accept Total : %lld\n"
			"Accept TPS : %d\n"
			"Update TPS : %d\n"
			"Send TPS : %d\n\n"

			"SecotrPosError : %d\n"
			"SectorAccountError : %d\n"
			"ChatAccountError : %d\n"
			"TypeError : %d\n"
			"HeadCodeError : %d\n"
			"CheckSumError : %d\n"
			"HeaderLenBig : %d\n\n"

			"Net_BuffChunkAlloc_Count : %d (Out : %d)\n"
			"Chat_MessageChunkAlloc_Count : %d (Out : %d)\n"
			"Chat_PlayerChunkAlloc_Count : %d (Out : %d)\n\n"

			"TokenMiss : %d\n"
			"TokenNotFound : %d\n"
			"TokenEraseCount : %d\n\n"

			"------------------------------------------------\n"
			"PacketPool_Lan : %d\n\n"

			"Token_umap_Count : %lld\n"
			"Token_UseNode_Count : %d\n\n"

			"Lan_BuffChunkAlloc_Count : %d (Out : %d)\n"
			"Token_ChunkAlloc_Count : %d (Out : %d)\n\n"

			"========================================================\n\n"
			"CPU usage [ChatServer:%.1f%% U:%.1f%% K:%.1f%%]\n",

			// ----------- 채팅 서버용
			m_Monitor_LanClient->GetClinetState(), m_Logn_LanClient->GetClinetState(),
			GetClientCount(), g_lAllocNodeCount,
			g_lUpdateStructCount, m_LFQueue->GetInNode(),
			g_lUpdateStruct_PlayerCount, m_mapPlayer.size(),
			g_ullAcceptTotal, AccpetTPS, UpdateTPS, SendTPS,
			m_SectorPosError, m_SectorNoError, m_ChatNoError, m_TypeError, m_HeadCodeError, m_ChackSumError, m_HeaderLenBig,
			CProtocolBuff_Net::GetChunkCount(), CProtocolBuff_Net::GetOutChunkCount(),
			m_MessagePool->GetAllocChunkCount(), m_MessagePool->GetOutChunkCount(),
			m_PlayerPool->GetAllocChunkCount(), m_PlayerPool->GetOutChunkCount(),
			g_lTokenMiss, g_lTokenNotFound, g_lJobThreadTokenEraseCount,

			// ----------- 랜 클라이언트용
			g_lAllocNodeCount_Lan,
			m_Logn_LanClient->m_umapTokenCheck.size(), g_lTokenNodeCount,
			CProtocolBuff_Lan::GetChunkCount(), CProtocolBuff_Lan::GetOutChunkCount(),
			m_Logn_LanClient->m_MTokenTLS->GetAllocChunkCount(), m_Logn_LanClient->m_MTokenTLS->GetOutChunkCount(),

			// ----------- 프로세스 사용량 
			ProcessUsage.ProcessTotal(), ProcessUsage.ProcessUser(), ProcessUsage.ProcessKernel());
		
	}

	// 채팅 서버 시작 함수
	// 내부적으로 NetServer의 Start도 같이 호출
	//
	// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
	// return true : 성공
	bool CChatServer::ServerStart()
	{
		// ------------------- 잡 스레드 생성
		hJobThraed = (HANDLE)_beginthreadex(NULL, 0, JobThread, this, 0, 0);

		// ------------------- 업데이트 스레드 생성
		hUpdateThraed = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, 0);

		// ------------------- 넷서버 가동
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept, m_stConfig.Nodelay, m_stConfig.MaxJoinUser,
			m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
			return false;

		// ------------------- 로그인 서버와 연결되는, 랜 클라이언트 가동
		if(m_Logn_LanClient->Start(m_stConfig.LoginServerIP, m_stConfig.LoginServerPort, m_stConfig.Client_CreateWorker, m_stConfig.Client_ActiveWorker, m_stConfig.Client_Nodelay) == false)
			return false;

		// ------------------- 모니터링 서버와 연결되는, 랜 클라이언트 가동
		if (m_Monitor_LanClient->ClientStart(m_stConfig.MonitorServerIP, m_stConfig.MonitorServerPort, m_stConfig.MonitorClientCreateWorker, 
			m_stConfig.MonitorClientActiveWorker, m_stConfig.MonitorClientNodelay) == false)
			return false;
		

		// 서버 오픈 로그 찍기		
		cChatLibLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");
		
		return true;
	}

	// 채팅 서버 종료 함수
	//
	// Parameter : 없음
	// return : 없음
	void CChatServer::ServerStop()
	{
		// 모니터링 클라 종료
		if (m_Monitor_LanClient->GetClinetState() == true)
			m_Monitor_LanClient->Stop();


		// 넷서버 스탑 (엑셉트, 워커 종료)
		Stop();

		// 잡 스레드 종료 신호
		SetEvent(CloseHandle);

		// 잡 스레드 종료 대기
		if (WaitForSingleObject(CloseHandle, INFINITE) == WAIT_FAILED)
		{
			DWORD Error = GetLastError();
			printf("JobThread Exit Error!!! (%d) \n", Error);
		}

		// 업데이트 스레드 종료 신호
		SetEvent(UpdateThreadEXITEvent);

		// 업데이트 스레드 종료 대기
		if (WaitForSingleObject(hUpdateThraed, INFINITE) == WAIT_FAILED)
		{
			DWORD Error = GetLastError();
			printf("UpdateThread Exit Error!!! (%d) \n", Error);
		}

		// ------------- 디비 저장
		// 현재 없음.


		// ------------- 리소스 초기화
		// 큐 내의 모든 노드, Free
		st_WorkNode* FreeNode;
		if (m_LFQueue->GetInNode() != 0)
		{
			if (m_LFQueue->Dequeue(FreeNode) == -1)
				m_ChatDump->Crash();

			m_MessagePool->Free(FreeNode);
		}

		// 섹터 list에서 모든 유저 제거
		for (int y = 0; y < SECTOR_Y_COUNT; ++y)
		{
			for (int x = 0; x < SECTOR_X_COUNT; ++x)
			{
				m_vectorSecotr[y][x].clear();
			}
		}

		// Playermap에 있는 모든 유저 반환
		auto itor = m_mapPlayer.begin();

		while (itor != m_mapPlayer.end())
		{
			// 메모리풀에 반환
			m_PlayerPool->Free(itor->second);
		}

		// umap 초기화
		m_mapPlayer.clear();

		// 업데이트 스레드 핸들 반환
		CloseHandle(hUpdateThraed);

		// 잡 스레드 핸들 반환
		CloseHandle(hJobThraed);	

		// 서버 종료 로그 찍기		
		cChatLibLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerStop...");
	}


	// ------------------------------------------------
	// 상속받은 virtual 함수
	// ------------------------------------------------

	bool CChatServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		// 만약, IP와 Port가 이상한 곳(예를들어 해외?)면 false 리턴.
		// false가 리턴되면 접속이 안된다.

		return true;
	}


	void CChatServer::OnClientJoin(ULONGLONG SessionID)
	{
		// 호출 시점 : 유저가 서버에 정상적으로 접속되었을 시
		// 호출 위치 : NetServer의 워커스레드
		// 하는 행동 : 컨텐츠가 들고있는 큐에, 유저 접속 메시지를 넣는다.

		// 1. 일감 Alloc

		st_WorkNode* NowMessage = m_MessagePool->Alloc();

		// 2. 패킷 시간 갱신(접속만 하고 로그인 패킷을 안보내는 유저도 있을 수 있기때문에. 걸러내기 용도)
		InsertLastTime(SessionID);

		InterlockedIncrement(&g_lUpdateStructCount);

		// 2. 타입 넣기
		NowMessage->m_wType = TYPE_JOIN;

		// 3.  세션 ID 채우기
		NowMessage->m_ullSessionID = SessionID;

		// 4. 메시지를 큐에 넣는다.
		m_LFQueue->Enqueue(NowMessage);

		// 5. 자고있는 Update스레드를 깨운다.
		SetEvent(UpdateThreadEvent);

	}


	void CChatServer::OnClientLeave(ULONGLONG SessionID)
	{
		// 호출 시점 : 유저가 서버에서 나갈 시
		// 호출 위치 : NetServer의 워커스레드
		// 하는 행동 : 컨텐츠가 들고있는 큐에, 유저 종료 메시지를 넣는다.

		// 1. 일감 Alloc
		st_WorkNode* NowMessage = m_MessagePool->Alloc();

		InterlockedIncrement(&g_lUpdateStructCount);

		// 2. Type채우기
		// 여기 타입은 [접속, 종료, 패킷] 총 3 개 중 하나이다.
		// 여기선 무조건 종료
		NowMessage->m_wType = TYPE_LEAVE;

		// 3. 세션 ID 채우기
		NowMessage->m_ullSessionID = SessionID;

		// 4. 메시지를 큐에 넣는다.
		m_LFQueue->Enqueue(NowMessage);

		// 5. 자고있는 Update스레드를 깨운다.
		SetEvent(UpdateThreadEvent);
	}


	void CChatServer::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		// 호출 시점 : 유저에게 패킷을 받을 시
		// 호출 위치 : NetServer의 워커스레드
		// 하는 행동 : 컨텐츠가 들고있는 메시지 큐에, 데이터를 넣는다.		

		// 1. 일감 Alloc
		st_WorkNode* NowMessage = m_MessagePool->Alloc();

		// 2. 마지막 패킷 시간 관리 자료구조에 유저 추가
		// 이미 있는 유저는 시간 갱신.
		InsertLastTime(SessionID);

		InterlockedIncrement(&g_lUpdateStructCount);

		// 2. Type채우기
		// 여기 타입은 [접속, 종료, 패킷] 총 3 개 중 하나이다.
		// 여기선 무조건 패킷.
		NowMessage->m_wType = TYPE_PACKET;

		// 3. 세션 ID 채우기
		NowMessage->m_ullSessionID = SessionID;

		// 4. 패킷 넣기
		Payload->Add();
		NowMessage->m_pPacket = Payload;

		// 5. 일감을 큐에 넣는다.
		m_LFQueue->Enqueue(NowMessage);

		// 6. 자고있는 Update스레드를 깨운다.
		SetEvent(UpdateThreadEvent);
	}


	void CChatServer::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	void CChatServer::OnWorkerThreadBegin()
	{}

	void CChatServer::OnWorkerThreadEnd()
	{}

	void CChatServer::OnError(int error, const TCHAR* errorStr)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		/*cChatLibLog->LogSave(L"ChatServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
			errorStr, error);*/

		// 에러 코드에 따라 로직 처리
		switch (error)
		{
		case (int)CNetServer::euError::NETWORK_LIB_ERROR__IOCP_ERROR:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__NOT_FIND_CLINET:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__WSASEND_FAIL:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__A_THREAD_IOCPCONNECT_FAIL:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__WFSO_ERROR:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__IOCP_IO_FAIL:
			break;

		case (int)CNetServer::euError::NETWORK_LIB_ERROR__JOIN_USER_FULL:
			break;

			// (네트워크) 헤더 코드 에러
		case (int)CNetServer::euError::NETWORK_LIB_ERROR__RECV_CODE_ERROR:
			m_HeadCodeError++;
			break;

			// (네트워크) 체크썸 에러
		case (int)CNetServer::euError::NETWORK_LIB_ERROR__RECV_CHECKSUM_ERROR:
			m_ChackSumError++;
			break;

			// (네트워크) 헤더의 Len사이즈가 비정상적으로 큼.
		case (int)CNetServer::euError::NETWORK_LIB_ERROR__RECV_LENBIG_ERROR:
			m_HeaderLenBig++;
			break;

		default:
			break;
		}
	}
}



// ---------------------------------------------
// 
// LoginServer의 LanServer와 통신하는 LanClient
// 
// ---------------------------------------------
namespace Library_Jingyu
{
	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	Chat_LanClient::Chat_LanClient()
		:CLanClient()
	{
		// 토큰을 관리하는 umap의 용량을 할당해둔다.
		m_umapTokenCheck.reserve(10000);

		// 토근 구조체 관리 TLS 동적할당
		m_MTokenTLS = new CMemoryPoolTLS< stToken >(0, false);

		// 락 초기화
		InitializeSRWLock(&srwl);
	}

	Chat_LanClient::~Chat_LanClient()
	{
		// 아직 연결이 되어있으면, 연결 해제
		if (GetClinetState() == true)
			Stop();

		// 토근 구조체 관리 TLS 동적해재
		delete m_MTokenTLS;
	}




	// -----------------------
	// 패킷 처리 함수
	// -----------------------

	// 새로운 유저가 로그인 서버로 들어올 시, 로그인 서버로부터 토큰키를 받는다.
	// 이 토큰키를 저장한 후 응답을 보내는 함수
	void Chat_LanClient::NewUserJoin(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. AccountNo 빼온다.
		INT64 AccountNo;
		Payload->GetData((char*)&AccountNo, 8);

		// 2. 토큰키 빼온다.	
		char Token[64];
		Payload->GetData(Token, 64);

		// 3. 파라미터 빼온다.
		INT64 Parameter;
		Payload->GetData((char*)&Parameter, 8);

		// 5. 자료구조에 토큰 저장
		InsertTokenFunc(AccountNo, Token);


		// 응답패킷 제작 후 보내기 -------------------

		// 1. 직렬화버퍼 Alloc
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		// 2. 타입 셋팅
		WORD Type = en_PACKET_SS_RES_NEW_CLIENT_LOGIN;

		// 3. 전송 데이터를 SendBUff에 넣는다
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&Parameter, 8);

		// 4. Send 한다.
		SendPacket(SessionID, SendBuff);
	}




	// -----------------------
	// 기능 함수
	// -----------------------

	// 토큰 관리 자료구조에, 새로 접속한 토큰 추가
	// 현재 umap으로 관리중
	// 
	// Parameter : AccountNo, Token(char*)
	// 없음
	void Chat_LanClient::InsertTokenFunc(INT64 AccountNo, char* Token)
	{
		// 1. 존재하는 유저인지 체크
		AcquireSRWLockExclusive(&srwl);		// ---------------- Lock
		auto Find = m_umapTokenCheck.find(AccountNo);
		
		// 이미 존재하는 유저라면, 값 교체만 한다.
		// !! 검색에 성공했으면 Find가 end가 아니다. !!
		if (Find != m_umapTokenCheck.end())
		{
			Find->second->m_ullInsertTime = GetTickCount64();
			memcpy_s(Find->second->m_cToken, 64, Token, 64);
		}

		// 없는 유저라면, stToken Alloc 후 추가한다.
		else
		{
			// 토큰 TLS에서 Alloc
			// !! 토큰 TLS  Free는, ChatServer에서, 실제 유저가 나갔을 때(OnClientLeave)에서 한다 !!
			stToken* NewToken = m_MTokenTLS->Alloc();

			NewToken->m_ullInsertTime = GetTickCount64();
			memcpy_s(NewToken->m_cToken, 64, Token, 64);

			InterlockedIncrement(&g_lTokenNodeCount);

			m_umapTokenCheck.insert(make_pair(AccountNo, NewToken));
		}

		ReleaseSRWLockExclusive(&srwl);		// ---------------- Unock		
	}


	// 토큰 관리 자료구조에서, 토큰 검색
	// 현재 umap으로 관리중
	// 
	// Parameter : AccountNo
	// return : 검색 성공 시, stToken*
	//		  : 검색 실패 시 nullptr
	Chat_LanClient::stToken* Chat_LanClient::FindTokenFunc(INT64 AccountNo)
	{
		AcquireSRWLockShared(&srwl);	// ---------------- Shared Lock
		auto FindToken = m_umapTokenCheck.find(AccountNo);
		ReleaseSRWLockShared(&srwl);	// ---------------- Shared UnLock

		if (FindToken == m_umapTokenCheck.end())
			return nullptr;

		return FindToken->second;
	}


	// 토큰 관리 자료구조에서, 토큰 제거
	// 현재 umap으로 관리중
	// 
	// Parameter : AccountNo
	// return : 성공 시, 제거된 토큰 stToken*
	//		  : 검색 실패 시 nullptr
	Chat_LanClient::stToken* Chat_LanClient::EraseTokenFunc(INT64 AccountNo)
	{
		// 1) map에서 유저 검색

		// erase까지가 한 작업이기 때문에, Exclusive 락 사용.
		AcquireSRWLockExclusive(&srwl);		// ---------------- Lock

		auto FindToken = m_umapTokenCheck.find(AccountNo);
		if (FindToken == m_umapTokenCheck.end())
		{
			ReleaseSRWLockExclusive(&srwl);	// ---------------- Unlock
			return nullptr;
		}

		stToken* ret = FindToken->second;

		// 2) 맵에서 제거
		m_umapTokenCheck.erase(FindToken);

		ReleaseSRWLockExclusive(&srwl);	// ---------------- Unlock

		return ret;
	}





	// -----------------------
	// 순수 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void Chat_LanClient::OnConnect(ULONGLONG SessionID)
	{
		// 로그인 서버로, 로그인 패킷 보냄.

		// 1. 타입 셋팅
		WORD Type = en_PACKET_SS_LOGINSERVER_LOGIN;

		// 2. 로그인에게 알려줄, 접속하는 서버의 타입
		BYTE ServerType = en_PACKET_SS_LOGINSERVER_LOGIN::dfSERVER_TYPE_CHAT;

		// 3. 서버 이름 셋팅. 아무거나
		WCHAR ServerName[32] = L"Chat_LanClient";

		// 4. 직렬화 버퍼를 Alloc받아서 셋팅
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerType, 1);
		SendBuff->PutData((char*)&ServerName, 64);

		// 5. Send한다.
		SendPacket(SessionID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void Chat_LanClient::OnDisconnect(ULONGLONG SessionID)
	{
		// 지금 하는거 없음. 
		// 연결이 끊기면, LanClient에서 이미 연결 다시 시도함.
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void Chat_LanClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 토큰키를 관리하는 자료구조에 토큰 삽입

		// 1. Type 얻어옴
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 2. 타입에 따라 로직 처리
		switch (Type)
		{
			// 새로운 유저가 접속했음 (로그인 서버로부터 받음)
		case en_PACKET_SS_REQ_NEW_CLIENT_LOGIN:
			NewUserJoin(SessionID, Payload);
			break;


		default:
			break;
		}
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void Chat_LanClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{
		// 현재 할거 없음
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void Chat_LanClient::OnWorkerThreadBegin()
	{
		// 현재 할거 없음
	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void Chat_LanClient::OnWorkerThreadEnd()
	{
		// 현재 할거 없음
	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void Chat_LanClient::OnError(int error, const TCHAR* errorStr)
	{
		// 현재 할거 없음
	}
}



// ---------------------------------------------
// 
// 모니터링 LanClient
// 
// ---------------------------------------------
namespace Library_Jingyu
{

	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	Chat_MonitorClient::Chat_MonitorClient()
		:CLanClient()
	{
		// 모니터링 서버 정보전송 스레드를 종료시킬 이벤트 생성
		//
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		m_hMonitorThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	}

	Chat_MonitorClient::~Chat_MonitorClient()
	{
		// 아직 연결이 되어있으면, 연결 해제
		if (GetClinetState() == true)
			ClientStop();

		// 이벤트 삭제
		CloseHandle(m_hMonitorThreadExitEvent);
	}


	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 시작 함수
	// 내부적으로, 상속받은 CLanClient의 Start호출.
	//
	// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
	// return : 성공 시 true , 실패 시 falsel 
	bool Chat_MonitorClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 모니터링 서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
			return false;

		// 모니터링 서버로 정보 전송할 스레드 생성
		m_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);

		return true;
	}

	// 종료 함수
	// 내부적으로, 상속받은 CLanClient의 Stop호출.
	// 추가로, 리소스 해제 등
	//
	// Parameter : 없음
	// return : 없음
	void Chat_MonitorClient::ClientStop()
	{
		// 1. 모니터링 서버 정보전송 스레드 종료
		SetEvent(m_hMonitorThreadExitEvent);

		// 종료 대기
		if (WaitForSingleObject(m_hMonitorThread, INFINITE) == WAIT_FAILED)
		{
			DWORD Error = GetLastError();
			printf("MonitorThread Exit Error!!! (%d) \n", Error);
		}

		// 2. 스레드 핸들 반환
		CloseHandle(m_hMonitorThread);

		// 3. 모니터링 서버와 연결 종료
		Stop();
	}

	// 채팅서버의 this를 입력받는 함수
	// 
	// Parameter : 쳇 서버의 this
	// return : 없음
	void Chat_MonitorClient::ParentSet(CChatServer* ChatThis)
	{
		m_ChatServer_this = ChatThis;
	}




	// -----------------------
	// 내부에서만 사용하는 기능 함수
	// -----------------------

	// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
	UINT	WINAPI Chat_MonitorClient::MonitorThread(LPVOID lParam)
	{
		// this 받아두기
		Chat_MonitorClient* g_This = (Chat_MonitorClient*)lParam;

		// 종료 신호 이벤트 받아두기
		HANDLE hEvent = g_This->m_hMonitorThreadExitEvent;

		// CPU 사용율 체크 클래스 (채팅서버 소프트웨어)
		CCpuUsage_Process CProcessCPU;

		// PDH용 클래스
		CPDH	CPdh;

		while (1)
		{
			// 1초에 한번 깨어나서 정보를 보낸다.
			DWORD Check = WaitForSingleObject(hEvent, 1000);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("MoniterThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 만약, 종료 신호가 왔다면 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;


			// 그게 아니라면, 일을 한다.

			// 프로세서 CPU 사용율, PDH 정보 갱신
			CProcessCPU.UpdateCpuTime();
			CPdh.SetInfo();

			// 채팅서버가 On일 경우, 패킷을 보낸다.
			if (g_This->m_ChatServer_this->GetServerState() == true)
			{
				// 타임스탬프 구하기
				int TimeStamp = (int)(time(NULL));

				// 1. 채팅서버 ON		
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_SERVER_ON, TRUE, TimeStamp);

				// 2. 채팅서버 CPU 사용률 (커널 + 유저)
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_CPU, (int)CProcessCPU.ProcessTotal(), TimeStamp);

				// 3. 채팅서버 메모리 유저 커밋 사용량 (Private) MByte
				int Data = (int)(CPdh.Get_UserCommit() / 1024 / 1024);
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT, Data, TimeStamp);

				// 4. 채팅서버 패킷풀 사용량
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, g_lAllocNodeCount + g_lAllocNodeCount_Lan, TimeStamp);
				
				// 5. 채팅서버 접속 세션전체
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_SESSION, (int)g_This->m_ChatServer_this->GetClientCount(), TimeStamp);

				// 6. 채팅서버 로그인을 성공한 전체 인원				
				g_This->InfoSend(dfMONITOR_DATA_TYPE_CHAT_PLAYER, ChatLoginCount, TimeStamp);

			}

		}

		return 0;
	}

	// 모니터링 서버로 데이터 전송
	//
	// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
	// return : 없음
	void Chat_MonitorClient::InfoSend(BYTE DataType, int DataValue, int TimeStamp)
	{
		WORD Type = en_PACKET_SS_MONITOR_DATA_UPDATE;

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&DataType, 1);
		SendBuff->PutData((char*)&DataValue, 4);
		SendBuff->PutData((char*)&TimeStamp, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}



	// -----------------------
	// 순수 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void Chat_MonitorClient::OnConnect(ULONGLONG SessionID)
	{
		m_ullSessionID = SessionID;

		// 모니터링 서버(Lan)로 로그인 패킷 보냄
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_SS_MONITOR_LOGIN;
		int ServerNo = dfSERVER_NO;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerNo, 4);

		SendPacket(SessionID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void Chat_MonitorClient::OnDisconnect(ULONGLONG SessionID)
	{}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, CProtocolBuff_Lan*
	// return : 없음
	void Chat_MonitorClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void Chat_MonitorClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void Chat_MonitorClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void Chat_MonitorClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void Chat_MonitorClient::OnError(int error, const TCHAR* errorStr)
	{}
}