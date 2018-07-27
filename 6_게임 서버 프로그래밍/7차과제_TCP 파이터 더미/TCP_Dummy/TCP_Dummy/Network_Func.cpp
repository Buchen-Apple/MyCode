#include "stdafx.h"
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
#include "Network_Func.h"

#include <map>

#define dfNETWORK_PORT		20000

// 논리적인 맵 크기
#define MAP_WIDTH			6400
#define MAP_HEIGHT			6400

// 유저 상태
#define PLAYER_ACTION_IDLE		0	// 정지
#define PLAYER_ACTION_MOVE		1	// 이동

// 1프레임에 이동하는 좌표
#define PLAYER_XMOVE_PIXEL 3
#define PLAYER_YMOVE_PIXEL 2

// 유저 이동제한
#define dfRANGE_MOVE_TOP	0 + 1
#define dfRANGE_MOVE_LEFT	0 + 2
#define dfRANGE_MOVE_RIGHT	MAP_WIDTH - 3
#define dfRANGE_MOVE_BOTTOM	MAP_HEIGHT - 1

// AIEnum
enum AI 
{
	MOVE_LL = 0, MOVE_LU, MOVE_UU, MOVE_RU, MOVE_RR, MOVE_RD, MOVE_DD, MOVE_LD, MOVE_STOP,
	ATTACK_01_LEFT, ATTACK_01_RIGHT, ATTACK_02_LEFT, ATTACK_02_RIGHT, ATTACK_03_LEFT, ATTACK_03_RIGHT
};



using namespace std;

// 더미 세션 구조체
struct stSession
{
	SOCKET m_sock;

	CRingBuff m_RecvBuff;
	CRingBuff m_SendBuff;

	// 연결된 더미클라를 알고있다.
	stDummyClient* m_DummyClient = nullptr;
};

// 더미 클라 구조체
struct stDummyClient
{	
	// 플레이어 ID
	DWORD	m_dwClientID;

	// 현재 X,Y 좌표
	WORD	m_wNowX;
	WORD	m_wNowY;

	// 보는 방향
	BYTE	m_byDir;

	// 체력
	BYTE	m_byHP;

	// 과거 액션, 현재 액션
	BYTE	m_byOldAction;
	BYTE	m_byNowAction;

	// 세션을 알고있다. 최초는 nullptr
	stSession* m_Session = nullptr;

	// -------------------
	// 데드레커닝 관련
	// -------------------
	// 데드레커닝을 위해, NowAction을 취한 시간. 
	ULONGLONG m_ullActionTick;

	// 데드레커닝을 위해, 액션 변경 시점의 X,Y좌표 저장(월드좌표)
	short	m_wActionX;
	short	m_wActionY;

	// -------------------
	// AI 관련
	// -------------------
	// AI 시작 시간. 시간을 저장해뒀다가 n초에 한번 AI를 한다.
	ULONGLONG m_ullAIStartTick;
};



// 외부변수
extern int		g_iDummyCount;					// main에서 셋팅한 Dummy 카운트												
extern TCHAR	g_tIP[30];						// main에서 셋팅한 서버의 IP

extern int		g_LogLevel;						// Main에서 입력한 로그 출력 레벨. 외부변수
extern TCHAR	g_szLogBuff[1024];				// Main에서 입력한 로그 출력용 임시 버퍼. 외부변수
												
extern int g_iRecvByte;							// Recv, Send바이트
extern int g_iSendByte;

extern DWORD g_dwJoinUserCount;					// Accept된 유저 수

extern DWORD g_dwSyncCount;						// 싱크받은 횟수 카운트


// 플레이어 관리 map
map <DWORD, stDummyClient*> map_Clientmap;

// 세션 관리 map
map <SOCKET, stSession*> map_Sessionmap;

// 생성된 플레이어 관리 배열
stDummyClient** g_PlayerArray;
DWORD g_PlayerArrayCount;

// -------------------
// RTT 관련
// -------------------
// RTT보냈는지 체크. true면 보내기 가능 상태.
bool g_RTTSendCheck = true;

// RTT 샌드 보낸 시간 저장.
ULONGLONG g_RTTSendTime;

// 보냈던 SendData를 기억해뒀다가 잘 왔는지 체크한다.
DWORD g_RTTSendData;	

// 1초동안 RTT를 보낸 후 받은 횟수 저장
extern int g_RTTSendCount;

// 1초동안의 RTT 평균 시간 저장.
ULONGLONG g_RTTAvgTime;

