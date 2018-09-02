#include "pch.h"
#include "ChatServer.h"

#include "CommonProtocol.h"

#include <strsafe.h>


// 들어온 유저를 알려주는 프로토콜 Type
#define JOIN_USER_PROTOCOL_TYPE		15000

// 나간 유저를 알려주는 프로토콜 Type
#define LEAVE_USER_PROTOCOL_TYPE	15001



// -------------------------------------
// 클래스 내부에서 사용하는 함수
// -------------------------------------

// Player 관리 자료구조에, 유저 추가
// 현재 map으로 관리중
// 
// Parameter : SessionID, stPlayer*
// return : 추가 성공 시, true
//		  : SessionID가 중복될 시(이미 접속중인 유저) false
bool CChatServer::InsertPlayerFunc(ULONGLONG SessionID, stPlayer* insertPlayer)
{
	// map에 추가
	auto ret = m_mapPlayer.insert(pair<ULONGLONG, stPlayer*>(SessionID, insertPlayer));

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
		return false;

	// 2) 맵에서 제거
	m_mapPlayer.erase(FindPlayer);

	return FindPlayer->second;
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
// parameter : SessionID, 보낼 버퍼, 섹터 9개
// return : 없음
void CChatServer::SendPacket_Sector(ULONGLONG SessionID, CProtocolBuff_Net* SendBuff, stSectorCheck* Sector)
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
			// SendPacket
			SendPacket(SessionID, SendBuff);
		}
	}
}







// -------------------------------------
// 클래스 내부에서만 사용하는 패킷 처리 함수
// -------------------------------------

// 섹터 이동요청 패킷 처리
//
// Parameter : SessionID, Packet
// return : 성공 시 true
//		  : 접속중이지 않은 유저알 시 false
bool CChatServer::Packet_Sector_Move(ULONGLONG SessionID, BINGNODE* Packet)
{
	// 1) 내가 필요한 형으로 형변환
	st_Protocol_CS_CHAT_REQ_SECTOR_MOVE* NowPacket = (st_Protocol_CS_CHAT_REQ_SECTOR_MOVE*)Packet;

	// 2) map에서 유저 검색
	stPlayer* FindPlayer = FindPlayerFunc(SessionID);
	if (FindPlayer == nullptr)
	{
		printf("Not Find Player!!\n");
		return false;
	}

	// 3) 유저의 섹터 정보 갱신
	// 현재 섹터에서 유저 제거
	m_listSecotr[FindPlayer->m_wSectorY][FindPlayer->m_wSectorX].remove(FindPlayer);

	// 섹터 갱신
	WORD wSectorX = NowPacket->SectorX;
	WORD wSectorY = NowPacket->SectorY;

	FindPlayer->m_wSectorX = wSectorX;
	FindPlayer->m_wSectorY = wSectorY;

	// 새로운 색터에 유저 추가
	m_listSecotr[wSectorY][wSectorX].push_back(FindPlayer);

	// 4) 클라이언트에게 보낼 패킷 조립 (섹터 이동 결과)
	st_Protocol_CS_CHAT_RES_SECTOR_MOVE stSend;

	// 타입, AccountNo, SecotrX, SecotrY
	stSend.Type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	stSend.AccountNo = NowPacket->AccountNo;
	stSend.SectorX = wSectorX;
	stSend.SectorY = wSectorY;

	CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

	SendBuff->PutData((const char*)&stSend, sizeof(st_Protocol_CS_CHAT_RES_SECTOR_MOVE));

	// 5) 클라에게 패킷 보내기(정확히는 NetServer의 샌드버퍼에 넣기)
	SendPacket(SessionID, SendBuff);

	return true;

}

// 채팅 보내기 요청
//
// Parameter : SessionID, Packet
// return : 성공 시 true
//		  : 접속중이지 않은 유저일 시 false
bool CChatServer::Packet_Chat_Message(ULONGLONG SessionID, BINGNODE* Packet)
{
	// 1) 내가 필요한 형으로 형변환
	st_Protocol_CS_CHAT_REQ_MESSAGE* NowPacket = (st_Protocol_CS_CHAT_REQ_MESSAGE*)Packet;

	// 2) 메시지 마지막에 널문자 넣기.
	// NowPacket->Message[NowPacket->MessageLen] = L'\0';

	// 3) map에서 유저 검색
	stPlayer* FindPlayer = FindPlayerFunc(SessionID);
	if (FindPlayer == nullptr)
	{
		printf("Not Find Player!!\n");
		return false;
	}

	// 4) 클라이언트에게 보낼 패킷 조립. (채팅보내기 응답)
	BINGNODE stSend;
	CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

	// 타입, AccountNo, ID, Nickname, MessageLen, Message
	stSend.Type = en_PACKET_CS_CHAT_RES_MESSAGE;
	stSend.AccountNo = NowPacket->AccountNo;

	StringCbCopy(stSend.ID, 20, FindPlayer->m_tLoginID);
	StringCbCopy(stSend.Nickname, 20, FindPlayer->m_tNickName);

	stSend.MessageLen = NowPacket->MessageLen;
	StringCbCopy(stSend.Message, NowPacket->MessageLen, NowPacket->Message);

	SendBuff->PutData((const char*)&stSend, sizeof(BINGNODE));

	// 5) 해당 유저의 주변 9개 섹터 구함.
	stSectorCheck SecCheck;
	GetSector(FindPlayer->m_wSectorX, FindPlayer->m_wSectorY, &SecCheck);

	// 6) 주변 유저에게 채팅 메시지 보냄
	// 모든 유저에게 보낸다 (채팅을 보낸 유저 포함)
	SendPacket_Sector(SessionID, SendBuff, &SecCheck);

	return true;
}

