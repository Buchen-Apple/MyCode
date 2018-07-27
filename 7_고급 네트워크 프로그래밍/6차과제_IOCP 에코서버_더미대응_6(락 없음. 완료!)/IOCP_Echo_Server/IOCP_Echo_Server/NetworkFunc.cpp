#include "stdafx.h"
#include "NetworkFunc.h"


SRWLOCK g_Session_map_srwl;
map<SOCKET, stSession*> map_Session;

extern long IOCountMinusCount;
extern long JoinUser;
extern long SessionDeleteCount;
extern int		g_LogLevel;			// Main에서 입력한 로그 출력 레벨. 외부변수
extern TCHAR	g_szLogBuff[1024];		// Main에서 입력한 로그 출력용 임시 버퍼. 외부변수



// 접속 종료
void Disconnect(stSession* DeleteSession)
{
	//_tprintf(L"[TCP 서버] 접속 종료 : IP 주소=%s, 포트=%d\n", DeleteSession->m_IP, DeleteSession->m_prot);

	// map에서 제외시키기
	LockSession();
	map_Session.erase(DeleteSession->m_Client_sock);		
	UnlockSession();

	// 클로즈 소켓, 세션 동적해제	
	closesocket(DeleteSession->m_Client_sock);
	delete DeleteSession;

	InterlockedIncrement(&SessionDeleteCount);	
	InterlockedDecrement(&JoinUser);	
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





// ------------
// 리시브 관련 함수들
// ------------
// RecvProc 함수. 큐의 내용 체크 후 PacketProc으로 넘긴다.
bool RecvProc(stSession* NowSession)
{
	// -----------------
	// Recv 큐 관련 처리
	// -----------------

	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
		WORD Header_PaylaodSize = 0;

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		int UseSize = NowSession->m_RecvBuff.GetUseSize();
		if (UseSize < dfNETWORK_PACKET_HEADER_SIZE)
		{
			break;
		}

		// 2. 헤더를 Peek으로 확인한다.  Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 
		// 버퍼가 비어있으면 접속 끊음.
		int PeekSize = NowSession->m_RecvBuff.Peek((char*)&Header_PaylaodSize, dfNETWORK_PACKET_HEADER_SIZE);
		if (PeekSize == -1)
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] RecvProc(). 헤더->Peek : Buff Empty\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);

			shutdown(NowSession->m_Client_sock, SD_BOTH);

			return false;
		}

		// 3. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size)
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (UseSize < (dfNETWORK_PACKET_HEADER_SIZE + Header_PaylaodSize))
		{
			break;
		}

		// 4. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		NowSession->m_RecvBuff.RemoveData(dfNETWORK_PACKET_HEADER_SIZE);

		// 5. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = Header_PaylaodSize;

		int DequeueSize = NowSession->m_RecvBuff.Dequeue(PayloadBuff.GetBufferPtr(), PayloadSize);
		// 버퍼가 비어있으면 접속 끊음
		if (DequeueSize == -1)
		{
			SYSTEMTIME lst;
			GetLocalTime(&lst);
			_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] RecvProc(). 페이로드->Dequeue : Buff Empty\n",
				lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);
			
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			return false;
		}
		PayloadBuff.MoveWritePos(DequeueSize);

		// 9. 헤더에 들어있는 타입에 따라 분기처리. false가 리턴되면 접속 끊음.
		if (PacketProc(NowSession, &PayloadBuff) == false)		
			return false;

	}

	return true;
}

