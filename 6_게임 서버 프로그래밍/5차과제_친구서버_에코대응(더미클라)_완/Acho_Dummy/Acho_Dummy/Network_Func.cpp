#include "stdafx.h"
#include "Network_Func.h"

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#include <time.h>

#include <map>

using namespace std;

// 더미 구조체
struct stDummyClient
{
	SOCKET		m_sock;							// 해당 더미의 소켓
	CRingBuff	m_RecvBuff;						// 리시브 버퍼
	CRingBuff	m_SendBuff;						// 샌드 버퍼

	UINT64		m_AccountNo;					// 더미와 연결된(로그인 된) 유저 회원 넘버. 0일 경우, 로그인 안된 더미. 현재는 모든 더미 다 0
	DWORD		m_SendTime = 0;					// 이번에 패킷 보낸 시간

	char*		m_SendString;					// 서버로 보낸 바이트문자열 저장. 서버가 나에게 돌려보내면 체크한다.
	bool		m_bRecvCheck = false;			// 내가 Send 후 recv를 기다리는 경우 true;	
};


// 전역변수
map<SOCKET, stDummyClient*> map_DummyClient;	// 더미 클라이언트를 관리하는 구조체
int g_iTPSCount;								// 현재 TPS는 send 횟수로 체크중

// 레이턴시 현재 값 변수들
DWORD g_TotalLaytencyTime = 1;					// 1초 기준 총 레이턴시 시간
DWORD g_TotalLaytencyCount;						// 1초 기준 레이턴시 체크 카운트.

// 레이턴시 평균 체크 변수들
DWORD g_TotalLaytencyTimeAvg;					// 현재까지 총 레이턴시 시간 (평균 계산용)
DWORD g_TotalLaytencyCountAvg;					// 현재까지 g_TotalLaytencyTimeAvg에 값을 더한 횟수. 현재 1초마다 1 증가 (평균 계산용)

// 외부변수
extern int g_iDummyCount;						// main에서 셋팅한 Dummy 카운트												
extern TCHAR g_tIP[30];							// main에서 셋팅한 서버의 IP
extern UINT64 g_iErrorCount;					// 에러 발생 횟수 체크하는 카운트. 증가하면 절대 초기화되지 않는다.


// 네트워크 셋팅 함수 (윈속초기화, 커넥트 등..)
bool Network_Init(int* TryCount, int* FailCount, int* SuccessCount)
{
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
		stDummyClient* NewDummy = new stDummyClient;

		// 1. 소켓 생성
		NewDummy->m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (NewDummy->m_sock == INVALID_SOCKET)
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

		DWORD dCheck = connect(NewDummy->m_sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));

		// 3. 커넥트 시도 횟수 증가
		(*TryCount)++;


		// 3. 커넥트 실패 시, 실패 카운트 1 증가
		if (dCheck == SOCKET_ERROR)
		{
			_tprintf(_T("connect 실패. %d (Error : %d)\n"), i + 1, WSAGetLastError());
			(*FailCount)++;
		}

		// 커넥트 성공 시, 성공 카운트 1증가. 그리고 해당 더미를 map에 추가
		else
		{
			_tprintf(_T("connect 성공. %d\n"), i + 1);
			map_DummyClient.insert(pair<SOCKET, stDummyClient*>(NewDummy->m_sock, NewDummy));

			(*SuccessCount)++;
		}
	}

	return true;
}

