#include "stdafx.h"
#include "Network_Func.h"
#include <map>
#include <list>

using namespace std;


// 논리적인 맵 크기
#define MAP_WIDTH			6400
#define MAP_HEIGHT			6400

// 섹터 1개의 크기
#define SECTOR_WIDTH		300
#define SECTOR_HEIGHT		200

// 게임에서 사용되는 섹터 수
// 현재 높이 : 6400/200 = 32, 현재 넓이 : 6400/300 = 21.333...(정수계산으로 21로 처리)
// 계산 식 : 논리 사이즈 / 섹터 1개의 크기
#define SECTOR_WIDTH_COUNT		(MAP_WIDTH / SECTOR_WIDTH) + 1
#define SECTOR_HEIGHT_COUNT		(MAP_HEIGHT / SECTOR_HEIGHT) + 0

// 유저가 생성되는 위치 디폴트 (지금 안쓰는중)
#define DEFAULT_X	6000
#define DEFAULT_Y	100

// 유저 상태
#define PLAYER_ACTION_IDLE	0	// 정지
#define PLAYER_ACTION_MOVE	1	// 이동

// 1프레임에 이동하는 좌표
#define PLAYER_XMOVE_PIXEL 6 // 3
#define PLAYER_YMOVE_PIXEL 4 // 2

// 유저 이동제한
#define dfRANGE_MOVE_TOP	0 + 1
#define dfRANGE_MOVE_LEFT	0 + 2
#define dfRANGE_MOVE_RIGHT	MAP_WIDTH - 3
#define dfRANGE_MOVE_BOTTOM	MAP_HEIGHT - 1

// 데드레커닝 체크 오차값 (픽셀단위)
#define dfERROR_RANGE		50

// 공격 별 범위
#define dfATTACK1_RANGE_X   80
#define dfATTACK2_RANGE_X   90
#define dfATTACK3_RANGE_X   100
#define dfATTACK1_RANGE_Y   10
#define dfATTACK2_RANGE_Y   10
#define dfATTACK3_RANGE_Y   20

// 공격 별 데미지
#define dfATTACK1_DAMAGE      1
#define dfATTACK2_DAMAGE      2
#define dfATTACK3_DAMAGE      3

// 공격 시, 대충 캐릭터 중간쯤 맞추기 위해, 캐릭터의 Y 좌표에서 뺴는 값.
#define dfATTACK_Y			  40


extern int		g_LogLevel;			// Main에서 입력한 로그 출력 레벨. 외부변수
extern TCHAR	g_szLogBuff[1024];		// Main에서 입력한 로그 출력용 임시 버퍼. 외부변수


// 플레이어 정보.
struct stPlayer
{
	// 회원 셋팅
	stAccount* m_Account;

	// 플레이어 ID
	DWORD	m_dwPlayerID;

	// 플레이어의 방향/X/Y/HP 셋팅
	BYTE	m_byDir;
	short	m_wNowX;
	short	m_wNowY;
	BYTE	m_byHP;

	// 과거 액션, 현재 액션
	BYTE	m_byOldAction;
	BYTE	m_byNowAction;

	// 현재 유저가 있는 섹터의 X,Y인덱스
	WORD m_SectorX;
	WORD m_SectorY;

	// 데드레커닝을 위해, 액션 변경 시점의 X,Y좌표 저장(월드좌표)
	short	m_wActionX;
	short	m_wActionY;

	// 데드레커닝을 위해, NowAction을 취한 시간. 생성 후 한번도 이동한적 없으면 0
	ULONGLONG m_dwACtionTick;
};

// 회원 정보
// 로그인 시, 플레이어정보를 갖게된다.
struct stAccount
{
	// 해당 회원 소켓. map의 키
	SOCKET m_sock;

	// 로그인 시, 플레이어 정보를 찌른다.
	stPlayer* m_LoginPlayer = nullptr;	

	// 해당 회원의 IP와 Port;
	TCHAR m_tIP[30];
	WORD m_wPort;	

	// 샌드, 리시브 버퍼
	CRingBuff m_SendBuff;
	CRingBuff m_RecvBuff;

};

struct stSectorCheck
{
	// 몇 개의 Sector Index가 저장되어 있는지
	DWORD m_dwCount;

	// 섹터의 X,Y 인덱스 좌표 저장.
	POINT m_Sector[9];
};


// 회원 정보 관리 map
// Key : 회원의 SOCKET
map <SOCKET, stAccount*> map_Account;

// 총 회원 수 카운트
DWORD g_dwAcceptCount = 0;	// 총 회원 수 카운트

// 플레이어 ID 유니크 값. 1부터 시작.
DWORD g_dwPlayerID = 1;

// 섹터에 있는 플레이어 관리.
// Value: 플레이어 구조체
list <stPlayer*> list_Sector[SECTOR_HEIGHT_COUNT][SECTOR_WIDTH_COUNT];







// ---------------------------------
// 기타 함수
// ---------------------------------
// 네트워크 프로세스
// 여기서 false가 반환되면, 프로그램이 종료된다.
bool Network_Process(SOCKET* listen_sock)
{
	BEGIN("Network_Process");

	// 클라와 통신에 사용할 변수
	FD_SET rset;
	FD_SET wset;

	SOCKADDR_IN clinetaddr;
	map <SOCKET, stAccount*>::iterator itor;
	map <SOCKET, stAccount*>::iterator enditor;
	TIMEVAL tval;
	tval.tv_sec = 0;
	tval.tv_usec = 0;

	itor = map_Account.begin();

	// 리슨 소켓 셋팅. 
	rset.fd_count = 0;
	wset.fd_count = 0;

	rset.fd_array[rset.fd_count] = *listen_sock;
	rset.fd_count++;

	while (1)
	{
		enditor = map_Account.end();

		// 모든 멤버를 순회하며, 해당 유저를 읽기 셋과 쓰기 셋에 넣는다.
		// 넣다가 64개가 되거나, end에 도착하면 break
		while (itor != enditor)
		{
			// 해당 클라이언트에게 받을 데이터가 있는지 체크하기 위해, 모든 클라를 rset에 소켓 설정
			rset.fd_array[rset.fd_count++] = itor->second->m_sock;

			// 만약, 해당 클라이언트의 SendBuff에 값이 있으면, wset에도 소켓 넣음.
			if (itor->second->m_SendBuff.GetUseSize() != 0)
				wset.fd_array[wset.fd_count++] = itor->second->m_sock;

			// 64개 꽉 찼는지, 끝에 도착했는지 체크		
			++itor;

			if (rset.fd_count == FD_SETSIZE || itor == enditor)
				break;

		}

		// Select()
		DWORD dCheck = select(0, &rset, &wset, 0, &tval);

		// Select()결과 처리
		if (dCheck == SOCKET_ERROR)
		{
			_tprintf(_T("select 에러(%d)\n"), WSAGetLastError());
			return false;
		}		

		// select의 값이 0보다 크다면 뭔가 할게 있다는 것이니 로직 진행
		if (dCheck > 0)
		{
			// 리슨 소켓 처리
			DWORD rsetCount = 0;
			if(rset.fd_array[rsetCount] == *listen_sock && rset.fd_count > 0)
			{
				int addrlen = sizeof(clinetaddr);
				SOCKET client_sock = accept(*listen_sock, (SOCKADDR*)&clinetaddr, &addrlen);

				// 에러가 발생하면, 그냥 그 유저는 소켓 등록 안함
				if (client_sock == INVALID_SOCKET)
					_tprintf(_T("accept 에러 %d\n"), WSAGetLastError());

				// 에러가 발생하지 않았다면, 누군가가 접속한 것. 이에 대해 처리
				else
					Accept(&client_sock, clinetaddr);	// Accept 처리.


				rsetCount++;
			}

			DWORD wsetCount = 0;

			while (1)
			{
				if (rsetCount < rset.fd_count)
				{
					stAccount* NowAccount = ClientSearch_AcceptList(rset.fd_array[rsetCount]);

					// Recv() 처리
					// 만약, RecvProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					if (RecvProc(NowAccount) == false)
						Disconnect(NowAccount);

					rsetCount++;
				}

				if (wsetCount < wset.fd_count)
				{
					stAccount* NowAccount = ClientSearch_AcceptList(wset.fd_array[wsetCount]);

					// Send() 처리
					// 만약, SendProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					if (SendProc(NowAccount) == false)
						Disconnect(NowAccount);

					wsetCount++;
				}

				if (rsetCount == rset.fd_count && wsetCount == wset.fd_count)
					break;

			}			
			
		}

		// 만약, 모든 Client에 대해 Select처리가 끝났으면, 이번 함수는 여기서 종료.
		if (itor == enditor)
			break;

		// select 준비	
		rset.fd_count = 0;
		wset.fd_count = 0;
	}

	END("Network_Process");
	return true;
}

