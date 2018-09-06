#include "pch.h"
#include "ChatServer.h"

#include "CommonProtocol.h"

#include <strsafe.h>

#include <time.h>
#include <process.h>
#include <Log\Log.h>

#include <map>

using namespace std;


// 패킷의 타입
#define TYPE_JOIN	0
#define TYPE_LEAVE	1
#define TYPE_PACKET	2

// 최초 입장 시, 임의로 셋팅해두는 섹터 X,Y 값
#define TEMP_SECTOR_POS	12345

extern ULONGLONG g_ullUpdateStructCount;
extern ULONGLONG g_ullUpdateStruct_PlayerCount;
extern LONG		 g_lUpdateTPS;

// 로그 찍을 전역변수 하나 받기.
CSystemLog* cChatLibLog = CSystemLog::GetInstance();


// ------------- 공격 테스트용
extern int m_SectorPosError;

extern int m_SizeError;

extern int m_SectorNoError;
extern int m_ChatNoError;

extern int m_TypeError;

extern int m_HeadCodeError;

extern int m_ChackSumError;
extern int m_NotPayloadDeq;
extern int m_HeaderLenBig;
extern int m_PayloadQueError;


// -------------------------------------
// 클래스 내부에서 사용하는 함수
// -------------------------------------

// 파일에서 Config 정보 읽어오기
// 
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
			printf("FileR ead Fail...\n");
			return false;
		}
	}

	// 구역 지정
	if (Parser.AreaCheck(_T("CHATSERVER")) == false)
		return false;

	// ------------ 읽어오기
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

// 섹터 기준 9방향 구하기
// 인자로 받은 X,Y를 기준으로, 인자로 받은 구조체에 9방향을 넣어준다.
//
// Parameter : 기준 SectorX, 기준 SectorY, (out)&stSectorCheck
// return : 없음
void CChatServer::GetSector(int SectorX, int SectorY, stSectorCheck* Sector)
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

// 인자로 받은 9개 섹터의 모든 유저(서버에 패킷을 보낸 클라 포함)에게 SendPacket 호출
//
// parameter : 보낼 버퍼, 섹터 9개
// return : 없음
void CChatServer::SendPacket_Sector(CProtocolBuff_Net* SendBuff, stSectorCheck* Sector)
{
	// 1. Sector안의 유저들에게 인자로 받은 패킷을 넣는다.
	DWORD dwCount = Sector->m_dwCount;
	for (DWORD i = 0; i < dwCount; ++i)
	{
		// 2. itor는 "i" 번째 섹터의 리스트를 가리킨다.
		auto itor = m_listSecotr[Sector->m_Sector[i].y][Sector->m_Sector[i].x].begin();
		auto Enditor = m_listSecotr[Sector->m_Sector[i].y][Sector->m_Sector[i].x].end();

		for (; itor != Enditor; ++itor)
		{
			// !! Sendpacket전에, 카운트 하나 증가 !!
			// NetServer쪽에서, 완료 통지가 오면 Free를 하기 때문에 Add해야 한다.
			SendBuff->Add();

			// SendPacket
			SendPacket((*itor)->m_ullSessionID , SendBuff);

		}
	}


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

		// ----------------- 일감이 있는지 확인. 일감 있으면 일한다.
		while (g_this->m_LFQueue->GetInNode() > 0)
		{
			// 1. 큐에서 일감 1개 빼오기	
			st_WorkNode* NowWork;

			if (g_this->m_LFQueue->Dequeue(NowWork) == -1)
				g_this->m_ChatDump->Crash();

			// 2. Type에 따라 로직 처리
			// 분기 예측 성공률을 높이기 위해, 가장 많이 호출될 법 한 것부터 switch case문 상단에 배치.
			switch (NowWork->m_wType)
			{
				// 패킷 처리
			case TYPE_PACKET:
				g_this->Packet_Normal(NowWork->m_ullSessionID, NowWork->m_pPacket);

				// 패킷 Free
				CProtocolBuff_Net::Free(NowWork->m_pPacket);
				break;

				// 접속 처리
			case TYPE_JOIN:
				g_this->Packet_Join(NowWork->m_ullSessionID);
				break;

				// 종료 처리
			case TYPE_LEAVE:
				g_this->Packet_Leave(NowWork->m_ullSessionID);
				break;

			default:
				break;
			}

			InterlockedIncrement(&g_lUpdateTPS);  // 테스트용!!

			// 3. 일감 Free
			g_this->m_MessagePool->Free(NowWork);
			InterlockedDecrement(&g_ullUpdateStructCount);

		}
		
	}

	printf("UpdateThread Exit!!\n");

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

	InterlockedIncrement(&g_ullUpdateStruct_PlayerCount);

	// 2) SessionID 셋팅
	JoinPlayer->m_ullSessionID = SessionID;	

	JoinPlayer->m_wSectorY = TEMP_SECTOR_POS;
	JoinPlayer->m_wSectorX = TEMP_SECTOR_POS;	

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
	{
		printf("Not Find Player!!\n");
		m_ChatDump->Crash();
	}	

	// 2) 최초 할당이 아닐 경우, 섹터에서 제거
	if (ErasePlayer->m_wSectorY != TEMP_SECTOR_POS &&
		ErasePlayer->m_wSectorX != TEMP_SECTOR_POS)
	{
		m_listSecotr[ErasePlayer->m_wSectorY][ErasePlayer->m_wSectorX].remove(ErasePlayer);
	}

	// 3) 초기화 ------------------
	// SectorPos를 초기 위치로 설정
	ErasePlayer->m_wSectorY = TEMP_SECTOR_POS;
	ErasePlayer->m_wSectorX = TEMP_SECTOR_POS;

	// 4) Player Free()
	m_PlayerPool->Free(ErasePlayer);

	InterlockedDecrement(&g_ullUpdateStruct_PlayerCount);
}