// 네트워크 프로세스
// 여기서 false가 반환되면, 프로그램이 종료된다.
bool Network_Process()
{
	//  통신에 사용할 변수
	FD_SET rset;
	FD_SET wset;
	TIMEVAL tval;
	tval.tv_sec = 0;
	tval.tv_usec = 0;

	map <SOCKET, stDummyClient*>::iterator itor;
	map <SOCKET, stDummyClient*>::iterator Enditor;
	itor = map_DummyClient.begin();

	DWORD dwFDCount = 0;

	// 최초 초기화
	rset.fd_count = 0;
	wset.fd_count = 0;
	SOCKET* NowSock[FD_SETSIZE];

	while (1)
	{
		Enditor = map_DummyClient.end();

		// 모든 멤버를 순회하며, 해당 더미를 읽기 셋과 쓰기 셋에 넣는다.
		// 넣다가 64개가 되거나, end에 도착하면 break
		while (itor != Enditor)
		{
			// 해당 더미에게 받을 데이터가 있는지 체크하기 위해, 모든 더미를 rset에 소켓 설정
			FD_SET(itor->second->m_sock, &rset);
			NowSock[dwFDCount] = &itor->second->m_sock;

			// 만약, 해당 더미의 SendBuff에 값이 있으면, wset에도 소켓 넣음.
			if (itor->second->m_SendBuff.GetUseSize() != 0)
				FD_SET(itor->second->m_sock, &wset);

			// 64개 꽉 찼는지, 끝에 도착했는지 체크		
			++itor;
			if (dwFDCount + 1 == FD_SETSIZE || itor == Enditor)
				break;

			++dwFDCount;
		}

		// Select()
		DWORD dCheck = select(0, &rset, &wset, 0, &tval);

		// Select()결과 처리
		if (dCheck == SOCKET_ERROR)
		{
			_tprintf(_T("select 에러(%d)\n"), WSAGetLastError());
			return true;
		}

		// select의 값이 0보다 크다면 뭔가 할게 있다는 것이니 로직 진행
		else if (dCheck > 0)
		{
			for (DWORD i = 0; i <= dwFDCount; ++i)
			{
				// 쓰기 셋 처리
				if (FD_ISSET(*NowSock[i], &wset))
				{
					stDummyClient* NowDummy = DummySearch(*NowSock[i]);

					// Send() 처리
					// 만약, SendProc()함수가 false가 리턴된다면, 해당 더미 접속 종료
					if (SendProc(&NowDummy->m_SendBuff, NowDummy->m_sock) == false)
					{
						Disconnect(*NowSock[i]);
						continue;
					}
				}

				// 읽기 셋 처리
				if (FD_ISSET(*NowSock[i], &rset))
				{
					stDummyClient* NowDummy = DummySearch(*NowSock[i]);						

					// Recv() 처리
					// 만약, RecvProc()함수가 false가 리턴된다면, 해당 더미 접속 종료
					if (RecvProc(NowDummy) == false)
						Disconnect(*NowSock[i]);
				}				
			}
		}

		// 만약, 모든 더미에 대해 Select처리가 끝났으면, 이번 함수는 여기서 종료.
		if (itor == Enditor)
			break;

		// select 준비	
		rset.fd_count = 0;
		wset.fd_count = 0;
		dwFDCount = 0;
	}

	return true;
}

// 인자로 받은 Socket 값을 기준으로 [더미 목록]에서 [더미를 골라낸다].(검색)
// 성공 시, 해당 더미의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stDummyClient* DummySearch(SOCKET sock)
{
	stDummyClient* NowDummy = nullptr;
	map <SOCKET, stDummyClient*>::iterator iter;

	iter = map_DummyClient.find(sock);
	if (iter == map_DummyClient.end())
		return nullptr;

	NowDummy = iter->second;
	return NowDummy;
}

// Disconnect 처리
void Disconnect(SOCKET sock)
{
	// 인자로 받은 sock을 기준으로 더미 목록에서 더미 알아오기
	stDummyClient* NowDummy = DummySearch(sock);

	// 예외사항 체크
	// 1. 해당 더미가 접속 중인가.
	// NowUser의 값이 nullptr이면, 해당 더미를 못찾은 것. false를 리턴한다.
	if (NowDummy == nullptr)
	{
		_tprintf(_T("Disconnect(). 접속 중이지 않은 더미를 삭제하려고 함.\n"));
		return;
	}

	// 2. 해당 Accept 유저를 AcceptList에서 제거
	map_DummyClient.erase(sock);

	// 3. 접속 종료한 유저의 ip, port, AccountNo, socket 출력	
	_tprintf(_T("Disconnect : AccountNo : %lld / Socket : %lld]\n"), NowDummy->m_AccountNo, NowDummy->m_sock);

	// 4. 해당 유저의 소켓 close
	closesocket(NowDummy->m_sock);

	// 5. 해당 Accept유저 동적 해제
	delete NowDummy;
}




///////////////////////////////
// 화면 출력용 값 반환 함수
///////////////////////////////
// 평균 레이턴시 반환 함수
DWORD GetAvgLaytency()
{
	//////////////평균 값 반환하는 코드 //////////////////		
	// 1. 평균 계산	
	
	// 이번 프레임(1초)의 레이턴시 값 구하기
	g_TotalLaytencyTimeAvg += (g_TotalLaytencyTime / g_TotalLaytencyCount) ;

	// g_TotalLaytencyTimeAvg에 값을 더한 횟수 증가
	g_TotalLaytencyCountAvg++;

	// 평균 계산
	DWORD LaytencyAvg = g_TotalLaytencyTimeAvg / g_TotalLaytencyCountAvg;
	
	// 2. 다음 셋팅을 위해 값 초기화
	g_TotalLaytencyTime = 0;
	g_TotalLaytencyCount = 0;

	return LaytencyAvg;	
	
	
	
	


	//////////////현재 값 반환하는 코드 //////////////////

	/*
	// 1. 이번 프레임(1초)의 레이턴시 값 구하기
	// ms단위이기 때문에 1000한다.
	if (g_TotalLaytencyCount == 0)
		g_TotalLaytencyCount = 1;
	DWORD NowLaytency = (g_TotalLaytencyTime / g_TotalLaytencyCount) / 1000;

	// 2. 다음 셋팅을 위해 값 초기화
	g_TotalLaytencyTime = 1;
	g_TotalLaytencyCount = 0;

	return NowLaytency;
	*/
	
}


