#include "stdafx.h"
#include "RingBuff.h"

// Main과 공통 라이브러리
#include "PlayerObject.h"
#include "Network_Func.h"
#include "EffectObject.h"

// 전역 변수들
SOCKET client_sock;								// 내 소켓
BOOL bSendFlag = FALSE;							// 샌드 가능 플래그
BOOL bConnectFlag = FALSE;						// 커넥트 여부 플래그
CRingBuff SendBuff;								// 샌드 버퍼
CRingBuff RecvBuff;								// 리시브 버퍼
HWND WindowHwnd;								// 윈도우 핸들.
DWORD MyID;										// 내 ID할당 저장해두기
extern CList<BaseObject*> list;					// Main에서 할당한 객체 저장 리스트	
extern BaseObject* g_pPlayerObject;				// Main에서 할당 내 플레이어 객체

// 텍스트 아웃으로 에러 찍는 함수
void ErrorTextOut(const TCHAR* ErrorLog)
{
	TCHAR Log[30];
	_tcscpy(Log, ErrorLog);
	HDC hdc = GetDC(WindowHwnd);
	TextOut(hdc, 100, 0, ErrorLog, _tcslen(Log));
	ReleaseDC(WindowHwnd, hdc);
}

// 윈속 초기화, 소켓 연결, 커넥트 등 함수
BOOL NetworkInit(HWND hWnd, TCHAR tIP[30])
{
	WindowHwnd = hWnd;	// 윈도우 핸들 가지고 있기.

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		MessageBox(WindowHwnd, _T("윈속 초기화 실패"), _T("윈속실패"), MB_ICONERROR);
		return FALSE;
	}

	// 소켓 생성
	client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_sock == SOCKET_ERROR)
	{
		MessageBox(WindowHwnd, _T("소켓 생성 실패"), _T("소켓실패"), MB_ICONERROR);
		return FALSE;
	}

	// WSAAsyncSelect(). 동시에 비동기 소켓이 된다.
	int retval = WSAAsyncSelect(client_sock, WindowHwnd, WM_SOCK, FD_WRITE | FD_READ | FD_CONNECT | FD_CLOSE);
	if (retval == SOCKET_ERROR)
	{
		MessageBox(WindowHwnd, _T("어싱크셀렉스 실패"), _T("어싱크셀렉트 실패"), MB_ICONERROR);
		return FALSE;
	}

	// Connect
	ULONG uIP;
	SOCKADDR_IN clientaddr;
	ZeroMemory(&clientaddr, sizeof(clientaddr));

	InetPton(AF_INET, tIP, &uIP);
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(SERVERPORT);
	clientaddr.sin_addr.s_addr = uIP;

	retval = connect(client_sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			MessageBox(hWnd, _T("커넥트 실패"), _T("커넥트 실패"), MB_ICONERROR);
			return FALSE;
		}
	}

	return TRUE;
}

// 윈속 해제, 소켓 해제 등 함수
void NetworkClose()
{
	closesocket(client_sock);
	WSACleanup();
}

// 네트워크 처리 함수.
// 해당 함수에서 FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
BOOL NetworkProc(LPARAM lParam)
{
	// 에러 먼저 처리
	if (WSAGETSELECTERROR(lParam) != 0)
	{
		ErrorTextOut(_T("NetworkProc(). 네트워크 에러 발생"));
		return FALSE;
	}

	// 각종 네트워크 처리
	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_CONNECT:
		bConnectFlag = TRUE;
		return TRUE;

	case FD_WRITE:
		bSendFlag = TRUE;

		// SendProc()에서 에러가 발생하면 FALSE가 리턴된다.
		if (SendProc() == FALSE)
			return FALSE;		
		return TRUE;

	case FD_READ:
		// RecvProc()에서 에러가 발생하면 FALSE가 리턴된다.
		if(RecvProc() == FALSE)
			return FALSE;	

		return TRUE;

	case FD_CLOSE:
		return FALSE;

	default:
		ErrorTextOut(_T("NetworkProc(). 알 수 없는 패킷"));
		return TRUE;
	}
}