// 일반 패킷처리 함수
// 
// Parameter : SessionID, CProtocolBuff_Net*
// return : 없음
void CChatServer::Packet_Normal(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
{
	// 1. 패킷 타입 확인
	WORD Type;
	*Packet >> Type;
	
	// 2. 타입에 따라 switch case
	// 분기 예측 성공률 증가를 위해 가장 많이 올 법한 일감을 상단에 배치 (현재는 채팅 메시지)
	try
	{
		switch (Type)
		{
			// 채팅서버 채팅보내기 요청
		case en_PACKET_CS_CHAT_REQ_MESSAGE:
			Packet_Chat_Message(SessionID, Packet);

			break;

			// 채팅서버 섹터 이동 요청
		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
			Packet_Sector_Move(SessionID, Packet);

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
		// char* pExc = exc.GetExceptionText();		

		//// 로그 찍기 (로그 레벨 : 에러)
		//cChatLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
		//	(TCHAR*)pExc);	

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

	// 2) 마샬링
	INT64 AccountNo;
	*Packet >> AccountNo;

	WORD wSectorX, wSectorY;
	*Packet >> wSectorX;
	*Packet >> wSectorY;
	
	// 패킷 검증 -----------------------------
	// 3) 이동하고자 하는 섹터가 정상인지 체크
	if (wSectorX < 0 || wSectorX > SECTOR_X_COUNT ||
		wSectorY < 0 || wSectorY > SECTOR_Y_COUNT)
	{
		m_SectorPosError++;

		// 비정상이면 밖으로 예외 던진다
		throw CException(_T("Packet_Sector_Move(). Sector pos Error"));
	}

	// 4) AccountNo 체크
	if (AccountNo != FindPlayer->m_i64AccountNo)
	{
		m_SectorNoError++;

		// 비정상이면 밖으로 예외 던진다
		throw CException(_T("Packet_Sector_Move(). AccountNo Error"));
	}
	
	// 4) 유저의 섹터 정보 갱신
	// 최초 섹터 패킷이 아니라면, 기존 섹터에서 뺀다.
	// 똑같은 값이기 때문에, 하나만 체크해도 된다.
	if (FindPlayer->m_wSectorY != TEMP_SECTOR_POS &&
		FindPlayer->m_wSectorX != TEMP_SECTOR_POS)
	{
		// 현재 섹터에서 유저 제거
		m_listSecotr[FindPlayer->m_wSectorY][FindPlayer->m_wSectorX].remove(FindPlayer);
	}

	// 색터 갱신
	FindPlayer->m_wSectorX = wSectorX;
	FindPlayer->m_wSectorY = wSectorY;

	// 새로운 색터에 유저 추가
	m_listSecotr[wSectorY][wSectorX].push_back(FindPlayer);

	// 5) 클라이언트에게 보낼 패킷 조립 (섹터 이동 결과)
	CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

	// 타입, AccountNo, SecotrX, SecotrY
	WORD SendType = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	*SendBuff << SendType;
	*SendBuff << AccountNo;
	*SendBuff << wSectorX;
	*SendBuff << wSectorY;

	// 6) 클라에게 패킷 보내기(정확히는 NetServer의 샌드버퍼에 넣기)
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

	// 2) 마샬링
	INT64 AccountNo;
	*Packet >> AccountNo;

	WORD MessageLen;
	*Packet >> MessageLen;

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
	
	// 3) 클라이언트에게 보낼 패킷 조립 (채팅 보내기 응답)
	CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

	WORD Type = en_PACKET_CS_CHAT_RES_MESSAGE;
	*SendBuff << Type;
	*SendBuff << AccountNo;
	SendBuff->PutData((char*)FindPlayer->m_tLoginID, 40);
	SendBuff->PutData((char*)FindPlayer->m_tNickName, 40);
	*SendBuff << MessageLen;
	SendBuff->PutData((char*)Message, MessageLen);
	
	// 4) 해당 유저의 주변 9개 섹터 구함.
	stSectorCheck SecCheck;
	GetSector(FindPlayer->m_wSectorX, FindPlayer->m_wSectorY, &SecCheck);

	// 5) 주변 유저에게 채팅 메시지 보냄
	// 모든 유저에게 보낸다 (채팅을 보낸 유저 포함)
	SendPacket_Sector(SendBuff, &SecCheck);	

	// 6) 패킷 Free
	// !! Net서버쪽 완통에서 Free 하지만, SendPacket_Sector이 안에서 하나씩 증가시키면서 보냈기 때문에 1개가 더 증가한 상태. !!	
	CProtocolBuff_Net::Free(SendBuff);
}


// 로그인 요청
//
// Parameter : SessionID, CProtocolBuff_Net*
// return : 없음
void CChatServer::Packet_Chat_Login(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
{
	// 1) map에서 유저 검색
	stPlayer* FindPlayer = FindPlayerFunc(SessionID);
	if (FindPlayer == nullptr)
		m_ChatDump->Crash();

	// 2) 마샬링
	INT64	AccountNo;
	*Packet >> AccountNo;
	FindPlayer->m_i64AccountNo = AccountNo;

	Packet->GetData((char*)FindPlayer->m_tLoginID, 40);
	Packet->GetData((char*)FindPlayer->m_tNickName, 40);
	Packet->GetData(FindPlayer->m_cToken, 64);

	// 3) 토큰 검사
	

	// 4) 클라이언트에게 보낼 패킷 조립 (로그인 요청 응답)
	CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

	WORD SendType = en_PACKET_CS_CHAT_RES_LOGIN;
	*SendBuff << SendType;

	BYTE Status = 1;
	*SendBuff << Status;

	*SendBuff << AccountNo;

	// 6) 클라에게 패킷 보내기(정확히는 NetServer의 샌드버퍼에 넣기)
	SendPacket(SessionID, SendBuff);
}



// ------------------------------------------------
// 외부에서 호출하는 함수
// ------------------------------------------------

// 생성자
CChatServer::CChatServer()
{
	// 할거 없음.
	// NetServer의 생성자만 호출되면 됨.
}

// 소멸자
CChatServer::~CChatServer()
{
	// 서버가 가동중이라면 스탑.
	if (GetServerState() == true)
		ServerStop();	
}

// 채팅 서버 시작 함수
// 내부적으로 NetServer의 Start도 같이 호출
//
// return false : 에러 발생 시. 에러코드 셋팅 후 false 리턴
// return true : 성공
bool CChatServer::ServerStart()
{
	//// ------------------- Config정보 셋팅
	stConfigFile config;
	if (SetFile(&config) == false)
		return false;


	// ------------------- 각종 리소스 할당
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

	// ------------------- 업데이트 스레드 생성
	hUpdateThraed = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, 0);
	   	

	// ------------------- 넷서버 가동
	if (Start(config.BindIP, config.Port, config.CreateWorker, config.ActiveWorker, config.CreateAccept, config.Nodelay, config.MaxJoinUser,
		config.HeadCode, config.XORCode1, config.XORCode2) == false)
		return false;			

	return true;
}

// 채팅 서버 종료 함수
//
// Parameter : 없음
// return : 없음
void CChatServer::ServerStop()
{
	// 넷서버 스탑 (엑셉트, 워커 종료)
	Stop();		

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
			m_listSecotr[y][x].clear();
		}
	}

	// Playermap에서 모든 유저 제거
	auto itor = m_mapPlayer.begin();

	while (itor != m_mapPlayer.end())
	{
		// 메모리풀에 반환
		m_PlayerPool->Free(itor->second);

		// map에서 제거
		itor = m_mapPlayer.erase(itor);
	}

	// 큐 동적해제.
	delete m_LFQueue;

	// 구조체 메시지 TLS 메모리 풀 동적해제
	delete m_MessagePool;

	// 플레이어 구조체 TLS 메모리풀 동적해제
	delete m_PlayerPool;

	// 업데이트 스레드 깨우기용 이벤트 해제
	CloseHandle(UpdateThreadEvent);

	// 업데이트 스레드 종료용 이벤트 해제
	CloseHandle(UpdateThreadEXITEvent);

	// 업데이트 스레드 핸들 반환
	CloseHandle(hUpdateThraed);
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

	InterlockedIncrement(&g_ullUpdateStructCount);

	// 2.  세션 ID 채우기
	NowMessage->m_ullSessionID = SessionID;

	// 3. 타입 넣기
	NowMessage->m_wType = TYPE_JOIN;

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

	InterlockedIncrement(&g_ullUpdateStructCount);

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

	InterlockedIncrement(&g_ullUpdateStructCount);

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
	// (네트워크) 페이로드만큼 디큐 실패
	if (error == 15)
		m_NotPayloadDeq++;

	// (네트워크) 헤더 코드 에러
	else if (error == 21)
		m_HeadCodeError++;

	// (네트워크) 체크썸 에러
	else if (error == 22)
		m_ChackSumError++;

	// (네트워크) 헤더의 Len사이즈가 비정상적으로 큼.
	else if (error == 23)
		m_HeaderLenBig++;

	// (네트워크) 헤더의 Len만큼 디큐 실패
	else if (error == 24)
		m_PayloadQueError++;

	// 로그 찍기 (로그 레벨 : 에러)
	/*cChatLibLog->LogSave(L"NetServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
		errorStr, error);*/
}
