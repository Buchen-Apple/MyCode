#include "stdafx.h"
#include "Network_Func.h"

// 전역 변수
SOCKET sock;							// 내 소켓	
#define SERVERIP	L"127.0.0.1"		// 서버 IP
CRingBuff m_SendBuff;					// 내 샌드 버퍼
UINT64 g_uiMyAccount = 0;				// 내가 로그인한 회원의 회원번호
TCHAR g_tMyNickName[dfNICK_MAX_LEN];	// 내가 로그인한 회원의 닉네임;

// 윈속 초기화, 소켓 연결, 커넥트 등 함수
bool Network_Init()
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_tprintf(_T("WSAStartup 실패!\n"));
		return false;
	}

	// 소켓 생성
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		_tprintf(_T("sock / socket 실패!\n"));
		return false;
	}

	// Connect
	SOCKADDR_IN clientaddr;
	ZeroMemory(&clientaddr, sizeof(clientaddr));	
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(dfNETWORK_PORT);	
	InetPton(AF_INET, SERVERIP, &clientaddr.sin_addr.s_addr);

	DWORD dCheck = connect(sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));
	if (dCheck == SOCKET_ERROR) 
	{
		_tprintf(_T("connect() 실패. Error : %d\n"), WSAGetLastError());
		return false;
	}

	return true;
}

// 메뉴에 따른 액션 처리
bool PacketProc(int SelectNum)
{
	bool check;
	switch (SelectNum)
	{
		// 회원 추가
		case 1:
			check = Network_Res_AccountAdd();
			if (check == false)
				return false;
			return true;

		// 로그인
		case 2:
			check = Network_Res_Login();
			if (check == false)
				return false;
			return true;

		// 회원 목록
		case 3:			
			check = Network_Res_AccountList();
			if (check == false)
				return false;
			return true;

		// 친구 목록
		case 4:
			check = Network_Res_FriendList();
			if (check == false)
				return false;
			return true;

		// 받은 친구요청
		case 5:
			check = Network_Res_ReplyList();
			if (check == false)
				return false;
			return true;

		// 보낸 친구요청
		case 6:
			check = Network_Res_RequestList();
			if (check == false)
				return false;
			return true;

		// 친구요청 보내기
		case 7:
			check = Network_Res_FriendRequest();
			if (check == false)
				return false;
			return true;

		// 보낸 친구요청 취소
		case 8:
			check = Network_Res_FriendCancel();
			if (check == false)
				return false;
			return true;

		// 받은 친구요청 수락
		case 9:
			check = Network_Res_FriendAgree();
			if (check == false)
				return false;
			return true;

		// 받은 친구요청 거절
		case 10:
			check = Network_Res_FriendDeny();
			if (check == false)
				return false;
			return true;

		// 친구 끊기
		case 11:
			check = Network_Res_FriendRemove();
			if (check == false)
				return false;
			return true;

		default:
			fputs("알수 없는 메뉴\n", stdout);
			return true;		
	}

}

// 패킷 헤더 조립
void CreateHeader(CProtocolBuff* headerBuff, WORD MsgType, WORD PayloadSize)
{
	BYTE byCode = dfPACKET_CODE;
	*headerBuff << byCode;
	*headerBuff << MsgType;
	*headerBuff << PayloadSize;
}

// 내 로그인정보 출력하는 함수
void LoginShow()
{
	if (g_uiMyAccount == 0)
		fputs("# 로그인이 안되어 있습니다.\n\n", stdout);
	else
		_tprintf_s(_T("\n닉네임 : %s\nAccountNo : %lld\n\n"), g_tMyNickName, g_uiMyAccount);
}





//////////////////////////
// 패킷 제작 함수들
/////////////////////////
// "회원 추가" 패킷 제작
bool Network_Res_AccountAdd()
{
	// 1. 추가할 회원 닉네임 입력 (scanf로 인해 블락상태 됨)
	TCHAR Nick[dfNICK_MAX_LEN];
	fputs("회원 닉네임 입력:", stdout);

	_tscanf_s(_T("%s"), Nick, (UINT)sizeof(Nick));

	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 2. Payload 만들기
	for(int i=0; i<dfNICK_MAX_LEN; ++i)
		payload << Nick[i];

	// 3. 헤더 만들기
	CreateHeader(&header, df_REQ_ACCOUNT_ADD, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다. 
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_ACCOUNT_ADD);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_AccountAdd(&RecvBuff);

	return true;

}