// 네트워크 처리 중, Recv 처리 함수
BOOL RecvProc()
{
	// bConnectFlag(커넥트 여부)가 FALSE라면 아직 연결 안된거니 그냥 리턴.
	if (bConnectFlag == FALSE)
		return TRUE;

	//////////////////////////////////////////////////
	// recv() 처리.
	// 받을게 없을 때 까지. WSAGetLastError()에 우드블럭이 떠야만 빠져나갈 수 있다.
	//////////////////////////////////////////////////
	while (1)
	{
		// 임시 수신버퍼 선언
		char TempBuff[1000];

		int retval = recv(client_sock, TempBuff, sizeof(TempBuff), 0);
		if (retval == SOCKET_ERROR)
		{
			// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있다는 것. recv할게 없다는 것이니 while문 종료
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;

			// WSAEWOULDBLOCK에러가 아니면 뭔가 이상한 것이니 접속 종료
			else
			{
				ErrorTextOut(_T("RecvProc(). retval 후 우드블럭이 아닌 에러가 뜸. 접속 종료"));
				return FALSE;		// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
			}

		}

		// 에러가 없으면, 받은 데이터는 무조건 리시브 큐에 넣는다. 넣은 후에 해석한다.이
		// retval에는, 이번에 링버퍼에 넣어야 하는 크기가 있다.
		DWORD BuffArray = 0;
		while (retval > 0)
		{
			int EnqueueCheck = RecvBuff.Enqueue(&TempBuff[BuffArray], retval);

			// -1이면 링버퍼가 꽉찬것.
			if (EnqueueCheck == -1)
			{
				ErrorTextOut(_T("RecvProc(). 인큐 중 링버퍼 꽉참. 접속 종료"));
				return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
			}

			// -1이 아니면 일단 넣었다는 뜻(사이즈에 0을 넣으면 0을 반환하는데, 이럴 경우는 가정하지 않는다.)
			retval -= EnqueueCheck;
			BuffArray += EnqueueCheck;
		}
	}


	//////////////////////////////////////////////////
	// 완료 패킷 처리 부분
	// RecvBuff에 들어있는 모든 완성 패킷을 처리한다.
	//////////////////////////////////////////////////
	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)
		st_NETWORK_PACKET_HEADER st_header;
		int len = sizeof(st_header);

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (RecvBuff.GetUseSize() < len)
			break;
		

		// 2. 헤더를 Peek으로 확인한다.		
		DWORD BuffArray = 0;
		while (len > 0)
		{
			int PeekSize = RecvBuff.Peek((char*)&st_header + BuffArray, len);

			// Peek()은 버퍼가 비었으면 -1을 반환한다. 버퍼가 비었으니 일단 끊는다.
			if (PeekSize == -1)
			{
				ErrorTextOut(_T("RecvProc(). 완료패킷 헤더 처리 중 RecvBuff비었음. 접속 종료"));
				return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
			}

			len -= PeekSize;
			BuffArray += PeekSize;
		}
		len = sizeof(st_header);	// 위 while문을 돌면서 len을 감소시켰으니, 헤더 사이즈 다시 넣기.


		// 3. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (st_header.byCode != dfNETWORK_PACKET_CODE)
		{
			ErrorTextOut(_T("RecvProc(). 완료패킷 헤더 처리 중 정상패킷 아님. 접속 종료"));
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}


		// 4. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 + Len + 엔드코드)
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (RecvBuff.GetUseSize() < (len + st_header.bySize + 1))
			break;


		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		RecvBuff.RemoveData(len);


		// 6. RecvBuff에서 Len만큼 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		BuffArray = 0;
		char PayloadBuff[1000];
		DWORD PayloadSize = st_header.bySize;

		while (PayloadSize > 0)
		{
			int DequeueSize = RecvBuff.Dequeue(&PayloadBuff[BuffArray], PayloadSize);

			// Dequeue()는 버퍼가 비었으면 -1을 반환. 버퍼가 비었으면 일단 끊는다.
			if (DequeueSize == -1)
			{
				ErrorTextOut(_T("RecvProc(). 완료패킷 페이로드 처리 중 RecvBuff비었음. 접속 종료"));
				return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
			}

			PayloadSize -= DequeueSize;
			BuffArray += DequeueSize;
		}

		// 7. RecvBuff에서 엔드코드 1Byte뽑음.	(디큐이다. Peek 아님)
		char EndCode;
		int DequeueSize = RecvBuff.Dequeue(&EndCode, 1);
		if (DequeueSize == -1)
		{
			ErrorTextOut(_T("RecvProc(). 완료패킷 엔드코드 처리 중 RecvBuff비었음. 접속 종료"));
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 8. 엔드코드 확인
		if(EndCode != dfNETWORK_PACKET_END)
		{
			ErrorTextOut(_T("RecvProc(). 완료패킷 엔드코드 처리 중 정상패킷 아님. 접속 종료"));
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 9. 헤더의 타입에 따라 분기처리. 함수로 처리한다.
		BOOL check = PacketProc(st_header.byType, PayloadBuff);
		if (check == FALSE)
			return FALSE;
	}	
	
	return TRUE;
}

