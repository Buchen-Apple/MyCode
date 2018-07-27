#include "stdafx.h"
#include "Chat_Client.h"

#pragma comment(lib, "ws2_32")
#include <WS2tcpip.h>

#include "Network_Func.h"
#include "RingBuff\RingBuff.h"
#pragma warning(disable:4996)

#include <list>
#include <map>

using namespace std;

// 유저 구조체
struct stUser
{
	DWORD m_UserID;
	TCHAR m_NickName[dfNICK_MAX_LEN] = _T("0");	// 유저 닉네임
};

// 룸 구조체
struct stRoom
{
	DWORD m_RoomID;							// 룸 고유 ID
	TCHAR* m_RoomName;						// 룸 이름

	map <DWORD, stUser*> m_JoinUserList;	// 해당 룸에 접속중인 유저들. 유저 ID가 key	
};


// 윈도우 관련 전역변수들
HINSTANCE Instance;								// 내 인스턴스
HWND LobbyhWnd;									// 로비 다이얼로그 핸들
HWND* RoomhWnd;									// 룸 다이얼로그 핸들
int nCmdShow;									// cmdShow 저장

// 통신 관련 전역변수들
SOCKET client_sock;								// 내 소켓
BOOL bConnectFlag;								// 연결 여부를 체크하는 변수.
BOOL bSendFlag;									// Send 가능 상태/ 불가능상태를 체크하는 변수
CRingBuff SendBuff(1000);						// 내 샌드 버퍼
CRingBuff RecvBuff(1000);						// 내 리시브 버퍼

// 내 정보 관련 전역변수들
TCHAR g_NickName[dfNICK_MAX_LEN];				// 내 닉네임
DWORD g_MyUserID;								// 내 유저 ID
DWORD g_JoinRoomNumber;							// 내가 접속중인 방 ID.
stRoom* MyRoom;									// 내가 입장중인 방

// 그 외 기타 전역변수
map <DWORD, stRoom*> map_RoomList;				// 방 목록(map)
TCHAR MyHopeRoomName[ROOMNAME_SIZE];			// 내가 요청한 방 이름을 저장하는 변수. [채팅방 생성 성공!] 메시지박스를 띄워야하는지 체크하는 용도

// RoomDialogProc() 함수 extern 선언
extern INT_PTR CALLBACK RoomDialolgProc(HWND, UINT, WPARAM, LPARAM);


/////////////////////////
// 기타 함수
/////////////////////////

// 각종 정보 전달받기. (로비 다이얼로그 핸들(메인), 인스턴스 등..)
void InfoGet(HWND g_LobbyhWnd, HWND* g_RoomhWnd, HINSTANCE hInst, int cmdShow)
{
	LobbyhWnd = g_LobbyhWnd;
	RoomhWnd = g_RoomhWnd;
	Instance = hInst;
	nCmdShow = cmdShow;

	// 최초, 내가 입장한 방은 NULL이다
	MyRoom = nullptr;
}

// 윈속 초기화, 소켓 연결, 커넥트 등 함수
BOOL NetworkInit(TCHAR tIP[30], TCHAR tNickName[dfNICK_MAX_LEN])
{
	_tcscpy_s(g_NickName, dfNICK_MAX_LEN, tNickName);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		MessageBox(LobbyhWnd, _T("윈속 초기화 실패"), _T("윈속실패"), MB_ICONERROR);
		return FALSE;
	}

	// 소켓 생성
	client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_sock == SOCKET_ERROR)
	{
		MessageBox(LobbyhWnd, _T("소켓 생성 실패"), _T("소켓실패"), MB_ICONERROR);
		return FALSE;
	}

	// WSAAsyncSelect(). 동시에 비동기 소켓이 된다.
	int retval = WSAAsyncSelect(client_sock, LobbyhWnd, WM_SOCK, FD_WRITE | FD_READ | FD_CONNECT | FD_CLOSE);
	if (retval == SOCKET_ERROR)
	{
		MessageBox(LobbyhWnd, _T("어싱크셀렉스 실패"), _T("어싱크셀렉트 실패"), MB_ICONERROR);
		return FALSE;
	}

	// Connect
	ULONG uIP;
	SOCKADDR_IN clientaddr;
	ZeroMemory(&clientaddr, sizeof(clientaddr));

	InetPton(AF_INET, tIP, &uIP);
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(dfNETWORK_PORT);
	clientaddr.sin_addr.s_addr = uIP;

	retval = connect(client_sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			MessageBox(LobbyhWnd, _T("커넥트 실패"), _T("커넥트 실패"), MB_ICONERROR);
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

// 네트워크 처리 함수
BOOL NetworkProc(LPARAM lParam)
{
	// 에러 먼저 처리
	// lParam의 상위 16비트는 오류코드가, 하위 16비트는 발생한 이벤트가 담겨있다.
	if (WSAGETSELECTERROR(lParam) != 0)
	{
		// 서버가 꺼져있는 상태
		if (WSAGETSELECTERROR(lParam) == 10061)
			MessageBox(LobbyhWnd, _T("NetworkProc(). 서버가 꺼져있습니다."), _T("접속 종료"), 0);

		else if (WSAGETSELECTERROR(lParam) == 10053)
			MessageBox(LobbyhWnd, _T("NetworkProc(). 상대가 갑자기 연결을 끊은 듯."), _T("접속 종료"), 0);

		// 그 외 상태
		else
			MessageBox(LobbyhWnd, _T("NetworkProc(). 네트워크 에러 발생"), _T("접속 종료"), 0);

		return FALSE;
	}

	// 각종 네트워크 처리
	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_CONNECT:
		{
			bConnectFlag = TRUE;

			// 연결에 성공하면, "로그인 요청" 패킷을 샌드큐에 넣어둔다.
			// 그리고, FD_CONNECT 다음에 발생하는 FD_WRITE 때, 로그인 요청을 보낸다.
			// 접속이 되면, [로그인 요청 -> 로그인 요청 결과 Recv -> 대화방 목록 요청 -> 대화방 목록 요청 Recv -> 끝]의 절차로 데이터를 주고받는 형태.
			CProtocolBuff header(dfHEADER_SIZE);
			CProtocolBuff payload;

			// "로그인 요청" 패킷 제작
			CreatePacket_Req_Login((char*)&header, (char*)&payload);

			// Send버퍼에 넣기
			// 넣다 실패하면 연결 끊김.
			if (SendPacket(&header, &payload) == FALSE)
				return FALSE;
		}
		return TRUE;

	case FD_WRITE:
		bSendFlag = TRUE;

		// SendProc()에서 에러가 발생하면 FALSE가 리턴된다.
		if (SendProc() == FALSE)
			return FALSE;
		return TRUE;

	case FD_READ:
		// RecvProc()에서 에러가 발생하면 FALSE가 리턴된다.
		if (RecvProc() == FALSE)
			return FALSE;

		return TRUE;

	case FD_CLOSE:
		MessageBox(LobbyhWnd, _T("NetworkProc(). FD_CLOSE 발생."), _T("접속 종료"), 0);
		return FALSE;

	default:
		MessageBox(LobbyhWnd, _T("NetworkProc(). 알 수 없는 패킷"), _T("접속 종료"), 0);
		return TRUE;
	}
}