// Accept 처리
bool Accept(SOCKET* client_sock, SOCKADDR_IN clinetaddr)
{
	BEGIN("Accept");

	// -----------------------------
	// 1. 인자로 받은 clinet_sock에 해당되는 유저 생성
	// -----------------------------
	stAccount* NewAccount = new stAccount;

	// -----------------------------
	// 2. 플레이어 생성 후 정보 셋팅
	// -----------------------------
	stPlayer* NowPlayer = new stPlayer;

	NowPlayer->m_Account = NewAccount;
	NowPlayer->m_dwPlayerID = g_dwPlayerID;
	NowPlayer->m_byHP = 100;
	NowPlayer->m_byDir = dfPACKET_MOVE_DIR_LL;
	NowPlayer->m_wNowX = (rand() % 6000) + 100;
	NowPlayer->m_wNowY = (rand() % 6000) + 100;
	NowPlayer->m_byOldAction = PLAYER_ACTION_IDLE;
	NowPlayer->m_byNowAction = PLAYER_ACTION_IDLE;

	// 유저가 생성된 좌표 기준, 위치한 섹터 셋팅 (현재 좌표 / 섹터 1개의 크기)
	NowPlayer->m_SectorX = NowPlayer->m_wNowX / SECTOR_WIDTH;
	NowPlayer->m_SectorY = NowPlayer->m_wNowY / SECTOR_HEIGHT;

	// 데드레커닝용 정보 셋팅. 최초에는 다 0이다.
	NowPlayer->m_wActionX = NowPlayer->m_wActionY = 0;

	// -----------------------------
	// 3. 회원 정보 셋팅 셋팅. (m_LoginPlayer에 새로운 유저를 셋팅해서 연결. 접속 즉시 로그인이다 지금은.)
	// -----------------------------
	NewAccount->m_sock = *client_sock;
	NewAccount->m_LoginPlayer = NowPlayer;

	// 섹터에 플레이어 추가
	list_Sector[NowPlayer->m_SectorY][NowPlayer->m_SectorX].push_back(NowPlayer);

	// IP와 Port 셋팅
	TCHAR Buff[33];
	InetNtop(AF_INET, &clinetaddr.sin_addr, Buff, sizeof(Buff));
	WORD port = ntohs(clinetaddr.sin_port);
	_tcscpy_s(NewAccount->m_tIP, _countof(Buff), Buff);
	NewAccount->m_wPort = port;

	// -----------------------------
	// 4. 접속한 유저의 ip, port, 플레이어 ID 로그로 찍기
	// -----------------------------
	/*SYSTEMTIME lst;
	GetLocalTime(&lst);
	_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] Accept : [%s : %d / PlayerID : %d]\n",
		lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, Buff, port, g_dwPlayerID);*/
	
	// -----------------------------
	// 5. 회원을 map에 추가
	// -----------------------------
	typedef pair<SOCKET, stAccount*> Itn_pair;
	map_Account.insert(Itn_pair(*client_sock, NewAccount));

	// -----------------------------
	// 6. 플레이어 수 증가.
	// -----------------------------
	g_dwPlayerID++;	

	// -----------------------------
	// 7. "나 접속" 패킷 생성
	// -----------------------------
	CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff payload;
	Network_Send_CreateMyCharacter(&header, &payload, g_dwPlayerID - 1, dfPACKET_MOVE_DIR_LL, NowPlayer->m_wNowX, NowPlayer->m_wNowY, 100);

	// -----------------------------
	// 8. 당사자의 SendBuff에 넣기
	// -----------------------------
	if (SendPacket(NewAccount, &header, &payload) == FALSE)
		return FALSE;

	// -----------------------------
	// 9. 주변 9방향의 섹터 Index를 알아온다.
	// -----------------------------
	stSectorCheck SecCheck;
	GetSector(NewAccount->m_LoginPlayer->m_SectorX, NewAccount->m_LoginPlayer->m_SectorY, &SecCheck);

	// -----------------------------
	// 10. "나"에게 "다른 사람의 생성" 패킷 넣기.
	// 주변 9방향에 있는 유저들의 정보를 만들어서 보낸다.
	// -----------------------------
	if (Accept_Surport(NowPlayer, &SecCheck) == FALSE)
		return FALSE;

	// -----------------------------
	// 11. "섹터 내 다른 사람"에게 "다른 사람 생성" 패킷 넣기 (여기서 다른사람은 "나" 이다)
	// -----------------------------
	CProtocolBuff Ohter_header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff Otehr_payload;
	Network_Send_CreateOtherCharacter(&Ohter_header, &Otehr_payload, NewAccount->m_LoginPlayer->m_dwPlayerID, 
		dfPACKET_MOVE_DIR_LL, NewAccount->m_LoginPlayer->m_wNowX, NewAccount->m_LoginPlayer->m_wNowY, NewAccount->m_LoginPlayer->m_byHP);

	if (SendPacket_Sector(NowPlayer, &Ohter_header, &Otehr_payload, &SecCheck) == FALSE)
		return FALSE;

	// -----------------------------
	// 12. 총 접속자 수 증가
	// -----------------------------
	g_dwAcceptCount++;

	END("Accept");

	return TRUE;
}

// Accept 시, 내 기준 9방향 섹터의 유저들 정보를 나한테 뿌리기
bool Accept_Surport(stPlayer* Player, stSectorCheck* Sector)
{
	BEGIN("Accept_Surport");

	// 1. SecCheck안에 있는 유저들을 모두, "나"에게 알려줘야 한다.
	for (DWORD i = 0; i < Sector->m_dwCount; ++i)
	{
		// 3. itor는 "i" 번째 섹터의 리스트를 가리킨다.
		list <stPlayer*>::iterator itor = list_Sector[Sector->m_Sector[i].y][Sector->m_Sector[i].x].begin();
		list <stPlayer*>::iterator enditor = list_Sector[Sector->m_Sector[i].y][Sector->m_Sector[i].x].end();

		for (; itor != enditor; ++itor)
		{
			if ((*itor) == Player)
				continue;

			// "i" 섹터의 n번째 유저 패킷 생성
			CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
			CProtocolBuff payload;	

			BYTE Dir = dfPACKET_MOVE_DIR_RR;

			if ((*itor)->m_byDir == dfPACKET_MOVE_DIR_LL ||
				(*itor)->m_byDir == dfPACKET_MOVE_DIR_LU ||
				(*itor)->m_byDir == dfPACKET_MOVE_DIR_UU ||
				(*itor)->m_byDir == dfPACKET_MOVE_DIR_LD)
			{
				Dir = dfPACKET_MOVE_DIR_LL;
			}

			Network_Send_CreateOtherCharacter(&header, &payload, (*itor)->m_dwPlayerID,
				Dir, (*itor)->m_wNowX, (*itor)->m_wNowY, (*itor)->m_byHP);

			// 4. 생성한 패킷을 이번에 나의 SendBuff에 넣기.
			if (SendPacket(Player->m_Account, &header, &payload) == FALSE)
				return FALSE;

			// 이동 중이면, 이동 패킷 생성
			if ((*itor)->m_byNowAction == PLAYER_ACTION_MOVE)
			{
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;				

				Network_Send_MoveStart(&header, &payload, (*itor)->m_dwPlayerID,
					(*itor)->m_byDir, (*itor)->m_wNowX, (*itor)->m_wNowY);

				// 5. 생성한 패킷을 이번에 나의 SendBuff에 넣기.
				if (SendPacket(Player->m_Account, &header, &payload) == FALSE)
					return FALSE;

			}
		}
	}

	END("Accept_Surport");

	return TRUE;
}

// Disconnect 처리
void Disconnect(stAccount* Account)
{
	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (Account == nullptr)
	{
		//_LOG(dfLOG_LEVEL_ERROR, L"Disconnect(). Accept 중이 아닌 유저를 대상으로 삭제 시도\n");
		return;
	}	

	// 2. 접속 종료한 유저의 ip, port, AccountNo 출력	
	/*SYSTEMTIME lst;
	GetLocalTime(&lst);
	_LOG(dfLOG_LEVEL_DEBUG, L"[%02d/%02d/%02d %02d:%02d:%02d] Disconnect : [%s : %d / PlayerID : %d]\n",
		lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, 
		Account->m_tIP, Account->m_wPort, Account->m_LoginPlayer->m_dwPlayerID);*/

	_tprintf(L"Disconnect : [%s:%d / PlayerID : %d]\n", Account->m_tIP, Account->m_wPort, Account->m_LoginPlayer->m_dwPlayerID);
		
	// 3. 해당 유저 기준 9방향에 있는 유저들에게 "다른 캐릭터 접속 종료" 패킷 발송. 
	// 당사자는 제외한다.
	CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff payload;
	Network_Send_RemoveOtherCharacteor(&header, &payload, Account->m_LoginPlayer->m_dwPlayerID);

	// 주변 9방향의 섹터 Index를 알아온다.
	stSectorCheck SecCheck;
	GetSector(Account->m_LoginPlayer->m_SectorX, Account->m_LoginPlayer->m_SectorY, &SecCheck);

	// 넣기
	SendPacket_Sector(Account->m_LoginPlayer, &header, &payload, &SecCheck);

	// 4. 해당 플레이어를, 섹터 List에서 제외
	list_Sector[Account->m_LoginPlayer->m_SectorY][Account->m_LoginPlayer->m_SectorX].remove(Account->m_LoginPlayer);

	// 5. 해당 유저의 소켓 close
	SOCKET Temp = Account->m_sock;	
	closesocket(Account->m_sock);

	// 6. 접속 종료한 플레이어 삭제
	delete Account->m_LoginPlayer;

	// 7. 접속 종료한 회원 삭제
	delete Account;
	map_Account.erase(Temp);

	// 8. Accept 유저 수 감소
	g_dwAcceptCount--;
}

// 데드레커닝 함수
// 인자로 (현재 액션, 액션 시작 시간, 액션 시작 위치, (OUT)계산 후 좌표)를 받는다.
void DeadReckoning(BYTE NowAction, ULONGLONG ActionTick, int ActionStartX, int ActionStartY, int* ResultX, int* ResultY)
{
	BEGIN("DeadReckoning");

	// ----------------------------
	// 액션 시작시간부터 현재 시간까지의 시간차를 구한다. 
	// 이 시간차로 몇 프레임이 지났는지 계산한다.
	// ----------------------------
	int IntervalFrame = (GetTickCount64() - ActionTick) / 40; // (40이 1프레임. 25프레임으로 맞춤)

	int PosX, PosY;

	// ---------------------------
	// 1. 계산된 프레임으로 X축, Y축 얼마나 갔는지 구함
	// 즉, PosX, PosY를 계산하는 것.
	// ---------------------------
	int iMoveX = IntervalFrame * PLAYER_XMOVE_PIXEL;
	int iMoveY = IntervalFrame * PLAYER_YMOVE_PIXEL;

	// 액션에 따라 이동값이 달라짐
	switch (NowAction)
	{
	case dfPACKET_MOVE_DIR_LL:
		PosX = ActionStartX - iMoveX;
		PosY = ActionStartY;
		break;

	case dfPACKET_MOVE_DIR_LU:
		PosX = ActionStartX - iMoveX;
		PosY = ActionStartY - iMoveY;
		break;

	case dfPACKET_MOVE_DIR_UU:
		PosX = ActionStartX;
		PosY = ActionStartY - iMoveY;
		break;

	case dfPACKET_MOVE_DIR_RU:
		PosX = ActionStartX + iMoveX;
		PosY = ActionStartY - iMoveY;
		break;

	case dfPACKET_MOVE_DIR_RR:
		PosX = ActionStartX + iMoveX;
		PosY = ActionStartY;
		break;

	case dfPACKET_MOVE_DIR_RD:
		PosX = ActionStartX + iMoveX;
		PosY = ActionStartY + iMoveY;
		break;

	case dfPACKET_MOVE_DIR_DD:
		PosX = ActionStartX;
		PosY = ActionStartY + iMoveY;
		break;

	case dfPACKET_MOVE_DIR_LD:
		PosX = ActionStartX - iMoveX;
		PosY = ActionStartY + iMoveY;
		break;
	}


	// -------------------------
	// 2. 계산된 좌표가 화면 이동 영역을 벗어났으면, 벗어난 만큼의 프레임을 구한다.
	// -------------------------
	int RemoveFrame = 0;
	int Value;

	if (PosX <= dfRANGE_MOVE_LEFT)
	{
		Value = abs(dfRANGE_MOVE_LEFT - abs(PosX)) / PLAYER_XMOVE_PIXEL;
		RemoveFrame = max(Value, RemoveFrame);
	}

	if (PosX >= dfRANGE_MOVE_RIGHT)
	{
		Value = abs(dfRANGE_MOVE_RIGHT - abs(PosX)) / PLAYER_XMOVE_PIXEL;
		RemoveFrame = max(Value, RemoveFrame);
	}

	if (PosY <= dfRANGE_MOVE_TOP)
	{
		Value = abs(dfRANGE_MOVE_TOP - abs(PosY)) / PLAYER_YMOVE_PIXEL;
		RemoveFrame = max(Value, RemoveFrame);
	}

	if (PosY >= dfRANGE_MOVE_BOTTOM)
	{
		Value = abs(dfRANGE_MOVE_BOTTOM - abs(PosY)) / PLAYER_YMOVE_PIXEL;
		RemoveFrame = max(Value, RemoveFrame);
	}

	// -------------------------
	// 3. 벗어난 프레임이 있으면, 그만큼 위치 다시 계산
	// -------------------------
	if (RemoveFrame > 0)
	{
		IntervalFrame -= RemoveFrame;

		iMoveX = IntervalFrame * PLAYER_XMOVE_PIXEL;
		iMoveY = IntervalFrame * PLAYER_YMOVE_PIXEL;

		switch (NowAction)
		{
		case dfPACKET_MOVE_DIR_LL:
			PosX = ActionStartX - iMoveX;
			PosY = ActionStartY;
			break;

		case dfPACKET_MOVE_DIR_LU:
			PosX = ActionStartX - iMoveX;
			PosY = ActionStartY - iMoveY;
			break;

		case dfPACKET_MOVE_DIR_UU:
			PosX = ActionStartX;
			PosY = ActionStartY - iMoveY;
			break;

		case dfPACKET_MOVE_DIR_RU:
			PosX = ActionStartX + iMoveX;
			PosY = ActionStartY - iMoveY;
			break;

		case dfPACKET_MOVE_DIR_RR:
			PosX = ActionStartX + iMoveX;
			PosY = ActionStartY;
			break;

		case dfPACKET_MOVE_DIR_RD:
			PosX = ActionStartX + iMoveX;
			PosY = ActionStartY + iMoveY;
			break;

		case dfPACKET_MOVE_DIR_DD:
			PosX = ActionStartX;
			PosY = ActionStartY + iMoveY;
			break;

		case dfPACKET_MOVE_DIR_LD:
			PosX = ActionStartX - iMoveX;
			PosY = ActionStartY + iMoveY;
			break;

		}		
	}

	// -------------------------
	// 4. 모든 계산이 완료되었으면, PosX와 PosY의 값을 맵 안으로 잡는다.
	// -------------------------
	PosX = min(PosX, dfRANGE_MOVE_RIGHT);
	PosX = max(PosX, dfRANGE_MOVE_LEFT);
	PosY = min(PosY, dfRANGE_MOVE_BOTTOM);
	PosY = max(PosY, dfRANGE_MOVE_TOP);

	// -------------------------
	// 5. 인자로받은 아웃변수에 갱신
	// -------------------------
	*ResultX = PosX;
	*ResultY = PosY;

	END("DeadReckoning");
}

