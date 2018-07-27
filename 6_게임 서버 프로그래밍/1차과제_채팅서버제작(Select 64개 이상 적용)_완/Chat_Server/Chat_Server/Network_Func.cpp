#include "stdafx.h"
#include "Network_Func.h"
#include "RingBuff\RingBuff.h"

#include <list>
#include <map>

using namespace std;

// 세션 구조체
struct stClinet
{
	SOCKET m_sock;								// 해당 클라이언트의 소켓
	CRingBuff m_RecvBuff;						// 리시브 버퍼
	CRingBuff m_SendBuff;						// 샌드 버퍼
	DWORD m_UserID;								// 유저 고유 ID

	TCHAR m_NickName[dfNICK_MAX_LEN] = _T("0");	// 유저 닉네임
	DWORD m_JoinRoomNumber;						// 현재 접속중인 룸 넘버. 접속중인 룸이 없으면 0
	TCHAR m_tIP[33];							// 해당 유저의 IP
	WORD  m_wPort;								// 해당 유저의 Port
};

// 룸 구조체
struct stRoom
{
	DWORD m_RoomID;						// 룸 고유 ID
	TCHAR* m_RoomName;					// 룸 이름

	list<DWORD> m_JoinUserList;			// 해당 룸에 접속중인 유저의, 유저 고유 ID
};

// Network_Process()함수에서, 이번에 select()를 시도한 Client를 저장하는 구조체
// 소켓과 유저 ID를 알고 있다.
struct SOCKET_SAVE
{
	SOCKET* m_sock;
	DWORD  m_UserID;
};

// 전역 변수
DWORD g_dwUserID = 0;
DWORD g_dwRoomID = 1;						// 룸넘버 0이면, 룸에 안들어가 있는 유저로 판단하기 때문에 1부터 시작한다.
map <DWORD, stClinet*> map_ClientList;		// 서버에 접속한 클라이언트를 관리 목록(map).
map <DWORD, stRoom*> map_RoomList;			// 서버에 존재하는 방 관리 목록(map)

// 인자로 받은 UserID를 기준으로 클라이언트 목록에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 세션 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stClinet* ClientSearch(DWORD UserID)
{
	stClinet* NowUser;
	map <DWORD, stClinet*>::iterator iter;
	for (iter = map_ClientList.begin(); iter != map_ClientList.end(); ++iter)
	{
		if (iter->first == UserID)
		{
			NowUser = iter->second;
			return NowUser;
		}
	}

	return nullptr;
}

// 인자로 받은 RoomID를 기준으로 방 목록에서 [방을 골라낸다].(검색)
// 성공 시, 해당 방의 세션 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stRoom* RoomSearch(DWORD RoomID)
{
	stRoom* NowRoom;
	map <DWORD, stRoom*>::iterator itor;
	for (itor = map_RoomList.begin(); itor != map_RoomList.end(); ++itor)
	{
		if (itor->first == RoomID)
		{
			NowRoom = itor->second;
			return NowRoom;
		}
	}

	return nullptr;
}

// 네트워크 프로세스
// 여기서 false가 반환되면, 프로그램이 종료된다.
bool Network_Process(SOCKET* listen_sock)
{
	// 클라와 통신에 사용할 변수
	static FD_SET rset;
	static FD_SET wset;

	SOCKADDR_IN clinetaddr;
	map <DWORD, stClinet*>::iterator itor;
	TIMEVAL tval;
	tval.tv_sec = 0;
	tval.tv_usec = 0;

	itor = map_ClientList.begin();

	DWORD dwFDCount;

	while (1)
	{
		// select 준비	
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(*listen_sock, &rset);
		dwFDCount = 0;

		SOCKET_SAVE NowSock[FD_SETSIZE-1];

		// 모든 멤버를 순회하며, 해당 유저를 읽기 셋과 쓰기 셋에 넣는다.
		// 넣다가 64개가 되거나, end에 도착하면 break
		while (itor != map_ClientList.end())
		{
			// 해당 클라이언트에게 받을 데이터가 있는지 체크하기 위해, 모든 클라를 rset에 소켓 설정
			FD_SET(itor->second->m_sock, &rset);
			NowSock[dwFDCount].m_sock = &itor->second->m_sock;
			NowSock[dwFDCount].m_UserID = itor->first;

			// 만약, 해당 클라이언트의 SendBuff에 값이 있으면, wset에도 소켓 넣음.
			if (itor->second->m_SendBuff.GetUseSize() != 0)
				FD_SET(itor->second->m_sock, &wset);				

			// 64개 꽉 찼는지, 끝에 도착했는지 체크
			++itor;
			if (dwFDCount+2 == FD_SETSIZE || itor == map_ClientList.end())
				break;

			++dwFDCount;			
		}

		// Select()
		DWORD dCheck = select(0, &rset, &wset, 0, &tval);

		// Select()결과 처리
		if (dCheck == SOCKET_ERROR)
		{
			_tprintf(_T("select 에러\n"));
			return false;
		}

		// select의 값이 0보다 크다면 뭔가 할게 있다는 것이니 로직 진행
		else if (dCheck > 0)
		{
			// 리슨 소켓 처리
			if (FD_ISSET(*listen_sock, &rset))
			{
				int addrlen = sizeof(clinetaddr);
				SOCKET client_sock = accept(*listen_sock, (SOCKADDR*)&clinetaddr, &addrlen);

				// 에러가 발생하면, 그냥 그 유저는 소켓 등록 안함
				if (client_sock == INVALID_SOCKET)
					_tprintf(_T("accept 에러\n"));

				// 에러가 발생하지 않았다면, "로그인 요청" 패킷이 온 것. 이에 대해 처리
				else
					Accept(&client_sock, clinetaddr);	// Accept 처리.
			}

			// 리슨소켓 외 처리
			for (DWORD i = 0; i <= dwFDCount; ++i)
			{
				// 읽기 셋 처리
				if (FD_ISSET(*NowSock[i].m_sock, &rset))
				{
					// Recv() 처리
					// 만약, RecvProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					bool Check = RecvProc(NowSock[i].m_UserID);
					if (Check == false)
					{
						Disconnect(NowSock[i].m_UserID);
						continue;
					}
				}

				// 쓰기 셋 처리
				if (FD_ISSET(*NowSock[i].m_sock, &wset))
				{
					// Send() 처리
					// 만약, SendProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					bool Check = SendProc(NowSock[i].m_UserID);
					if (Check == false)
						Disconnect(NowSock[i].m_UserID);
				}
			}
		}

		// 만약, 모든 Client에 대해 Select처리가 끝났으면, 이번 함수는 여기서 종료.
		if (itor == map_ClientList.end())
			break;

	}

	return true;
}