// RTT Max
extern ULONGLONG g_RTTMax;




// ---------------------------------
// 기타 함수
// ---------------------------------
// 네트워크 셋팅 함수 (윈속초기화, 커넥트 등..)
bool Network_Init(int* TryCount, int* FailCount, int* SuccessCount)
{
	// 입력한 더미 수 만큼 동적할당
	g_PlayerArray = new stDummyClient*[g_iDummyCount];

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_tprintf(_T("WSAStartup 실패!\n"));
		return false;
	}

	// Dummy 카운트만큼 돌면서 소켓 생성
	for (int i = 0; i < g_iDummyCount; ++i)
	{
		stSession* NewAccount = new stSession;

		// 1. 소켓 생성
		NewAccount->m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (NewAccount->m_sock == INVALID_SOCKET)
		{
			_tprintf(_T("%d번째 소켓 생성 실패!\n"), i);
			return false;
		}

		// 2. Connect
		SOCKADDR_IN clientaddr;
		ZeroMemory(&clientaddr, sizeof(clientaddr));
		clientaddr.sin_family = AF_INET;
		clientaddr.sin_port = htons(dfNETWORK_PORT);
		InetPton(AF_INET, g_tIP, &clientaddr.sin_addr.s_addr);

		DWORD dCheck = connect(NewAccount->m_sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));

		// 3. 커넥트 시도 횟수 증가
		(*TryCount)++;

		// 3. 커넥트 실패 시, 실패 카운트 1 증가
		if (dCheck == SOCKET_ERROR)
		{
			_tprintf(_T("connect 실패. %d (Error : %d)\n"), i + 1, WSAGetLastError());
			(*FailCount)++;
		}
		// 커넥트 성공 시, 성공 카운트 1증가. 
		// 세션 관리목록, 플레이어 목록에 추가
		else
		{
			// 클라 더미 셋팅
			stDummyClient* NewClient = new stDummyClient;
			NewClient->m_Session = NewAccount;	
						
			// 세션 더미 셋팅 후 map에 추가.
			NewAccount->m_DummyClient = NewClient;
			map_Sessionmap.insert(pair<SOCKET, stSession*>(NewAccount->m_sock, NewAccount));

			// 화면 출력
			_tprintf(_T("connect 성공. %d\n"), i + 1);

			// 각종 카운트 증가
			(*SuccessCount)++;
			g_dwJoinUserCount++;		
			
		}

	}

	return true;
}