// Accept용 RecvProc함수
bool RecvPost_Accept(stSession* NowSession)
{
	// ------------------
	// 비동기 입출력 시작
	// ------------------
	// 1. WSABUF 셋팅.
	WSABUF wsabuf[2];
	int wsabufCount = 0;

	int FreeSize = NowSession->m_RecvBuff.GetFreeSize();
	int Size = NowSession->m_RecvBuff.GetNotBrokenPutSize();

	if (Size < FreeSize)
	{
		wsabuf[0].buf = NowSession->m_RecvBuff.GetRearBufferPtr();
		wsabuf[0].len = Size;

		wsabuf[1].buf = NowSession->m_RecvBuff.GetBufferPtr();
		wsabuf[1].len = FreeSize - Size;
		wsabufCount = 2;

	}
	else
	{
		wsabuf[0].buf = NowSession->m_RecvBuff.GetRearBufferPtr();
		wsabuf[0].len = Size;

		wsabufCount = 1;
	}

	// 2. Overlapped 구조체 초기화
	ZeroMemory(&NowSession->m_RecvOverlapped, sizeof(NowSession->m_RecvOverlapped));

	// 3. WSARecv()
	DWORD recvBytes = 0, flags = 0;
	InterlockedIncrement(&NowSession->m_IOCount);
	int retval = WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_RecvOverlapped, NULL);


	// 4. 에러 처리
	if (retval == SOCKET_ERROR)
	{
		int Error = WSAGetLastError();

		// 비동기 입출력이 시작된게 아니라면
		if (Error != WSA_IO_PENDING)
		{
			// I/O카운트 1감소.
			long Nowval = InterlockedDecrement(&NowSession->m_IOCount);

			// I/O 카운트가 0이라면 접속 종료.
			if(Nowval == 0)
				Disconnect(NowSession);

			// I/O카운트가 마이너스가 된다면 마이너스 카운트 증가.
			if (Nowval < 0)
				InterlockedIncrement(&IOCountMinusCount);

			if (Error != WSAECONNRESET && Error != WSAESHUTDOWN)
			{
				SYSTEMTIME lst;
				GetLocalTime(&lst);
				_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] RecvPost_Accept(). 에러 발생. (%d)\n",
					lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, Error);
			}

			// 근데, 버퍼 부족이라면
			else if (Error == WSAENOBUFS)
			{
				// 화면에 버퍼부족 출력 
				SYSTEMTIME lst;
				GetLocalTime(&lst);
				_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] RecvPost_Accept(). 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n",
					lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, NowSession->m_IP, NowSession->m_prot);

				shutdown(NowSession->m_Client_sock, SD_BOTH);
			}

			return false;
		}
	}

	return true;
}

// RecvPost 함수. 비동기 입출력 시작
bool RecvPost(stSession* NowSession)
{
	// ------------------
	// 비동기 입출력 시작
	// ------------------
	// 1. WSABUF 셋팅.
	WSABUF wsabuf[2];
	int wsabufCount = 0;

	int FreeSize = NowSession->m_RecvBuff.GetFreeSize();
	int Size = NowSession->m_RecvBuff.GetNotBrokenPutSize();

	if (Size < FreeSize)
	{
		wsabuf[0].buf = NowSession->m_RecvBuff.GetRearBufferPtr();
		wsabuf[0].len = Size;

		wsabuf[1].buf = NowSession->m_RecvBuff.GetBufferPtr();
		wsabuf[1].len = FreeSize - Size;
		wsabufCount = 2;

	}
	else
	{
		wsabuf[0].buf = NowSession->m_RecvBuff.GetRearBufferPtr();
		wsabuf[0].len = Size;

		wsabufCount = 1;
	}

	// 2. Overlapped 구조체 초기화
	ZeroMemory(&NowSession->m_RecvOverlapped, sizeof(NowSession->m_RecvOverlapped));

	// 3. WSARecv()
	DWORD recvBytes = 0 , flags = 0;
	InterlockedIncrement(&NowSession->m_IOCount);
	int retval = WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_RecvOverlapped, NULL);
	
	// 4. 에러 처리
	if (retval == SOCKET_ERROR)
	{
		int Error = WSAGetLastError();

		// 비동기 입출력이 시작된게 아니라면
		if (Error != WSA_IO_PENDING)
		{
			// I/O카운트 1감소.
			int Nowval = InterlockedDecrement(&NowSession->m_IOCount);

			// I/O카운트가 마이너스가 된다면 마이너스 카운트 증가.
			if (Nowval < 0)
				InterlockedIncrement(&IOCountMinusCount);

			if (Error != WSAECONNRESET && Error != WSAESHUTDOWN)
			{
				SYSTEMTIME lst;
				GetLocalTime(&lst);
				_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] RecvPost(). 에러 발생. (%d)\n",
					lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, Error);
			}

			// 근데, 버퍼 부족이라면
			else if (Error == WSAENOBUFS)
			{
				// 화면에 버퍼부족 출력 
				SYSTEMTIME lst;
				GetLocalTime(&lst);
				_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] RecvPost(). 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n",
					lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, NowSession->m_IP, NowSession->m_prot);

				shutdown(NowSession->m_Client_sock, SD_BOTH);
			}

			return false;
		}
	}	

	return true;
}