// Accept 처리
void Accept(SOCKET* client_sock, SOCKADDR_IN clinetaddr)
{
	// 인자로 받은 clinet_sock에 해당되는 유저 생성
	stClinet* NewUser = new stClinet;

	// ID, 조인 방넘버, sock 셋팅
	NewUser->m_UserID = g_dwUserID;
	NewUser->m_JoinRoomNumber = 0;
	NewUser->m_sock = *client_sock;

	// 해당 유저를 클라이언트 관리목록에 추가.
	typedef pair<DWORD, stClinet*> Itn_pair;
	map_ClientList.insert(Itn_pair(g_dwUserID, NewUser));

	// 유저 고유 ID 증가
	g_dwUserID++;

	// 접속한 유저의 ip, port, 유저ID 출력
	TCHAR Buff[33];
	InetNtop(AF_INET, &clinetaddr.sin_addr, Buff, sizeof(Buff));
	WORD port = ntohs(clinetaddr.sin_port);

	_tcscpy_s(NewUser->m_tIP, _countof(Buff), Buff);
	NewUser->m_wPort = port;
	
	_tprintf(_T("Accept : [%s : %d / UserID : %d]\n"), Buff, port, NewUser->m_UserID);
}

// Disconnect 처리
void Disconnect(DWORD UserID)
{
	// 인자로 받은 UserID를 기준으로 클라이언트 목록에서 유저 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("Disconnect(). 로그인 중이 아닌 유저를 대상으로 삭제 시도.\n"));
		return;
	}		

	// 2. 해당 유저가 방 안에 있었다면, 같은 방 안의 유저에게 방퇴장 메시지를 발송해야 한다.
	// 해당 유저를 제외한 같은 방 유저의 SendBuff에 데이터를 넣어두고, 다음 Select때 발송한다.
	if (NowUser->m_JoinRoomNumber != 0)
	{
		char* packet = NULL;
		Network_Req_RoomLeave(UserID, packet);
	}

	// 3. 해당 유저를 map 목록에서 제거
	map_ClientList.erase(UserID);

	// 4. 해당 유저의 소켓 close
	closesocket(NowUser->m_sock);

	// 5. 접속 종료한 유저의 ip, port, 유저ID 출력	
	_tprintf(_T("Disconnect : [%s : %d / UserID : %d]\n"), NowUser->m_tIP, NowUser->m_wPort, UserID);

	// 6. 해당 유저 동적 해제
	delete NowUser;

}

// 서버에 접속한 모든 유저의 SendBuff에 패킷 넣기
void BroadCast_All(CProtocolBuff* header, CProtocolBuff* Packet)
{
	map <DWORD, stClinet*>::iterator itor;
	for (itor = map_ClientList.begin(); itor != map_ClientList.end(); ++itor)
	{
		bool Check = SendPacket(itor->first, header, Packet);
		if (Check == false)
			Disconnect(itor->first);
	}	
}

// 같은 방에 있는 모든 유저의, SendBuff에 패킷 넣기
// UserID에 -1이 들어오면 방 안의 모든 유저 대상.
// UserID에 다른 값이 들어오면, 해당 ID의 유저는 제외.
void BroadCast_Room(CProtocolBuff* header, CProtocolBuff* Packet, DWORD RoomID, int UserID)
{
	// 해당 ID의 룸을 알아온다.
	stRoom* NowRoom = RoomSearch(RoomID);

	// 해당 룸 안의 모든 유저의 SendBuff에 메시지 넣기
	// UserID는 0부터 시작이기 때문에 -1이 들어오면, 그냥 없는유저이다.
	list<DWORD>::iterator listitor;
	for (listitor = NowRoom->m_JoinUserList.begin(); listitor != NowRoom->m_JoinUserList.end(); ++listitor)
	{
		if (UserID == *listitor)
			continue;		

		bool Check = SendPacket(*listitor, header, Packet);
		if (Check == false)
			Disconnect(*listitor);
	}	
}