// 로그 찍기 함수
void Log(TCHAR *szString, int LogLevel)
{
	_tprintf(L"%s", szString);

	// 로그 레벨이 Warning, Error면 파일로 복사한다.
	if (LogLevel >= dfLOG_LEVEL_WARNING)
	{
		// 현재 년도와, 월 알아오기.
		SYSTEMTIME lst;
		GetLocalTime(&lst);

		// 파일 이름 만들기
		TCHAR tFileName[30];
		swprintf_s(tFileName, _countof(tFileName), L"%d%02d Log.txt", lst.wYear, lst.wMonth);

		// 파일 열기
		FILE *fp;
		_tfopen_s(&fp, tFileName, L"at");

		fwprintf(fp, L"%s", szString);

		fclose(fp);

	}
}








// ---------------------------------
// 섹터처리용 함수
// ---------------------------------
// 섹터 체인지 체크
void SectorChange(stAccount* Account)
{
	BEGIN("SectorChange");

	// 1. 현재 좌표 기준, 섹터 Index 구하기
	WORD SectorX = Account->m_LoginPlayer->m_wNowX / SECTOR_WIDTH;
	WORD SectorY = Account->m_LoginPlayer->m_wNowY / SECTOR_HEIGHT;

	// 2. 기존 섹터와 Index가 같으면 retrun. 섹터가 안바뀐것.
	if (Account->m_LoginPlayer->m_SectorX == SectorX &&
		Account->m_LoginPlayer->m_SectorY == SectorY)
		return;

	// 3. 여기까지 오면 섹터가 바뀐것.
	// 기존 List에서 나 삭제 후, 새로운 List로 이동
	//_LOG(dfLOG_LEVEL_DEBUG, L"[DEBUG] 섹터 변경[%d, %d -> %d, %d]\n", 
	//						Account->m_LoginPlayer->m_SectorY, Account->m_LoginPlayer->m_SectorX, SectorY, SectorX);
	list_Sector[Account->m_LoginPlayer->m_SectorY][Account->m_LoginPlayer->m_SectorX].remove(Account->m_LoginPlayer);
	list_Sector[SectorY][SectorX].push_back(Account->m_LoginPlayer);

	//  캐릭터 삭제, 추가 작업 진행
	CharacterSectorUpdate(Account->m_LoginPlayer, SectorX, SectorY);
	
	// 4. 그리고 내 섹터 갱신
	Account->m_LoginPlayer->m_SectorX = SectorX;
	Account->m_LoginPlayer->m_SectorY = SectorY;

	// 5. 변경된 섹터에 따라 화면 프린트 (디버깅용)
	//Print_Sector(Account->m_LoginPlayer);

	END("SectorChange");
}

// 섹터 기준 9방향 구하기
// 인자로 받은 X,Y를 기준으로, 인자로 받은 구조체에 9방향을 넣어준다.
void GetSector(int SectorX, int SectorY, stSectorCheck* Sector)
{
	BEGIN("GetSector");

	int iCurX, iCurY;

	SectorX--;
	SectorY--;

	Sector->m_dwCount = 0;

	for (iCurY = 0; iCurY < 3; iCurY++)
	{
		if (SectorY + iCurY < 0 || SectorY + iCurY >= SECTOR_HEIGHT_COUNT)
			continue;

		for (iCurX = 0; iCurX < 3; iCurX++)
		{
			if (SectorX + iCurX < 0 || SectorX + iCurX >= SECTOR_WIDTH_COUNT)
				continue;

			Sector->m_Sector[Sector->m_dwCount].x = SectorX + iCurX;
			Sector->m_Sector[Sector->m_dwCount].y = SectorY + iCurY;
			Sector->m_dwCount++;

		}
	}

	END("GetSector");
}

// 다중 섹터에 SendBuff
// 인자로 받은 Sector의 유저들의 SendBuff에 인자로 받은 패킷 넣기
bool SendPacket_Sector(stPlayer* Player, CProtocolBuff* header, CProtocolBuff* payload, stSectorCheck* Sector)
{
	BEGIN("SendPacket_Sector");

	// 1. Sector안의 유저들에게 인자로 받은 패킷을 넣는다.
	for (DWORD i = 0; i < Sector->m_dwCount; ++i)
	{
		// 2. itor는 "i" 번째 섹터의 리스트를 가리킨다.
		list <stPlayer*>::iterator itor = list_Sector[Sector->m_Sector[i].y][Sector->m_Sector[i].x].begin();
		list <stPlayer*>::iterator enditor = list_Sector[Sector->m_Sector[i].y][Sector->m_Sector[i].x].end();

		for (; itor != enditor; ++itor)
		{
			// 3. 만약, 나 자신이라면 안보냄.
			if ((*itor) == Player)
				continue;

			// NowAccount에게 인자로 받은 패킷 전달.
			if (SendPacket((*itor)->m_Account, header, payload) == FALSE)
				return FALSE;
		}
	}

	END("SendPacket_Sector");

	return TRUE;
}

// 1개 섹터에 SendBuff
// 인자로 받은 섹터 1개의 유저들의 SendBuff에 인자로 받은 패킷 넣기.
bool SendPacket_SpecialSector(stPlayer* Player, CProtocolBuff* header, CProtocolBuff* payload, int SectorX, int SectorY)
{
	BEGIN("SendPacket_SpecialSector");

	// 1. 특정 섹터 유저의 SendBuff에 패킷 넣기.
	list <stPlayer*>::iterator itor = list_Sector[SectorY][SectorX].begin();
	list <stPlayer*>::iterator enditor = list_Sector[SectorY][SectorX].end();

	for (; itor != enditor; ++itor)
	{
		// 나에게는 안보낸다!
		if ((*itor) == Player)
			continue;

		// NowAccount에게 인자로 받은 패킷 전달.
		if (SendPacket((*itor)->m_Account, header, payload) == FALSE)
			return FALSE;
	}

	END("SendPacket_SpecialSector");

	return TRUE;
}

// 올드 섹터 9개, 신규섹터 9개 구하기
// 인자로 받은 올드 섹터좌표, 신 섹터좌표를 기준으로 캐릭터를 삭제해야하는 섹터, 캐릭터를 추가해야하는 섹터를 구한다.
void GetUpdateSectorArount(int OldSectorX, int OldSectorY, int NewSectorX, int NewSectorY, stSectorCheck* RemoveSector, stSectorCheck* AddSector)
{
	BEGIN("GetUpdateSectorArount");

	DWORD iCntOld, iCntCur;
	bool bFind;
	
	stSectorCheck OldSector, NewSector;

	OldSector.m_dwCount = 0;
	NewSector.m_dwCount = 0;

	RemoveSector->m_dwCount = 0;
	AddSector->m_dwCount = 0;

	// 올드 섹터와 신규 섹터 좌표 구함.
	GetSector(OldSectorX, OldSectorY, &OldSector);
	GetSector(NewSectorX, NewSectorY, &NewSector);

	// --------------------------------------------------
	// OldSector 중, NewSector에 없는 정보를 찾아서 RemoveSector에 넣음.
	// --------------------------------------------------
	for (iCntOld = 0; iCntOld < OldSector.m_dwCount; ++iCntOld)
	{
		bFind = false;
		for (iCntCur = 0; iCntCur < NewSector.m_dwCount; ++iCntCur)
		{
			if (OldSector.m_Sector[iCntOld].x == NewSector.m_Sector[iCntCur].x &&
				OldSector.m_Sector[iCntOld].y == NewSector.m_Sector[iCntCur].y)
			{
				bFind = true;
				break;
			}
		}
		// bFind가 false면 같은것을 못찾음. 즉, 삭제해야하는 Sector이다.
		if (bFind == false)
		{
			RemoveSector->m_Sector[RemoveSector->m_dwCount] = OldSector.m_Sector[iCntOld];
			RemoveSector->m_dwCount++;			
		}
	}

	// --------------------------------------------------
	// NewSector 중, OldSector에 없는 정보를 찾아서 AddSector에 넣음.
	// --------------------------------------------------
	for (iCntCur = 0; iCntCur < NewSector.m_dwCount; ++iCntCur)
	{
		bFind = false;
		for (iCntOld = 0; iCntOld < OldSector.m_dwCount; ++iCntOld)
		{
			if (OldSector.m_Sector[iCntOld].x == NewSector.m_Sector[iCntCur].x &&
				OldSector.m_Sector[iCntOld].y == NewSector.m_Sector[iCntCur].y)
			{
				bFind = true;
				break;
			}
		}
		// bFind가 false면 같은것을 못찾음. 즉, 추가해야하는 Sector이다.
		if (bFind == false)
		{
			AddSector->m_Sector[AddSector->m_dwCount] = NewSector.m_Sector[iCntCur];
			AddSector->m_dwCount++;
		}
	}

	END("GetUpdateSectorArount");

}