// "로그인" 패킷 제작
bool Network_Res_Login()
{
	// 1. 로그인 할 회원의 ID 입력 (scanf로 인해 블락상태 됨)
	fputs("로그인 AccountNo:", stdout);

	UINT64 AccountNo;
	_tscanf_s(_T("%lld"), &AccountNo);

	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 2. Payload 만들기
	payload << AccountNo;

	// 3. 헤더 만들기
	CreateHeader(&header, df_REQ_LOGIN, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_LOGIN);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_Login(&RecvBuff);

	return true;

}

// "회원 목록 요청" 패킷 제작
bool Network_Res_AccountList()
{
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기 (페이로드 없음)

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_ACCOUNT_LIST, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_ACCOUNT_LIST);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_AccountList(&RecvBuff);

	return true;

}

// "친구목록 요청" 패킷 제작
bool Network_Res_FriendList()
{
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기 (페이로드 없음)

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_LIST, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_LIST);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_FriendList(&RecvBuff);

	return true;
}

// "받은 친구요청 보기" 패킷 제작
bool Network_Res_ReplyList()
{
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기 (페이로드 없음)

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_REPLY_LIST, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_REPLY_LIST);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_ReplyList(&RecvBuff);

	return true;
}

// "보낸 친구요청 보기" 패킷 제작
bool Network_Res_RequestList()
{
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기 (페이로드 없음)

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_REQUEST_LIST, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_REQUEST_LIST);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_RequestList(&RecvBuff);

	return true;
}

// "친구요청 보내기" 패킷 제작
bool Network_Res_FriendRequest()
{
	// 1. 친구 요청할 회원 ID 입력 (scanf로 인해 블락상태 됨)
	fputs("요청할 AccountNo 입력:", stdout);

	UINT64 AccountNo;
	_tscanf_s(_T("%lld"), &AccountNo);

	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기
	payload << AccountNo;

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_REQUEST, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_REQUEST);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_FriendRequest(&RecvBuff);

	return true;

}

// "친구요청 취소" 패킷 제작
bool Network_Res_FriendCancel()
{
	// 1. 보낸 요청 취소할 회원 ID 입력 (scanf로 인해 블락상태 됨)
	fputs("요청 취소할 AccountNo 입력:", stdout);

	UINT64 AccountNo;
	_tscanf_s(_T("%lld"), &AccountNo);

	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기
	payload << AccountNo;

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_CANCEL, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_CANCEL);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_FriendCancel(&RecvBuff);

	return true;
}

// "받은요청 수락" 패킷 제작
bool Network_Res_FriendAgree()
{
	// 1. 요청 수락할 회원 ID 입력 (scanf로 인해 블락상태 됨)
	fputs("수락할 회원의 AccountNo 입력:", stdout);

	UINT64 AccountNo;
	_tscanf_s(_T("%lld"), &AccountNo);

	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기
	payload << AccountNo;

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_AGREE, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_AGREE);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_FriendAgree(&RecvBuff);

	return true;
}

// "받은요청 거절" 패킷 제작
bool Network_Res_FriendDeny()
{
	// 1. 거절할 회원 ID 입력 (scanf로 인해 블락상태 됨)
	fputs("거절할 회원의 AccountNo 입력:", stdout);

	UINT64 AccountNo;
	_tscanf_s(_T("%lld"), &AccountNo);

	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기
	payload << AccountNo;

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_DENY, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_DENY);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_FriendDeny(&RecvBuff);

	return true;
}

// "친구 끊기" 패킷 제작
bool Network_Res_FriendRemove()
{
	// 1. 친구를 끊을 회원 ID 입력 (scanf로 인해 블락상태 됨)
	fputs("친구 해제할 AccountNo 입력:", stdout);

	UINT64 AccountNo;
	_tscanf_s(_T("%lld"), &AccountNo);

	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff payload;

	// 1. Payload 만들기
	payload << AccountNo;

	// 2. 헤더 만들기
	CreateHeader(&header, df_REQ_FRIEND_REMOVE, payload.GetUseSize());

	// 4. 데이터 SendBuff에 넣기
	SendPacket(&header, &payload);

	// 5. 데이터 Send
	SendProc();

	// 6. 리시브 한다.
	CProtocolBuff RecvBuff;
	bool check = RecvProc(&RecvBuff, df_RES_FRIEND_REMOVE);
	if (check == false)
		return false;

	// 7. 패킷을 받았으면 패킷처리한다.
	Network_Req_FriendRemove(&RecvBuff);

	return true;
}