// 헤더 조립하기
void CreateHeader(CProtocolBuff* header, CProtocolBuff* payload, WORD Type)
{
	// 헤더 코드, 타입, 페이로드 사이즈 셋팅
	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = Type;
	WORD wPayloadSize = payload->GetUseSize();

	// 헤더의 체크썸 셋팅
	CProtocolBuff ChecksumBuff;
	DWORD UseSize = payload->GetUseSize();

	memcpy(ChecksumBuff.GetBufferPtr(), payload->GetBufferPtr(), UseSize);
	ChecksumBuff.MoveWritePos(UseSize);

	for (int i = 0; i< dfNICK_MAX_LEN; ++i)
		ChecksumBuff << g_NickName[i];

	DWORD Checksum = 0;
	for (int i = 0; i < wPayloadSize; ++i)
	{
		BYTE bPayloadByte;
		ChecksumBuff >> bPayloadByte;
		Checksum += bPayloadByte;
	}

	BYTE* MsgType = (BYTE*)&wMsgType;
	for (int i = 0; i < sizeof(wMsgType); ++i)
		Checksum += MsgType[i];

	BYTE SaveChecksum = Checksum % 256;

	// 헤더 완성하기
	*header << byCode;
	*header << SaveChecksum;
	*header << wMsgType;
	*header << wPayloadSize;
}

// 내 채팅을 만든 후 보내기. 그리고 내 에디터에 출력하기까지 하는 함수
void ChatLogic()
{
	// 1. 내가 입력한 문자를 에디터에서 가져온다. (룸 다이얼로그의 채팅 에디터이다)
	TCHAR Typing[CHAT_SIZE];
	GetDlgItemText(*RoomhWnd, IDC_ROOM_TYPING, Typing, CHAT_SIZE);
	DWORD size = (DWORD)_tcslen(Typing);

	// 2. "채팅 송신" 패킷 생성
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	CreatePacket_Req_ChatSend((char*)&header, (char*)&payload, size, Typing);

	// 3. 만든 패킷을 SendBuff에 넣는다.
	SendPacket(&header, &payload);

	// 4. SendBuff의 데이터를 Send한다.
	SendProc();

	// 5. 내용을 리스트에 추가한다 (내 채팅은 서버 검증없이 그냥 출력한다)
	TCHAR ShowString[4 + CHAT_SIZE];	// "나 : " (총 4개 문자)가 추가되어서 4를 함.
	swprintf_s(ShowString, _countof(ShowString), _T("%s : %s"), _T("나"), Typing);
	HWND RoomChathWnd = GetDlgItem(*RoomhWnd, IDC_ROOM_CHAT);
	SendMessage(RoomChathWnd, LB_ADDSTRING, 0, (LPARAM)ShowString);

	// 6. 자동 스크롤 기능.
	// 현재 리스트 박스에 추가된 Index를 얻어온 다음에
	// 인덱스 -1을 Top 인덱스로 셋팅한다. 그러면 자동 스크롤 되는것 처럼 보인다.
	LRESULT iIndex = SendMessage(RoomChathWnd, LB_GETCOUNT, 0, 0);
	SendMessage(RoomChathWnd, LB_SETTOPINDEX, iIndex - 1, 0);

	// 7. 에디터의 내용을 공백으로 만든다. 채팅 전송했으니까!
	SetDlgItemText(*RoomhWnd, IDC_ROOM_TYPING, _T(""));
}