// 네트워크 프로세스
// 여기서 false가 반환되면, 프로그램이 종료된다.
bool Network_Process()
{
	// 통신에 사용할 변수
	FD_SET rset;
	FD_SET wset;

	map <SOCKET, stSession*>::iterator itor;
	map <SOCKET, stSession*>::iterator enditor;
	TIMEVAL tval;
	tval.tv_sec = 0;
	tval.tv_usec = 0;

	itor = map_Sessionmap.begin();

	// 리슨 소켓 셋팅. 
	rset.fd_count = 0;
	wset.fd_count = 0;

	while (1)
	{
		enditor = map_Sessionmap.end();

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
			if(WSAGetLastError() != WSAEINVAL && rset.fd_count != 0 && wset.fd_count != 0)
				_tprintf(_T("select 에러(%d)\n"), WSAGetLastError());

			return false;
		}

		// select의 값이 0보다 크다면 뭔가 할게 있다는 것이니 로직 진행
		if (dCheck > 0)
		{
			DWORD rsetCount = 0;
			DWORD wsetCount = 0;

			while (1)
			{
				if (rsetCount < rset.fd_count)
				{
					stSession* NowAccount = ClientSearch_AcceptList(rset.fd_array[rsetCount]);

					// Recv() 처리
					// 만약, RecvProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					if (RecvProc(NowAccount) == false)
						Disconnect(NowAccount);

					rsetCount++;
				}

				if (wsetCount < wset.fd_count)
				{
					stSession* NowAccount = ClientSearch_AcceptList(wset.fd_array[wsetCount]);

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

	return true;
}

// Disconnect 처리
void Disconnect(stSession* Account)
{
	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (Account == nullptr)
	{
		//_LOG(dfLOG_LEVEL_ERROR, L"Disconnect(). Accept 중이 아닌 유저를 대상으로 삭제 시도\n");
		return;
	}

	fputs("Disconnect!\n", stdout);

	// 해당 유저의 소켓 close
	closesocket(Account->m_sock);

	// 해당 유저를 세션 맵에서 제외
	map_Sessionmap.erase(Account->m_sock);

	// 해당 유저를 플레이어 맵에서 제외
	map_Clientmap.erase(Account->m_DummyClient->m_dwClientID);

	// 배열에서도 제거
	for (int i = 0; i < g_PlayerArrayCount; ++i)
	{
		// 삭제할 더미를 찾았으면
		if (g_PlayerArray[i] == Account->m_DummyClient)
		{
			// 가장 끝의 더미를 그 위치로 변경
			g_PlayerArray[i] = g_PlayerArray[g_PlayerArrayCount - 1];
			g_PlayerArrayCount--;
			break;
		}
	}
	
	// 플레이어 동적해제
	delete Account->m_DummyClient;

	// 세션 동적해제
	delete Account;	

	

	// 접속한 유저 수 감소
	g_dwJoinUserCount--;	
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

// 데드레커닝 함수
// 인자로 (현재 액션, 액션 시작 시간, 액션 시작 위치, (OUT)계산 후 좌표)를 받는다.
void DeadReckoning(BYTE NowAction, ULONGLONG ActionTick, int ActionStartX, int ActionStartY, int* ResultX, int* ResultY)
{
	// ----------------------------
	// 액션 시작시간부터 현재 시간까지의 시간차를 구한다. 
	// 이 시간차로 몇 프레임이 지났는지 계산한다.
	// ----------------------------
	int IntervalFrame = (GetTickCount64() - ActionTick) / 20; // (20이 1프레임. 50프레임으로 맞춤)

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
	// 4. 모든 계산이 완료되었으면, PosX와 PosY의 값을 맵 안으로 잡는다.(경계처리)
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
}

// 초당 RTT 계산.
ULONGLONG RTTAvr()
{	
	ULONGLONG returnRTT = 0;

	if (g_RTTAvgTime == 0 || g_RTTSendCount == 0)
		return returnRTT;

	// 평균 구하기
	returnRTT = g_RTTAvgTime / g_RTTSendCount;

	// 다음 계산을 위해 값 초기화
	g_RTTAvgTime = 0;

	return returnRTT;
}







// ---------------------------------
// 검색용 함수
// --------------------------------
// 인자로 받은 Socket 값을 기준으로 [회원 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stSession* ClientSearch_AcceptList(SOCKET sock)
{
	map <SOCKET, stSession*>::iterator iter;

	iter = map_Sessionmap.find(sock);
	if (iter == map_Sessionmap.end())
		return nullptr;

	return iter->second;
}

// 인자로 받은 유저 ID로 [플레이어 목록]에서 [플레이어를 골라낸다.] (검색)
// 성공 시, 해당 플레이어의 구조체 주소를 리턴
// 실패 시 nullptr 리턴
stDummyClient* ClientSearch_ClientList(DWORD ID)
{
	map <DWORD, stDummyClient*>::iterator iter;

	iter = map_Clientmap.find(ID);
	if (iter == map_Clientmap.end())
		return nullptr;

	return iter->second;
}






// ---------------------------------
// AI 함수
// ---------------------------------
void DummyAI()
{
	static DWORD dwNowDummyID = 0;			// 현재 체크해야하는 더미의 ID
	static DWORD dwAIPlayTick = 3000;		// AI는 3000밀리세컨드(3초)에 1회 진행.
	int Count = 0;

	// 모든 Player캐릭터를, 돌면서 AI 할 때가 되었으면 한다. 1프레임에 20명씩 처리. 1초에 50프레임이니, 1초에 총 1000명의 더미 처리
	while (Count < 20 && Count < g_PlayerArrayCount && dwNowDummyID < g_PlayerArrayCount)
	{
		// RTT패킷을 보낸다.
		if (g_RTTSendCheck == true)
		{
			// RTT 패킷 만든다.
			CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
			CProtocolBuff payload;

			Network_Send_Echo(&header, &payload);

			// 0번째 더미의 샌드버퍼에 넣고
			SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);

			// 보낸 시간 저장
			g_RTTSendTime = GetTickCount64();

			g_RTTSendCheck = false;
		}

		// 3초가 되었다면
		if ((GetTickCount64() - g_PlayerArray[dwNowDummyID]->m_ullAIStartTick) >= dwAIPlayTick)
		{			
			// AI를 한다.
			// AI는 이동시작(8방향), 정지, 공격시작(1,2,3번. 공격 1개당 좌,우 2개씩) 중 1개 선택. 총 15개

			int AICheck = rand() % 14;

			switch (AICheck)
			{
			case AI::MOVE_LL:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_LL)
					break;

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// Idel 상태가 아니면 데드레커닝으로 좌표 이동
				if(g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				//  액션 시작 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_LL;				

				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);				
			}
			break;

			
			case AI::MOVE_LU:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_LU)
					break;	

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();
				
				// Idel 상태가 아니면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_LU;
				
				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);

				
			}
			break;

			
			case AI::MOVE_UU:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_UU)
					break;		

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// Idel 상태가 아니면 데드레커닝으로, 이전 행동의 좌표 조정
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_UU;

				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);

				
			}
			break;
			
			case AI::MOVE_RU:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_RU)
					break;	

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// Idel 상태가 아니면 데드레커닝으로, 이전 행동의 좌표 조정
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_RU;

				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);
				
			}
			break;
			
			case AI::MOVE_RR:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_RR)
					break;	

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// Idel 상태가 아니면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// 액션 시작 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_RR;

				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);
			}
			break;

			case AI::MOVE_RD:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_RD)
					break;	

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// Idel 상태가 아니면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// 액션 시작 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_RD;

				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);
			}
			break;

			case AI::MOVE_DD:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_DD)
					break;	

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// Idel 상태가 아니면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// 액션 시작 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_DD;

				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);
			}
			break;

			case AI::MOVE_LD:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE && g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_LD)
					break;

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// Idel 상태가 아니면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction != PLAYER_ACTION_IDLE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// 액션 시작 시간, 현재 액션, 방향 갱신
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_MOVE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_LD;

				// 이동 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStart(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);
			}
			break;
			
			case AI::MOVE_STOP:
			{
				// 동일한 AI가 또 나왔으면 무시
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_IDLE)
					break;

				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// 데드레커닝으로 좌표 이동
				Action_Move(g_PlayerArray[dwNowDummyID]);		

				// 시간, 현재 액션, 방향 갱신	
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_IDLE;	
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_RR;

				if (g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_LL ||
					g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_LU ||
					g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_UU ||
					g_PlayerArray[dwNowDummyID]->m_byDir == dfPACKET_MOVE_DIR_LD)
				{
					g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_LL;
				}

				// 정지 시작 패킷을 만든다.
				CProtocolBuff header(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload;
				Network_Send_MoveStop(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header, &payload);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header, &payload);
				
			}
			break;		
			
			case AI::ATTACK_01_LEFT:
			{	
				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// 이동중이었다면 데드레커닝으로 좌표 이동
				if(g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// -----------------------------
				// 공격시작 패킷 만들기
				// -----------------------------
				// 시간, 현재 액션, 방향 갱신	
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_IDLE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_LL;

				// 공격 시작 패킷을 만든다.
				CProtocolBuff header_Attack(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload_Attack;
				Network_Send_Attack_01(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header_Attack, &payload_Attack);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header_Attack, &payload_Attack);

			}
			break;
			
			
			case AI::ATTACK_01_RIGHT:
			{
				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// 이동중이었다면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// -----------------------------
				// 공격시작 패킷 만들기
				// -----------------------------
				// 시간, 현재 액션, 방향 갱신	
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_IDLE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_RR;

				// 공격 시작 패킷을 만든다.
				CProtocolBuff header_Attack(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload_Attack;
				Network_Send_Attack_01(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header_Attack, &payload_Attack);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header_Attack, &payload_Attack);

			}
			break;

			case AI::ATTACK_02_LEFT:
			{
				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// 이동중이었다면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE)
					Action_Move(g_PlayerArray[dwNowDummyID]);

				// -----------------------------
				// 공격시작 패킷 만들기
				// -----------------------------
				// 시간, 현재 액션, 방향 갱신	
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_IDLE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_LL;

				// 공격 시작 패킷을 만든다.
				CProtocolBuff header_Attack(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload_Attack;
				Network_Send_Attack_02(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header_Attack, &payload_Attack);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header_Attack, &payload_Attack);

			}
			break;

			case AI::ATTACK_02_RIGHT:
			{
				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// 이동중이었다면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE)
					Action_Move(g_PlayerArray[dwNowDummyID]);
				
				// -----------------------------
				// 공격시작 패킷 만들기
				// -----------------------------
				// 시간, 현재 액션, 방향 갱신	
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_IDLE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_RR;

				// 공격 시작 패킷을 만든다.
				CProtocolBuff header_Attack(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload_Attack;
				Network_Send_Attack_02(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header_Attack, &payload_Attack);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header_Attack, &payload_Attack);

			}
			break;

			case AI::ATTACK_03_LEFT:
			{
				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// 이동중이었다면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE)
					Action_Move(g_PlayerArray[dwNowDummyID]);
				
				// -----------------------------
				// 공격시작 패킷 만들기
				// -----------------------------
				// 방향 갱신
				// 시간, 현재 액션, 방향 갱신	
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_IDLE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_LL;

				// 공격 시작 패킷을 만든다.
				CProtocolBuff header_Attack(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload_Attack;
				Network_Send_Attack_03(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header_Attack, &payload_Attack);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header_Attack, &payload_Attack);

			}
			break;

			case AI::ATTACK_03_RIGHT:
			{
				// AI 시작 시간 갱신
				g_PlayerArray[dwNowDummyID]->m_ullAIStartTick = GetTickCount64();

				// 이동중이었다면 데드레커닝으로 좌표 이동
				if (g_PlayerArray[dwNowDummyID]->m_byNowAction == PLAYER_ACTION_MOVE)
					Action_Move(g_PlayerArray[dwNowDummyID]);
				
				// -----------------------------
				// 공격시작 패킷 만들기
				// -----------------------------
				// 시간, 현재 액션, 방향 갱신	
				g_PlayerArray[dwNowDummyID]->m_ullActionTick = GetTickCount64();
				g_PlayerArray[dwNowDummyID]->m_byNowAction = PLAYER_ACTION_IDLE;
				g_PlayerArray[dwNowDummyID]->m_byDir = dfPACKET_MOVE_DIR_RR;

				// 공격 시작 패킷을 만든다.
				CProtocolBuff header_Attack(dfNETWORK_PACKET_HEADER_SIZE);
				CProtocolBuff payload_Attack;
				Network_Send_Attack_03(g_PlayerArray[dwNowDummyID]->m_byDir, g_PlayerArray[dwNowDummyID]->m_wNowX, g_PlayerArray[dwNowDummyID]->m_wNowY, &header_Attack, &payload_Attack);

				// 해당 더미의 SendBuff에 넣는다.
				SendPacket(g_PlayerArray[dwNowDummyID]->m_Session, &header_Attack, &payload_Attack);
			}
			break;
			
			
			default:
				break;
			}
		}

		Count++;
		dwNowDummyID++;
		if (dwNowDummyID == g_dwJoinUserCount)
			dwNowDummyID = 0;
	}	
}