//////////////////////////
// 받은 패킷 처리 함수들
/////////////////////////
// "회원 추가 결과" 패킷 처리
void Network_Req_AccountAdd(CProtocolBuff* payload)
{
	// 1. 페이로드에서 새로 생성된 AccountNo를 알아낸다.
	UINT64 AccountNo;
	*payload >> AccountNo;

	// 2. 정상적으로 추가됐다고 화면에 표시한다.
	printf("\n신규 회원 추가 완료 AccountNo:%lld\n\n", AccountNo);
}

// "로그인 결과" 패킷 처리
void Network_Req_Login(CProtocolBuff* payload)
{
	// 1. 로그인 한 회원의 AccountNo 알아오기
	UINT64 AccountNo;
	*payload >> AccountNo;

	// 만약, AccountNo가 0이라면, 로그인 실패한 것
	if (AccountNo == 0)
	{
		fputs("\n로그인 실패...\n\n", stdout);
		return;
	}

	// 2. Account가 0이 아니라면, AccountNo 셋팅 후, 닉네임도 빼온다.
	g_uiMyAccount = AccountNo;
	*payload >> g_tMyNickName;

	// 3. 로그인 성공 메시지 표시
	_tprintf_s(_T("\n로그인 성공! [닉네임 : %s, AccountNo : %lld]\n\n"), g_tMyNickName, g_uiMyAccount);
}

// "회원목록 요청" 패킷 처리
void Network_Req_AccountList(CProtocolBuff* payload)
{
	// 1. 회원 수 알아오기
	UINT AccountSize;
	*payload >> AccountSize;
	fputs("-----------회원 목록-----------\n\n", stdout);
	printf("총 회원 수 : %u명\n\n", AccountSize);

	// 2. 회원 수만큼 돌면서 AccountNo와 닉네임 빼오기
	for (UINT i = 0; i < AccountSize; ++i)
	{
		// AccountNo
		UINT64 AccountNo;
		*payload >> AccountNo;

		// 닉네임
		TCHAR Nick[dfNICK_MAX_LEN];
		*payload >> Nick;

		// 두개 표시하기
		_tprintf(_T("%lld / %s \n"), AccountNo, Nick);
	}
	fputc('\n', stdout);
}

// "친구목록 요청" 패킷 처리
void Network_Req_FriendList(CProtocolBuff* payload)
{
	// 1. 친구 수 알아오기
	UINT FriendSize;
	*payload >> FriendSize;
	fputs("-----------친구 목록-----------\n\n", stdout);
	printf("총 친구 수 : %u명\n\n", FriendSize);

	// 2. 친구 수만큼 돌면서 AccountNo와 닉네임 빼오기
	for (UINT i = 0; i < FriendSize; ++i)
	{
		// AccountNo
		UINT64 AccountNo;
		*payload >> AccountNo;

		// 닉네임
		TCHAR Nick[dfNICK_MAX_LEN];
		*payload >> Nick;

		// 두개 표시하기
		_tprintf(_T("내 친구. %lld / %s \n"), AccountNo, Nick);
	}
	fputc('\n', stdout);

}

// "받은 친구요청 보기 결과" 패킷 처리
void Network_Req_ReplyList(CProtocolBuff* payload)
{
	// 1. 받은 친구요청 수 알아오기
	UINT FriendSize;
	*payload >> FriendSize;
	fputs("-----------받은 친구요청 목록-----------\n\n", stdout);
	printf("총 받은 요청 수 : %u명\n\n", FriendSize);

	// 2. 친구 수만큼 돌면서 AccountNo와 닉네임 빼오기
	for (UINT i = 0; i < FriendSize; ++i)
	{
		// AccountNo
		UINT64 AccountNo;
		*payload >> AccountNo;

		// 닉네임
		TCHAR Nick[dfNICK_MAX_LEN];
		*payload >> Nick;

		// 두개 표시하기
		_tprintf(_T("요청 받음! %lld / %s \n"), AccountNo, Nick);
	}
	fputc('\n', stdout);
}