/////////////////////////
// Recv 처리 함수들
/////////////////////////
// Recv() 처리
bool RecvProc(DWORD UserID)
{
	// 인자로 받은 UserID를 기준으로 클라이언트 목록에서 유저 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("RecvProc(). 로그인 중이 아닌 유저를 대상으로 RecvProc 호출됨. 접속 종료\n"));
		return false;
	}

	////////////////////////////////////////////////////////////////////////
	// 소켓 버퍼에 있는 모든 recv 데이터를, 해당 유저의 recv링버퍼로 리시브.
	////////////////////////////////////////////////////////////////////////

	// 1. recv 링버퍼 포인터 얻어오기.
	char* recvbuff = NowUser->m_RecvBuff.GetBufferPtr();

	// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = NowUser->m_RecvBuff.GetNotBrokenPutSize();

	// 3. 한 번에 쓸 수 있는 공간이 0이라면
	if (Size == 0)
	{
		// rear 1칸 이동
		NowUser->m_RecvBuff.MoveWritePos(1);

		// 그리고 한 번에 쓸 수 있는 위치 다시 알기.
		Size = NowUser->m_RecvBuff.GetNotBrokenPutSize();
	}

	// 4. 그게 아니라면, 1칸 이동만 하기. 		
	else
	{
		// rear 1칸 이동
		NowUser->m_RecvBuff.MoveWritePos(1);
	}

	// 5. 1칸 이동된 rear 얻어오기
	int* rear = (int*)NowUser->m_RecvBuff.GetWriteBufferPtr();

	// 6. recv()
	int retval = recv(NowUser->m_sock, &recvbuff[*rear], Size, 0);

	// 7. 리시브 에러체크
	if (retval == SOCKET_ERROR)
	{
		// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있다는 것. recv할게 없다는 것이니 함수 종료.
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;
		
		// 10054 에러는 상대방이 강제로 종료한 것. 0이 리턴되는것은 정상종료.
		// 이 때는 그냥 return false로 나도 상대방과 연결을 끊는다.
		else if (WSAGetLastError() == 10054 || retval == 0)
			return false;

		// WSAEWOULDBLOCK에러가 아니면 뭔가 이상한 것이니 접속 종료
		else
		{
			_tprintf(_T("RecvProc(). retval 후 우드블럭이 아닌 에러가 뜸(%d). 접속 종료\n"), WSAGetLastError());
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	NowUser->m_RecvBuff.MoveWritePos(retval - 1);


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
		if (NowUser->m_RecvBuff.GetUseSize() < len)
			break;

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = NowUser->m_RecvBuff.Peek((char*)&HeaderBuff, len);

		// Peek()은 버퍼가 비었으면 -1을 반환한다. 버퍼가 비었으니 일단 끊는다.
		if (PeekSize == -1)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 RecvBuff비었음. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 3. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (HeaderBuff.byCode != dfPACKET_CODE)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 PacketCode틀림. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 4. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size )
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (NowUser->m_RecvBuff.GetUseSize() < (len + HeaderBuff.wPayloadSize))
			break;

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		NowUser->m_RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		DWORD BuffArray = 0;
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.wPayloadSize;

		while (PayloadSize > 0)
		{
			int DequeueSize = NowUser->m_RecvBuff.Dequeue(PayloadBuff.GetBufferPtr() + BuffArray, PayloadSize);

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

		// 8. 헤더에 들어있는 타입에 따라 분기처리.
		bool check = PacketProc(HeaderBuff.wMsgType, UserID, (char*)&PayloadBuff);
		if (check == false)
			return false;
	}


	return true;
}

