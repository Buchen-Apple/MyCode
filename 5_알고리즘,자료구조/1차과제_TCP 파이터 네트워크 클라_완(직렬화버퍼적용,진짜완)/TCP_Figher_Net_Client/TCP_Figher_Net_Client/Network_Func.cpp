#include "stdafx.h"
#include "RingBuff.h"
#include "ProtocolBuff.h"
#include "ExceptionClass.h"

// Main과 공통 라이브러리
#include "PlayerObject.h"
#include "Network_Func.h"
#include "EffectObject.h"

// 전역 변수들
SOCKET client_sock;								// 내 소켓
BOOL bSendFlag = FALSE;							// 샌드 가능 플래그
BOOL bConnectFlag = FALSE;						// 커넥트 여부 플래그
CRingBuff SendBuff(100);						// 샌드 버퍼
CRingBuff RecvBuff(16);						// 리시브 버퍼
HWND WindowHwnd;								// 윈도우 핸들.
DWORD MyID;										// 내 ID할당 저장해두기
extern CList<BaseObject*> list;					// Main에서 할당한 객체 저장 리스트	
extern BaseObject* g_pPlayerObject;				// Main에서 할당 내 플레이어 객체

// 텍스트 아웃으로 에러 찍는 함수
void ErrorTextOut(const TCHAR* ErrorLog)
{
	TCHAR Log[100];
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
	// 테스트 코드
	static  int a = 0;
	
	// bConnectFlag(커넥트 여부)가 FALSE라면 아직 연결 안된거니 그냥 리턴.
	if (bConnectFlag == FALSE)
		return TRUE;

	//////////////////////////////////////////////////
	// recv()
	// 받을게 없을 때 까지. WSAGetLastError()에 우드블럭이 떠야만 빠져나갈 수 있다.
	//////////////////////////////////////////////////

	// 테스트 코드
	if (a == 0)
	{
		int* rear = (int*)RecvBuff.GetWriteBufferPtr();
		int* front = (int*)RecvBuff.GetReadBufferPtr();

		*rear = 1;
		*front = 1;
		a = 1;
	}

	// 1. 리시브 링버퍼로 recv()
	char* recvbuff = RecvBuff.GetBufferPtr();

	// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = RecvBuff.GetNotBrokenPutSize();

	// 3. 한 번에 쓸 수 있는 공간이 0이라면
	if (Size == 0)
	{
		// rear 1칸 이동
		RecvBuff.MoveWritePos(1);

		// 그리고 한 번에 쓸 수 있는 위치 다시 알기.
		Size = RecvBuff.GetNotBrokenPutSize();
	}

	// 4. 그게 아니라면, 1칸 이동만 하기. 		
	else
	{
		// rear 1칸 이동
		RecvBuff.MoveWritePos(1);
	}

	// 5. 1칸 이동된 rear 얻어오기
	int* rear = (int*)RecvBuff.GetWriteBufferPtr();

	
	// 6. recv()
	int retval = recv(client_sock, &recvbuff[*rear], Size, 0);

	// 7. 리시브 에러체크
	if (retval == SOCKET_ERROR)
	{
		// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있다는 것. recv할게 없다는 것이니 while문 종료
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return TRUE;

		// WSAEWOULDBLOCK에러가 아니면 뭔가 이상한 것이니 접속 종료
		else
		{
			TCHAR Test[80] = _T("RecvProc(). retval 후 우드블럭이 아닌 에러가 뜸. 접속 종료");
			TCHAR ErrorText[80];

			swprintf_s(ErrorText, _countof(ErrorText), _T("%s %d"), Test, WSAGetLastError());
			ErrorTextOut(ErrorText);
			return FALSE;		// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	RecvBuff.MoveWritePos(retval -1);

	//////////////////////////////////////////////////
	// 완료 패킷 처리 부분
	// RecvBuff에 들어있는 모든 완성 패킷을 처리한다.
	//////////////////////////////////////////////////
	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)		
		int len = dfNETWORK_PACKET_HEADER_SIZE;
		BYTE HeaderBuff[dfNETWORK_PACKET_HEADER_SIZE];

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (RecvBuff.GetUseSize() < len)
			break;		

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = RecvBuff.Peek((char*)&HeaderBuff, len);

		// Peek()은 버퍼가 비었으면 -1을 반환한다. 버퍼가 비었으니 일단 끊는다.
		if (PeekSize == -1)
		{
			int* rear = (int*)RecvBuff.GetReadBufferPtr();
			int* front = (int*)RecvBuff.GetWriteBufferPtr();

			ErrorTextOut(_T("RecvProc(). 완료패킷 헤더 처리 중 RecvBuff비었음. 접속 종료"));
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 3. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (HeaderBuff[0] != dfNETWORK_PACKET_CODE)
		{
			int* rear = (int*)RecvBuff.GetReadBufferPtr();
			int* front = (int*)RecvBuff.GetWriteBufferPtr();
			int Size = RecvBuff.GetUseSize();

			ErrorTextOut(_T("RecvProc(). 완료패킷 헤더 처리 중 정상패킷 아님. 접속 종료"));				
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 4. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 + Len + 엔드코드)
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (RecvBuff.GetUseSize() < (len + HeaderBuff[1] + 1))
			break;

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 Len만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		DWORD BuffArray = 0;
		char PayloadBuff[1000];
		DWORD PayloadSize = HeaderBuff[1];

		while (PayloadSize > 0)
		{
			int DequeueSize = RecvBuff.Dequeue(&PayloadBuff[BuffArray], PayloadSize);

			// Dequeue()는 버퍼가 비었으면 -1을 반환. 버퍼가 비었으면 일단 끊는다.
			if (DequeueSize == -1)
			{
				int* rear = (int*)RecvBuff.GetReadBufferPtr();
				int* front = (int*)RecvBuff.GetWriteBufferPtr();
				int Size = RecvBuff.GetUseSize();

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
			int* rear = (int*)RecvBuff.GetReadBufferPtr();
			int* front = (int*)RecvBuff.GetWriteBufferPtr();
			int Size = RecvBuff.GetUseSize();

			ErrorTextOut(_T("RecvProc(). 완료패킷 엔드코드 처리 중 RecvBuff비었음. 접속 종료"));
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 8. 엔드코드 확인
		if(EndCode != dfNETWORK_PACKET_END)
		{
			int* rear = (int*)RecvBuff.GetReadBufferPtr();
			int* front = (int*)RecvBuff.GetWriteBufferPtr();
			int Size = RecvBuff.GetUseSize();

			ErrorTextOut(_T("RecvProc(). 완료패킷 엔드코드 처리 중 정상패킷 아님. 접속 종료"));
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 9. 헤더의 타입에 따라 분기처리. 함수로 처리한다.
		BOOL check = PacketProc(HeaderBuff[2], PayloadBuff);
		if (check == FALSE)
			return FALSE;
	}	
	
	return TRUE;
}

// 실제 Send 함수
BOOL SendProc()
{
	// 1. Send가능 여부 확인.
	if (bSendFlag == FALSE)
		return TRUE;

	// 2. SendBuff에 보낼 데이터가 있는지 확인.
	if (SendBuff.GetUseSize() == 0)
		return TRUE;	

	// 3. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = SendBuff.GetBufferPtr();

	// 4. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{		
		// 5. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (SendBuff.GetUseSize() == 0)
			break;

		// 6. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = SendBuff.GetNotBrokenGetSize();

		// 7. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 8. front 포인터 얻어옴
		int *front = (int*)SendBuff.GetReadBufferPtr();

		// 9. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = SendBuff.NextIndex(*front, 1);		
		
		// 10. Send()
		int SendSize = send(client_sock, &sendbuff[BuffArrayCheck], Size, 0);

		// 11. 에러 체크. 우드블럭이면 더 이상 못보내니 SendFlag를 FALSE로 만들고 while문 종료
		if (SendSize == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				bSendFlag = FALSE;
				break;
			}
			else
			{
				TCHAR Test[80] = _T("SendProc(). Send중 에러 발생. 접속 종료");
				TCHAR ErrorText[80];

				swprintf_s(ErrorText, _countof(ErrorText), _T("%s %d"), Test, WSAGetLastError());
				ErrorTextOut(ErrorText);
				return FALSE;
			}
		}

		// 12. 보낸 사이즈가 나왔으면, 그 만큼 remove
		SendBuff.RemoveData(SendSize);

	}

	return TRUE;
}

// 패킷 타입에 따른 패킷처리 함수
BOOL PacketProc(BYTE PacketType, char* Packet)
{
	BOOL check;
	try
	{
		// Recv데이터 처리
		switch (PacketType)
		{
			// 내 캐릭터 생성
		case dfPACKET_SC_CREATE_MY_CHARACTER:
			check = PacketProc_CharacterCreate_My(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 다른 사람 캐릭터 생성
		case dfPACKET_SC_CREATE_OTHER_CHARACTER:
			check = PacketProc_CharacterCreate_Other(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 캐릭터 삭제
		case dfPACKET_SC_DELETE_CHARACTER:
			check = PacketProc_CharacterDelete(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 다른 캐릭터 이동 시작 패킷
		case dfPACKET_SC_MOVE_START:
			check = PacketProc_MoveStart(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 다른 캐릭터 이동 중지 패킷
		case dfPACKET_SC_MOVE_STOP:
			check = PacketProc_MoveStop(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 다른 캐릭터 공격 패킷 (1번 공격)
		case dfPACKET_SC_ATTACK1:
			check = PacketProc_Atk_01(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 다른 캐릭터 공격 패킷 (2번 공격)
		case dfPACKET_SC_ATTACK2:
			check = PacketProc_Atk_02(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 다른 캐릭터 공격 패킷 (3번 공격)
		case dfPACKET_SC_ATTACK3:
			check = PacketProc_Atk_03(Packet);
			if (check = FALSE)
				return FALSE;
			break;

			// 데미지 패킷
		case dfPACKET_SC_DAMAGE:
			check = PacketProc_Damage(Packet);
			if (check = FALSE)
				return FALSE;
			break;


		default:
			ErrorTextOut(_T("PacketProc(). 알 수 없는 패킷"));
			return FALSE;
		}

	}
	catch (CException exc)
	{
		TCHAR* text = (TCHAR*)exc.GetExceptionText();
		ErrorTextOut(text);
		return FALSE;
	}

	return TRUE;
}


////////////////////////////////////////////////
// PacketProc()에서 패킷 처리 시 호출하는 함수
///////////////////////////////////////////////
// [내 캐릭터 생성 패킷] 처리 함수
BOOL PacketProc_CharacterCreate_My(char* Packet)
{
	if (g_pPlayerObject != NULL)
		ErrorTextOut(_T("PacketProc_CharacterCreate_My(). 있는 캐릭터 또 생성하라고 함!"));		// 이거 에러처리 해야하는지 고민중..

	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff CreatePacket(dfPACKET_SC_CREATE_MY_CHARACTER_SIZE);
	for (int i = 0; i < dfPACKET_SC_CREATE_MY_CHARACTER_SIZE; ++i)
		CreatePacket << Packet[i];

	// ID, Dir, X, Y, HP 분리
	DWORD	byID;
	BYTE	byDir;
	WORD	byX;
	WORD	byY;
	BYTE	byHP;

	CreatePacket >> byID;
	CreatePacket >> byDir;
	CreatePacket >> byX;
	CreatePacket >> byY;
	CreatePacket >> byHP;

	// 플레이어 ID, 오브젝트 타입(1: 플레이어 / 2: 이펙트), X좌표, Y좌표, 바라보는 방향 세팅
	g_pPlayerObject = new PlayerObject(byID, 1, byX, byY, byDir);

	// 내 캐릭터 여부, HP 셋팅
	((PlayerObject*)g_pPlayerObject)->MemberSetFunc(TRUE, byHP);

	// 최초 액션 세팅 (아이들 모션)
	g_pPlayerObject->ActionInput(dfACTION_IDLE);

	// 최초 시작 스프라이트 셋팅
	if(byDir == dfPACKET_MOVE_DIR_RR)
		g_pPlayerObject->SetSprite(CSpritedib::ePLAYER_STAND_R01, 5, 5);			
	else if (byDir == dfPACKET_MOVE_DIR_LL)
		g_pPlayerObject->SetSprite(CSpritedib::ePLAYER_STAND_L01, 5, 5);			// 최초 시작 스프라이트 셋팅 (Idle 오른쪽)

	// 리스트에 추가
	list.push_back(g_pPlayerObject);

	return TRUE;
}

// [다른 사람 캐릭터 생성 패킷] 처리 함수
BOOL PacketProc_CharacterCreate_Other(char* Packet)
{
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff OtherCreatePacket(dfPACKET_SC_CREATE_OTHER_CHARACTER_SIZE);
	for (int i = 0; i < dfPACKET_SC_CREATE_OTHER_CHARACTER_SIZE; ++i)
		OtherCreatePacket << Packet[i];

	// ID, Dir, X, Y, HP 분리
	DWORD	byID;
	BYTE	byDir;
	WORD	byX;
	WORD	byY;
	BYTE	byHP;

	OtherCreatePacket >> byID;
	OtherCreatePacket >> byDir;
	OtherCreatePacket >> byX;
	OtherCreatePacket >> byY;
	OtherCreatePacket >> byHP;

	// 플레이어 ID, 오브젝트 타입(1: 플레이어 / 2: 이펙트), X좌표, Y좌표, 바라보는 방향 세팅
	BaseObject* AddPlayer = new PlayerObject(byID, 1, byX, byY, byDir);

	// 내 캐릭터 여부, HP 셋팅
	((PlayerObject*)AddPlayer)->MemberSetFunc(FALSE, byHP);

	// 최초 액션 세팅 (아이들 모션)
	AddPlayer->ActionInput(dfACTION_IDLE);

	// 최초 시작 스프라이트 셋팅
	if (byDir == dfDIRECTION_RIGHT)
		AddPlayer->SetSprite(CSpritedib::ePLAYER_STAND_R01, 5, 5);
	else if (byDir == dfDIRECTION_LEFT)
		AddPlayer->SetSprite(CSpritedib::ePLAYER_STAND_L01, 5, 5);			// 최초 시작 스프라이트 셋팅 (Idle 오른쪽)

	// 리스트에 추가
	list.push_back(AddPlayer);

	return TRUE;
}

// [캐릭터 삭제 패킷] 처리 함수
BOOL PacketProc_CharacterDelete(char* Packet)
{
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff DeletePacket(dfPACKET_SC_DELETE_CHARACTER_SIZE);
	for (int i = 0; i < dfPACKET_SC_DELETE_CHARACTER_SIZE; ++i)
		DeletePacket << Packet[i];

	// ID분리
	DWORD	byID;
	DeletePacket >> byID;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == byID)
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
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff MStartPacket(dfPACKET_SC_MOVE_START_SIZE);
	for (int i = 0; i < dfPACKET_SC_MOVE_START_SIZE; ++i)
		MStartPacket << Packet[i];

	// ID, Dir, X, Y 분리
	DWORD	byID;
	BYTE	byDir;
	WORD	byX;
	WORD	byY;

	MStartPacket >> byID;
	MStartPacket >> byDir;
	MStartPacket >> byX;
	MStartPacket >> byY;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* MovePlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == byID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			MovePlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			MovePlayer->MoveCurXY(byX, byY);

			// 액션 셋팅(마치 키 누른 것 처럼 셋팅)
			MovePlayer->ActionInput(byDir);
			return TRUE;
		}
	}

	return FALSE;
}

// [다른 캐릭터 이동 중지] 패킷
BOOL PacketProc_MoveStop(char* Packet)
{
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff MStopPacket(dfPACKET_SC_MOVE_STOP_SIZE);
	for (int i = 0; i < dfPACKET_SC_MOVE_STOP_SIZE; ++i)
		MStopPacket << Packet[i];

	// ID, Dir, X, Y 분리
	DWORD	byID;
	BYTE	byDir;
	WORD	byX;
	WORD	byY;

	MStopPacket >> byID;
	MStopPacket >> byDir;
	MStopPacket >> byX;
	MStopPacket >> byY;


	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* MovePlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == byID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			MovePlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			MovePlayer->MoveCurXY(byX, byY);

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
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff AtkPacket(dfPACKET_SC_ATTACK1_SIZE);
	for (int i = 0; i < dfPACKET_SC_ATTACK1_SIZE; ++i)
		AtkPacket << Packet[i];

	// ID, Dir, X, Y 분리
	DWORD	byID;
	BYTE	byDir;
	WORD	byX;
	WORD	byY;

	AtkPacket >> byID;
	AtkPacket >> byDir;
	AtkPacket >> byX;
	AtkPacket >> byY;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* AtkPlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == byID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 방향 셋팅
			AtkPlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			AtkPlayer->MoveCurXY(byX, byY);

			// 방향 셋팅
			AtkPlayer->DirChange(byDir);

			// 해당 공격 키를 누른 것 처럼 처리
			AtkPlayer->ActionInput(dfACTION_ATTACK_01_LEFT);

			// 그리고 공격 타입 설정
			if (byDir == dfPACKET_MOVE_DIR_LL)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_01_LEFT);
			if (byDir == dfPACKET_MOVE_DIR_RR)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_01_RIGHT);

			return TRUE;
		}
	}

	return FALSE;
}

// [다른 캐릭터 공격 2번 시작] 패킷
BOOL PacketProc_Atk_02(char* Packet)
{
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff AtkPacket(dfPACKET_SC_ATTACK2_SIZE);
	for (int i = 0; i < dfPACKET_SC_ATTACK2_SIZE; ++i)
		AtkPacket << Packet[i];

	// ID, Dir, X, Y 분리
	DWORD	byID;
	BYTE	byDir;
	WORD	byX;
	WORD	byY;

	AtkPacket >> byID;
	AtkPacket >> byDir;
	AtkPacket >> byX;
	AtkPacket >> byY;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* AtkPlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == byID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			AtkPlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			AtkPlayer->MoveCurXY(byX, byY);

			// 방향 셋팅
			AtkPlayer->DirChange(byDir);

			// 해당 공격 키를 누른 것 처럼 처리
			AtkPlayer->ActionInput(dfACTION_ATTACK_02_LEFT);

			// 그리고 공격 타입 설정
			if (byDir == dfPACKET_MOVE_DIR_LL)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_02_LEFT);
			if (byDir == dfPACKET_MOVE_DIR_RR)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_02_RIGHT);

			return TRUE;
		}
	}

	return FALSE;
}