// 섹터가 이동되었을 때 호출되는 함수. 나를 기준으로 캐릭터 삭제/추가를 체크한다.
void CharacterSectorUpdate(stPlayer* NowPlayer, int NewSectorX, int NowSectorY)
{
	BEGIN("CharacterSectorUpdate");

	DWORD iCnt;
	list<stPlayer*> *pSectorList;
	list<stPlayer*>::iterator itor;
	list<stPlayer*>::iterator enditor;

	// RemoveSector와 AddSector 구하기.
	stSectorCheck RemoveSector, AddSector;
	GetUpdateSectorArount(NowPlayer->m_SectorX, NowPlayer->m_SectorY, NewSectorX, NowSectorY, &RemoveSector, &AddSector);

	// --------------------------------
	// RemoveSector에 보내기
	// -------------------------------
	// 1. RemoveSector에 "나 삭제" 패킷 보내기
	CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff payload;

	Network_Send_RemoveOtherCharacteor(&header, &payload, NowPlayer->m_dwPlayerID);

	for (iCnt = 0; iCnt < RemoveSector.m_dwCount; ++iCnt)
	{
		// 섹터 1개의 유저의 SendBuff에 인자로 전달받은 패킷을 넣는다.
		SendPacket_SpecialSector(NowPlayer, &header, &payload, RemoveSector.m_Sector[iCnt].x, RemoveSector.m_Sector[iCnt].y);
	}

	// 2. 나에게 "RemoveSector의 유저들 삭제" 패킷 보내기
	for (iCnt = 0; iCnt < RemoveSector.m_dwCount; ++iCnt)
	{
		pSectorList = &list_Sector[RemoveSector.m_Sector[iCnt].y][RemoveSector.m_Sector[iCnt].x];

		itor = pSectorList->begin();
		enditor = pSectorList->end();

		for (; itor != enditor; ++itor)
		{
			CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
			CProtocolBuff payload;

			Network_Send_RemoveOtherCharacteor(&header, &payload, (*itor)->m_dwPlayerID);

			SendPacket(NowPlayer->m_Account, &header, &payload);
		}
	}

	// --------------------------------
	// AddSector에 보내기
	// -------------------------------
	// 1. AddSector에 "나 생성" 패킷 보내기
	CProtocolBuff Create_header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff Create_payload;

	BYTE Dir = dfPACKET_MOVE_DIR_RR;
	if (NowPlayer->m_byDir == dfPACKET_MOVE_DIR_LL ||
		NowPlayer->m_byDir == dfPACKET_MOVE_DIR_LU ||
		NowPlayer->m_byDir == dfPACKET_MOVE_DIR_UU ||
		NowPlayer->m_byDir == dfPACKET_MOVE_DIR_LD)
	{
		Dir = dfPACKET_MOVE_DIR_LL;
	}

	Network_Send_CreateOtherCharacter(&Create_header, &Create_payload, NowPlayer->m_dwPlayerID,	Dir, NowPlayer->m_wNowX, NowPlayer->m_wNowY, NowPlayer->m_byHP);

	for (iCnt = 0; iCnt < AddSector.m_dwCount; ++iCnt)
	{
		// 섹터 1개의 유저의 SendBuff에 인자로 전달받은 패킷을 넣는다.
		SendPacket_SpecialSector(NowPlayer, &Create_header, &Create_payload, AddSector.m_Sector[iCnt].x, AddSector.m_Sector[iCnt].y);
	}

	// 2. AddSector에 "나 이동" 패킷 보내기
	CProtocolBuff Move_header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff Move_payload;

	Network_Send_MoveStart(&Move_header, &Move_payload, NowPlayer->m_dwPlayerID, NowPlayer->m_byDir, NowPlayer->m_wNowX, NowPlayer->m_wNowY);

	for (iCnt = 0; iCnt < AddSector.m_dwCount; ++iCnt)
	{
		// 섹터 1개의 유저의 SendBuff에 인자로 전달받은 패킷을 넣는다.
		SendPacket_SpecialSector(NowPlayer, &Move_header, &Move_payload, AddSector.m_Sector[iCnt].x, AddSector.m_Sector[iCnt].y);
	}


	// 3. 나에게 AddSector의 캐릭터들 생성 패킷 보내기
	for (iCnt = 0; iCnt < AddSector.m_dwCount; ++iCnt)
	{
		pSectorList = &list_Sector[AddSector.m_Sector[iCnt].y][AddSector.m_Sector[iCnt].x];

		itor = pSectorList->begin();
		enditor = pSectorList->end();

		// 섹터마다 등록된 캐릭터들을 뽑아서, 생성패킷 만든 후 나에게 보냄
		for (; itor != enditor; ++itor)
		{
			// 내가 아닐때만 보냄.
			if ((*itor) == NowPlayer)
				continue;


			// 섹터의 유저 생성패킷 제작 후 나에게 보냄
			CProtocolBuff Create_header(dfNETWORK_PACKET_HEADER_SIZE);
			CProtocolBuff Create_payload;
			BYTE Dir = dfPACKET_MOVE_DIR_RR;
			if ((*itor)->m_byDir == dfPACKET_MOVE_DIR_LL ||
				(*itor)->m_byDir == dfPACKET_MOVE_DIR_LU ||
				(*itor)->m_byDir == dfPACKET_MOVE_DIR_UU ||
				(*itor)->m_byDir == dfPACKET_MOVE_DIR_LD)
			{
				Dir = dfPACKET_MOVE_DIR_LL;
			}

			Network_Send_CreateOtherCharacter(&Create_header, &Create_payload, (*itor)->m_dwPlayerID,
				Dir, (*itor)->m_wNowX, (*itor)->m_wNowY, (*itor)->m_byHP);

			SendPacket(NowPlayer->m_Account, &Create_header, &Create_payload);

			
			// 생성 패킷 보낸 유저가, 이동중이라면 이동패킷 만들어서 보낸다.
			if ((*itor)->m_byNowAction == PLAYER_ACTION_MOVE)
			{
				CProtocolBuff Move_header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;

				Network_Send_MoveStart(&Move_header, &payload, (*itor)->m_dwPlayerID, (*itor)->m_byDir, (*itor)->m_wNowX, (*itor)->m_wNowY);

				SendPacket(NowPlayer->m_Account, &Move_header, &payload);
			}

		}
	}

	END("CharacterSectorUpdate");
}

// 내 기준, 9방향의 Index, 인원 수를 찍는 함수. 디버깅용
void Print_Sector(stPlayer* NowPlayer)
{
	// 1. 내가 위치한 섹터의 Index 알아오기
	WORD SectorX = NowPlayer->m_SectorX;
	WORD SectorY = NowPlayer->m_SectorY;

	// 2. 이 인덱스 기준, 9방향 Index의 유저 수 출력
	// 위 1줄
	printf("[%d,%d : %zd]   [%d,%d : %zd]   [%d,%d : %zd]  \n",
		SectorY - 1, SectorX - 1, list_Sector[SectorY - 1][SectorX - 1].size(),
		SectorY - 1, SectorX - 0, list_Sector[SectorY - 1][SectorX - 0].size(),
		SectorY - 1, SectorX + 1, list_Sector[SectorY - 1][SectorX + 1].size()
	);

	// 중간 1줄
	printf("[%d,%d : %zd]   [%d,%d : %zd]   [%d,%d : %zd]  \n",
		SectorY - 0, SectorX - 1, list_Sector[SectorY - 0][SectorX - 1].size(),
		SectorY - 0, SectorX - 0, list_Sector[SectorY - 0][SectorX - 0].size(),
		SectorY - 0, SectorX + 1, list_Sector[SectorY - 0][SectorX + 1].size()
	);

	// 아래 1줄
	printf("[%d,%d : %zd]   [%d,%d : %zd]   [%d,%d : %zd]  \n\n",
		SectorY + 1, SectorX - 1, list_Sector[SectorY + 1][SectorX - 1].size(),
		SectorY + 1, SectorX - 0, list_Sector[SectorY + 1][SectorX - 0].size(),
		SectorY + 1, SectorX + 1, list_Sector[SectorY + 1][SectorX + 1].size()
	);

}







// ---------------------------------
// Update()에서 유저 액션 처리 함수
// ---------------------------------
// 각 플레이어 액션 체크
void ActionProc()
{
	BEGIN("ActionProc");

	// 접속중인 모든 플레이어를 체크하며 행동을 한다.
	map <SOCKET, stAccount*>::iterator	itor;
	map <SOCKET, stAccount*>::iterator	enditor;

	enditor = map_Account.end();

	// 1. 현재 유저의 액션에 따라 처리
	for (itor = map_Account.begin(); itor != enditor; ++itor)
	{
		// 차후 다른 처리가 들어갈 수도 있으니 switch case 문으로 처리
		switch (itor->second->m_LoginPlayer->m_byNowAction)
		{
			// 이동 액션
		case PLAYER_ACTION_MOVE:
			// 이동 중일때는 1프레임마다 정해진 방향을 정해진 수치만큼 이동
			Action_Move(itor->second->m_LoginPlayer);

			// 이동 후 좌표에 따라 섹터 이동여부 체크
			SectorChange(itor->second);
			
			break;

		default:
			break;
		}

		// 처리 후, 현재 액션을 이전 액션으로 변경한다. 그래야, 다음 처리 시 체크 가능
		itor->second->m_LoginPlayer->m_byOldAction = itor->second->m_LoginPlayer->m_byNowAction;
	}

	END("ActionProc");
}