// TPS 반환 함수
int GetTPS()
{
	int temp = g_iTPSCount;
	g_TotalLaytencyCount = g_iTPSCount;
	g_iTPSCount = 0;

	return temp;
}


///////////////////////////////
// 더미의 할 일.
///////////////////////////////
// 랜덤 문자열 생성 후 SendBuff에 넣는다.
void DummyWork_CreateString()
{
	map<SOCKET, stDummyClient*>::iterator itor;

	// 처음부터 끝까지 순회. (느릴듯한데.. 좀 더 고민 필요)
	for (itor = map_DummyClient.begin(); itor != map_DummyClient.end(); ++itor)
	{
		// 리시브 대기중인 더미라면, 문자열 생성 안한다.
		if (itor->second->m_bRecvCheck == true)
			continue;

		// 1. N개의 랜덤한 숫자 생성 후 문자열로 넣음.
		WORD Size = rand() % 1000;
		char* SendByte = new char[Size];
		for (int i = 0; i<Size; ++i)
			SendByte[i] = rand() % 128;

		//  2. 해당 더미의 m_SendString에 복사
		itor->second->m_SendString = new char[Size];
		memcpy(itor->second->m_SendString, SendByte, Size);

		// 3. 페이로드 생성		
		CProtocolBuff payload;
		payload << Size;
		memcpy(payload.GetBufferPtr() + 2, SendByte, Size);
		payload.MoveWritePos(Size);

		// 4. 헤더 제작
		CProtocolBuff header(dfHEADER_SIZE);
		BYTE byCode = dfPACKET_CODE;
		WORD MsgType = df_REQ_STRESS_ECHO;
		WORD PayloadSize = Size + 2;

		header << byCode;
		header << MsgType;
		header << PayloadSize;

		// 5. 만든 패킷을 해당 더미의, SendBuff에 넣기
		SendPacket(itor->second, &header, &payload);

		// 해당 더미를 "recv 대기상태"로 변경
		itor->second->m_bRecvCheck = true;
	}
}




/////////////////////////
// Recv 처리 함수들
/////////////////////////
bool RecvProc(stDummyClient* NowDummy)
{
	////////////////////////////////////////////////////////////////////////
	// 소켓 버퍼에 있는 모든 recv 데이터를, 해당 유저의 recv링버퍼로 리시브.
	////////////////////////////////////////////////////////////////////////

	// 1. recv 링버퍼 포인터 얻어오기.
	char* recvbuff = NowDummy->m_RecvBuff.GetBufferPtr();

	// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = NowDummy->m_RecvBuff.GetNotBrokenPutSize();

	// 3. 한 번에 쓸 수 있는 공간이 0이라면
	if (Size == 0)
	{
		// rear 1칸 이동
		NowDummy->m_RecvBuff.MoveWritePos(1);

		// 그리고 한 번에 쓸 수 있는 위치 다시 알기.
		Size = NowDummy->m_RecvBuff.GetNotBrokenPutSize();
	}

	// 4. 그게 아니라면, 1칸 이동만 하기. 		
	else
	{
		// rear 1칸 이동
		NowDummy->m_RecvBuff.MoveWritePos(1);
	}

	// 5. 1칸 이동된 rear 얻어오기
	int* rear = (int*)NowDummy->m_RecvBuff.GetWriteBufferPtr();

	// 6. recv()
	int retval = recv(NowDummy->m_sock, &recvbuff[*rear], Size, 0);			
	
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
		else if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)
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

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	NowDummy->m_RecvBuff.MoveWritePos(retval - 1);



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
		if (NowDummy->m_RecvBuff.GetUseSize() < len)
			break;

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = NowDummy->m_RecvBuff.Peek((char*)&HeaderBuff, len);

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
		if (NowDummy->m_RecvBuff.GetUseSize() < (len + HeaderBuff.wPayloadSize))
			break;

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		NowDummy->m_RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		DWORD BuffArray = 0;
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.wPayloadSize;

		while (PayloadSize > 0)
		{
			int DequeueSize = NowDummy->m_RecvBuff.Dequeue(PayloadBuff.GetBufferPtr() + BuffArray, PayloadSize);

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

		// 7. 헤더에 들어있는 타입에 따라 분기처리. (지금은 에코처리 함수 호출)
		bool check = Network_Res_Acho(HeaderBuff.wMsgType, NowDummy, &PayloadBuff);
		if (check == false)
			return false;
	}

	return true;
}