// 실제 Send 함수
BOOL SendProc()
{
	// 1. 임시 버퍼 선언
	char TempBuff[1000];

	// 2. Send가능 여부 확인.
	if (bSendFlag == FALSE)
		return TRUE;

	// 3. SendBuff에 보낼 데이터가 있는지 확인.
	if (SendBuff.GetUseSize() == 0)
		return TRUE;

	int size = SendBuff.GetUseSize();
	// SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 4. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (SendBuff.GetUseSize() == 0)
			break;

		// 5. SendBuff의 데이터를 임시버퍼로 Peek
		int PeekSize = SendBuff.Peek(TempBuff, size);
		if (PeekSize == -1)
		{
			ErrorTextOut(_T("SendProc(). Peek중 버퍼 비었음. 접속 종료"));
			return FALSE;
		}

		// 6. Send()
		int SendSize = send(client_sock, TempBuff, PeekSize, 0);

		// 7. 에러 체크. 우드블럭이면 더 이상 못보내니 SendFlag를 FALSE로 만들고 while문 종료
		if (SendSize == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				bSendFlag = FALSE;
				break;
			}
			else
			{
				ErrorTextOut(_T("SendProc(). Send중 에러 발생. 접속 종료"));
				return FALSE;
			}
		}

		// 8. 보낸 사이즈가 나왔으면, 그 만큼 remove
		size -= SendSize;
		SendBuff.RemoveData(SendSize);
	}

	return TRUE;
}