// 유저 이동 액션 처리
void Action_Move(stPlayer* NowPlayer)
{
	BEGIN("Action_Move");

	// 1. 방향에 따라 X,Y값 이동
	switch (NowPlayer->m_byDir)
	{
		// LL
	case dfPACKET_MOVE_DIR_LL:
		NowPlayer->m_wNowX -= PLAYER_XMOVE_PIXEL;

		// 일정이상 나가지 못하게 제한
		if (NowPlayer->m_wNowX < dfRANGE_MOVE_LEFT)
			NowPlayer->m_wNowX = dfRANGE_MOVE_LEFT;
		break;

		// LU
	case dfPACKET_MOVE_DIR_LU:
		// 현재 Y좌표가 dfRANGE_MOVE_TOP(벽의 위 끝)이라면, x좌표 갱신하지 않음
		if (NowPlayer->m_wNowY != dfRANGE_MOVE_TOP)
		{
			NowPlayer->m_wNowX -= PLAYER_XMOVE_PIXEL;
			if (NowPlayer->m_wNowX < dfRANGE_MOVE_LEFT)
				NowPlayer->m_wNowX = dfRANGE_MOVE_LEFT;
		}

		// 현재, X좌표가 dfRANGE_MOVE_LEFT(벽의 왼쪽 끝)이라면, Y좌표는 갱신하지 않음. 
		if (NowPlayer->m_wNowX != dfRANGE_MOVE_LEFT)
		{
			NowPlayer->m_wNowY -= PLAYER_YMOVE_PIXEL;
			if (NowPlayer->m_wNowY < dfRANGE_MOVE_TOP)
				NowPlayer->m_wNowY = dfRANGE_MOVE_TOP;
		}

		break;

		// UU
	case dfPACKET_MOVE_DIR_UU:
		NowPlayer->m_wNowY -= PLAYER_YMOVE_PIXEL;
		if (NowPlayer->m_wNowY < dfRANGE_MOVE_TOP)
			NowPlayer->m_wNowY = dfRANGE_MOVE_TOP;
		break;

		// RU
	case dfPACKET_MOVE_DIR_RU:
		// 현재 Y좌표가 dfRANGE_MOVE_TOP(벽의 위 끝)이라면, x좌표 갱신하지 않음
		if (NowPlayer->m_wNowY != dfRANGE_MOVE_TOP)
		{
			NowPlayer->m_wNowX += PLAYER_XMOVE_PIXEL;
			if (NowPlayer->m_wNowX > dfRANGE_MOVE_RIGHT)
				NowPlayer->m_wNowX = dfRANGE_MOVE_RIGHT;
		}

		// 만약, X좌표가 dfRANGE_MOVE_RIGHT(벽의 오른쪽 끝)이라면, Y좌표는 갱신하지 않음. 
		if (NowPlayer->m_wNowX != dfRANGE_MOVE_RIGHT)
		{
			NowPlayer->m_wNowY -= PLAYER_YMOVE_PIXEL;
			if (NowPlayer->m_wNowY < dfRANGE_MOVE_TOP)
				NowPlayer->m_wNowY = dfRANGE_MOVE_TOP;
		}
		break;

		// RR
	case dfPACKET_MOVE_DIR_RR:
		NowPlayer->m_wNowX += PLAYER_XMOVE_PIXEL;
		if (NowPlayer->m_wNowX > dfRANGE_MOVE_RIGHT)
			NowPlayer->m_wNowX = dfRANGE_MOVE_RIGHT;
		break;

		// RD
	case dfPACKET_MOVE_DIR_RD:
		// 현재 Y좌표가 dfRANGE_MOVE_BOTTOM(벽의 바닥 끝)이라면, X좌표 갱신하지 않음
		if (NowPlayer->m_wNowY != dfRANGE_MOVE_BOTTOM)
		{
			NowPlayer->m_wNowX += PLAYER_XMOVE_PIXEL;
			if (NowPlayer->m_wNowX > dfRANGE_MOVE_RIGHT)
				NowPlayer->m_wNowX = dfRANGE_MOVE_RIGHT;
		}

		// 만약, X좌표가 dfRANGE_MOVE_RIGHT(벽의 오른쪽 끝)이라면, Y좌표는 갱신하지 않음. 
		if (NowPlayer->m_wNowX != dfRANGE_MOVE_RIGHT)
		{
			NowPlayer->m_wNowY += PLAYER_YMOVE_PIXEL;
			if (NowPlayer->m_wNowY > dfRANGE_MOVE_BOTTOM)
				NowPlayer->m_wNowY = dfRANGE_MOVE_BOTTOM;
		}
		break;

		// DD
	case dfPACKET_MOVE_DIR_DD:
		NowPlayer->m_wNowY += PLAYER_YMOVE_PIXEL;
		if (NowPlayer->m_wNowY > dfRANGE_MOVE_BOTTOM)
			NowPlayer->m_wNowY = dfRANGE_MOVE_BOTTOM;
		break;

		// LD
	case dfPACKET_MOVE_DIR_LD:
		// 현재 Y좌표가 dfRANGE_MOVE_BOTTOM(벽의 바닥 끝)이라면, X좌표 갱신하지 않음
		if (NowPlayer->m_wNowY != dfRANGE_MOVE_BOTTOM)
		{
			NowPlayer->m_wNowX -= PLAYER_XMOVE_PIXEL;
			if (NowPlayer->m_wNowX < dfRANGE_MOVE_LEFT)
				NowPlayer->m_wNowX = dfRANGE_MOVE_LEFT;
		}

		// 만약, X좌표가 dfRANGE_MOVE_LEFT(벽의 왼쪽 끝)이라면, Y좌표는 갱신하지 않음.
		if (NowPlayer->m_wNowX != dfRANGE_MOVE_LEFT)
		{
			NowPlayer->m_wNowY += PLAYER_YMOVE_PIXEL;
			if (NowPlayer->m_wNowY > dfRANGE_MOVE_BOTTOM)
				NowPlayer->m_wNowY = dfRANGE_MOVE_BOTTOM;
		}
		break;

	default:
		printf("Action_Move() 알수 없는 방향.(PlayerID: %d)\n", NowPlayer->m_dwPlayerID);
		break;
	}

	END("Action_Move");

	//printf("X : %d / Y : %d\n", NowPlayer->m_wNowX, NowPlayer->m_wNowY);
}








// ---------------------------------
// 검색용 함수
// --------------------------------
// 인자로 받은 Socket 값을 기준으로 [회원 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stAccount* ClientSearch_AcceptList(SOCKET sock)
{
	BEGIN("ClientSearch_AcceptList");

	stAccount* NowAccount = nullptr;
	map <SOCKET, stAccount*>::iterator iter;

	iter = map_Account.find(sock);
	if (iter == map_Account.end())
		return nullptr;

	NowAccount = iter->second;

	END("ClientSearch_AcceptList");
	return NowAccount;
}








// ---------------------------------
// 패킷 생성 함수
// ---------------------------------
// 헤더 제작 함수
void CreateHeader(CProtocolBuff* header, BYTE PayloadSize, BYTE PacketType)
{
	// 패킷 코드 (1Byte)
	*header << dfNETWORK_PACKET_CODE;

	// 페이로드 사이즈 (1Byte)
	*header << PayloadSize;

	// 패킷 타입 (1Byte)
	*header << PacketType;

	// 사용안함 (1Byte)
	BYTE temp = 12;
	*header << temp;
}

// "내 캐릭터 생성" 패킷 제작
void Network_Send_CreateMyCharacter(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y, BYTE HP)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << Dir;
	*payload << X;
	*payload << Y;
	*payload << HP;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_CREATE_MY_CHARACTER);
}

// "다른 사람 캐릭터 생성" 패킷 제작
void Network_Send_CreateOtherCharacter(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y, BYTE HP)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << Dir;
	*payload << X;
	*payload << Y;
	*payload << HP;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_CREATE_OTHER_CHARACTER);
}

// "다른 사람 삭제" 패킷 제작
void Network_Send_RemoveOtherCharacteor(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID)
{
	// 1. 페이로드 제작
	*payload << ID;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_DELETE_CHARACTER);
}

// "다른 사람 이동 시작" 패킷 제작
void Network_Send_MoveStart(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_MOVE_START);
}

// "다른 사람 정지" 패킷 제작
void Network_Send_MoveStop(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_MOVE_STOP);
}

// "싱크 맞추기" 패킷 제작
void Network_Send_Sync(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, WORD X, WORD Y)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << X;
	*payload << Y;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_SYNC);
}

// "1번 공격 시작" 패킷 만들기
void Network_Send_Attack_1(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_ATTACK1);
}

// "2번 공격 시작" 패킷 만들기
void Network_Send_Attack_2(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_ATTACK2);

}

// "3번 공격 시작" 패킷 만들기
void Network_Send_Attack_3(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y)
{
	// 1. 페이로드 제작
	*payload << ID;
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_ATTACK3);

}

// "데미지" 패킷 만들기
void Network_Send_Damage(CProtocolBuff *header, CProtocolBuff* payload, DWORD AttackID, DWORD DamageID, BYTE HP)
{
	// 1. 페이로드 제작
	*payload << AttackID;
	*payload << DamageID;
	*payload << HP;

	// 2. 헤더 제작
	CreateHeader(header, payload->GetUseSize(), dfPACKET_SC_DAMAGE);
}







