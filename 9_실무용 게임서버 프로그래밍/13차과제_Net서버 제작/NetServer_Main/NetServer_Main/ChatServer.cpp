#include "pch.h"
#include "ChatServer.h"

#include "CommonProtocol.h"
#include "ProtocolStruct.h"

#include <strsafe.h>


// 들어온 유저를 알려주는 프로토콜 Type
#define JOIN_USER_PROTOCOL_TYPE		15000

// 나간 유저를 알려주는 프로토콜 Type
#define LEAVE_USER_PROTOCOL_TYPE	15001

// ------------------------------------------------
// 구조체 & 클래스 멤버 함수
// ------------------------------------------------

// 생성자
//
// Parameter : 업데이트 스레드를 깨울 때 사용할 이벤트
CChatServer::CChatServer(HANDLE* UpdateThreadEvent)
{
	// 구조체 메시지 TLS 메모리풀 동적할당
	m_MessagePool = new CMemoryPoolTLS<stWork>(100, false);

	// 플레이어 구조체 TLS 메모리풀 동적할당
	m_PlayerPool = new CMemoryPoolTLS<stPlayer>(100, false);

	// 락프리 큐 동적할당 (네트워크가 컨텐츠에게 일감 던지는 큐)
	// 사이즈가 0인 이유는, UpdateThread에서 큐가 비었는지 체크하고 쉬러 가야하기 때문에.
	m_LFQueue = new CLF_Queue<stWork*>(0);

	// 업데이트스레드 깨우기 용도 이벤트 생성 후 멤버로 셋팅
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
	// 1. 큐에서 데이터 1개 빼오기
	stWork* NowWork;

	if (m_LFQueue->Dequeue(NowWork) == -1)
		m_ChatDump->Crash();

	// 2. SessionID와 Type을 인자로 받아둔다.
	ULONGLONG ullSessionID = NowWork->m_SessionID;

	CProtocolBuff_Net* localPacket = NowWork->m_Packet;
	WORD wType;
	localPacket->PutData((char*)&wType, 2);

	// 3. Type에 따라 로직 처리
	switch (wType)
	{
		// 채팅서버 섹터 이동 요청
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		Packet_Sector_Move(NowWork->m_SessionID, localPacket);		

		break;

		// 채팅서버 채팅보내기 요청
	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		Packet_Chat_Message(NowWork->m_SessionID, localPacket);

		break;

		// 하트비트
	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		break;

	default:
		break;
	}

	// 4. 다 쓴 메시지 Free
	CProtocolBuff_Net::Free(localPacket);

	// 5. 일감 구조체 Free
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

// 섹터 이동요청 패킷 처리
//
// Parameter : SessionID, Packet
// return : map에서 유저 검색 실패 시 false
bool CChatServer::Packet_Sector_Move(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
{
	// 1) 패킷을 st_Protocol_CS_CHAT_REQ_SECTOR_MOVE 형태로 받아오기
	st_Protocol_CS_CHAT_REQ_SECTOR_MOVE NowMessage;
	Packet->PutData((const char*)&NowMessage, Packet->GetUseSize());

	// 2) map에서 유저 검색
	auto FindPlayer = m_mapPlayer.find(SessionID);
	if (FindPlayer == m_mapPlayer.end())
	{
		printf("Not Find Player!!\n");
		return false;
	}

	// 3) 유저의 섹터 정보 갱신
	FindPlayer->second->m_wSectorX = NowMessage.SectorX;
	FindPlayer->second->m_wSectorY = NowMessage.SectorY;

	// 4) 클라이언트에게 보낼 패킷 조립 (섹터 이동 결과)
	st_Protocol_CS_CHAT_RES_SECTOR_MOVE stSend;

	// 타입, AccountNo, SecotrX, SecotrY
	stSend.Type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	stSend.AccountNo = NowMessage.AccountNo;
	stSend.SectorX = NowMessage.SectorX;
	stSend.SectorY = NowMessage.SectorY;

	CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

	SendBuff->PutData((const char*)&stSend, sizeof(st_Protocol_CS_CHAT_RES_SECTOR_MOVE));

	// 5) 클라에게 패킷 보내기(정확히는 NetServer의 샌드버퍼에 넣기)
	if (SendPacket(SessionID, SendBuff) == false)
	{
		// 여기서 false가 뜨면 맵에서 유저 삭제?
		m_mapPlayer.erase(SessionID);

		// 그리고 Player 구조체를 메모리풀에 반환한다.
		m_PlayerPool->Free(FindPlayer->second);
	}

	return true;

}


// 채팅 보내기 요청
//
// Parameter : SessionID, Packet
// return : map에서 유저 검색 실패 시 false
bool CChatServer::Packet_Chat_Message(ULONGLONG SessionID, CProtocolBuff_Net* Packet)
{
	// 1) 패킷을 st_Protocol_CS_CHAT_REQ_MESSAGE 형태로 받아오기
	st_Protocol_CS_CHAT_REQ_MESSAGE NowMessage;
	Packet->PutData((const char*)&NowMessage, Packet->GetUseSize());

	NowMessage.Message[NowMessage.MessageLen] = (WCHAR)L'\0';

	// 2) map에서 유저 검색
	auto FindPlayer = m_mapPlayer.find(SessionID);
	if (FindPlayer == m_mapPlayer.end())
	{
		printf("Not Find Player!!\n");
		return false;
	}

	// 3) 클라이언트에게 보낼 패킷 조립. (채팅보내기 응답)
	st_Protocol_CS_CHAT_RES_MESSAGE stSend;

	// 타입, AccountNo, ID, Nickname, MessageLen, Message
	stSend.Type = en_PACKET_CS_CHAT_RES_MESSAGE;
	stSend.AccountNo = NowMessage.AccountNo;

	StringCbCopy(stSend.ID, 20, FindPlayer->second->m_tLoginID);
	StringCbCopy(stSend.Nickname, 20, FindPlayer->second->m_tNickName);

	stSend.MessageLen = NowMessage.MessageLen;
	StringCbCopy(stSend.Message, NowMessage.MessageLen, NowMessage.Message);

	CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

	SendBuff->PutData((const char*)&stSend, sizeof(st_Protocol_CS_CHAT_RES_MESSAGE));

	// 4) 해당 유저의 주변 9개 섹터 구함.
	stSectorCheck SecCheck;
	GetSector(FindPlayer->second->m_wSectorX, FindPlayer->second->m_wSectorY, &SecCheck);

	// 5) 주변 유저에게 채팅 메시지 보냄
	// 모든 유저에게 보낸다 (채팅을 보낸 유저 포함)
	SendPacket_Sector(SessionID, SendBuff, &SecCheck);

	return true;
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
void CChatServer::SendPacket_Sector(ULONGLONG ClinetID, CProtocolBuff_Net* SendBuff, stSectorCheck* Sector)
{
	// 1. Sector안의 유저들에게 인자로 받은 패킷을 넣는다.
	for (DWORD i = 0; i < Sector->m_dwCount; ++i)
	{
		// 2. itor는 "i" 번째 섹터의 리스트를 가리킨다.
		auto itor = m_listSecotr[Sector->m_Sector[i].y][Sector->m_Sector[i].x].begin();

		for (; itor != m_listSecotr[Sector->m_Sector[i].y][Sector->m_Sector[i].x].end(); ++itor)
		{
			// SendPacket
			SendPacket(ClinetID, SendBuff);
		}
	}
}



// ------------------------------------------------
// 채팅서버의 가상함수
// ------------------------------------------------

bool CChatServer::OnConnectionRequest(TCHAR* IP, USHORT port)
{
	// 만약, IP와 Port가 이상한 곳(예를들어 해외?)면 false 리턴.
	// false가 리턴되면 접속이 안된다.

	return true;
}

void CChatServer::OnClientJoin(ULONGLONG ClinetID)
{
	// 호출 시점 : 유저가 서버 정상적으로 접속되었을 시
	// 호출 위치 : NetServer의 워커스레드
	// 하는 행동 : 컨텐츠가 들고있는 큐에, 데이터를 넣는다.

	// 1. 메시지 구조체 관리 메모리풀에서 Alloc
	stWork* NowMessage = m_MessagePool->Alloc();

	// 2. 패킷 Alloc
	CProtocolBuff_Net* Temp = CProtocolBuff_Net::Alloc();

	// 3. 세션 ID 채우기
	NowMessage->m_SessionID = ClinetID;

	// 4. 타입 넣기
	WORD Type = JOIN_USER_PROTOCOL_TYPE;
	Temp->PutData((const char*)&Type, 2);

	// 5. 메시지에 연결
	NowMessage->m_Packet = Temp;

	// 6. 메시지를 큐에 넣는다.
	m_LFQueue->Enqueue((stWork*)NowMessage);

	// 7. 자고있는 Update스레드를 깨운다.
	SetEvent(*m_UpdateThreadEvent);

}


void CChatServer::OnClientLeave(ULONGLONG ClinetID)
{
	// 호출 시점 : 유저가 서버에서 나갈 시
	// 호출 위치 : NetServer의 워커스레드
	// 하는 행동 : 컨텐츠가 들고있는 큐에, 데이터를 넣는다.


}

void CChatServer::OnRecv(ULONGLONG ClinetID, CProtocolBuff_Net* Payload)
{
	// 호출 시점 : 유저에게 패킷을 받을 시
	// 호출 위치 : NetServer의 워커스레드
	// 하는 행동 : 컨텐츠가 들고있는 메시지 큐에, 데이터를 넣는다.	

	// 1. 메시지 구조체 관리 메모리풀에서 Alloc
	stWork* NowMessage = m_MessagePool->Alloc();

	// 2. 세션 ID 채우기
	NowMessage->m_SessionID = ClinetID;

	// 3. NowMessage 셋팅
	// refCount 1 증가 , 패킷 대입
	Payload->Add();
	NowMessage->m_Packet = Payload;

	// 4.메시지를 큐에 넣는다.
	m_LFQueue->Enqueue(NowMessage);

	// 5. 자고있는 Update스레드를 깨운다.
	SetEvent(*m_UpdateThreadEvent);
}


void CChatServer::OnSend(ULONGLONG ClinetID, DWORD SendSize)
{}

void CChatServer::OnWorkerThreadBegin()
{}

void CChatServer::OnWorkerThreadEnd()
{}

void CChatServer::OnError(int error, const TCHAR* errorStr)
{}