// 패킷 처리. RecvProc()에서 받은 패킷 처리.
bool PacketProc(stSession* NowSession, CProtocolBuff* Payload)
{
	if (Recv_Packet_Echo(NowSession, Payload) == false)
		return false;	

	return true;	
}




// ------------
// 패킷 처리 함수들
// ------------
// 에코 패킷 처리 함수
bool Recv_Packet_Echo(stSession* NowSession, CProtocolBuff* Payload)
{
	// 1. 페이로드 사이즈 얻기
	int Size = Payload->GetUseSize();

	// 2. 그 사이즈만큼 데이터 memcpy
	char* Text = new char[Size];
	memcpy_s(Text, Size, Payload->GetBufferPtr(), Size);

	// 3. 에코 패킷 만들기. Buff안에 헤더까지 다 들어간다.	
	CProtocolBuff Buff;
	Send_Packet_Echo(&Buff, Size, Text, Size);

	delete[] Text;

	// 4. 에코 패킷을 SendBuff에 넣기
	if (SendPacket(NowSession, &Buff) == false)		
		return false;

	// 5. 데이터 샌드하기.
	if (SendPost(NowSession) == false)
		return false;

	return true;
}


// ------------
// 패킷 만들기 함수들
// ------------
// 에코패킷 만들기
void Send_Packet_Echo(CProtocolBuff* Buff, WORD PayloadSize, char* RetrunText, int RetrunTextSize)
{
	// 1. 헤더 제작
	// 지금 제작할게 없음. 인자로받은 PayloadSize가 곧 헤더임

	// 2. 헤더 넣기
	*Buff << PayloadSize;

	// 3. 페이로드 제작 후 넣기
	memcpy_s(Buff->GetBufferPtr() + Buff->GetUseSize(), Buff->GetFreeSize(), RetrunText, RetrunTextSize);
	Buff->MoveWritePos(RetrunTextSize);
}





// ------------
// 샌드 관련 함수들
// ------------
// 샌드 버퍼에 데이터 넣기
bool SendPacket(stSession* NowSession, CProtocolBuff* Buff)
{
	// 1. 넣을 사이즈 구하기.
	int Size = Buff->GetUseSize();

	// 2. 넣기
	int EnqueueCheck = NowSession->m_SendBuff.Enqueue(Buff->GetBufferPtr(), Size);
	if (EnqueueCheck == -1)
	{		
		SYSTEMTIME lst;
		GetLocalTime(&lst);
		_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] SendPacket(). Enqueue : Size Full\n",
			lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond);		

		return false;
	}	

	return true;
}