// ---------------------------------
// Recv 처리 함수들
// ---------------------------------
// Recv() 처리
bool RecvProc(stAccount* Account)
{
	if (Account == nullptr)
		return false;

	////////////////////////////////////////////////////////////////////////
	// 소켓 버퍼에 있는 모든 recv 데이터를, 해당 유저의 recv링버퍼로 리시브.
	////////////////////////////////////////////////////////////////////////
	
	BEGIN("RecvProc");

	// 1. recv 링버퍼 포인터 얻어오기.
	char* recvbuff = Account->m_RecvBuff.GetBufferPtr();

	// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = Account->m_RecvBuff.GetNotBrokenPutSize();

	// 3. 한 번에 쓸 수 있는 공간이 0이라면
	if (Size == 0)
	{
		// rear 1칸 이동
		Account->m_RecvBuff.MoveWritePos(1);

		// 그리고 한 번에 쓸 수 있는 위치 다시 알기.
		Size = Account->m_RecvBuff.GetNotBrokenPutSize();
	}

	// 4. 그게 아니라면, 1칸 이동만 하기. 		
	else
	{
		// rear 1칸 이동
		Account->m_RecvBuff.MoveWritePos(1);		
	}

	// 5. 1칸 이동된 rear 얻어오기
	int* rear = (int*)Account->m_RecvBuff.GetWriteBufferPtr();	

	// 6. recv()
	int retval = recv(Account->m_sock, &recvbuff[*rear], Size, 0);	

	// 7. 리시브 에러체크
	if (retval == SOCKET_ERROR)
	{		
		// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있다는 것. recv할게 없다는 것이니 함수 종료.
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;

		// 10053, 10054 둘다 어쩃든 연결이 끊어진 상태
		// WSAECONNABORTED(10053) :  프로토콜상의 에러나 타임아웃에 의해 연결(가상회선. virtual circle)이 끊어진 경우
		// WSAECONNRESET(10054) : 원격 호스트가 접속을 끊음.
		// 이 때는 그냥 return false로 나도 상대방과 연결을 끊는다.
		if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)
			return false;

		// WSAEWOULDBLOCK에러가 아니면 뭔가 이상한 것이니 접속 종료
		else
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]RecvProc(). retval 후 우드블럭이 아닌 에러가 뜸(%d). 접속 종료\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, WSAGetLastError());
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// 0이 리턴되는 것은 정상종료.
	else if (retval == 0)
		return false;

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	Account->m_RecvBuff.MoveWritePos(retval - 1);


	//////////////////////////////////////////////////
	// 완료 패킷 처리 부분
	// RecvBuff에 들어있는 모든 완성 패킷을 처리한다.
	//////////////////////////////////////////////////
	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
		st_PACKET_HEADER HeaderBuff;
		int len = sizeof(HeaderBuff);

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (Account->m_RecvBuff.GetUseSize() < len)
			break;

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = Account->m_RecvBuff.Peek((char*)&HeaderBuff, len);
		if (PeekSize == -1)
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]RecvProc(). 완료패킷 헤더 처리 중 RecvBuff비었음. 접속 종료\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 3. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size + endCode(1바이트))
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (Account->m_RecvBuff.GetUseSize() < (len + HeaderBuff.bySize + 1))
			break;

		// 4. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (HeaderBuff.byCode != dfNETWORK_PACKET_CODE)
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]RecvProc(). 완료패킷 헤더 처리 중 PacketCode틀림. 접속 종료\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 PacketCode틀림. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}		

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		Account->m_RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.bySize;

		int DequeueSize = Account->m_RecvBuff.Dequeue(PayloadBuff.GetBufferPtr(), PayloadSize);
		if (DequeueSize == -1)
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]RecvProc(). 완료패킷 페이로드 처리 중 RecvBuff비었음. 접속 종료\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}
		PayloadBuff.MoveWritePos(DequeueSize);
		

		// 7. RecvBuff에서 엔드코드 1Byte뽑음.	(디큐이다. Peek 아님)
		BYTE EndCode;
		DequeueSize = Account->m_RecvBuff.Dequeue((char*)&EndCode, 1);
		if (DequeueSize == -1)
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]RecvProc(). 완료패킷 엔드코드 처리 중 RecvBuff비었음. 접속 종료\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}			

		// 8. 엔드코드 확인
		if (EndCode != dfNETWORK_PACKET_END)
		{			
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]RecvProc(). 완료패킷 엔드코드 처리 중 정상패킷 아님. 접속 종료\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}		

		// 7. 헤더에 들어있는 타입에 따라 분기처리.
		bool check = PacketProc(HeaderBuff.byType, Account, &PayloadBuff);
		if (check == false)
			return false;	
	}

	END("RecvProc");
	return true;
}