// 패킷 타입에 따른 패킷처리 함수
BOOL PacketProc(BYTE PacketType, char* Packet)
{
	BOOL check;
	// Recv데이터 처리
	switch (PacketType)
	{
		// 내 캐릭터 생성
	case dfPACKET_SC_CREATE_MY_CHARACTER:
		check = PacketProc_CharacterCreate_My(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 다른 사람 캐릭터 생성
	case dfPACKET_SC_CREATE_OTHER_CHARACTER:
		check = PacketProc_CharacterCreate_Other(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 캐릭터 삭제
	case dfPACKET_SC_DELETE_CHARACTER:
		check = PacketProc_CharacterDelete(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 다른 캐릭터 이동 시작 패킷
	case dfPACKET_SC_MOVE_START:
		check = PacketProc_MoveStart(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 다른 캐릭터 이동 중지 패킷
	case dfPACKET_SC_MOVE_STOP:
		check = PacketProc_MoveStop(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 다른 캐릭터 공격 패킷 (1번 공격)
	case dfPACKET_SC_ATTACK1:
		check = PacketProc_Atk_01(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 다른 캐릭터 공격 패킷 (2번 공격)
	case dfPACKET_SC_ATTACK2:
		check = PacketProc_Atk_02(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 다른 캐릭터 공격 패킷 (3번 공격)
	case dfPACKET_SC_ATTACK3:
		check = PacketProc_Atk_03(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;

		// 데미지 패킷
	case dfPACKET_SC_DAMAGE:
		check = PacketProc_Damage(Packet);
		if (check = FALSE)
			return FALSE;
		return TRUE;


	default:
		ErrorTextOut(_T("PacketProc(). 알 수 없는 패킷"));
		return FALSE;
	}
}


////////////////////////////////////////////////
// PacketProc()에서 패킷 처리 시 호출하는 함수
///////////////////////////////////////////////
// [내 캐릭터 생성 패킷] 처리 함수
BOOL PacketProc_CharacterCreate_My(char* Packet)
{
	if (g_pPlayerObject != NULL)
		ErrorTextOut(_T("PacketProc_CharacterCreate_My(). 있는 캐릭터 또 생성하라고 함!"));		// 이거 에러처리 해야하는지 고민중..

	stPACKET_SC_CREATE_CHARACTER* CreatePacket = (stPACKET_SC_CREATE_CHARACTER*)Packet;
	MyID = CreatePacket->ID;

	// 플레이어 ID, 오브젝트 타입(1: 플레이어 / 2: 이펙트), X좌표, Y좌표, 바라보는 방향 세팅
	g_pPlayerObject = new PlayerObject(CreatePacket->ID, 1, CreatePacket->X, CreatePacket->Y, CreatePacket->Direction);
	
	// 내 캐릭터 여부, HP 셋팅
	((PlayerObject*)g_pPlayerObject)->MemberSetFunc(TRUE, CreatePacket->HP);	

	// 최초 액션 세팅 (아이들 모션)
	g_pPlayerObject->ActionInput(dfACTION_IDLE);

	// 최초 시작 스프라이트 셋팅
	if(CreatePacket->Direction == dfPACKET_MOVE_DIR_RR)
		g_pPlayerObject->SetSprite(CSpritedib::ePLAYER_STAND_R01, 5, 5);			
	else if (CreatePacket->Direction == dfPACKET_MOVE_DIR_LL)
		g_pPlayerObject->SetSprite(CSpritedib::ePLAYER_STAND_L01, 5, 5);			// 최초 시작 스프라이트 셋팅 (Idle 오른쪽)

	// 리스트에 추가
	list.push_back(g_pPlayerObject);

	return TRUE;
}

// [다른 사람 캐릭터 생성 패킷] 처리 함수
BOOL PacketProc_CharacterCreate_Other(char* Packet)
{
	stPACKET_SC_CREATE_CHARACTER* OtherCreatePacket = (stPACKET_SC_CREATE_CHARACTER*)Packet;

	// 플레이어 ID, 오브젝트 타입(1: 플레이어 / 2: 이펙트), X좌표, Y좌표, 바라보는 방향 세팅
	BaseObject* AddPlayer = new PlayerObject(OtherCreatePacket->ID, 1, OtherCreatePacket->X, OtherCreatePacket->Y, OtherCreatePacket->Direction);

	// 내 캐릭터 여부, HP 셋팅
	((PlayerObject*)AddPlayer)->MemberSetFunc(FALSE, OtherCreatePacket->HP);

	// 최초 액션 세팅 (아이들 모션)
	AddPlayer->ActionInput(dfACTION_IDLE);

	// 최초 시작 스프라이트 셋팅
	if (OtherCreatePacket->Direction == dfDIRECTION_RIGHT)
		AddPlayer->SetSprite(CSpritedib::ePLAYER_STAND_R01, 5, 5);
	else if (OtherCreatePacket->Direction == dfDIRECTION_LEFT)
		AddPlayer->SetSprite(CSpritedib::ePLAYER_STAND_L01, 5, 5);			// 최초 시작 스프라이트 셋팅 (Idle 오른쪽)

	// 리스트에 추가
	list.push_back(AddPlayer);

	return TRUE;
}

// [캐릭터 삭제 패킷] 처리 함수
BOOL PacketProc_CharacterDelete(char* Packet)
{
	stPACKET_SC_CREATE_CHARACTER* DeletePacket = (stPACKET_SC_CREATE_CHARACTER*)Packet;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == DeletePacket->ID)
		{
			// 2. 해당 유저를 리스트에서 제외 및 삭제
			list.erase(itor);
			return TRUE;
		}
	}	

	return FALSE;
}

// [다른 캐릭터 이동 시작] 패킷
BOOL PacketProc_MoveStart(char* Packet)
{
	stPACKET_SC_MOVE_START* MStartPacket = (stPACKET_SC_MOVE_START*)Packet;	

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* MovePlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == MStartPacket->ID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			MovePlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			MovePlayer->MoveCurXY(MStartPacket->X, MStartPacket->Y);

			// 액션 셋팅(마치 키 누른 것 처럼 셋팅)
			MovePlayer->ActionInput(MStartPacket->Direction);
			return TRUE;
		}
	}

	return FALSE;
}

// [다른 캐릭터 이동 중지] 패킷
BOOL PacketProc_MoveStop(char* Packet)
{
	stPACKET_SC_MOVE_STOP* MStopPacket = (stPACKET_SC_MOVE_STOP*)Packet;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* MovePlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == MStopPacket->ID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			MovePlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			MovePlayer->MoveCurXY(MStopPacket->X, MStopPacket->Y);

			// 아이들 액션 셋팅(마치 키 누른 것 처럼 셋팅)
			MovePlayer->ActionInput(dfACTION_IDLE);

			return TRUE;
		}
	}

	return FALSE;
}

// [다른 캐릭터 공격 1번 시작] 패킷
BOOL PacketProc_Atk_01(char* Packet)
{
	stPACKET_SC_ATTACK* AtkPacket = (stPACKET_SC_ATTACK*)Packet;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* AtkPlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == AtkPacket->ID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 방향 셋팅
			AtkPlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			AtkPlayer->MoveCurXY(AtkPacket->X, AtkPacket->Y);

			// 방향 셋팅
			AtkPlayer->DirChange(AtkPacket->Direction);

			// 해당 공격 키를 누른 것 처럼 처리
			AtkPlayer->ActionInput(dfACTION_ATTACK_01_LEFT);

			// 그리고 공격 타입 설정
			if(AtkPacket->Direction == dfPACKET_MOVE_DIR_LL)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_01_LEFT);
			if (AtkPacket->Direction == dfPACKET_MOVE_DIR_RR)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_01_RIGHT);

			return TRUE;
		}
	}

	return FALSE;
}