// "보낸 친구요청 보기 결과" 패킷 처리
void Network_Req_RequestList(CProtocolBuff* payload)
{
	// 1. 보낸 친구요청 수 알아오기
	UINT FriendSize;
	*payload >> FriendSize;
	fputs("-----------보낸 친구요청 목록-----------\n\n", stdout);
	printf("총 보낸 요청 수 : %u명\n\n", FriendSize);

	// 2. 친구 수만큼 돌면서 AccountNo와 닉네임 빼오기
	for (UINT i = 0; i < FriendSize; ++i)
	{
		// AccountNo
		UINT64 AccountNo;
		*payload >> AccountNo;

		// 닉네임
		TCHAR Nick[dfNICK_MAX_LEN];
		*payload >> Nick;

		// 두개 표시하기
		_tprintf(_T("요청중... %lld / %s \n"), AccountNo, Nick);
	}
	fputc('\n', stdout);

}

// "친구요청 보내기 결과" 패킷 처리
void Network_Req_FriendRequest(CProtocolBuff* payload)
{
	// 1. 요청했던 AccountNo 알아오기
	UINT64 AccountNo;
	*payload >> AccountNo;

	// 2. 결과 알아오기
	BYTE result;
	*payload >> result;

	// 3. result가 df_RESULT_FRIEND_REQUEST_OK가 아니라면 실패 메시지 표시
	if (result != df_RESULT_FRIEND_REQUEST_OK)
	{
		fputs("\n친구요청 실패...\n\n", stdout);
		return;
	}

	// 4. df_RESULT_FRIEND_REQUEST_OK라면 친구요청 성공 결과 표시
	printf("\n친구요청 성공! AccountNo : %lld\n\n", AccountNo);

}

// "친구요청 취소 결과" 패킷 처리
void Network_Req_FriendCancel(CProtocolBuff* payload)
{
	// 1. 요청했던 AccountNo 알아오기
	UINT64 AccountNo;
	*payload >> AccountNo;

	// 2. 결과 알아오기
	BYTE result;
	*payload >> result;

	// 3. result가 df_RESULT_FRIEND_CANCEL_OK가 아니라면 실패 메시지 표시
	if (result != df_RESULT_FRIEND_CANCEL_OK)
	{
		fputs("\n친구요청 취소 실패...\n\n", stdout);
		return;
	}

	// 4. df_RESULT_FRIEND_CANCEL_OK라면 성공 결과 표시
	printf("\n친구요청 취소 성공! AccountNo : %lld\n\n", AccountNo);

}

// "받은요청 수락 결과" 패킷 처리
void Network_Req_FriendAgree(CProtocolBuff* payload)
{
	// 1. 요청했던 AccountNo 알아오기
	UINT64 AccountNo;
	*payload >> AccountNo;

	// 2. 결과 알아오기
	BYTE result;
	*payload >> result;

	// 3. result가 df_RESULT_FRIEND_AGREE_OK가 아니라면 실패 메시지 표시
	if (result != df_RESULT_FRIEND_AGREE_OK)
	{
		fputs("\n요청수락 실패...\n\n", stdout);
		return;
	}

	// 4. df_RESULT_FRIEND_AGREE_OK라면 성공 결과 표시
	printf("\n요청수락 성공! 친구가 생겼습니다. [AccountNo : %lld]\n\n", AccountNo);

}

// "받은요청 거절 결과" 패킷 처리
void Network_Req_FriendDeny(CProtocolBuff* payload)
{
	// 1. 요청했던 AccountNo 알아오기
	UINT64 AccountNo;
	*payload >> AccountNo;

	// 2. 결과 알아오기
	BYTE result;
	*payload >> result;

	// 3. result가 df_RESULT_FRIEND_DENY_OK가 아니라면 실패 메시지 표시
	if (result != df_RESULT_FRIEND_DENY_OK)
	{
		fputs("\n요청거절 실패...\n\n", stdout);
		return;
	}

	// 4. df_RESULT_FRIEND_DENY_OK라면 성공 결과 표시
	printf("\n요청거절 성공! AccountNo : %lld\n\n", AccountNo);
}