// [다른 캐릭터 공격 3번 시작] 패킷
BOOL PacketProc_Atk_03(char* Packet)
{
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff AtkPacket(dfPACKET_SC_ATTACK3_SIZE);
	for (int i = 0; i < dfPACKET_SC_ATTACK3_SIZE; ++i)
		AtkPacket << Packet[i];

	// ID, Dir, X, Y 분리
	DWORD	byID;
	BYTE	byDir;
	WORD	byX;
	WORD	byY;

	AtkPacket >> byID;
	AtkPacket >> byDir;
	AtkPacket >> byX;
	AtkPacket >> byY;

	// 1. 해당 패킷에 날라온 ID의 유저를 찾아낸다.
	CList<BaseObject*>::iterator itor;
	PlayerObject* AtkPlayer;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == byID)
		{
			// 2. 패킷에 날라온 Dir을 보고 X,Y와 액션을 셋팅 (8방향)
			AtkPlayer = (PlayerObject*)(*itor);

			// x, y 셋팅
			AtkPlayer->MoveCurXY(byX, byY);

			// 방향 셋팅
			AtkPlayer->DirChange(byDir);

			// 해당 공격 키를 누른 것 처럼 처리
			AtkPlayer->ActionInput(dfACTION_ATTACK_03_LEFT);

			// 그리고 공격 타입 설정
			if (byDir == dfPACKET_MOVE_DIR_LL)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_03_LEFT);
			if (byDir == dfPACKET_MOVE_DIR_RR)
				AtkPlayer->ChangeNowAtkType(dfACTION_ATTACK_03_RIGHT);

			return TRUE;
		}
	}

	return FALSE;
}