// [다른 캐릭터 공격 2번 시작] 패킷
BOOL PacketProc_Atk_02(char* Packet)
{
	stPACKET_SC_ATTACK* AtkPacket = (stPACKET_SC_ATTACK*)Packet;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* AtkPlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == AtkPacket->ID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			AtkPlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			AtkPlayer->MoveCurXY(AtkPacket->X, AtkPacket->Y);

			// 방향 셋팅
			AtkPlayer->DirChange(AtkPacket->Direction);

			// 해당 공격 키를 누른 것 처럼 처리
			AtkPlayer->ActionInput(dfACTION_ATTACK_02_LEFT);

			// 그리고 공격 타입 설정
			if (AtkPacket->Direction == dfPACKET_MOVE_DIR_LL)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_02_LEFT);
			if (AtkPacket->Direction == dfPACKET_MOVE_DIR_RR)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_02_RIGHT);

			return TRUE;
		}
	}

	return FALSE;
}

// [다른 캐릭터 공격 3번 시작] 패킷
BOOL PacketProc_Atk_03(char* Packet)
{
	stPACKET_SC_ATTACK* AtkPacket = (stPACKET_SC_ATTACK*)Packet;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* AtkPlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == AtkPacket->ID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			AtkPlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			AtkPlayer->MoveCurXY(AtkPacket->X, AtkPacket->Y);

			// 방향 셋팅
			AtkPlayer->DirChange(AtkPacket->Direction);

			// 해당 공격 키를 누른 것 처럼 처리
			AtkPlayer->ActionInput(dfACTION_ATTACK_03_LEFT);

			// 그리고 공격 타입 설정
			if (AtkPacket->Direction == dfPACKET_MOVE_DIR_LL)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_03_LEFT);
			if (AtkPacket->Direction == dfPACKET_MOVE_DIR_RR)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_03_RIGHT);

			return TRUE;
		}
	}

	return FALSE;

}

// [데미지] 패킷
BOOL PacketProc_Damage(char* Packet)
{
	stPACKET_SC_DAMAGE* DamagePacket = (stPACKET_SC_DAMAGE*)Packet;

	// 1. 해당 패킷에 날라온 피해자 유저를 찾아낸다.	
	CList<BaseObject*>::iterator itor;	

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == DamagePacket->DamageID)
		{
			// 2. 패킷에 날라온 HP를 피해자의 HP로 반영
			PlayerObject* DamagePlayer = (PlayerObject*)(*itor);

			// HP 세팅
			DamagePlayer->DamageFunc(DamagePacket->DamageHP, NULL);

			// 만약, 내가 피해자였다면 내 hp가 감소되었으 것이다. 내 HP가 0이 되면 게임 종료
			if (((PlayerObject*)g_pPlayerObject)->GetHP() == 0)
				return FALSE;

			break;
		}
	}

	// 3. 공격자 유저를 찾아낸 후 이펙트 생성
	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == DamagePacket->AttackID)
		{
			// 4. 공격자 객체에게 피해자 셋팅. 피해자가 없으면 이펙트 생성 안함.			
			PlayerObject* AtkPlayer = (PlayerObject*)(*itor);

			// -1을 적으면, HP는 반영을 안하겠다는 것.
			AtkPlayer->DamageFunc(-1, DamagePacket->DamageID);

			// 5. 피해자가 있으면 이펙트 생성!
			if (DamagePacket->DamageID != 0)
			{				
				// 공격자의 공격 타입(공격 1,2,3번)에 따라 이펙트 생성
				BaseObject* Effect = new EffectObject(NULL, 2, AtkPlayer->GetCurX(), AtkPlayer->GetCurY(), AtkPlayer->GetNowAtkType());

				// 생성한 이펙트의 최초 스프라이트 세팅
				Effect->SetSprite(CSpritedib::eEFFECT_SPARK_01, 4, 4);

				// 리스트에 등록
				list.push_back(Effect);
			}

			return TRUE;
		}
		
	}

	return FALSE;
}