// "친구끊기 결과" 패킷 처리
void Network_Req_FriendRemove(CProtocolBuff* payload)
{
	// 1. 요청했던 AccountNo 알아오기
	UINT64 AccountNo;
	*payload >> AccountNo;

	// 2. 결과 알아오기
	BYTE result;
	*payload >> result;

	// 3. result가 df_RESULT_FRIEND_REMOVE_OK가 아니라면 실패 메시지 표시
	if (result != df_RESULT_FRIEND_REMOVE_OK)
	{
		fputs("\n친구 해제 실패...\n\n", stdout);
		return;
	}

	// 4. df_RESULT_FRIEND_REMOVE_OK라면 성공 결과 표시
	printf("\n친구 해제 성공! 친구가 1명 사라졌습니다. AccountNo : %lld\n\n", AccountNo);
}




/////////////////////////
// recv 처리
/////////////////////////
bool RecvProc(CProtocolBuff* RecvBuff, WORD MsgType)
{	
	// 1. 헤더 recv()
	st_PACKET_HEADER header;
	int retval = recv(sock, (char*)&header, dfHEADER_SIZE, 0);

	// recv에서 0이 리턴되면 접속이 종료된것.
	if (retval == 0)
	{
		fputs("서버와 연결이 끊어짐.\n", stdout);
		return false;
	}

	// 내가 기다리고 있는 패킷에 대한 응답인지 체크
	if (header.wMsgType != MsgType)
	{
		fputs("내가 보냈던 패킷의 응답이 아님.\n", stdout);
		return false;
	}
	
	// 2. 헤더의 페이로드만큼 recv()
	retval = recv(sock, RecvBuff->GetBufferPtr(), header.wPayloadSize, 0);
	if (retval == 0)
	{
		fputs("서버와 연결이 끊어짐.\n", stdout);
		return false;
	}

	RecvBuff->MoveWritePos(retval);		

	return true;
}





/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff)
{
	// 1. 샌드 q에 헤더 넣기
	int Size = headerBuff->GetUseSize();
	DWORD BuffArray = 0;
	int a = 0;
	while (Size > 0)
	{
		int EnqueueCheck = m_SendBuff.Enqueue(headerBuff->GetBufferPtr() + BuffArray, Size);
		if (EnqueueCheck == -1)
		{
			_tprintf(_T("SendPacket(). 헤더 넣는 중 샌드큐 꽉참. 접속 종료\n"));
			return false;
		}

		Size -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	// 2. 샌드 q에 페이로드 넣기
	WORD PayloadLen = payloadBuff->GetUseSize();
	BuffArray = 0;
	while (PayloadLen > 0)
	{
		int EnqueueCheck = m_SendBuff.Enqueue(payloadBuff->GetBufferPtr() + BuffArray, PayloadLen);
		if (EnqueueCheck == -1)
		{
			_tprintf(_T("SendPacket(). 페이로드 넣는 중 샌드큐 꽉참. 접속 종료\n"));
			return false;
		}

		PayloadLen -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}


	return true;
}

// Send버퍼의 데이터를 Send하기
bool SendProc()
{	
	// 1. SendBuff에 보낼 데이터가 있는지 확인.
	if (m_SendBuff.GetUseSize() == 0)
		return true;

	// 3. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = m_SendBuff.GetBufferPtr();

	// 4. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 5. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (m_SendBuff.GetUseSize() == 0)
			break;

		// 6. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = m_SendBuff.GetNotBrokenGetSize();

		// 7. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 8. front 포인터 얻어옴
		int *front = (int*)m_SendBuff.GetReadBufferPtr();

		// 9. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = m_SendBuff.NextIndex(*front, 1);

		// 10. Send()
		int SendSize = send(sock, &sendbuff[BuffArrayCheck], Size, 0);

		// 11. 에러 체크. 우드블럭이면 더 이상 못보내니 while문 종료. 다음 Select때 다시 보냄
		if (SendSize == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;

			else
			{
				_tprintf(_T("SendProc(). Send중 에러 발생. 접속 종료\n"));
				return false;
			}
		}

		// 12. 보낸 사이즈가 나왔으면, 그 만큼 remove
		m_SendBuff.RemoveData(SendSize);
	}

	return true;
}