// 패킷 처리
// PacketProc() 함수에서 false가 리턴되면 해당 유저는 접속이 끊긴다.
bool PacketProc(WORD PacketType, stAccount* Account, CProtocolBuff* Payload)
{
	/*_LOG(dfLOG_LEVEL_DEBUG, L"[DEBUG] PacketRecv [PlayerID : %d / PacketType : %d]\n",
		Account->m_LoginPlayer->m_dwPlayerID, PacketType);*/
	
	bool check = true;

	try
	{
		switch (PacketType)
		{

			// 이동시작 패킷
		case dfPACKET_CS_MOVE_START:
			{
				check = Network_Req_MoveStart(Account, Payload);
				if (check == false)
					return false;	// 해당 유저 접속 끊김
			}
			break;	

			// 정지
		case dfPACKET_CS_MOVE_STOP:
		{
			check = Network_Req_MoveStop(Account, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 공격1
		case dfPACKET_CS_ATTACK1:
		{
			check = Network_Req_Attack_1(Account, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 공격2
		case dfPACKET_CS_ATTACK2:
		{
			check = Network_Req_Attack_2(Account, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 공격3
		case dfPACKET_CS_ATTACK3:
		{
			check = Network_Req_Attack_3(Account, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 에코
		case dfPACKET_CS_ECHO:
		{
			check = Network_Req_StressTest(Account, Payload);
			if(check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;
		

		default:
			_tprintf(_T("이상한 패킷입니다.\n"));
			//return false;
			break;
		}


	}
	catch (CException exc)
	{
		TCHAR* text = (TCHAR*)exc.GetExceptionText();
		_tprintf(_T("%s.\n"), text);
		return false;

	}

	return true;
}

// "캐릭터 이동 시작" 패킷 처리
bool Network_Req_MoveStart(stAccount* Account, CProtocolBuff* payload)
{
	// ----------------------------------
	// 1. 유저가 보낸 Dir, X, Y 얻어오기
	// ----------------------------------
	BYTE Dir;
	WORD X;
	WORD Y;

	*payload >> Dir;
	*payload >> X;
	*payload >> Y;

	
	// ----------------------------------
	// 2, 데드레커닝 여부 체크
	// 유저가 보낸 x,y와 서버의 x,y 너무 큰 차이가 나면, 데드레커닝 체크	
	// ----------------------------------
	if (abs(Account->m_LoginPlayer->m_wNowX - X) > dfERROR_RANGE ||
		abs(Account->m_LoginPlayer->m_wNowY - Y) > dfERROR_RANGE)
	{
		int resultX, resultY;

		if (Dir == dfPACKET_MOVE_DIR_RU)
			int abc = 10;

		// 데드레커닝 하기
		DeadReckoning(Account->m_LoginPlayer->m_byDir, Account->m_LoginPlayer->m_dwACtionTick,
			Account->m_LoginPlayer->m_wActionX, Account->m_LoginPlayer->m_wActionY, &resultX, &resultY);

		// 보정된 좌표로 재확인. 그래도 차이가 많이난다면, 싱크패킷 보냄
		if (abs(resultX - X) > dfERROR_RANGE ||
			abs(resultY - Y) > dfERROR_RANGE)
		{
			// Sync 패킷 만들기
			CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
			CProtocolBuff payload;
			Network_Send_Sync(&header, &payload, Account->m_LoginPlayer->m_dwPlayerID, resultX, resultY);
			
			// 나에게 보내기
			if (SendPacket(Account, &header, &payload) == FALSE)
				return FALSE;

			SYSTEMTIME lst;
			GetLocalTime(&lst);

			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR] MoverStart Sync! [%d, %d] -> [%d, %d]\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, X, Y, resultX, resultY);
		}

		// 데드레커닝 결과로 서버의 X,Y 셋팅
		Account->m_LoginPlayer->m_wNowX = resultX;
		Account->m_LoginPlayer->m_wNowY = resultY;

	}

	// 데드레커닝을 안했다면, 기존의 X/Y로 좌표 갱신. 싱크도 보낼 필요 없음.
	else
	{
		Account->m_LoginPlayer->m_wNowX = X;
		Account->m_LoginPlayer->m_wNowY = Y;
	}

	// ----------------------------------
	// 3. 좌표 외 정보 갱신
	// ----------------------------------
	Account->m_LoginPlayer->m_byDir = Dir;
	Account->m_LoginPlayer->m_byNowAction = PLAYER_ACTION_MOVE;

	// 데드레커닝용 정보 갱신
	Account->m_LoginPlayer->m_wActionX = X;
	Account->m_LoginPlayer->m_wActionY = Y;
	Account->m_LoginPlayer->m_dwACtionTick = GetTickCount64();
		
	
	// 3.  서버의 X,Y를 가지고, 해당 유저의 주변 섹터 9개 유저의 SendBuff에 "다른 캐릭터 이동 시작" 패킷 넣기. (자기 자신한테는 안보냄)
	CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff payload_2;
	Network_Send_MoveStart(&header, &payload_2, Account->m_LoginPlayer->m_dwPlayerID, Dir, X, Y);

	// 현재 좌표 기준, 섹터 Index 구하기
	int SectorX = X / SECTOR_WIDTH;
	int SectorY = Y / SECTOR_HEIGHT; 

	stSectorCheck secSector;
	GetSector(SectorX, SectorY, &secSector);

	SendPacket_Sector(Account->m_LoginPlayer, &header, &payload_2, &secSector);
	

	return true;
}

// "캐릭터 이동 정지" 패킷 처리
bool Network_Req_MoveStop(stAccount* Account, CProtocolBuff* payload)
{
	// ----------------------------------
	// 1. 해당 유저의 Dir, X, Y를 꺼낸다
	// ----------------------------------
	BYTE Dir;
	WORD X;
	WORD Y;

	*payload >> Dir;
	*payload >> X;
	*payload >> Y;


	// ----------------------------------
	// 2, 데드레커닝 여부 체크
	// 유저가 보낸 x,y와 서버의 x,y 너무 큰 차이가 나면, 데드레커닝 체크	
	// ----------------------------------
	if (abs(Account->m_LoginPlayer->m_wNowX - X) > dfERROR_RANGE ||
		abs(Account->m_LoginPlayer->m_wNowY - Y) > dfERROR_RANGE)
	{
		int resultX, resultY;

		// 데드레커닝 하기
		DeadReckoning(Account->m_LoginPlayer->m_byDir, Account->m_LoginPlayer->m_dwACtionTick,
			Account->m_LoginPlayer->m_wActionX, Account->m_LoginPlayer->m_wActionY, &resultX, &resultY);

		// 보정된 좌표로 재확인. 그래도 차이가 많이난다면, 싱크패킷 보냄
		if (abs(resultX - X) > dfERROR_RANGE ||
			abs(resultY - Y) > dfERROR_RANGE)
		{
			// Sync 패킷 만들기
			CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
			CProtocolBuff payload;
			Network_Send_Sync(&header, &payload, Account->m_LoginPlayer->m_dwPlayerID, resultX, resultY);
			
			// 나에게 보내기
			if (SendPacket(Account, &header, &payload) == FALSE)
				return FALSE;			

			SYSTEMTIME lst;
			GetLocalTime(&lst);

			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR] MoverStop Sync! [%d, %d] -> [%d, %d]\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, X, Y, resultX, resultY);

		}

		// 데드레커닝 결과로 서버의 X,Y 셋팅
		Account->m_LoginPlayer->m_wNowX = resultX;
		Account->m_LoginPlayer->m_wNowY = resultY;
		
	}

	// 데드레커닝을 안했다면, 기존의 X/Y로 좌표 갱신. 싱크도 보낼 필요 없음.
	else
	{
		Account->m_LoginPlayer->m_wNowX = X;
		Account->m_LoginPlayer->m_wNowY = Y;
	}


	// ----------------------------------
	// 3. 방향 셋팅 후, 현재 상태를 정지로 변경
	// ----------------------------------
	Account->m_LoginPlayer->m_byDir = Dir;
	Account->m_LoginPlayer->m_byNowAction = PLAYER_ACTION_IDLE;



	// ----------------------------------
	// 4. 해당 유저 주변 9개 섹터에 있는 유저들에게 정지패킷 보내기. (나 자신 제외)
	// ----------------------------------
	CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff payload_2;
	Network_Send_MoveStop(&header, &payload_2, Account->m_LoginPlayer->m_dwPlayerID, Dir, X, Y);
	
	// 현재 좌표 기준, 섹터 Index 구하기
	int SectorX = Account->m_LoginPlayer->m_wNowX / SECTOR_WIDTH;
	int SectorY = Account->m_LoginPlayer->m_wNowY / SECTOR_HEIGHT;

	// 주변 9섹터 구하기
	stSectorCheck secSector;
	GetSector(SectorX, SectorY, &secSector);

	// 보내기
	SendPacket_Sector(Account->m_LoginPlayer, &header, &payload_2, &secSector);
	
	return true;
}

// "공격 1번" 패킷 처리
bool Network_Req_Attack_1(stAccount* Account, CProtocolBuff* payload)
{
	// ---------------------------------
	// 공격 방향, X,Y 빼오기. 방향은 좌우만 받음
	// ---------------------------------
	BYTE Dir;
	WORD X, Y;

	*payload >> Dir;
	*payload >> X;
	*payload >> Y;

	// 일단 공격자를 Idle상태로 만든다.
	Account->m_LoginPlayer->m_byNowAction = PLAYER_ACTION_IDLE;

	// ---------------------------------
	// 공격자 기준, 9방향에 섹터에 "1번 공격 시작" 패킷을 보낸다.
	// ---------------------------------	
	// 공격 시작 패킷을 만든다.
	CProtocolBuff Attack_Header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff Attack_Payload;
	Network_Send_Attack_1(&Attack_Header, &Attack_Payload, Account->m_LoginPlayer->m_dwPlayerID, Dir, X, Y);

	// 공격자 기준 9방향 섹터에 공격시작 패킷을 보낸다. (공격자 자신 제외)
	stSectorCheck secSector;
	GetSector(Account->m_LoginPlayer->m_SectorX, Account->m_LoginPlayer->m_SectorY, &secSector);

	if (SendPacket_Sector(Account->m_LoginPlayer, &Attack_Header, &Attack_Payload, &secSector) == FALSE)
		return FALSE;
	
	// ---------------------------------
	// 데미지 패킷 처리
	// ---------------------------------
	if (Network_Requ_Attck_1_Damage(Account->m_LoginPlayer->m_dwPlayerID, Account->m_LoginPlayer->m_wNowX, Account->m_LoginPlayer->m_wNowY, Dir) == FALSE)
		return FALSE;


	return TRUE;
}

// "공격 2번" 패킷 처리
bool Network_Req_Attack_2(stAccount* Account, CProtocolBuff* payload)
{
	// ---------------------------------
	// 공격 방향, X,Y 빼오기. 방향은 좌우만 받음
	// ---------------------------------
	BYTE Dir;
	WORD X, Y;

	*payload >> Dir;
	*payload >> X;
	*payload >> Y;

	// 일단 공격자를 Idle상태로 만든다.
	Account->m_LoginPlayer->m_byNowAction = PLAYER_ACTION_IDLE;


	// ---------------------------------
	// 공격자 기준, 9방향에 섹터에 "2번 공격 시작" 패킷을 보낸다.
	// ---------------------------------	
	// 공격 시작 패킷을 만든다.
	CProtocolBuff Attack_Header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff Attack_Payload;
	Network_Send_Attack_2(&Attack_Header, &Attack_Payload, Account->m_LoginPlayer->m_dwPlayerID, Dir, X, Y);

	// 공격자 기준 9방향 섹터에 공격시작 패킷을 보낸다. (공격자 자신 제외)
	stSectorCheck secSector;
	GetSector(Account->m_LoginPlayer->m_SectorX, Account->m_LoginPlayer->m_SectorY, &secSector);

	if (SendPacket_Sector(Account->m_LoginPlayer, &Attack_Header, &Attack_Payload, &secSector) == FALSE)
		return FALSE;

	// ---------------------------------
	// 데미지 패킷 처리
	// ---------------------------------
	if (Network_Requ_Attck_2_Damage(Account->m_LoginPlayer->m_dwPlayerID, Account->m_LoginPlayer->m_wNowX, Account->m_LoginPlayer->m_wNowY, Dir) == FALSE)
		return FALSE;

	return TRUE;
}

// "공격 3번" 패킷 처리
bool Network_Req_Attack_3(stAccount* Account, CProtocolBuff* payload)
{
	// ---------------------------------
	// 공격 방향, X,Y 빼오기. 방향은 좌우만 받음
	// ---------------------------------
	BYTE Dir;
	WORD X, Y;

	*payload >> Dir;
	*payload >> X;
	*payload >> Y;

	// 일단 공격자를 Idle상태로 만든다.
	Account->m_LoginPlayer->m_byNowAction = PLAYER_ACTION_IDLE;


	// ---------------------------------
	// 공격자 기준, 9방향에 섹터에 "3번 공격 시작" 패킷을 보낸다.
	// ---------------------------------	
	// 공격 시작 패킷을 만든다.
	CProtocolBuff Attack_Header(dfNETWORK_PACKET_HEADER_SIZE);
	CProtocolBuff Attack_Payload;
	Network_Send_Attack_3(&Attack_Header, &Attack_Payload, Account->m_LoginPlayer->m_dwPlayerID, Dir, X, Y);

	// 공격자 기준 9방향 섹터에 공격시작 패킷을 보낸다. (공격자 자신 제외)
	stSectorCheck secSector;
	GetSector(Account->m_LoginPlayer->m_SectorX, Account->m_LoginPlayer->m_SectorY, &secSector);

	if (SendPacket_Sector(Account->m_LoginPlayer, &Attack_Header, &Attack_Payload, &secSector) == FALSE)
		return FALSE;

	// ---------------------------------
	// 데미지 패킷 처리
	// ---------------------------------
	if (Network_Requ_Attck_3_Damage(Account->m_LoginPlayer->m_dwPlayerID, Account->m_LoginPlayer->m_wNowX, Account->m_LoginPlayer->m_wNowY, Dir) == FALSE)
		return FALSE;

	return TRUE;
}

// "공격 1번" 패킷의 데미지 처리. Network_Req_Attack_1()에서 사용.
bool Network_Requ_Attck_1_Damage(DWORD AttackID, int AttackX, int AttackY, BYTE AttackDir)
{
	// 공격 Y좌표를 캐릭터의 중간쯤으로 갱신
	AttackY -= dfATTACK_Y;

	// 공격 방향에 다르게 처리.
	map<SOCKET, stAccount*>::iterator itor = map_Account.begin();
	map<SOCKET, stAccount*>::iterator enditor = map_Account.end();

	switch (AttackDir)
	{
	case dfPACKET_MOVE_DIR_LL:
	{
		for (; itor != enditor; ++itor)
		{
			// 공격자의 공격범위에 피해자가 있는지 체크
			// 공격 범위에 있으면, 피해자 기준 9방향에 데미지 패킷을 보낸다.
			if (itor->second->m_LoginPlayer->m_wNowX > AttackX - dfATTACK1_RANGE_X &&
				itor->second->m_LoginPlayer->m_wNowX < AttackX &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y > AttackY - dfATTACK1_RANGE_Y &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y < AttackY + dfATTACK1_RANGE_Y)
			{
				// 피해자의 HP 감소시키기.
				itor->second->m_LoginPlayer->m_byHP -= dfATTACK1_DAMAGE;

				// 데미지 패킷 만들기
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_Damage(&header, &payload, AttackID, itor->second->m_LoginPlayer->m_dwPlayerID, itor->second->m_LoginPlayer->m_byHP);

				// 피해자 기준 9방향 유저에게 보내기 (피해자 자신에게는 안간다.)
				stSectorCheck secSector;
				GetSector(itor->second->m_LoginPlayer->m_SectorX, itor->second->m_LoginPlayer->m_SectorY, &secSector);

				SendPacket_Sector(itor->second->m_LoginPlayer, &header, &payload, &secSector);

				// 피해자 자신에게 보내기
				if (SendPacket(itor->second, &header, &payload) == FALSE)
					return FALSE;

				// 피해자의 남은 Hp가 0이라면 접속 종료시킨다.
				//if (itor->second->m_LoginPlayer->m_byHP == 0)
					//Disconnect(itor->second);


				break;
			}

		}

	}
	break;

	case dfPACKET_MOVE_DIR_RR:
	{
		for (; itor != enditor; ++itor)
		{
			// 공격자의 공격범위에 피해자가 있는지 체크
			// 공격 범위에 있으면, 피해자 기준 9방향에 데미지 패킷을 보낸다.
			if (itor->second->m_LoginPlayer->m_wNowX < AttackX + dfATTACK1_RANGE_X &&
				itor->second->m_LoginPlayer->m_wNowX > AttackX &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y > AttackY - dfATTACK1_RANGE_Y &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y < AttackY + dfATTACK1_RANGE_Y)
			{
				// 피해자의 HP 감소시키기.
				itor->second->m_LoginPlayer->m_byHP -= dfATTACK1_DAMAGE;

				// 데미지 패킷 만들기
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_Damage(&header, &payload, AttackID, itor->second->m_LoginPlayer->m_dwPlayerID, itor->second->m_LoginPlayer->m_byHP);

				// 피해자 기준 9방향 유저에게 보내기 (피해자 자신에게는 안간다.)
				stSectorCheck secSector;
				GetSector(itor->second->m_LoginPlayer->m_SectorX, itor->second->m_LoginPlayer->m_SectorY, &secSector);

				if (SendPacket_Sector(itor->second->m_LoginPlayer, &header, &payload, &secSector) == FALSE)
					return FALSE;

				// 피해자 자신에게 보내기
				if (SendPacket(itor->second->m_LoginPlayer->m_Account, &header, &payload) == FALSE)
					return FALSE;

				// 피해자의 남은 Hp가 0이라면 접속 종료시킨다.
				//if (itor->second->m_LoginPlayer->m_byHP == 0)
					//Disconnect(itor->second);

				break;
			}

		}
	}
	break;

	}


	return TRUE;
}

// "공격 2번" 패킷의 데미지 처리. Network_Req_Attack_2()에서 사용.
bool Network_Requ_Attck_2_Damage(DWORD AttackID, int AttackX, int AttackY, BYTE AttackDir)
{
	// 공격 Y좌표를 캐릭터의 중간쯤으로 갱신
	AttackY -= dfATTACK_Y;

	// 공격 방향에 다르게 처리.
	map<SOCKET, stAccount*>::iterator itor = map_Account.begin();
	map<SOCKET, stAccount*>::iterator enditor = map_Account.end();

	switch (AttackDir)
	{
	case dfPACKET_MOVE_DIR_LL:
	{
		for (; itor != enditor; ++itor)
		{
			// 공격자의 공격범위에 피해자가 있는지 체크
			// 공격 범위에 있으면, 피해자 기준 9방향에 데미지 패킷을 보낸다.
			if (itor->second->m_LoginPlayer->m_wNowX > AttackX - dfATTACK2_RANGE_X &&
				itor->second->m_LoginPlayer->m_wNowX < AttackX &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y > AttackY - dfATTACK2_RANGE_Y &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y < AttackY + dfATTACK2_RANGE_Y)
			{
				// 피해자의 HP 감소시키기.
				itor->second->m_LoginPlayer->m_byHP -= dfATTACK2_DAMAGE;

				// 데미지 패킷 만들기
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_Damage(&header, &payload, AttackID, itor->second->m_LoginPlayer->m_dwPlayerID, itor->second->m_LoginPlayer->m_byHP);

				// 피해자 기준 9방향 유저에게 보내기 (피해자 자신에게는 안간다.)
				stSectorCheck secSector;
				GetSector(itor->second->m_LoginPlayer->m_SectorX, itor->second->m_LoginPlayer->m_SectorY, &secSector);

				SendPacket_Sector(itor->second->m_LoginPlayer, &header, &payload, &secSector);

				// 피해자 자신에게 보내기
				if (SendPacket(itor->second, &header, &payload) == FALSE)
					return FALSE;

				// 피해자의 남은 Hp가 0이라면 접속 종료시킨다.
				//if (itor->second->m_LoginPlayer->m_byHP == 0)
					//Disconnect(itor->second);

				break;
			}

		}

	}
	break;

	case dfPACKET_MOVE_DIR_RR:
	{
		for (; itor != enditor; ++itor)
		{
			// 공격자의 공격범위에 피해자가 있는지 체크
			// 공격 범위에 있으면, 피해자 기준 9방향에 데미지 패킷을 보낸다.
			if (itor->second->m_LoginPlayer->m_wNowX < AttackX + dfATTACK2_RANGE_X &&
				itor->second->m_LoginPlayer->m_wNowX > AttackX &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y > AttackY - dfATTACK2_RANGE_Y &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y < AttackY + dfATTACK2_RANGE_Y)
			{
				// 피해자의 HP 감소시키기.
				itor->second->m_LoginPlayer->m_byHP -= dfATTACK2_DAMAGE;

				// 데미지 패킷 만들기
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_Damage(&header, &payload, AttackID, itor->second->m_LoginPlayer->m_dwPlayerID, itor->second->m_LoginPlayer->m_byHP);

				// 피해자 기준 9방향 유저에게 보내기 (피해자 자신에게는 안간다.)
				stSectorCheck secSector;
				GetSector(itor->second->m_LoginPlayer->m_SectorX, itor->second->m_LoginPlayer->m_SectorY, &secSector);

				if (SendPacket_Sector(itor->second->m_LoginPlayer, &header, &payload, &secSector) == FALSE)
					return FALSE;

				// 피해자 자신에게 보내기
				if (SendPacket(itor->second->m_LoginPlayer->m_Account, &header, &payload) == FALSE)
					return FALSE;

				// 피해자의 남은 Hp가 0이라면 접속 종료시킨다.
				//if (itor->second->m_LoginPlayer->m_byHP == 0)
				//Disconnect(itor->second);

				break;
			}

		}
	}
	break;

	}

	return TRUE;

}

// "공격 3번" 패킷의 데미지 처리. Network_Req_Attack_3()에서 사용.
bool Network_Requ_Attck_3_Damage(DWORD AttackID, int AttackX, int AttackY, BYTE AttackDir)
{
	// 공격 Y좌표를 캐릭터의 중간쯤으로 갱신
	AttackY -= dfATTACK_Y;

	// 공격 방향에 다르게 처리.
	map<SOCKET, stAccount*>::iterator itor = map_Account.begin();
	map<SOCKET, stAccount*>::iterator enditor = map_Account.end();

	switch (AttackDir)
	{
	case dfPACKET_MOVE_DIR_LL:
	{
		for (; itor != enditor; ++itor)
		{
			// 공격자의 공격범위에 피해자가 있는지 체크
			// 공격 범위에 있으면, 피해자 기준 9방향에 데미지 패킷을 보낸다.
			if (itor->second->m_LoginPlayer->m_wNowX > AttackX - dfATTACK3_RANGE_X &&
				itor->second->m_LoginPlayer->m_wNowX < AttackX &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y > AttackY - dfATTACK3_RANGE_Y &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y < AttackY + dfATTACK3_RANGE_Y)
			{
				// 피해자의 HP 감소시키기.
				itor->second->m_LoginPlayer->m_byHP -= dfATTACK3_DAMAGE;

				// 데미지 패킷 만들기
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_Damage(&header, &payload, AttackID, itor->second->m_LoginPlayer->m_dwPlayerID, itor->second->m_LoginPlayer->m_byHP);

				// 피해자 기준 9방향 유저에게 보내기 (피해자 자신에게는 안간다.)
				stSectorCheck secSector;
				GetSector(itor->second->m_LoginPlayer->m_SectorX, itor->second->m_LoginPlayer->m_SectorY, &secSector);

				SendPacket_Sector(itor->second->m_LoginPlayer, &header, &payload, &secSector);

				// 피해자 자신에게 보내기
				if (SendPacket(itor->second, &header, &payload) == FALSE)
					return FALSE;

				// 피해자의 남은 Hp가 0이라면 접속 종료시킨다.
				//if (itor->second->m_LoginPlayer->m_byHP == 0)
					//Disconnect(itor->second);

				break;
			}

		}

	}
	break;

	case dfPACKET_MOVE_DIR_RR:
	{
		for (; itor != enditor; ++itor)
		{
			// 공격자의 공격범위에 피해자가 있는지 체크
			// 공격 범위에 있으면, 피해자 기준 9방향에 데미지 패킷을 보낸다.
			if (itor->second->m_LoginPlayer->m_wNowX < AttackX + dfATTACK3_RANGE_X &&
				itor->second->m_LoginPlayer->m_wNowX > AttackX &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y > AttackY - dfATTACK3_RANGE_Y &&
				itor->second->m_LoginPlayer->m_wNowY - dfATTACK_Y < AttackY + dfATTACK3_RANGE_Y)
			{
				// 피해자의 HP 감소시키기.
				itor->second->m_LoginPlayer->m_byHP -= dfATTACK3_DAMAGE;

				// 데미지 패킷 만들기
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_Damage(&header, &payload, AttackID, itor->second->m_LoginPlayer->m_dwPlayerID, itor->second->m_LoginPlayer->m_byHP);

				// 피해자 기준 9방향 유저에게 보내기 (피해자 자신에게는 안간다.)
				stSectorCheck secSector;
				GetSector(itor->second->m_LoginPlayer->m_SectorX, itor->second->m_LoginPlayer->m_SectorY, &secSector);

				if (SendPacket_Sector(itor->second->m_LoginPlayer, &header, &payload, &secSector) == FALSE)
					return FALSE;

				// 피해자 자신에게 보내기
				if (SendPacket(itor->second->m_LoginPlayer->m_Account, &header, &payload) == FALSE)
					return FALSE;

				// 피해자의 남은 Hp가 0이라면 접속 종료시킨다.
				//if (itor->second->m_LoginPlayer->m_byHP == 0)
				//Disconnect(itor->second);

				break;
			}

		}
	}
	break;

	}
	return TRUE;
}

// 에코용 패킷 처리(RTT체크)
bool Network_Req_StressTest(stAccount* Account, CProtocolBuff* Packet)
{
	// 4바이트 꺼내오기
	DWORD Time;
	*Packet >> Time;

	// 2. 페이로드 제작
	// 페이로드에 시간 복사
	CProtocolBuff Payload;
	Payload << Time;

	// 3. 헤더 제작
	CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
	CreateHeader(&header, Payload.GetUseSize(), dfPACKET_SC_ECHO);
		
	// 4. 만든 패킷을 해당 유저의, SendBuff에 넣기
	SendPacket(Account, &header, &Payload);

	return true;
}





// ---------------------------------
// Send
// ---------------------------------
// SendBuff에 데이터 넣기
BOOL SendPacket(stAccount* Account, CProtocolBuff* header, CProtocolBuff* payload)
{	
	BEGIN("SendPacket");
	char* recvbuff = Account->m_SendBuff.GetBufferPtr();

	// 1. 샌드 q에 헤더 넣기
	int Size = dfNETWORK_PACKET_HEADER_SIZE;
	int EnqueueCheck = Account->m_SendBuff.Enqueue(header->GetBufferPtr(), Size);
	if (EnqueueCheck == -1)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]SendPacket() 헤더넣는 중 링버퍼 꽉참.(PlayerID : %d)\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, Account->m_LoginPlayer->m_dwPlayerID);

		return FALSE;
	}	

	// 2. 샌드 q에 페이로드 넣기
	int PayloadLen = payload->GetUseSize();
	EnqueueCheck = Account->m_SendBuff.Enqueue(payload->GetBufferPtr(), PayloadLen);
	if (EnqueueCheck == -1)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]SendPacket() 페이로드 넣는 중 링버퍼 꽉참.(PlayerID : %d)\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, Account->m_LoginPlayer->m_dwPlayerID);

		return FALSE;
	}	

	// 3. 샌드 q에 엔드코드 넣기
	char EndCode = dfNETWORK_PACKET_END;
	EnqueueCheck = Account->m_SendBuff.Enqueue(&EndCode, 1);
	if (EnqueueCheck == -1)
	{
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] [ERROR]SendPacket() 엔드코드 넣는 중 링버퍼 꽉참.(PlayerID : %d)\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, Account->m_LoginPlayer->m_dwPlayerID);

		return FALSE;	
	}	

	END("SendPacket");

	return TRUE;
}