// ---------------------------------
// Update()에서 유저 액션 처리 함수
// ---------------------------------
// 유저 이동 액션 처리
void Action_Move(stDummyClient* NowPlayer)
{
	// 데드레커닝으로 이동
	int ResultX, ResultY;	
	DeadReckoning(NowPlayer->m_byDir, NowPlayer->m_ullActionTick, NowPlayer->m_wActionX, NowPlayer->m_wActionY, &ResultX, &ResultY);
	
	// 이동 후 좌표 갱신	
	NowPlayer->m_wNowX = ResultX;
	NowPlayer->m_wNowY = ResultY;
	NowPlayer->m_wActionX = ResultX;
	NowPlayer->m_wActionY = ResultY;	
}









// ---------------------------------
// Recv 처리 함수들
// ---------------------------------
// Recv() 처리
bool RecvProc(stSession* Account)
{
	if (Account == nullptr)
		return false;

	////////////////////////////////////////////////////////////////////////
	// 소켓 버퍼에 있는 모든 recv 데이터를, 해당 유저의 recv링버퍼로 리시브.
	////////////////////////////////////////////////////////////////////////
	
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
			_tprintf(_T("RecvProc(). retval 후 우드블럭이 아닌 에러가 뜸(%d). 접속 종료\n"), WSAGetLastError());
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// 0이 리턴되는 것은 정상종료.
	else if (retval == 0)
		return false;

	g_iRecvByte += retval;

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	Account->m_RecvBuff.MoveWritePos(retval - 1);


	//////////////////////////////////////////////////
	// 완료 패킷 처리 부분
	// RecvBuff에 들어있는 모든 완성 패킷을 처리한다.
	//////////////////////////////////////////////////
	while (1)
	{
		char* b = Account->m_RecvBuff.GetReadBufferPtr();
		
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
		st_PACKET_HEADER HeaderBuff;
		int len = sizeof(HeaderBuff);

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (Account->m_RecvBuff.GetUseSize() < len)
			break;

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = Account->m_RecvBuff.Peek((char*)&HeaderBuff, len);

		// Peek()은 버퍼가 비었으면 -1을 반환한다. 버퍼가 비었으니 일단 끊는다.
		if (PeekSize == -1)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 RecvBuff비었음. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 4. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size + endCode(1바이트))
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (Account->m_RecvBuff.GetUseSize() < (len + HeaderBuff.bySize + 1))
			break;

		// 4. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (HeaderBuff.byCode != dfNETWORK_PACKET_CODE)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 PacketCode틀림. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		Account->m_RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		DWORD BuffArray = 0;
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.bySize;

		while (PayloadSize > 0)
		{
			int DequeueSize = Account->m_RecvBuff.Dequeue(PayloadBuff.GetBufferPtr() + BuffArray, PayloadSize);

			// Dequeue()는 버퍼가 비었으면 -1을 반환. 버퍼가 비었으면 일단 끊는다.
			if (DequeueSize == -1)
			{
				_tprintf(_T("RecvProc(). 완료패킷 페이로드 처리 중 RecvBuff비었음. 접속 종료\n"));
				return false;	// FALSE가 리턴되면, 접속이 끊긴다.
			}

			PayloadSize -= DequeueSize;
			BuffArray += DequeueSize;
		}
		PayloadBuff.MoveWritePos(BuffArray);

		// 7. RecvBuff에서 엔드코드 1Byte뽑음.	(디큐이다. Peek 아님)
		BYTE EndCode;
		DWORD EndCodeSize = 1;
		BuffArray = 0;
		while (EndCodeSize > 0)
		{
			int DequeueSize = Account->m_RecvBuff.Dequeue((char*)&EndCode + BuffArray, 1);
			if (DequeueSize == -1)
			{
				_tprintf(_T("RecvProc(). 완료패킷 엔드코드 처리 중 RecvBuff비었음. 접속 종료\n"));
				return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
			}

			EndCodeSize -= DequeueSize;
			BuffArray += DequeueSize;
		}

		// 8. 엔드코드 확인
		if (EndCode != dfNETWORK_PACKET_END)
		{
			_tprintf(_T("RecvProc(). 완료패킷 엔드코드 처리 중 정상패킷 아님. 접속 종료\n"));
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 7. 헤더에 들어있는 타입에 따라 분기처리.
		bool check = PacketProc(HeaderBuff.byType, Account->m_DummyClient, &PayloadBuff);
		if (check == false)
			return false;
	}

	return true;
}