// "에코 패킷" 처리
bool Network_Res_Acho(WORD Type, stDummyClient* NowDummy, CProtocolBuff* payload)
{
	/////// 여기 오면 완성된 데이터를 받았다는 것이니 레이턴시 체크 ///////////
	g_TotalLaytencyTime += timeGetTime() - NowDummy->m_SendTime;
	NowDummy->m_SendTime = 0;

	// 일단, recv 대기상태를 false로 변경. 다음 데이터 send를 위해
	NowDummy->m_bRecvCheck = false;

	// 1. 패킷 타입 체크
	// 에코가 아니라면,
	if (Type != df_RES_STRESS_ECHO)
	{
		// Error 메시지 표시
		fputs("Type Error!!!\n", stdout);

		// 에러카운트 추가
		g_iErrorCount++;

		return true;
	}

	// 2. 문자열 체크
	// 일단 문자열 사이즈 얻어오기 (바이트 단위)
	WORD wStringSize;
	*payload >> wStringSize;

	// 사이즈 만큼 문자열 가져오기
	char* TestString = new char[wStringSize];
	memcpy(TestString, payload->GetBufferPtr() + 2, wStringSize);

	// 내가 보냈던 문자열이 아니라면
	if (memcmp(NowDummy->m_SendString, TestString, wStringSize) != 0)
	{
		// Error 메시지 표시
		fputs("Error Data!!!\n", stdout);

		// 에러카운트 추가
		g_iErrorCount++;
	}

	// 3. 기존의 문자열 동적해제
	delete NowDummy->m_SendString;
	delete TestString;

	return true;
}




/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(stDummyClient* NowDummy, CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff)
{
	// 1. 샌드 q에 헤더 넣기
	int Size = headerBuff->GetUseSize();
	DWORD BuffArray = 0;
	int a = 0;
	while (Size > 0)
	{
		int EnqueueCheck = NowDummy->m_SendBuff.Enqueue(headerBuff->GetBufferPtr() + BuffArray, Size);
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
		int EnqueueCheck = NowDummy->m_SendBuff.Enqueue(payloadBuff->GetBufferPtr() + BuffArray, PayloadLen);
		if (EnqueueCheck == -1)
		{
			_tprintf(_T("SendPacket(). 페이로드 넣는 중 샌드큐 꽉참. 접속 종료\n"));
			return false;
		}

		PayloadLen -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	/////// 샌드 큐에 넣고 후 바로 레이턴시 시간 갱신 ///////////
	NowDummy->m_SendTime = timeGetTime();
	g_iTPSCount++;
	return true;
}

// Send버퍼의 데이터를 Send하기
bool SendProc(CRingBuff* SendBuff, SOCKET sock)
{
	// 2. SendBuff에 보낼 데이터가 있는지 확인.
	if (SendBuff->GetUseSize() == 0)
		return true;

	// 3. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = SendBuff->GetBufferPtr();

	// 4. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 5. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (SendBuff->GetUseSize() == 0)
			break;

		// 6. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = SendBuff->GetNotBrokenGetSize();

		// 7. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 8. front 포인터 얻어옴
		int *front = (int*)SendBuff->GetReadBufferPtr();

		// 9. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = SendBuff->NextIndex(*front, 1);		

		// 10. Send()
		int SendSize = send(sock, &sendbuff[BuffArrayCheck], Size, 0);

		// 11. 에러 체크. 우드블럭이면 더 이상 못보내니 while문 종료. 다음 Select때 다시 보냄
		if (SendSize == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;

			// 10053, 10054 둘다 어쩃든 연결이 끊어진 상태
			// WSAECONNABORTED(10053) :  프로토콜상의 에러나 타임아웃에 의해 연결(가상회선. virtual circle)이 끊어진 경우
			// WSAECONNRESET(10054) : 원격 호스트가 접속을 끊음.
			// 이 때는 그냥 return false로 나도 상대방과 연결을 끊는다.
			else if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)
				return false;

			else
			{
				_tprintf(_T("SendProc(). Send중 에러 발생. 접속 종료\n"));
				return false;
			}


		}

		// 12. 보낸 사이즈가 나왔으면, 그 만큼 remove
		SendBuff->RemoveData(SendSize);
	}

	return true;
}