// 샌드 버퍼의 데이터 WSASend() 하기
bool SendPost(stSession* NowSession)
{
	while (1)
	{
		// ------------------
		// send 가능 상태인지 체크
		// ------------------
		// 1. SendFlag(1번인자)가 0(3번인자)과 같다면, SendFlag(1번인자)를 1(2번인자)으로 변경
		// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)이었다는 것.
		if (InterlockedCompareExchange(&NowSession->m_SendFlag, TRUE, FALSE) == TRUE)
			return true;


		// 2. SendBuff에 데이터가 있는지 확인
		// 여기서 구한 UseSize는 이제 스냅샷 이다. 아래에서 버퍼 셋팅할때도 사용한다.
		int UseSize = NowSession->m_SendBuff.GetUseSize();
		if (UseSize == 0)
		{
			// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
			NowSession->m_SendFlag = 0;

			// 3. 진짜로 사이즈가 없는지 다시한번 체크. 락 풀고 왔는데, 컨텍스트 스위칭 일어나서 다른 스레드가 건드렸을 가능성
			// 사이즈 있으면 위로 올라가서 한번 더 시도
			if (NowSession->m_SendBuff.GetUseSize() > 0)
				continue;

			break;
		}

		// ------------------
		// Send 준비하기
		// ------------------
		// 1. WSABUF 셋팅.
		WSABUF wsabuf[2];
		int wsabufCount = 0;

		// 2. BrokenSize 구하기
		// !!!!여기 주의!!! 
		// 만약 위에서 구한 BrokenSize를, UseSize보다 먼저 구해오면 흐름이 깨진다!!! 
		// 
		// ------------
		// 시나리오...
		// [Front = 0, rear = 10]인 상황에 BrokenSize를 구하면 10이 나온다. (BrokenSize = 10)
		// 그 사이에 컨텍스트 스위칭이 일어나서 다른 스레드가 데이터를 넣는다. (Fornt = 0, rear = 20)
		// 다시 컨텍스트 스위칭으로 해당 스레드로 돌아온 후 UseSize를 구하면 20이 나온다 (Front = 0, rear = 20, BrokenSize = 10, UseSize = 20)
		// ------------
		// 
		// 이 상황에서, BrokenSize와 UseSize를 비교하면, 끊긴 상황이 아닌데 버퍼를 2개 사용하게된다.
		// 이 경우, [0]번 버퍼에 &Buff[0]이 들어가고 사이즈가 10, [1]번 버퍼에도 &Buff[0]이 들어가고 사이즈가 10이 들어가는 상황이 발생한다.
		// 동일한 데이터가 가게 된다!! 
		// 이 경우, 상대쪽에서는 보내지 않은 데이터?라서 에러를 보낼수도 있고.. 혹은 그냥 뭐가 이상하게 처리되서 서버로 샌드를 안할수도 있고 그런다.
		// 주의하자!!!!!

		int BrokenSize = NowSession->m_SendBuff.GetNotBrokenGetSize();

		// 3. UseSize가 더 크다면, 버퍼가 끊겼다는 소리. 2개를 끊어서 보낸다.
		if (BrokenSize <  UseSize)
		{
			// fornt 위치의 버퍼를 포인터로 던짐(나는 1칸 앞에 쓰기 때문에, 이 안에서 1칸 앞까지 계산해줌)
			wsabuf[0].buf = NowSession->m_SendBuff.GetFrontBufferPtr();
			wsabuf[0].len = BrokenSize;

			wsabuf[1].buf = NowSession->m_SendBuff.GetBufferPtr();;
			wsabuf[1].len = UseSize - BrokenSize;
			wsabufCount = 2;
		}

		// 3-2. 그게 아니라면, WSABUF를 1개만 셋팅한다.
		else
		{
			// fornt 위치의 버퍼를 포인터로 던짐(나는 1칸 앞에 쓰기 때문에, 이 안에서 1칸 앞까지 계산해줌)
			wsabuf[0].buf = NowSession->m_SendBuff.GetFrontBufferPtr();
			wsabuf[0].len = BrokenSize;

			wsabufCount = 1;
		}

		// 4. Overlapped 구조체 초기화
		ZeroMemory(&NowSession->m_SendOverlapped, sizeof(NowSession->m_SendOverlapped));

		// 5. WSASend()
		DWORD SendBytes = 0, flags = 0;
		InterlockedIncrement(&NowSession->m_IOCount);
		int retval = WSASend(NowSession->m_Client_sock, wsabuf, wsabufCount, &SendBytes, flags, &NowSession->m_SendOverlapped, NULL);

		// 6. 에러 처리
		if (retval == SOCKET_ERROR)
		{
			int Error = WSAGetLastError();

			// 비동기 입출력이 시작된게 아니라면
			if (Error != WSA_IO_PENDING)
			{
				// I/O카운트 1 감소.

				int Nowval = InterlockedDecrement(&NowSession->m_IOCount);

				// I/O카운트가 마이너스라면 마이너스 카운트 증가
				if (Nowval < 0)
					InterlockedIncrement(&IOCountMinusCount);

				if (Error != WSAESHUTDOWN && Error != WSAECONNRESET)
				{
					SYSTEMTIME lst;
					GetLocalTime(&lst);
					_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] SendPost(). 에러 발생 (%d)\n",
						lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, Error);
				}

				// 버퍼 부족이라면
				else if (Error == WSAENOBUFS)
				{
					// 화면에 버퍼부족 출력 
					SYSTEMTIME lst;
					GetLocalTime(&lst);
					_LOG(dfLOG_LEVEL_ERROR, L"[%02d/%02d/%02d %02d:%02d:%02d] SendPost(). 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n",
						lst.wYear - 2000, lst.wMonth, lst.wDay, lst.wHour, lst.wMinute, lst.wSecond, NowSession->m_IP, NowSession->m_prot);
				}	

				return false;
			}
		}
		break;
	}

	return true;
}