// 패킷 처리
// PacketProc() 함수에서 false가 리턴되면 해당 유저는 접속이 끊긴다.
bool PacketProc(WORD PacketType, stDummyClient* Player, CProtocolBuff* Payload)
{
	/*_LOG(dfLOG_LEVEL_DEBUG, L"[DEBUG] PacketRecv [PlayerID : %d / PacketType : %d]\n",
		Player->m_dwClientID, PacketType);*/

	bool check = true;

	try
	{
		switch (PacketType)
		{

		// 내캐릭터 생성
		case dfPACKET_SC_CREATE_MY_CHARACTER:
		{
			check = Network_Recv_CreateMyCharacter(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 다른 사람 캐릭터 생성
		case dfPACKET_SC_CREATE_OTHER_CHARACTER:
		{
			check = Network_Recv_CreateOtherCharacter(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 캐릭터 삭제
		case dfPACKET_SC_DELETE_CHARACTER:
		{
			check = Network_Recv_DeleteCharacter(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 다른 유저 이동 시작
		case dfPACKET_SC_MOVE_START:
		{
			check = Network_Recv_OtherMoveStart(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 다른 유저 이동 정지
		case dfPACKET_SC_MOVE_STOP:
		{
			check = Network_Recv_OtherMoveStop(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 다른 유저 공격 1번 시작
		case dfPACKET_SC_ATTACK1:
		{
			check = Network_Recv_OtherAttack_01(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 다른 유저 공격 2번 시작
		case dfPACKET_SC_ATTACK2:
		{
			check = Network_Recv_OtherAttack_02(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 다른 유저 공격 3번 시작
		case dfPACKET_SC_ATTACK3:
		{
			check = Network_Recv_OtherAttack_03(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 데미지 패킷 처리
		case dfPACKET_SC_DAMAGE:
		{
			check = Network_Recv_Damage(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 싱크 패킷
		case dfPACKET_SC_SYNC:
		{
			check = Network_Recv_Sync(Player, Payload);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 에코 패킷
		case dfPACKET_SC_ECHO:
		{
			check = Network_Recv_Echo(Player, Payload);
			if (check == false)
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

// 내 캐릭터 생성
bool Network_Recv_CreateMyCharacter(stDummyClient* Player, CProtocolBuff* Payload)
{
	if (Player == nullptr)
	{
		// 인자로받은 Player가 nullptr임
		_LOG(dfLOG_LEVEL_DEBUG, L"[DEBUG] 내캐릭터 생성. 인자로 받은 Player가 nullptr \n");
		return false;
	}

	// 1. 페이로드에서 받은 데이터 뽑아내기.
	DWORD	ID;
	BYTE	Direction;
	WORD	X;
	WORD	Y;
	BYTE	HP;

	*Payload >> ID;
	*Payload >> Direction;
	*Payload >> X;
	*Payload >> Y;
	*Payload >> HP;

	// 2. 받은 정보와 시간 셋팅
	Player->m_dwClientID = ID;
	Player->m_byDir = Direction;
	Player->m_wNowX = X;
	Player->m_wNowY = Y;
	Player->m_byHP = HP;
	Player->m_ullAIStartTick = GetTickCount64();

	// Idle 동작을 시작한 것 처럼 처리
	Player->m_ullActionTick = GetTickCount64(); 
	Player->m_wActionX = X;						
	Player->m_wActionY = Y;

	// 3. 최초 액션은 Idle이다
	Player->m_byNowAction = PLAYER_ACTION_IDLE;

	// 4. 생성된 더미는 더미 관리 map에 추가.
	map_Clientmap.insert(pair<DWORD, stDummyClient*>(ID, Player));

	// 5. 그리고 배열에도 추가
	g_PlayerArray[g_PlayerArrayCount] = Player;
	g_PlayerArrayCount++;

	return true;
}

// 다른 사람 캐릭터 생성
bool Network_Recv_CreateOtherCharacter(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 다른사람 캐릭터 생성은 무시
	return true;
}

// 캐릭터 삭제
bool Network_Recv_DeleteCharacter(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 할거없음

	return true;
}

// 다른 유저 이동 시작
bool Network_Recv_OtherMoveStart(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 다른 유저 이동 시작 패킷은 무시된다.
	return true;
}

// 다른 유저 이동 정지
bool Network_Recv_OtherMoveStop(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 다른 유저 이동 정지 패킷은 무시된다.
	return true;
}

// 다른 유저 공격 1번 시작
bool Network_Recv_OtherAttack_01(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 무시
	return true;
}

// 다른 유저 공격 2번 시작
bool Network_Recv_OtherAttack_02(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 무시
	return true;
}

// 다른 유저 공격 3번 시작
bool Network_Recv_OtherAttack_03(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 무시
	return true;
}

// 데미지 패킷
bool Network_Recv_Damage(stDummyClient* Player, CProtocolBuff* Payload)
{
	// 무시
	return true;
}

// 싱크 패킷
bool Network_Recv_Sync(stDummyClient* Player, CProtocolBuff* Payload)
{
	// ID, X, Y 알아오기
	DWORD ID;
	WORD X;
	WORD Y;

	*Payload >> ID;
	*Payload >> X;
	*Payload >> Y;

	// 나한테 온 싱크패킷이 아니면 무시.
	if (Player->m_dwClientID != ID)
	{
		//_LOG(dfLOG_LEVEL_DEBUG, L"[DEBUG] 싱크 패킷. payload의 ID와, 인자로 받은 Player의 ID가 다름 \n");
		return true;
	}

	printf("[X %d, Y %d] -> [X %d, Y %d]\n", Player->m_wNowX, Player->m_wNowY, X, Y);

	// X,Y 셋팅
	Player->m_wNowX = X;
	Player->m_wNowY = Y;
	
	//fputs("Sync!\n", stdout);
	g_dwSyncCount++;

	return true;
}

// 에코 패킷
bool Network_Recv_Echo(stDummyClient* Player, CProtocolBuff* Payload)
{
	g_RTTSendCheck = true;

	// 검증
	DWORD Time;
	*Payload >> Time;

	if (g_RTTSendData != Time)
		return false;

	LONGLONG temp = GetTickCount64() - g_RTTSendTime;

	// Max갱신
	if (g_RTTMax < temp)
		g_RTTMax = temp;

	// 시간 갱신
	g_RTTAvgTime += temp;

	// 받았으니 카운트 증가
	g_RTTSendCount++;

	// 현재 하는거 없음. 차후 RTT 추가
	return true;
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

// 이동 시작 패킷 만들기
bool Network_Send_MoveStart(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload)
{
	// 페이로드 만들기
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 헤더 만들기
	CreateHeader(header, payload->GetUseSize(), dfPACKET_CS_MOVE_START);

	return true;
}

// 이동 정지 패킷 만들기
bool Network_Send_MoveStop(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload)
{
	// 페이로드 만들기
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 헤더 만들기
	CreateHeader(header, payload->GetUseSize(), dfPACKET_CS_MOVE_STOP);

	return true;
}

// 공격 1번 시작 패킷 만들기
bool Network_Send_Attack_01(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload)
{
	// 페이로드 만들기
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 헤더 만들기
	CreateHeader(header, payload->GetUseSize(), dfPACKET_CS_ATTACK1);

	return true;

}

// 공격 2번 시작 패킷 만들기
bool Network_Send_Attack_02(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload)
{
	// 페이로드 만들기
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 헤더 만들기
	CreateHeader(header, payload->GetUseSize(), dfPACKET_CS_ATTACK2);

	return true;

}

// 공격 3번 시작 패킷 만들기
bool Network_Send_Attack_03(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload)
{
	// 페이로드 만들기
	*payload << Dir;
	*payload << X;
	*payload << Y;

	// 헤더 만들기
	CreateHeader(header, payload->GetUseSize(), dfPACKET_CS_ATTACK3);

	return true;

}

// 에코 패킷 만들기
bool Network_Send_Echo(CProtocolBuff *header, CProtocolBuff* payload)
{
	// DWORD형 시간 얻어오기.
	DWORD Time = timeGetTime();

	// 기억해두기
	g_RTTSendData = Time;

	// 페이로드에 넣기
	*payload << Time;

	// 헤더 만들기
	CreateHeader(header, payload->GetUseSize(), dfPACKET_CS_ECHO);

	return true;
}






// ---------------------------------
// Send
// ---------------------------------
// SendBuff에 데이터 넣기
bool SendPacket(stSession* Account, CProtocolBuff* header, CProtocolBuff* payload)
{
	char* recvbuff = Account->m_RecvBuff.GetBufferPtr();

	// 1. 샌드 q에 헤더 넣기
	int Size = dfNETWORK_PACKET_HEADER_SIZE;
	DWORD BuffArray = 0;
	while (Size > 0)
	{
		int EnqueueCheck = Account->m_SendBuff.Enqueue(header->GetBufferPtr() + BuffArray, Size);
		if (EnqueueCheck == -1)
		{
			printf("SendPacket() 헤더넣는 중 링버퍼 꽉참.(PlayerID : %d)\n", Account->m_DummyClient->m_dwClientID);
			return FALSE;
		}

		Size -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	// 2. 샌드 q에 페이로드 넣기
	int PayloadLen = payload->GetUseSize();

	BuffArray = 0;
	while (PayloadLen > 0)
	{
		int EnqueueCheck = Account->m_SendBuff.Enqueue(payload->GetBufferPtr() + BuffArray, PayloadLen);
		if (EnqueueCheck == -1)
		{
			printf("SendPacket() 페이로드 넣는 중 링버퍼 꽉참.(PlayerID : %d)\n", Account->m_DummyClient->m_dwClientID);
			return FALSE;
		}

		PayloadLen -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	// 3. 샌드 q에 엔드코드 넣기
	char EndCode = dfNETWORK_PACKET_END;
	int EnqueueCheck = Account->m_SendBuff.Enqueue(&EndCode, 1);
	if (EnqueueCheck == -1)
	{
		printf("SendPacket() 엔드코드 넣는 중 링버퍼 꽉참.(PlayerID : %d)\n", Account->m_DummyClient->m_dwClientID);
		return FALSE;
	}

	return TRUE;
}

// SendBuff의 데이터를 Send하기
bool SendProc(stSession* Account)
{
	if (Account == nullptr)
		return false;	

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

		g_iSendByte += SendSize;

		// 11. 보낸 사이즈가 나왔으면, 그 만큼 remove
		Account->m_SendBuff.RemoveData(SendSize);
	}

	return true;
}