// Accept 성공 (채팅 서버에 입장)
// 네트워크 통신은 아니며, Net서버에서 Chat 서버로 알려준다.
// 
// Parameter : SessionID, Packet
// return : 성공 시 true
//		  : 이미 접속중인 유저라면 false
bool CChatServer::Packet_Chat_Join(ULONGLONG SessionID, BINGNODE* Packet)
{
	// 1) 내가 원하는 형으로 형변환
	st_Protocol_NetChat_OnClientJoin* NowPacket = (st_Protocol_NetChat_OnClientJoin*)Packet;

	// 2) Player Alloc()
	stPlayer* JoinPlayer = m_PlayerPool->Alloc();

	// 3) SessionID 셋팅
	JoinPlayer->m_ullSessionID = SessionID;

	// 4) Player 관리 자료구조에 유저 추가
	if (InsertPlayerFunc(SessionID, JoinPlayer) == false)
	{
		printf("duplication SessionID!!\n");
		return false;
	}

	return true;
}

// 유저 종료 (채팅 서버에서 나감)
// 네트워크 통신은 아니며, Net서버에서 Chat 서버로 알려준다.
// 
// Parameter : SessionID, Packet
// return : 이미 나간유저일 시 false
bool CChatServer::Packet_Chat_Leave(ULONGLONG SessionID, BINGNODE* Packet)
{
	// 1) 내가 원하는 형으로 형변환
	st_Protocol_NetChat_OnClientLeave* NowPacket = (st_Protocol_NetChat_OnClientLeave*)Packet;

	// 2) Player 자료구조에서 제거
	stPlayer* ErasePlayer = ErasePlayerFunc(SessionID);
	if (ErasePlayer == nullptr)
	{
		printf("Not Find Player!!\n");
		return false;
	}

	// 3) Player Free()
	m_PlayerPool->Free(ErasePlayer);	

	return true;
}





// ------------------------------------------------
// 외부에서 호출하는 함수
// ------------------------------------------------

// 생성자
//
// Parameter : 업데이트 스레드를 깨울 때 사용할 이벤트
CChatServer::CChatServer(HANDLE* UpdateThreadEvent)
{
	// 구조체 메시지 TLS 메모리풀 동적할당
	// 총 100개의 청크. (1개당 200개의 데이터를 다루니, 200*100 = 20000. 총 20000개의 구조체 메시지 사용 가능)
	m_MessagePool = new CMemoryPoolTLS<BINGNODE>(100, false);

	// 플레이어 구조체 TLS 메모리풀 동적할당
	// 총 100개의 청크. (1개당 200개의 데이터를 다루니, 200*100 = 20000. 총 20000개의 플레이어 구조체 사용 가능)
	m_PlayerPool = new CMemoryPoolTLS<stPlayer>(100, false);

	// 락프리 큐 동적할당 (네트워크가 컨텐츠에게 일감 던지는 큐)
	// 사이즈가 0인 이유는, UpdateThread에서 큐가 비었는지 체크하고 쉬러 가야하기 때문에.
	m_LFQueue = new CLF_Queue<BINGNODE*>(0);

	// 업데이트스레드 깨우기 용도 이벤트 생성 후 멤버로 셋팅
	// 
	// 메뉴얼 리셋 Event 
	// 최초 생성 시 non-signalled 상태ㅍ
	// 이름 없는 Event
	*UpdateThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_UpdateThreadEvent = UpdateThreadEvent;	
}

// 소멸자
//
// Parameter : Event 객체 주소
CChatServer::~CChatServer()
{
	// 큐를 먼저 동적해제. 이 안에, m_MessagePool가 관리하는 데이터가 있을 수 있기 때문에
	delete m_LFQueue;

	// 구조체 메시지 TLS 메모리 풀 동적해제
	delete m_MessagePool;

	// 플레이어 구조체 TLS 메모리풀 동적해제
	delete m_PlayerPool;

	// 서버 스탑.
	Stop();
}