// [데미지] 패킷
BOOL PacketProc_Damage(char* Packet)
{
	// 직렬화 버퍼에 패킷 넣기
	CProtocolBuff DamagePacket(dfPACKET_SC_DAMAGE_SIZE);
	for (int i = 0; i < dfPACKET_SC_DAMAGE_SIZE; ++i)
		DamagePacket << Packet[i];

	// 공격자 ID, 피해자 ID, 피해자의 남은 HP 분리
	DWORD	AttackID;
	DWORD	DamageID;
	BYTE	DamageHP;

	DamagePacket >> AttackID;
	DamagePacket >> DamageID;
	DamagePacket >> DamageHP;


	// 1. 해당 패킷에 날라온 피해자 유저를 찾아낸다.	
	CList<BaseObject*>::iterator itor;

	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == DamageID)
		{
			// 2. 패킷에 날라온 HP를 피해자의 HP로 반영
			PlayerObject* DamagePlayer = (PlayerObject*)(*itor);

			// HP 세팅
			DamagePlayer->DamageFunc(DamageHP, NULL);

			// 만약, 내가 피해자였다면 내 hp가 감소되었으 것이다. 내 HP가 0이 되면 게임 종료
			if (((PlayerObject*)g_pPlayerObject)->GetHP() == 0)
				return FALSE;

			break;
		}
	}

	// 3. 공격자 유저를 찾아낸 후 이펙트 생성
	for (itor = list.begin(); itor != list.end(); itor++)
	{
		if ((*itor)->GetObjectID() == AttackID)
		{
			// 4. 공격자 객체에게 피해자 셋팅. 피해자가 없으면 이펙트 생성 안함.			
			PlayerObject* AtkPlayer = (PlayerObject*)(*itor);

			// -1을 적으면, HP는 반영을 안하겠다는 것.
			AtkPlayer->DamageFunc(-1, DamageID);

			// 5. 피해자가 있으면 이펙트 생성!
			if (DamageID != 0)
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
void SendProc_MoveStart(char* header, char* packet, int dir, int x, int y)
{
	// 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfNETWORK_PACKET_CODE;
	BYTE bySize = dfPACKET_CS_MOVE_START_SIZE;
	BYTE byType = dfPACKET_CS_MOVE_START;

	*headerBuff << byCode;
	*headerBuff << bySize;
	*headerBuff << byType;

	// 이동 시작 패킷 조립
	CProtocolBuff* paylodBuff = (CProtocolBuff*)packet;

	BYTE byDir = dir;
	WORD byX = x;
	WORD byY = y;

	*paylodBuff << byDir;
	*paylodBuff << byX;
	*paylodBuff << byY;
}

// 정지 패킷 만들기
void SendProc_MoveStop(char* header, char* packet, int dir, int x, int y)
{
	// 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfNETWORK_PACKET_CODE;
	BYTE bySize = dfPACKET_CS_MOVE_STOP_SIZE;
	BYTE byType = dfPACKET_CS_MOVE_STOP;

	*headerBuff << byCode;
	*headerBuff << bySize;
	*headerBuff << byType;

	// 정지 패킷 조립
	CProtocolBuff* paylodBuff = (CProtocolBuff*)packet;

	BYTE byDir = dir;
	WORD byX = x;
	WORD byY = y;

	*paylodBuff << byDir;
	*paylodBuff << byX;
	*paylodBuff << byY;
}

// 공격 패킷 만들기 (1번 공격)
void SendProc_Atk_01(char* header, char* packet, int dir, int x, int y)
{
	// 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfNETWORK_PACKET_CODE;
	BYTE bySize = dfPACKET_CS_ATTACK1_SIZE;
	BYTE byType = dfPACKET_CS_ATTACK1;

	*headerBuff << byCode;
	*headerBuff << bySize;
	*headerBuff << byType;

	// 공격 패킷 조립
	CProtocolBuff* paylodBuff = (CProtocolBuff*)packet;

	BYTE byDir = dir;
	WORD byX = x;
	WORD byY = y;

	*paylodBuff << byDir;
	*paylodBuff << byX;
	*paylodBuff << byY;
}

// 공격 패킷 만들기 (2번 공격)
void SendProc_Atk_02(char* header, char* packet, int dir, int x, int y)
{

	// 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfNETWORK_PACKET_CODE;
	BYTE bySize = dfPACKET_CS_ATTACK2_SIZE;
	BYTE byType = dfPACKET_CS_ATTACK2;

	*headerBuff << byCode;
	*headerBuff << bySize;
	*headerBuff << byType;

	// 공격 패킷 조립
	CProtocolBuff* paylodBuff = (CProtocolBuff*)packet;

	BYTE byDir = dir;
	WORD byX = x;
	WORD byY = y;

	*paylodBuff << byDir;
	*paylodBuff << byX;
	*paylodBuff << byY;
}

// 공격 패킷 만들기 (3번 공격)
void SendProc_Atk_03(char* header, char* packet, int dir, int x, int y)
{
	// 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfNETWORK_PACKET_CODE;
	BYTE bySize = dfPACKET_CS_ATTACK3_SIZE;
	BYTE byType = dfPACKET_CS_ATTACK3;

	*headerBuff << byCode;
	*headerBuff << bySize;
	*headerBuff << byType;

	// 공격 패킷 조립
	CProtocolBuff* paylodBuff = (CProtocolBuff*)packet;

	BYTE byDir = dir;
	WORD byX = x;
	WORD byY = y;

	*paylodBuff << byDir;
	*paylodBuff << byX;
	*paylodBuff << byY;
}


/////////////////////////
// Send 링버퍼에 데이터 넣기
/////////////////////////
BOOL SendPacket(char* header, char* packet)
{
	// 1. 내 캐릭터가 생성되어있는 상태인지 체크. 아직 생성 안됐으면 그냥 돌아간다.
	if (g_pPlayerObject == NULL)
		return TRUE;

	// 2. 샌드 q에 헤더 넣기
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	int Size = dfNETWORK_PACKET_HEADER_SIZE;
	DWORD BuffArray = 0;
	while (Size > 0)
	{
		int EnqueueCheck = SendBuff.Enqueue(headerBuff->GetBufferPtr() + BuffArray, Size);
		if (EnqueueCheck == -1)
		{
			ErrorTextOut(_T("SendPacket() 헤더 넣는 중 샌드큐 꽉참. 접속종료"));
			return FALSE;
		}

		Size -= EnqueueCheck;
		BuffArray += EnqueueCheck;

	}

	// 3. 샌드 q에 페이로드 넣기
	CProtocolBuff* payloadBuff = (CProtocolBuff*)packet;

	BYTE PayloadLen;
	*headerBuff >> PayloadLen;
	*headerBuff >> PayloadLen;

	BuffArray = 0;
	while (PayloadLen > 0)
	{
		int EnqueueCheck = SendBuff.Enqueue(payloadBuff->GetBufferPtr() + BuffArray, PayloadLen);
		if (EnqueueCheck == -1)
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
	if (EnqueueCheck == -1)
	{
		ErrorTextOut(_T("SendPacket() 엔드코드 넣는 중 샌드큐 꽉참. 접속종료"));
		return FALSE;
	}

	// 5. 실제 Send 시도 함수 호출
	SendProc();

	return TRUE;
}