// 패킷 처리
// PacketProc() 함수에서 false가 리턴되면 해당 유저는 접속이 끊긴다.
bool PacketProc(WORD PacketType, DWORD UserID, char* Packet)
{
	_tprintf(_T("PacketRecv [UserID : %d / TypeID : %d]\n"), UserID, PacketType);

	bool check = true;

	try
	{
		switch (PacketType)
		{

		// 채팅 요청 (클->서)
		case df_REQ_CHAT:
		{
			check = Network_Req_Chat(UserID, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 방입장 요청 (클->서)
		case df_REQ_ROOM_ENTER:
		{
			check = Network_Req_RoomJoin(UserID, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;


		// 방 퇴장 요청 (클->서)
		case df_REQ_ROOM_LEAVE:
		{
			check = Network_Req_RoomLeave(UserID, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 방생성 요청 (클->서)
		case df_REQ_ROOM_CREATE:
		{
			check = Network_Req_RoomCreate(UserID, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 로그인 요청 (클->서)
		case df_REQ_LOGIN:
		{
			check = Network_Req_Login(UserID, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;


		// 방목록 요청 (클->서) 
		case df_REQ_ROOM_LIST:
		{
			check = Network_Req_RoomList(UserID, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;			

		default:
			_tprintf(_T("이상한 메시지입니다.\n"));
			return false;
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

// "로그인 요청" 패킷 처리
bool Network_Req_Login(DWORD UserID, char* Packet)
{
	// 1. 직렬화 버퍼에 페이로드 넣기(닉네임)
	CProtocolBuff* LoginPacket = (CProtocolBuff*)Packet;

	// 2. 직렬화 버퍼에서 닉네임 골라내기.
	TCHAR NickName[dfNICK_MAX_LEN];
	*LoginPacket >> NickName;

	// 3. 해당 ID의 유저 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 4. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("Network_Req_Login(). Accept중이 아닌 유저임(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	// 4. 닉네임 중복 체크
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff packet;

	map <DWORD, stClinet*>::iterator iter;
	for (iter = map_ClientList.begin(); iter != map_ClientList.end(); ++iter)
	{
		// 중복 닉네임이 있다면, "중복 닉네임" 패킷을 만든다.
		if (_tcscmp(iter->second->m_NickName, NickName) == 0)
		{
			// 중복 닉네임 패킷 만들기
			CreatePacket_Res_Login((char*)&header, (char*)&packet, UserID, df_RESULT_LOGIN_DNICK);

			// "로그인 요청 결과"를 해당 유저의 SendBuff에 넣기
			SendPacket(UserID, &header, &packet);

			// 그리고 대상 유저에게 Send()
			SendProc(UserID);

			// return fase를 통해 접속을 종료시킨다.
			return false;
		}
	}

	// 5. 중복 닉네임이 없다면, [닉네임 셋팅, 정상 패킷 제작] 진행
	// 닉네임 셋팅
	_tcscpy_s(NowUser->m_NickName, dfNICK_MAX_LEN, NickName);

	// 정상 패킷 제작
	CreatePacket_Res_Login((char*)&header, (char*)&packet, UserID, df_RESULT_LOGIN_OK);

	// 6. "로그인 요청 결과"를 해당 유저의 SendBuff에 넣기.
	bool Check = SendPacket(UserID, &header, &packet);
	if (Check == false)
		return false;

	return true;
}

// "방목록 요청" 패킷 처리
bool Network_Req_RoomList(DWORD UserID, char* Packet)
{
	// 1. 해당 ID의 유저 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("Network_Req_RoomList(). 로그인 되지 않은 유저가 방목록을 요청함(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	// 3. "방 목록 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff packet;

	CreatePacket_Res_RoomList((char*)&header, (char*)&packet);

	// 4. 만든 패킷을 해당 유저의 SendBuff에 넣기.
	bool Check = SendPacket(UserID, &header, &packet);
	if (Check == false)
		return false;


	return true;
}

// "대화방 생성 요청" 패킷 처리
bool Network_Req_RoomCreate(DWORD UserID, char* Packet)
{
	// 1. 해당 ID의 유저 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("Network_Req_RoomCreate(). 로그인 되지 않은 유저가 방생성 요청(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	CProtocolBuff* RoomCreatePacket = (CProtocolBuff*)Packet;

	// 3. 2바이트 꺼내기. 이 2바이트에는 방 제목의 Size가 들어있다 (바이트 단위. 널문자 제외. 문자는 유니코드)
	WORD wRoomNameSize = 0;
	*RoomCreatePacket >> wRoomNameSize;

	// 4. 문자 뽑아내기
	// 널문자 완성을 위해 동적할당 시, +1(TCHAR이기 때문에 2바이트)을 한다.
	// 이거 안해서 메모리 침범 엄청 나왔었음...후
	TCHAR* RoomName = new TCHAR[(wRoomNameSize/2)+1];
	int i;
	for (i = 0; i < wRoomNameSize / 2; ++i)
		*RoomCreatePacket >> RoomName[i];

	RoomName[i] = '\0';

	// 5. 룸이름 중복 체크
	CProtocolBuff RCHeader(dfHEADER_SIZE);
	CProtocolBuff RCPacket;

	map <DWORD, stRoom*>::iterator iter;
	for (iter = map_RoomList.begin(); iter != map_RoomList.end(); ++iter)
	{
		// 룸 이름이 중복이라면, 중복 룸이름 패킷을 만든다.
		if (_tcscmp(iter->second->m_RoomName, RoomName) == 0)
		{
			// 중복 룸이름 패킷 만들기
			CreatePacket_Res_RoomCreate((char*)&RCHeader, (char*)&RCPacket, df_RESULT_ROOM_CREATE_DNICK, wRoomNameSize, (char*)RoomName);

			// "대화방 생성 요청 결과"를 해당 유저의 SendBuff에 넣기
			bool Check = SendPacket(UserID, &RCHeader, &RCPacket);
			if (Check == false)
				return false;

			return true;			
		}
	}


	// 6. 룸 이름이 중복되지 않는다면, [룸 셋팅, 정상패킷 제작] 진행
	// 룸 셋팅
	stRoom* NewRoom = new stRoom;
	NewRoom->m_RoomID = g_dwRoomID;
	NewRoom->m_RoomName = new TCHAR[wRoomNameSize];
	NewRoom->m_RoomName = RoomName;

	typedef pair<DWORD, stRoom*> Itn_pair;
	map_RoomList.insert(Itn_pair(g_dwRoomID, NewRoom));		

	// 정상 패킷 제작
	CreatePacket_Res_RoomCreate((char*)&RCHeader, (char*)&RCPacket, df_RESULT_ROOM_CREATE_OK, wRoomNameSize, (char*)RoomName);

	// 룸 번호 증가
	g_dwRoomID++;

	// 7. "대화방 생성 요청 결과"를 모든 유저의 SendBuff에 넣기.
	BroadCast_All(&RCHeader, &RCPacket);

	return true;
}

// "방입장 요청" 패킷 처리
bool Network_Req_RoomJoin(DWORD UserID, char* Packet)
{
	// 1. 해당 ID의 유저 세션 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("Network_Req_RoomJoin(). 로그인 되지 않은 유저가 방입장 요청(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	// 3. Packet에서 입장하고자 하는 방의 ID를 뽑아온다.
	CProtocolBuff* ID = (CProtocolBuff*)Packet;
	DWORD dwJoinRoomID;

	*ID >> dwJoinRoomID;

	// 4. 해당 ID의 방을 알아온다.
	stRoom* NowRoom = RoomSearch(dwJoinRoomID);
	
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff packet;
	BYTE result;
	DWORD RoomID;

	// 5. 만약, 방이 없다면(방 No오류) 결과에 "방오류"를 넣는다.
	if (NowRoom == nullptr)
	{
		result = df_RESULT_ROOM_ENTER_NOT;
		RoomID = 0;
	}

	// 방이 있다면, 결과에 "OK"를 넣는다
	else
	{
		result = df_RESULT_ROOM_ENTER_OK;

		// 그리고 유저를, 입장을 원했던 방에 추가한다.
		NowUser->m_JoinRoomNumber = dwJoinRoomID;
		NowRoom->m_JoinUserList.push_back(UserID);
		RoomID = NowRoom->m_RoomID;
	}

	// 6. "방입장 요청 결과" 패킷 제작
	CreatePacket_Res_RoomJoin((char*)&header, (char*)&packet, result, RoomID);

	// 7. 만든 패킷을 해당 유저의 SendBuff에 넣기.
	bool Check = SendPacket(UserID, &header, &packet);
	if (Check == false)
		return false;

	// 9. 해당 유저가 방에 들어갔다면,
	// 해당 유저와 같은 방에 있는 다른 유저에게, 해당 유저가 접속했다고 알려줘야한다.
	// "타 사용자 입장" 패킷을 만들고 보낸다.
	if (result == df_RESULT_ROOM_ENTER_OK)
	{
		Check = Network_Req_UserRoomJoin(UserID);
		if (Check == false)
			return false;
	}

	return true;
}

// "채팅 요청" 패킷 처리
bool Network_Req_Chat(DWORD UserID, char* Packet)
{
	// 1. 해당 ID의 유저 세션 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("Network_Req_Chat(). 로그인 되지 않은 유저가 채팅 요청을 보냄(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	// 방에 없는 유저가 채팅요청을 보낸다면, 연결을 끊는다.
	if (NowUser->m_JoinRoomNumber == 0)
	{
		_tprintf(_T("Network_Req_Chat(). 방에 있지 않은 유저가 채팅요청을 보냄.(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	// 3. 메시지 길이를 알아온다.
	CProtocolBuff* MessagePacket = (CProtocolBuff*)Packet;
	WORD MessageSize;
	*MessagePacket >> MessageSize;

	// 4. 메시지를 뽑아온다.
	TCHAR* Message = new TCHAR[MessageSize / 2];
	for (int i = 0; i < MessageSize / 2; ++i)
		*MessagePacket >> Message[i];

	// 5. "채팅 요청 결과" 패킷 제작 
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff packet;

	CreatePacket_Res_RoomJoin((char*)&header, (char*)&packet, MessageSize, (char*)Message, UserID);

	// 6. 만든 패킷을, 같은 방에 있는 모든 유저의 SendBuff에 넣는다. (송신자 1명 제외)
	BroadCast_Room(&header, &packet, NowUser->m_JoinRoomNumber, UserID);

	return true;
}

// "방 퇴장 요청" 패킷 처리
bool Network_Req_RoomLeave(DWORD UserID, char* Packet)
{
	// 1. 해당 ID의 유저 세션 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("Network_Req_RoomLeave(). 로그인 되지 않은 유저가 방 퇴장 요청을 보냄(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	// 방에 없는 유저가 채팅요청을 보낸다면, 연결을 끊는다.
	if (NowUser->m_JoinRoomNumber == 0)
	{
		_tprintf(_T("Network_Req_RoomLeave(). 방에 있지 않은 유저가 방 퇴장 요청을 보냄.(UserID : %d). 접속 종료\n"), UserID);
		return false;
	}

	// 3. "방 퇴장 요청 결과" 패킷 제작.
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff packet;

	CreatePacket_Res_RoomLeave((char*)&header, (char*)&packet, UserID, NowUser->m_JoinRoomNumber);

	// 4. 만든 패킷을 같은 방 안의 모든 유저의 SendBuff에 넣는다. (나 포함)
	BroadCast_Room(&header, &packet, NowUser->m_JoinRoomNumber, -1);

	// 5. 방 퇴장을 요청한 유저를, 방에서 제외시킨다.
	stRoom* NowRoom = RoomSearch(NowUser->m_JoinRoomNumber);
	list<DWORD>::iterator listitor;
	for (listitor = NowRoom->m_JoinUserList.begin(); listitor != NowRoom->m_JoinUserList.end(); ++listitor)
	{
		if (*listitor == UserID)
		{
			NowRoom->m_JoinUserList.erase(listitor);
			break;
		}
	}
	NowUser->m_JoinRoomNumber = 0;

	// 6. 만약, 해당 방의 유저가 0명이 되었다면, 방 삭제 패킷을 모든 유저에게 보낸다.
	if (NowRoom->m_JoinUserList.size() == 0)
		Network_Req_RoomDelete(NowRoom->m_RoomID);

	return true;
}

// "방 삭제 결과"를 만들고 보낸다 
// 방에 사람이 있다가, 0명이 되는 순간 발생한다.
bool Network_Req_RoomDelete(DWORD RoomID)
{
	// 1. "방 삭제 결과" 패킷 만들기
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff packet;

	CreatePacket_Res_RoomDelete((char*)&header, (char*)&packet, RoomID);

	// 2. 로그인 된 모든 유저의 SendBuff에 "방 삭제 결과" 패킷 넣기
	BroadCast_All(&header, &packet);

	// 그리고 그 방은 방 리스트에서 제외시킨다.
	map_RoomList.erase(RoomID);
	return true;
}

// "타 사용자 입장" 패킷을 만들고 보낸다.
// 어떤 유저가 방에 접속 시, 해당 방에 있는 다른 유저에게 다른 사용자가 접속했다고 알려주는 패킷 (서->클 만 가능)
bool Network_Req_UserRoomJoin(DWORD JoinUserID)
{
	// 1. 해당 유저의 세션을 알아온다.
	stClinet* NowUser = ClientSearch(JoinUserID);

	// 2. 예외처리
	// 로그인 중인 유저인지
	if (NowUser == nullptr)
	{
		_tprintf(_T("Network_Req_UserRoomJoin(). 로그인 되지 않은 유저가 방에 접속했다고 함.(UserID : %d). 접속 종료\n"), JoinUserID);
		return false;
	}

	// 방에 있는 유저인지
	if (NowUser->m_JoinRoomNumber == 0)
	{
		_tprintf(_T("Network_Res_UserRoomJoin(). 방이 없는 유저가 방에 접속했다고 처리됨. (UserID : %d). 접속 종료\n"), JoinUserID);
		return false;
	}

	// 3. "타 사용자 입장" 패킷 만들기
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff packet;	

	CreatePacket_Res_UserRoomJoin((char*)&header, (char*)&packet, (char*)NowUser->m_NickName, JoinUserID);

	// 4. 해당 유저와 같은방에 있는 모든 유저(해당 유저 제외)의 SendBuff에 패킷을 넣는다.
	BroadCast_Room(&header, &packet, NowUser->m_JoinRoomNumber, JoinUserID);

	return true;
}



/////////////////////////
// 패킷 제작 함수
/////////////////////////
// "로그인 요청 결과" 패킷 제작
void CreatePacket_Res_Login(char* header, char* Packet, int UserID, char result)
{
	// 1. "로그인 요청 결과" 패킷 조립
	CProtocolBuff* paylodBuff = (CProtocolBuff*)Packet;

	BYTE bResult = result;
	DWORD dwUserID = UserID;

	*paylodBuff << bResult;
	*paylodBuff << dwUserID;

	// 2. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_LOGIN;
	WORD wPayloadSize = df_RES_LOGIN_SIZE;

	// 헤더의 체크썸 조립	
	CProtocolBuff ChecksumBuff;
	ChecksumBuff << bResult;
	ChecksumBuff << dwUserID;

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

	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;
}

// "방 목록 요청 결과" 패킷 제작
void CreatePacket_Res_RoomList(char* header, char* Packet)
{
	// 1. "방 목록 요청 결과" 패킷 조립 (페이로드)
	CProtocolBuff* paylodBuff = (CProtocolBuff*)Packet;
	CProtocolBuff ChecksumBuff;

	// 서버에 존재하는 룸 수
	WORD wCount = (WORD)map_RoomList.size();
	*paylodBuff << wCount;

	map <DWORD, stRoom*>::iterator itor;
	for (itor = map_RoomList.begin(); itor != map_RoomList.end(); ++itor)
	{
		// 방 No
		DWORD dwRoomNumber = itor->second->m_RoomID;

		// 방 이름의 Size
		WORD wRoomNameSize = (WORD)(_tcslen(itor->second->m_RoomName) * 2);

		// 방 이름(유니코드)
		TCHAR* tRoomName = itor->second->m_RoomName;

		// 참여 인원
		BYTE bJoinUser = (BYTE)itor->second->m_JoinUserList.size();

		// 여기까지 페이로드 버퍼에 넣음.
		*paylodBuff << dwRoomNumber;
		*paylodBuff << wRoomNameSize;	

		for (int i = 0; i < wRoomNameSize/2; ++i)
			*paylodBuff << tRoomName[i];

		*paylodBuff << bJoinUser;

		// 참여 인원들의 닉네임(유니코드 15글자씩 끊어서)
		list<DWORD>::iterator listitor;
		for (listitor = itor->second->m_JoinUserList.begin(); listitor != itor->second->m_JoinUserList.end(); ++listitor)
		{			
			stClinet* NowUser = ClientSearch(*listitor);

			for(int i=0; i<dfNICK_MAX_LEN; ++i)
				*paylodBuff << NowUser->m_NickName[i];
		}		
	}

	// 2. 체크섬용 직렬화버퍼 셋팅 (페이로드)
	memcpy(ChecksumBuff.GetBufferPtr(), paylodBuff->GetBufferPtr(), paylodBuff->GetUseSize());
	ChecksumBuff.MoveWritePos(paylodBuff->GetUseSize());

	// 3. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_ROOM_LIST;
	WORD wPayloadSize = paylodBuff->GetUseSize();

	// 헤더의 체크썸 채우기
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

	// 4. 마지막 채우기
	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;
}

// "대화방 생성 요청 결과" 패킷 제작
void CreatePacket_Res_RoomCreate(char* header, char* Packet, char result, WORD RoomSize, char* cRoomName)
{
	// 1. "대화방 생성 요청 결과" 패킷 조립 (페이로드)
	CProtocolBuff* paylodBuff = (CProtocolBuff*)Packet;

	BYTE bResult = result;
	DWORD dwRoomID = g_dwRoomID;
	WORD bRoomNameSize = RoomSize;
	TCHAR* RoomName = (TCHAR*)cRoomName;

	*paylodBuff << bResult;
	*paylodBuff << dwRoomID;
	*paylodBuff << bRoomNameSize;

	for(int i=0; i<bRoomNameSize/2; ++i)
		*paylodBuff << RoomName[i];

	// 2. 아래에서 헤더의 체크썸 조립에 사용하기 위한, 체크썸 직렬화버퍼 셋팅 (페이로드)
	CProtocolBuff ChecksumBuff;
	memcpy(ChecksumBuff.GetBufferPtr(), paylodBuff->GetBufferPtr(), paylodBuff->GetUseSize());
	ChecksumBuff.MoveWritePos(paylodBuff->GetUseSize());

	// 3. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_ROOM_CREATE;
	WORD wPayloadSize = paylodBuff->GetUseSize();

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

	BYTE SaveChecksum = (BYTE)(Checksum % 256);

	// 4. 마지막 완성
	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;
}

// "방입장 요청 결과" 패킷 제작
void CreatePacket_Res_RoomJoin(char* header, char* Packet, char result, DWORD RoomID)
{
	// ID를 이용해 해당 방을 알아온다.
	stRoom* NowRoom = RoomSearch(RoomID);

	// 1. "방입장 요청 결과" 패킷 조립 (페이로드)
	CProtocolBuff* paylodBuff = (CProtocolBuff*)Packet;

	// 인자 result가 df_RESULT_ROOM_ENTER_NOT(방오류)라면, result만 페이로드에 넣는다. 이걸로 페이로드 완성
	if (result == df_RESULT_ROOM_ENTER_NOT)
	{
		BYTE bResult = result;
		*paylodBuff << bResult;
	}

	// 인자 result가 df_RESULT_ROOM_ENTER_OK(방오류 아님)라면, 
	else if(result == df_RESULT_ROOM_ENTER_OK)
	{
		// 결과, 방ID, 방이름 사이즈, 방 이름(유니코드) 페이로드에 넣기
		BYTE bResult = result;
		DWORD dwRoomID = RoomID;
		WORD bRoomNameSize = (WORD)(_tcslen(NowRoom->m_RoomName) * 2);
		TCHAR* RoomName = NowRoom->m_RoomName;

		*paylodBuff << bResult;
		*paylodBuff << dwRoomID;
		*paylodBuff << bRoomNameSize;

		for (int i = 0; i<bRoomNameSize / 2; ++i)
			*paylodBuff << RoomName[i];

		// 참가인원 넣기
		BYTE bJoinCount = (BYTE)NowRoom->m_JoinUserList.size();
		*paylodBuff << bJoinCount;

		// 참가인원의 닉네임(15바이트 고정)과 유저ID를 페이로드에 추가
		list <DWORD>::iterator listitor;
		for (listitor = NowRoom->m_JoinUserList.begin(); listitor != NowRoom->m_JoinUserList.end(); ++listitor)
		{
			// 방에 있는 유저 ID
			DWORD dwUserID = *listitor;

			// 해당 ID의 유저 닉네임 알아내기
			TCHAR UserNicname[dfNICK_MAX_LEN];
			map <DWORD, stClinet*>::iterator clinetitor;
			for (clinetitor = map_ClientList.begin(); clinetitor != map_ClientList.end(); ++clinetitor)
			{
				if (dwUserID == clinetitor->first)
				{
					_tcscpy_s(UserNicname, dfNICK_MAX_LEN, clinetitor->second->m_NickName);
					break;
				}
			}

			// 닉네임, ID 순서대로 페이로드에 추가
			for (int i = 0; i < dfNICK_MAX_LEN; ++i)
				*paylodBuff << UserNicname[i];
			
			*paylodBuff << dwUserID;
		}
	}

	// 2. 아래에서 헤더의 체크썸 조립에 사용하기 위한, 체크썸 직렬화버퍼 셋팅 (페이로드)
	CProtocolBuff ChecksumBuff;
	memcpy(ChecksumBuff.GetBufferPtr(), paylodBuff->GetBufferPtr(), paylodBuff->GetUseSize());
	ChecksumBuff.MoveWritePos(paylodBuff->GetUseSize());

	// 3. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_ROOM_ENTER;
	WORD wPayloadSize = paylodBuff->GetUseSize();

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

	// 4. 마지막 완성
	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;	
}

// "채팅 요청 결과" 패킷 제작
void CreatePacket_Res_RoomJoin(char* header, char* Packet, WORD MessageSize, char* cMessage, DWORD UserID)
{
	// 1. 페이로드 완성하기
	DWORD dwSenderID = UserID;
	WORD wMessageSize = MessageSize;
	TCHAR* tMessage = (TCHAR*)cMessage;

	CProtocolBuff* payloadBuff = (CProtocolBuff*)Packet;
	*payloadBuff << dwSenderID;
	*payloadBuff << wMessageSize;
	for (int i = 0; i < wMessageSize / 2; ++i)
		*payloadBuff << tMessage[i];

	// 2. 아래에서 헤더의 체크썸 조립에 사용하기 위한, 체크썸 직렬화버퍼 셋팅 (페이로드)
	CProtocolBuff ChecksumBuff;
	memcpy(ChecksumBuff.GetBufferPtr(), payloadBuff->GetBufferPtr(), payloadBuff->GetUseSize());
	ChecksumBuff.MoveWritePos(payloadBuff->GetUseSize());

	// 3. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_CHAT;
	WORD wPayloadSize = payloadBuff->GetUseSize();

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

	// 4. 마지막 완성
	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;

}

// "방 퇴장 요청 결과" 패킷 제작
void CreatePacket_Res_RoomLeave(char* header, char* Packet, DWORD UserID, DWORD RoomID)
{
	// 1. 페이로드 만들기
	CProtocolBuff* payloadBuff = (CProtocolBuff*)Packet;
	DWORD dwUserID = UserID;

	*payloadBuff << dwUserID;

	// 2. 아래에서 헤더의 체크썸 조립에 사용하기 위한, 체크썸 직렬화버퍼 셋팅 (페이로드)
	CProtocolBuff ChecksumBuff;
	ChecksumBuff << dwUserID;

	// 3. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_ROOM_LEAVE;
	WORD wPayloadSize = payloadBuff->GetUseSize();

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

	// 4. 마지막 완성
	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;

}

// "방 삭제 결과" 패킷 제작
void CreatePacket_Res_RoomDelete(char* header, char* Packet, DWORD RoomID)
{
	// 1. 페이로드 만들기
	CProtocolBuff* payloadBuff = (CProtocolBuff*)Packet;
	DWORD dwRoomID = RoomID;

	*payloadBuff << RoomID;

	// 2. 아래에서 헤더의 체크썸 조립에 사용하기 위한, 체크썸 직렬화버퍼 셋팅 (페이로드)
	CProtocolBuff ChecksumBuff;
	ChecksumBuff << RoomID;

	// 3. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_ROOM_DELETE;
	WORD wPayloadSize = payloadBuff->GetUseSize();

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

	// 4. 마지막 완성
	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;
}

// "타 사용자 입장" 패킷 제작
void CreatePacket_Res_UserRoomJoin(char* header, char* Packet, char* cNickName, DWORD UserID)
{
	// 1. 페이로드 만들기
	CProtocolBuff* payloadBuff = (CProtocolBuff*)Packet;

	TCHAR* UserNickName = (TCHAR*)cNickName;
	DWORD dwUserID = UserID;

	for (int i = 0; i < dfNICK_MAX_LEN; ++i)
		*payloadBuff << UserNickName[i];

	*payloadBuff << dwUserID;

	// 2. 아래에서 헤더의 체크썸 조립에 사용하기 위한, 체크썸 직렬화버퍼 셋팅 (페이로드)
	CProtocolBuff ChecksumBuff;
	memcpy(ChecksumBuff.GetBufferPtr(), payloadBuff->GetBufferPtr(), payloadBuff->GetUseSize());
	ChecksumBuff.MoveWritePos(payloadBuff->GetUseSize());

	// 3. 헤더 조립
	CProtocolBuff* headerBuff = (CProtocolBuff*)header;

	BYTE byCode = dfPACKET_CODE;
	WORD wMsgType = df_RES_USER_ENTER;
	WORD wPayloadSize = payloadBuff->GetUseSize();

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

	// 4. 마지막 완성
	*headerBuff << byCode;
	*headerBuff << SaveChecksum;
	*headerBuff << wMsgType;
	*headerBuff << wPayloadSize;
}



/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(DWORD UserID, CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff)
{
	// 인자로 받은 UserID를 기준으로 클라이언트 목록에서 유저 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("SendPacket(). 로그인 중이 아닌 유저를 대상으로 SendPacket이 호출됨. 접속 종료\n"));
		return false;
	}

	// 1. 샌드 q에 헤더 넣기
	int Size = headerBuff->GetUseSize();
	DWORD BuffArray = 0;
	int a = 0;
	while (Size > 0)
	{
		int EnqueueCheck = NowUser->m_SendBuff.Enqueue(headerBuff->GetBufferPtr() + BuffArray, Size);
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
		int EnqueueCheck = NowUser->m_SendBuff.Enqueue(payloadBuff->GetBufferPtr() + BuffArray, PayloadLen);
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
bool SendProc(DWORD UserID)
{
	// 인자로 받은 UserID를 기준으로 클라이언트 목록에서 유저 알아오기.
	stClinet* NowUser = ClientSearch(UserID);

	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowUser == nullptr)
	{
		_tprintf(_T("SendProc(). 로그인 중이 아닌 유저를 대상으로 SendProc이 호출됨. 접속 종료\n"));
		return false;
	}

	// 1. SendBuff에 보낼 데이터가 있는지 확인.
	if (NowUser->m_SendBuff.GetUseSize() == 0)
		return true;

	// 3. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = NowUser->m_SendBuff.GetBufferPtr();

	// 4. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 5. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (NowUser->m_SendBuff.GetUseSize() == 0)
			break;

		// 6. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = NowUser->m_SendBuff.GetNotBrokenGetSize();

		// 7. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 8. front 포인터 얻어옴
		int *front = (int*)NowUser->m_SendBuff.GetReadBufferPtr();

		// 9. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = NowUser->m_SendBuff.NextIndex(*front, 1);

		// 10. Send()
		int SendSize = send(NowUser->m_sock, &sendbuff[BuffArrayCheck], Size, 0);

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
		NowUser->m_SendBuff.RemoveData(SendSize);
	}

	return true;
}