/////////////////////////
// Send 패킷 만들기 함수
/////////////////////////
// 이동 패킷 만들기
void SendProc_MoveStart(st_NETWORK_PACKET_HEADER* header, stPACKET_CS_MOVE_START* packet, int dir, int x, int y)
{
	header->byCode = dfNETWORK_PACKET_CODE;
	header->bySize = sizeof(stPACKET_CS_MOVE_START);
	header->byType = dfPACKET_CS_MOVE_START;

	packet->Direction = dir;
	packet->X = x;
	packet->Y = y;
}

// 정지 패킷 만들기
void SendProc_MoveStop(st_NETWORK_PACKET_HEADER* header, stPACKET_CS_MOVE_STOP* packet, int dir, int x, int y)
{
	header->byCode = dfNETWORK_PACKET_CODE;
	header->bySize = sizeof(stPACKET_CS_MOVE_STOP);
	header->byType = dfPACKET_CS_MOVE_STOP;

	packet->Direction = dir;
	packet->X = x;
	packet->Y = y;
}

// 공격 패킷 만들기 (1번 공격)
void SendProc_Atk_01(st_NETWORK_PACKET_HEADER* header, stPACKET_CS_ATTACK* packet, int dir, int x, int y)
{
	header->byCode = dfNETWORK_PACKET_CODE;
	header->bySize = sizeof(stPACKET_CS_ATTACK);
	header->byType = dfPACKET_CS_ATTACK1;

	packet->Direction = dir;
	packet->X = x;
	packet->Y = y;
}

// 공격 패킷 만들기 (2번 공격)
void SendProc_Atk_02(st_NETWORK_PACKET_HEADER* header, stPACKET_CS_ATTACK* packet, int dir, int x, int y)
{
	header->byCode = dfNETWORK_PACKET_CODE;
	header->bySize = sizeof(stPACKET_CS_ATTACK);
	header->byType = dfPACKET_CS_ATTACK2;

	packet->Direction = dir;
	packet->X = x;
	packet->Y = y;
}

// 공격 패킷 만들기 (3번 공격)
void SendProc_Atk_03(st_NETWORK_PACKET_HEADER* header, stPACKET_CS_ATTACK* packet, int dir, int x, int y)
{
	header->byCode = dfNETWORK_PACKET_CODE;
	header->bySize = sizeof(stPACKET_CS_ATTACK);
	header->byType = dfPACKET_CS_ATTACK3;

	packet->Direction = dir;
	packet->X = x;
	packet->Y = y;
}


/////////////////////////
// Send 큐에 데이터 넣기
/////////////////////////
BOOL SendPacket(st_NETWORK_PACKET_HEADER* header, char* packet)
{
	// 1. 내 캐릭터가 생성되어있는 상태인지 체크. 아직 생성 안됐으면 그냥 돌아간다.
	if (g_pPlayerObject == NULL)
		return TRUE;

	// 2. 샌드 q에 헤더 넣기
	int Size = sizeof(st_NETWORK_PACKET_HEADER);
	DWORD BuffArray = 0;
	while (Size > 0)
	{
		int EnqueueCheck = SendBuff.Enqueue((char*)header + BuffArray, Size);
		if (EnqueueCheck == -1)
		{
			ErrorTextOut(_T("SendPacket() 헤더 넣는 중 샌드큐 꽉참. 접속종료"));
			return FALSE;
		}

		Size -= EnqueueCheck;
		BuffArray += EnqueueCheck;

	}

	// 3. 샌드 q에 페이로드 넣기
	int PayloadLen = header->bySize;
	BuffArray = 0;
	while (PayloadLen > 0)
	{
		int EnqueueCheck = SendBuff.Enqueue(packet + BuffArray, PayloadLen);
		if(EnqueueCheck == -1)
		{
			ErrorTextOut(_T("SendPacket() 페이로드 넣는 중 샌드큐 꽉참. 접속종료"));
			return FALSE;
		}

		PayloadLen -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	// 4. 샌드 q에 엔드코드 넣기
	char EndCode = dfNETWORK_PACKET_END;
	int EnqueueCheck = SendBuff.Enqueue(&EndCode, 1);
	if(EnqueueCheck == -1)
	{
		ErrorTextOut(_T("SendPacket() 엔드코드 넣는 중 샌드큐 꽉참. 접속종료"));
		return FALSE;
	}

	// 5. 실제 Send 시도 함수 호출
	SendProc();

	return TRUE;
}