/////////////////////////
// 패킷 제작 함수
/////////////////////////

// "로그인 요청" 패킷 제작
void CreatePacket_Req_Login(char* header, char* Packet)
{
	// 1. 페이로드 제작
	CProtocolBuff* PayloadBuff = (CProtocolBuff*)Packet;
	for(int i=0; i< dfNICK_MAX_LEN; ++i)
		*PayloadBuff << g_NickName[i];

	// 2. 헤더 조립하기
	CreateHeader((CProtocolBuff*)header, PayloadBuff, df_REQ_LOGIN);
}

// "대화방 목록 요청" 패킷 제작
void CreatePacket_Req_RoomList(char* header)
{
	// 1. "대화방 목록 요청"은 페이로드가 없음
	CProtocolBuff payload;

	// 2. 헤더 조립하기
	CreateHeader((CProtocolBuff*)header, &payload, df_REQ_ROOM_LIST);
	
}

// "방 생성 요청" 패킷 제작
void CreatePacket_Req_RoomCreate(char* header, char* Packet, TCHAR RoomName[ROOMNAME_SIZE])
{
	CProtocolBuff* PayloadBuff = (CProtocolBuff*)Packet;

	// 1. 페이로드 제작
	// 일단 방 제목 사이즈 알아오기
	WORD RoomNameSize = (WORD)(_tcslen(RoomName) * 2);
	*PayloadBuff << RoomNameSize;
	
	// 방 이름, 페이로드에 넣기
	for (int i = 0; i < RoomNameSize / 2; ++i)
		*PayloadBuff << RoomName[i];

	_tcscpy_s(MyHopeRoomName, RoomNameSize, RoomName);

	// 2. 헤더 조립하기
	CreateHeader((CProtocolBuff*)header, PayloadBuff, df_REQ_ROOM_CREATE);
}

// "방 입장 요청" 패킷 제작
void CreatePacket_Req_RoomJoin(char* header, char* Packet, DWORD RoomNo)
{
	CProtocolBuff* PayloadBuff = (CProtocolBuff*)Packet;

	// 1. 페이로드 제작	
	// 알아온 방 넘버로 Payload 채우기
	*PayloadBuff << RoomNo;

	// 2. 헤더 만들기
	CreateHeader((CProtocolBuff*)header, PayloadBuff, df_REQ_ROOM_ENTER);
}

// "채팅 송신" 패킷 제작
void CreatePacket_Req_ChatSend(char* header, char* Packet, DWORD MessageSize, TCHAR Chat[CHAT_SIZE])
{
	CProtocolBuff* PayloadBuff = (CProtocolBuff*)Packet;

	// 1. 페이로드 제작
	// 채팅 사이즈 저장 (인자에는 유니코드 사이즈이다. 바이트 Size로 변경 후 저장.)
	WORD Size = (WORD)(MessageSize * 2);
	*PayloadBuff << Size;

	// 채팅 내용 저장
	for(DWORD i=0; i<MessageSize; ++i)
		*PayloadBuff << Chat[i];

	// 2. 헤더 만들기
	CreateHeader((CProtocolBuff*)header, PayloadBuff, df_REQ_CHAT);

}

// "방 퇴장 요청" 패킷 제작
void CreatePacket_Req_RoomLeave(char* header)
{
	// 1. 페이로드 제작
	// 방 퇴장 요청은 페이로드가 없다.
	CProtocolBuff PayloadBuff;

	// 2. 헤더 만들기
	CreateHeader((CProtocolBuff*)header, &PayloadBuff, df_REQ_ROOM_LEAVE);
}



/////////////////////////
// Send 처리
/////////////////////////