// 패킷 처리 함수
// 내부에서 각종 패킷 처리
// 
// Parameter : 없음
// return : 없음
void CChatServer::PacketHandling()
{
	// 1. 큐에서 일감 1개 빼오기
	BINGNODE* NowWork;

	if (m_LFQueue->Dequeue(NowWork) == -1)
		m_ChatDump->Crash();

	// 2. Type에 따라 로직 처리
	// 분기 예측 성공률을 높이기 위해, 가장 많이 호출될 법 한 것부터 switch case문 상단에 배치.
	switch (NowWork->Type)
	{
		// 채팅서버 채팅보내기 요청
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		Packet_Chat_Message(NowWork->SessionID, NowWork);

		break;

		// 채팅서버 섹터 이동 요청
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		Packet_Sector_Move(NowWork->SessionID, NowWork);

		break;		

		// Accept 성공
	case JOIN_USER_PROTOCOL_TYPE:
		Packet_Chat_Join(NowWork->SessionID, NowWork);

		break;

		// 서버에서 Leave
	case LEAVE_USER_PROTOCOL_TYPE:
		Packet_Chat_Leave(NowWork->SessionID, NowWork);
		break;

		// 하트비트
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		break;

	default:
		printf("Type Error!!! (%d)\n", NowWork->Type);
		break;
	}

	// 3. 일감을 메모리풀에 Free
	m_MessagePool->Free(NowWork);

}

// 처리할 일감이 있는지 체크하는 함수
// 내부적으로는 QueueSize를 체크하는 것.
//
// Parameter : 없음
// return : 현재 일감 수
LONG CChatServer::WorkCheck()
{
	return m_LFQueue->GetInNode();
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
	st_Protocol_NetChat_OnClientJoin* NowMessage = (st_Protocol_NetChat_OnClientJoin*)m_MessagePool->Alloc();

	// 2.  세션 ID 채우기
	NowMessage->SessionID = SessionID;

	// 3. 타입 넣기
	NowMessage->Type = JOIN_USER_PROTOCOL_TYPE;

	// 4. 메시지를 큐에 넣는다.
	m_LFQueue->Enqueue((BINGNODE*)NowMessage);

	// 5. 자고있는 Update스레드를 깨운다.
	SetEvent(*m_UpdateThreadEvent);

}


void CChatServer::OnClientLeave(ULONGLONG SessionID)
{
	// 호출 시점 : 유저가 서버에서 나갈 시
	// 호출 위치 : NetServer의 워커스레드
	// 하는 행동 : 컨텐츠가 들고있는 큐에, 유저 종료 메시지를 넣는다.

		// 1. 일감 Alloc
	st_Protocol_NetChat_OnClientLeave* NowMessage = (st_Protocol_NetChat_OnClientLeave*)m_MessagePool->Alloc();

	// 2.  세션 ID 채우기
	NowMessage->SessionID = SessionID;

	// 3. 타입 넣기
	NowMessage->Type = LEAVE_USER_PROTOCOL_TYPE;

	// 4. 메시지를 큐에 넣는다.
	m_LFQueue->Enqueue((BINGNODE*)NowMessage);

	// 5. 자고있는 Update스레드를 깨운다.
	SetEvent(*m_UpdateThreadEvent);

}


void CChatServer::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
{
	// 호출 시점 : 유저에게 패킷을 받을 시
	// 호출 위치 : NetServer의 워커스레드
	// 하는 행동 : 컨텐츠가 들고있는 메시지 큐에, 데이터를 넣는다.	

	// 1. 일감 Alloc
	BINGNODE* NowMessage = m_MessagePool->Alloc();

	// 2. 세션 ID 채우기
	NowMessage->SessionID = SessionID;

	// 3. Copy
	// 어떤 메시지든 가장 앞 8바이트는 SessioniID가 들어있기 때문에 8바이트 뒤부터 복사.
	int GetUseSize = Payload->GetUseSize();
	memcpy_s(&NowMessage[8], GetUseSize, Payload->GetBufferPtr(), GetUseSize);

	// 4.메시지를 큐에 넣는다.
	m_LFQueue->Enqueue(NowMessage);

	// 5. 자고있는 Update스레드를 깨운다.
	SetEvent(*m_UpdateThreadEvent);
}


void CChatServer::OnSend(ULONGLONG SessionID, DWORD SendSize)
{}

void CChatServer::OnWorkerThreadBegin()
{}

void CChatServer::OnWorkerThreadEnd()
{}

void CChatServer::OnError(int error, const TCHAR* errorStr)
{}