// SendBuff의 데이터를 Send하기
bool SendProc(stAccount* Account)
{
	if (Account == nullptr)
		return false;


	BEGIN("SendProc");

	// 1. SendBuff에 보낼 데이터가 있는지 확인.
	if (Account->m_SendBuff.GetUseSize() == 0)
		return true;

	// 2. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = Account->m_SendBuff.GetBufferPtr();

	// 3. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 4. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (Account->m_SendBuff.GetUseSize() == 0)
			break;

		// 5. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = Account->m_SendBuff.GetNotBrokenGetSize();

		// 6. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 7. front 포인터 얻어옴
		int *front = (int*)Account->m_SendBuff.GetReadBufferPtr();

		// 8. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = Account->m_SendBuff.NextIndex(*front, 1);

		// 9. Send()
		int SendSize = send(Account->m_sock, &sendbuff[BuffArrayCheck], Size, 0);

		// 10. 에러 체크. 우드블럭이면 더 이상 못보내니 while문 종료. 다음 Select때 다시 보냄
		if (SendSize == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK || 
				WSAGetLastError() == 10054)
				break;

			else
			{
				_tprintf(_T("SendProc(). Send중 에러 발생. 접속 종료 (%d)\n"), WSAGetLastError());
				return false;
			}

		}

		// 11. 보낸 사이즈가 나왔으면, 그 만큼 remove
		Account->m_SendBuff.RemoveData(SendSize);		
	}

	END("SendProc");

	return true;
}