// Send버퍼에 데이터 넣기
BOOL SendPacket(CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff)
{
	// 연결된 상태인지 확인
	if (bConnectFlag == FALSE)
		return TRUE;

	// 1. 샌드 q에 헤더 넣기
	int Size = headerBuff->GetUseSize();
	DWORD BuffArray = 0;
	int a = 0;
	while (Size > 0)
	{
		int EnqueueCheck = SendBuff.Enqueue(headerBuff->GetBufferPtr() + BuffArray, Size);
		if (EnqueueCheck == -1)
		{
			MessageBox(LobbyhWnd, _T("SendPacket(). 헤더 넣는 중 샌드큐 꽉참. 접속 종료"), _T("접속 종료"), MB_ICONERROR);
			return FALSE;
		}

		Size -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	// 2. 샌드 q에 페이로드 넣기
	WORD PayloadLen = payloadBuff->GetUseSize();
	BuffArray = 0;
	while (PayloadLen > 0)
	{
		int EnqueueCheck = SendBuff.Enqueue(payloadBuff->GetBufferPtr() + BuffArray, PayloadLen);
		if (EnqueueCheck == -1)
		{
			MessageBox(LobbyhWnd, _T("SendPacket(). 페이로드 넣는 중 샌드큐 꽉참. 접속 종료"), _T("접속 종료"), MB_ICONERROR);
			return FALSE;
		}

		PayloadLen -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	return TRUE;
}

// Send버퍼의 데이터를 Send하기
BOOL SendProc()
{
	// 1.Send가능 여부 확인.
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

		// 11. 에러 체크. 우드블럭이면 더 이상 못보내니 while문 종료. 다음 Select때 다시 보냄
		if (SendSize == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;

			else
			{
				MessageBox(LobbyhWnd, _T("SendProc(). Send중 에러 발생"), _T("접속 종료"), MB_ICONERROR);
				return FALSE;
			}
		}

		// 12. 보낸 사이즈가 나왔으면, 그 만큼 remove
		SendBuff.RemoveData(SendSize);
	}

	return TRUE;
}




/////////////////////////
// Recv 처리 함수
/////////////////////////

// 네트워크 처리 중, Recv 처리 함수
BOOL RecvProc()
{
	// bConnectFlag(커넥트 여부)가 FALSE라면 아직 연결 안된거니 그냥 리턴.
	if (bConnectFlag == FALSE)
		return TRUE;

	//////////////////////////////////////////////////
	// recv()
	//////////////////////////////////////////////////

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

			MessageBox(LobbyhWnd, ErrorText, _T("접속 종료"), 0);
			return FALSE;		// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	RecvBuff.MoveWritePos(retval - 1);

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
		if (RecvBuff.GetUseSize() < len)
			break;

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = RecvBuff.Peek((char*)&HeaderBuff, len);

		// Peek()은 버퍼가 비었으면 -1을 반환한다. 버퍼가 비었으니 일단 끊는다.
		if (PeekSize == -1)
		{
			MessageBox(LobbyhWnd, _T("RecvProc(). 완료패킷 헤더 처리 중 RecvBuff비었음. 접속 종료"), _T("접속 종료"), 0);
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 3. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (HeaderBuff.byCode != dfPACKET_CODE)
		{
			MessageBox(LobbyhWnd, _T("RecvProc(). 완료패킷 헤더 처리 중 정상패킷 아님. 접속 종료"), _T("접속 종료"), 0);
			return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
		}

		// 4. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size )
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (RecvBuff.GetUseSize() < (len + HeaderBuff.wPayloadSize))
			break;

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 Len만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		DWORD BuffArray = 0;
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.wPayloadSize;

		while (PayloadSize > 0)
		{
			int DequeueSize = RecvBuff.Dequeue(PayloadBuff.GetBufferPtr() + BuffArray, PayloadSize);

			// Dequeue()는 버퍼가 비었으면 -1을 반환. 버퍼가 비었으면 일단 끊는다.
			if (DequeueSize == -1)
			{
				MessageBox(LobbyhWnd, _T("RecvProc(). 완료패킷 페이로드 처리 중 RecvBuff비었음. 접속 종료"), _T("접속 종료"), 0);
				return FALSE;	// FALSE가 리턴되면, 해당 유저는 접속이 끊긴다.
			}

			PayloadSize -= DequeueSize;
			BuffArray += DequeueSize;
		}
		PayloadBuff.MoveWritePos(BuffArray);

		// 7. 헤더의 체크섬 확인 (2차 검증)
		CProtocolBuff ChecksumBuff;
		memcpy(ChecksumBuff.GetBufferPtr(), PayloadBuff.GetBufferPtr(), PayloadBuff.GetUseSize());
		ChecksumBuff.MoveWritePos(PayloadBuff.GetUseSize());

		DWORD Checksum = 0;
		for (int i = 0; i < HeaderBuff.wPayloadSize; ++i)
		{
			BYTE ChecksumAdd;
			ChecksumBuff >> ChecksumAdd;
			Checksum += ChecksumAdd;
		}

		BYTE* MsgType = (BYTE*)&HeaderBuff.wMsgType;
		for (int i = 0; i < sizeof(HeaderBuff.wMsgType); ++i)
			Checksum += MsgType[i];

		Checksum = Checksum % 256;

		if (Checksum != HeaderBuff.byCheckSum)
		{
			_tprintf(_T("RecvProc(). 완료패킷 체크섬 오류. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 8. 헤더의 타입에 따라 분기처리. 함수로 처리한다.
		BOOL check = PacketProc(HeaderBuff.wMsgType, (char*)&PayloadBuff);
		if (check == FALSE)
			return FALSE;
	}

	return TRUE;
}

// 패킷 타입에 따른 패킷처리 함수
BOOL PacketProc(WORD PacketType, char* Packet)
{
	BOOL check;
	try
	{
		// Recv데이터 처리
		switch (PacketType)
		{
			// 채팅 송신 결과(나와 같은 방에 있는, 다른 사람의 채팅만 옴)
			// 내가 보낸것은 결과로 안날라옴.
		case df_RES_CHAT:
			check = Network_Res_ChatRecv(Packet);
			if (check == FALSE)
				return FALSE;
			break;

			// 내가 있는 방에 다른 사용자 입장
		case df_RES_USER_ENTER:
			check = Network_Res_UserEnter(Packet);
			if (check == FALSE)
				return FALSE;
			break;

			// 방 퇴장 결과 (내가 보내지 않은 것도 온다.)
			// 즉, 나랑 같은 방에 있는 누군가의 방퇴장도 날라올 수 있다.
		case df_RES_ROOM_LEAVE:
			check = Network_Res_RoomLeave(Packet);
			if (check == FALSE)
				return FALSE;
			break;

			// 대화방 생성 결과 (내가 보냄 + 다른사람이 생성한 것도 옴)
		case df_RES_ROOM_CREATE:
			check = Network_Res_RoomCreate(Packet);
			if (check == FALSE)
				return FALSE;
			break;

			// 대화방 입장 결과 (내가 보냄)
		case df_RES_ROOM_ENTER:
			check = Network_Res_RoomJoin(Packet);
			if (check == FALSE)
				return FALSE;
			break;

			// 방 삭제 결과
			// 삭제된 방이 날라온다.
		case df_RES_ROOM_DELETE:
			check = Network_Res_RoomDelete(Packet);
			if (check == FALSE)
				return FALSE;
			break;

			// 로그인 요청 결과 (내가 보냄)
		case df_RES_LOGIN:
			check = Network_Res_Login(Packet);
			if (check == FALSE)
				return FALSE;
			break;

			// 대화방 리스트 결과 (내가 보냄)
		case df_RES_ROOM_LIST:
			check = Network_Res_RoomList(Packet);
			if (check == FALSE)
				return FALSE;
			break;			

		default:
			MessageBox(LobbyhWnd, _T("PacketProc(). 알 수 없는 패킷"), _T("접속 종료"), 0);
			return FALSE;
		}

	}
	catch (CException exc)
	{
		TCHAR* text = (TCHAR*)exc.GetExceptionText();
		MessageBox(LobbyhWnd, text, _T("PacketProc(). 예외 발생"), 0);
		return FALSE;
	}

	return TRUE;
}

// "로그인 요청 결과" 패킷 처리
BOOL Network_Res_Login(char* Packet)
{
	CProtocolBuff* LoginPacket = (CProtocolBuff*)Packet;

	// 1. 결과(1BYTE)뽑아오기.
	BYTE result;
	*LoginPacket >> result;

	// 2. 결과가 df_RESULT_LOGIN_OK인지 체크. 
	if (result != df_RESULT_LOGIN_OK)
	{
		// df_RESULT_LOGIN_OK가 아니라면, 잘못된 것.
		// 에러 발생시키고 종료
		if(result == df_RESULT_LOGIN_DNICK)
			MessageBox(LobbyhWnd, _T("Network_Res_Login(). 중복 닉네임"), _T("접속 종료"), 0);
		else
			MessageBox(LobbyhWnd, _T("Network_Res_Login(). 로그인 요청 에러"), _T("접속 종료"), 0);

		return FALSE;
	}

	// 3. 결과가 df_RESULT_LOGIN_OK라면, 사용자 No를 뽑아온다.
	// 그리고 내 UserID를 저장한다.
	*LoginPacket >> g_MyUserID;

	// 4. "대화방 목록 요청" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;
	CreatePacket_Req_RoomList((char*)&header);

	// 5. SendBuff에 넣기.
	if (SendPacket(&header, &payload) == FALSE)
		return FALSE;

	// 6. Send하기
	if (SendProc() == FALSE)
	{
		MessageBox(LobbyhWnd, _T("Network_Res_Login(). 대화방 목록 요청 Send실패"), _T("접속 종료"), 0);
		return FALSE;
	}

	return TRUE;

}

// "대화방 목록 요청 결과" 패킷 처리
BOOL Network_Res_RoomList(char* Packet)
{	
	CProtocolBuff* RoomListPacket = (CProtocolBuff*)Packet;

	// 1. 내 닉네임과 유저ID를 스태틱에 표시한다.
	TCHAR tUserID[10];
	_itot_s(g_MyUserID, tUserID, _countof(tUserID), 10);

	SetDlgItemText(LobbyhWnd, IDC_NICKNAME, g_NickName);
	SetDlgItemText(LobbyhWnd, IDC_USERNUM, tUserID);

	// 2. 룸 리스트에 룸 표시하기 + 룸 목록에 룸 저장하기.
	// 룸 개수 뽑아오기
	WORD RoomCount;
	*RoomListPacket >> RoomCount;

	// 룸 개수만큼 반복하면서 룸을 뽑아와, 룸 목록에 저장.
	HWND RoomListBox = GetDlgItem(LobbyhWnd, IDC_LIST1);
	for (int i = 0; i < RoomCount; ++i)
	{
		// 이번 방의 정보를 저장할 구조체 선언
		stRoom* NowRoom = new stRoom;

		//	4Byte : 방 No
		DWORD RoomNo;
		*RoomListPacket >> RoomNo;

		//	2Byte : 방이름 byte size
		WORD RoomNameSize;
		*RoomListPacket >> RoomNameSize;

		//	Size  : 방이름 (유니코드)
		TCHAR* RoomName = new TCHAR[(RoomNameSize/2)+1];
		int j;
		for (j = 0; j < RoomNameSize/2; ++j)
			*RoomListPacket >> RoomName[j];

		RoomName[j] = '\0';

		// 여기까지 정보 셋팅
		NowRoom->m_RoomID = RoomNo;
		NowRoom->m_RoomName = RoomName;

		//	1Byte : 참여인원
		//	{
		//		WHCAR[15] : 닉네임
		//	}
		// 이건 빼기만하고 사용하지 않는다. 
		BYTE JoinCount;
		*RoomListPacket >> JoinCount;

		for (int h = 0; h < JoinCount; ++h)
		{
			TCHAR JoinUserNick[dfNICK_MAX_LEN];
			for (int a = 0; a < dfNICK_MAX_LEN; ++a)
				*RoomListPacket >> JoinUserNick[a];
		}

		// 셋팅된 룸을, 룸 목록에 저장
		typedef pair<DWORD, stRoom*> Itn_pair;
		map_RoomList.insert(Itn_pair(RoomNo, NowRoom));

		// 룸을 리스트 박스에 표시
		DWORD ListIndex = (DWORD)SendMessage(RoomListBox, LB_ADDSTRING, 0, (LPARAM)RoomName);
		SendMessage(RoomListBox, LB_SETITEMDATA, WPARAM(ListIndex), (LPARAM)RoomNo);
	}	

	return TRUE;
}

// "방 생성 요청 결과" 패킷 처리
BOOL Network_Res_RoomCreate(char* Packet)
{
	CProtocolBuff* RoomCreatePacket = (CProtocolBuff*)Packet;

	// 1. 결과를 뽑아온다.
	BYTE result;
	*RoomCreatePacket >> result;

	// 2. 결과가 df_RESULT_LOGIN_OK인지 체크.
	if (result != df_RESULT_ROOM_CREATE_OK)
	{
		// df_RESULT_ROOM_CREATE_OK가 아니라면, 잘못된 것.
		// 에러 발생시키고 종료
		MessageBox(LobbyhWnd, _T("Network_Res_RoomCreate(). 방생성 요청 결과가 OK가 아님"), _T("접속 종료"), MB_ICONERROR);
		return FALSE;
	}

	// 3. OK라면 방 No, 방제목 바이트Size, 방제목(유니코드) 뽑아와서 새로운 방 완성
	stRoom* NewRoom = new stRoom;

	// 방 넘버.
	DWORD RoomNo;
	*RoomCreatePacket >> RoomNo;

	// 방 제목 바이트Size
	WORD RoomNameSize;
	*RoomCreatePacket >> RoomNameSize;

	// 방 제목
	TCHAR* RoomName = new TCHAR[(RoomNameSize / 2) + 1];
	int i;
	for (i = 0; i < RoomNameSize / 2; ++i)
		*RoomCreatePacket >> RoomName[i];

	RoomName[i] = '\0';

	// 뽑아온 정보 셋팅
	NewRoom->m_RoomID = RoomNo;
	NewRoom->m_RoomName = RoomName;	

	// 4. 내가 생성을 요청한 방이라면, 채팅방이 생성되었다는 메시지 박스 표시
	if(_tcscmp(MyHopeRoomName, RoomName) == 0)
		MessageBox(LobbyhWnd, _T("채팅방 생성 완료"), _T("방 생성"), NULL);

	// 유저가 OK를 누르면
	// 5. 추가한 방을 리스트 박스에 표시한다.
	DWORD ListIndex = (DWORD)SendMessage(GetDlgItem(LobbyhWnd, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)RoomName);
	SendMessage(GetDlgItem(LobbyhWnd, IDC_LIST1), LB_SETITEMDATA, (WPARAM)ListIndex, (LPARAM)RoomNo);

	// 6. 셋팅된 방을, 방 목록에 추가
	typedef pair<DWORD, stRoom*> Itn_pair;
	map_RoomList.insert(Itn_pair(RoomNo, NewRoom));

	// 7. 방 이름을 입력하는 리스트를 공백으로 변경한다.
	SetDlgItemText(LobbyhWnd, IDC_ROOMNAME, _T(""));

	return TRUE;
}

// "방 입장 요청 결과" 패킷 처리
BOOL Network_Res_RoomJoin(char* Packet)
{
	CProtocolBuff* RoomJoinPacket = (CProtocolBuff*)Packet;

	// 1. 결과를 뽑아온다.
	BYTE result;
	*RoomJoinPacket >> result;

	// 2. 결과가 df_RESULT_LOGIN_OK인지 체크.
	if (result != df_RESULT_ROOM_ENTER_OK)
	{
		// df_RESULT_ROOM_ENTER_OK가 아니라면, 잘못된 것.
		// 에러 발생시키고 종료
		MessageBox(LobbyhWnd, _T("Network_Res_RoomJoin(). 방 입장 요청 결과가 OK가 아님"), _T("접속 종료"), MB_ICONERROR);
		return FALSE;
	}

	// 3. OK라면 방 정보를 빼온다.
	// 방 No를 뺀 후, 방 No로 방을 알아온다.
	DWORD RoomNo;
	*RoomJoinPacket >> RoomNo;	

	stRoom* NowRoom = new stRoom;
	NowRoom = nullptr;
	map <DWORD, stRoom*>::iterator itor;
	for (itor = map_RoomList.begin(); itor != map_RoomList.end(); ++itor)
	{
		if (itor->second->m_RoomID == RoomNo)
		{
			NowRoom = itor->second;
			break;
		}
	}

	if (NowRoom == nullptr)
	{
		MessageBox(LobbyhWnd, _T("Network_Res_RoomJoin(). 서버에서, 없는 방에 입장하라고 함."), _T("접속 종료"), MB_ICONERROR);
		return FALSE;
	}

	// 방 제목 바이트 Size
	WORD RoomNameSize;
	*RoomJoinPacket >> RoomNameSize;

	// 방 제목
	TCHAR* RoomName = new TCHAR[(RoomNameSize / 2) + 1];
	int i;
	for (i = 0; i < RoomNameSize / 2; ++i)
		*RoomJoinPacket >> RoomName[i];

	RoomName[i] = '\0';

	// 참가인원
	BYTE JoinCount;
	*RoomJoinPacket >> JoinCount;
	for (int i = 0; i < JoinCount; ++i)
	{
		stUser* NowUser = new stUser;

		// 닉네임 빼기
		TCHAR RoomJoinUSer[dfNICK_MAX_LEN];
		for (int j = 0; j < dfNICK_MAX_LEN; ++j)
			*RoomJoinPacket >> RoomJoinUSer[j];

		RoomJoinUSer[_tcslen(RoomJoinUSer)] = '\0';

		// 사용자 ID 빼기
		DWORD UserID;
		*RoomJoinPacket >> UserID;

		// 정보 셋팅
		_tcscpy_s(NowUser->m_NickName, _countof(RoomJoinUSer), RoomJoinUSer);
		NowUser->m_UserID = UserID;

		// 셋팅한 유저, 방에 추가하기
		typedef pair<DWORD, stUser*> Itn_pair;
		NowRoom->m_JoinUserList.insert(Itn_pair(UserID, NowUser));
	}

	// 4. 나를 방에 추가한다.
	stUser* MyInfo = new stUser;
	MyInfo->m_UserID = g_MyUserID;
	_tcscpy_s(MyInfo->m_NickName, _countof(g_NickName), g_NickName);

	typedef pair<DWORD, stUser*> Itn_pair;
	NowRoom->m_JoinUserList.insert(Itn_pair(g_MyUserID, MyInfo));
	
	// 5. 룸 다이얼로그 생성
	*RoomhWnd = CreateDialog(Instance, MAKEINTRESOURCE(IDD_ROOM), LobbyhWnd, RoomDialolgProc);

	// 6. 룸 다이얼로그 정보 셋팅
	// 룸 이름 표시
	SetDlgItemText(*RoomhWnd, IDC_ROOM_NAME, NowRoom->m_RoomName);

	// 룸 유저 리스트에, 유저 닉네임 표시
	HWND RoomUserhWnd = GetDlgItem(*RoomhWnd, IDC_ROOM_USER);
	map <DWORD, stUser*>::iterator Useritor;
	for (Useritor = NowRoom->m_JoinUserList.begin(); Useritor != NowRoom->m_JoinUserList.end(); ++Useritor)
		SendMessage(RoomUserhWnd, LB_ADDSTRING, 0, (LPARAM)Useritor->second->m_NickName);

	// 7. 윈도우 표시하기
	ShowWindow(*RoomhWnd, nCmdShow);	

	// 8. 내가 입장한 방번호와, 방 정보를 기억한다.
	g_JoinRoomNumber = RoomNo;
	MyRoom = NowRoom;

	return TRUE;
}

// "채팅 결과" 패킷 처리 (나와 같은 방에 있는 다른사람의 채팅 처리하기)
BOOL Network_Res_ChatRecv(char* Packet)
{
	CProtocolBuff* ChatRecvPacket = (CProtocolBuff*)Packet;

	// 1. 송신자 No알아오기
	DWORD SenderNo;
	*ChatRecvPacket >> SenderNo;

	// 2. 메시지 Size 알아오기
	WORD ChatSize;
	*ChatRecvPacket >> ChatSize;

	// 3. 채팅 내용 알아오기
	TCHAR* Chat = new TCHAR[(ChatSize / 2) + 1];
	int i;
	for(i=0; i<ChatSize / 2; ++i)
		*ChatRecvPacket >> Chat[i];

	Chat[i] = '\0';

	// 4. 내가 있는 방에, 해당 유저를 알아온다.
	stUser* NowUser = nullptr;
	map <DWORD, stUser*>::iterator useritor;
	for (useritor = MyRoom->m_JoinUserList.begin(); useritor != MyRoom->m_JoinUserList.end(); ++useritor)
	{
		// 유저 번호가 일치한다면, 선언해뒀던 NowUser로, 해당 데이터를 찌른다.
		if (useritor->first == SenderNo)
		{
			NowUser = useritor->second;
			break;
		}
	}	

	if (NowUser == nullptr)
	{
		MessageBox(LobbyhWnd, _T("Network_Res_ChatRecv(). 채팅 처리 중, 방에 없는 유저의 채팅이 날라옴."), _T("접속 종료"), MB_ICONERROR);
		return FALSE;
	}

	// 5. 내용을 리스트 박스에 추가한다 (내 채팅은 서버 검증없이 그냥 추가한다)
	TCHAR ShowString[dfNICK_MAX_LEN + CHAT_SIZE];
	swprintf_s(ShowString, _countof(ShowString), _T("%s : %s"), NowUser->m_NickName, Chat);
	HWND RoomChathWnd = GetDlgItem(*RoomhWnd, IDC_ROOM_CHAT);
	SendMessage(RoomChathWnd, LB_ADDSTRING, 0, (LPARAM)ShowString);

	// 6. 자동 스크롤 기능.
	// 현재 리스트 박스에 추가된 Index를 얻어온 다음에
	// 인덱스 -1을 Top 인덱스로 셋팅한다. 그러면 자동 스크롤 되는것 처럼 보인다.
	LRESULT iIndex = SendMessage(RoomChathWnd, LB_GETCOUNT, 0, 0);
	SendMessage(RoomChathWnd, LB_SETTOPINDEX, iIndex - 1, 0);

	return TRUE;
}

// "타 사용자 입장" 패킷 처리
BOOL Network_Res_UserEnter(char* Packet)
{
	CProtocolBuff* UserEnterPacket = (CProtocolBuff*)Packet;

	// 1. 닉네임 빼오기 (유니코드 15글자)
	TCHAR EnterUser[dfNICK_MAX_LEN];
	for (int i = 0; i < dfNICK_MAX_LEN; ++i)
		*UserEnterPacket >> EnterUser[i];

	// 2. 유저 No 빼오기
	DWORD JoinUserNo;
	*UserEnterPacket >> JoinUserNo;

	// 3. 유저 셋팅하기
	stUser* NewUser = new stUser;

	NewUser->m_UserID = JoinUserNo;
	_tcscpy_s(NewUser->m_NickName, 15, EnterUser);

	// 4. 내 방에 해당 유저 추가
	typedef pair<DWORD, stUser*> Itn_pair;
	MyRoom->m_JoinUserList.insert(Itn_pair(JoinUserNo, NewUser));

	// 5. 해당 유저를, 내 룸의 유저 목록에 표시
	SendMessage(GetDlgItem(*RoomhWnd, IDC_ROOM_USER), LB_ADDSTRING, 0, (LPARAM)EnterUser);

	return TRUE;
}

// "방 퇴장 요청 결과" 패킷 처리 (내가 안보낸것도 옴)
BOOL Network_Res_RoomLeave(char* Packet)
{
	CProtocolBuff* RoomLeavePacket = (CProtocolBuff*)Packet;

	// 1. 방에서 퇴장할 유저 No를 알아온다.
	DWORD LeaveUserNo;
	*RoomLeavePacket >> LeaveUserNo;

	// 2. 퇴장할 유저가 나인지 확인한다.
	// 즉, 내가 보낸 방퇴장 요청인지 확인
	if (LeaveUserNo == g_MyUserID)
		EndDialog(*RoomhWnd, TRUE);

	// 3. 퇴장할 유저가 내가 아니라면, 아래 로직 처리
	else
	{
		// 일단, 해당 유저를 찾는다.
		BOOL Check = FALSE;
		map<DWORD, stUser*>::iterator itor;
		for (itor = MyRoom->m_JoinUserList.begin(); itor != MyRoom->m_JoinUserList.end(); ++itor)
		{
			// 유저를 찾았으면
			if (itor->first == LeaveUserNo)
			{	
				// 유저를 내 방 유저 목록(리스트 박스)에서 제외한다.
				HWND RoomUserhWnd = GetDlgItem(*RoomhWnd, IDC_ROOM_USER);
				LRESULT index = SendMessage(RoomUserhWnd, LB_FINDSTRING, -1, (LPARAM)itor->second->m_NickName);
				SendMessage(RoomUserhWnd, LB_DELETESTRING, index, 0);

				// 유저를 내 방에서 제외시킨다.
				MyRoom->m_JoinUserList.erase(itor->first);

				// 그리고 해당 룸과 관련된 동적 데이터 해제
				delete itor->second;

				Check = TRUE;
				break;
			}
		}
		
		// 유저를 못찾았다면 에러 발생
		if (Check == FALSE)
		{
			MessageBox(LobbyhWnd, _T("Network_Res_RoomLeave(). 방에 없는 유저가 방에서 나간다고 함 ."), _T("접속 종료"), MB_ICONERROR);
			return FALSE;
		}			
	}

	return TRUE;
}

// "방 삭제 결과" 패킷 처리
BOOL Network_Res_RoomDelete(char* Packet)
{
	CProtocolBuff* RoomDeletePacket = (CProtocolBuff*)Packet;

	// 1. 삭제될 룸 ID를 알아온다.
	DWORD DeleteRoomID;
	*RoomDeletePacket >> DeleteRoomID;

	// 2. 해당 ID의 룸을, 룸 목록에서 찾는다.
	stRoom* DeleteRoom = nullptr;
	map <DWORD, stRoom*>::iterator itor;
	for (itor = map_RoomList.begin(); itor != map_RoomList.end(); ++itor)
	{
		if (itor->first == DeleteRoomID)
		{			
			DeleteRoom = itor->second;
			break;
		}
	}

	if (DeleteRoom == nullptr)
	{
		MessageBox(LobbyhWnd, _T("Network_Res_RoomDelete(). 없는 방을 삭제하라고 함 ."), _T("접속 종료"), MB_ICONERROR);
		return FALSE;
	}

	// 3. 찾은 룸을, 룸 리스트 박스에서 제거한다.
	// 유저를 내 방 유저 목록(리스트 박스)에서 제외한다.
	HWND RoomListhWnd = GetDlgItem(LobbyhWnd, IDC_LIST1);
	LRESULT index = SendMessage(RoomListhWnd, LB_FINDSTRING, -1, (LPARAM)DeleteRoom->m_RoomName);
	SendMessage(RoomListhWnd, LB_DELETESTRING, index, 0);

	// 4. 룸 목록에서 제거한다. (데이터)
	map_RoomList.erase(DeleteRoomID);	

	// 5. 그리고 해당 룸과 관련된 동적 데이터 해제
	delete DeleteRoom->m_RoomName;
	delete DeleteRoom;

	return TRUE;
}