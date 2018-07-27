#include "stdafx.h"
#include "NetworkFunc.h"

SRWLOCK g_Session_map_srwl;
map<SOCKET, stSession*> map_Session;



// 접속 종료
void Disconnect(stSession* DeleteSession)
{
	// 이 함수가 호출되었을 때, 세션의 I/O Count가 0이라면 삭제한다.
	if (DeleteSession->m_IOCount > 0)
		return;

	//_tprintf(L"[TCP 서버] 접속 종료 : IP 주소=%s, 포트=%d\n", DeleteSession->m_IP, DeleteSession->m_prot);

	SOCKET TempSock = DeleteSession->m_Client_sock;

	// 클로즈 소켓, 세션 동적해제	
	closesocket(DeleteSession->m_Client_sock);
	delete DeleteSession;

	// map에서 제외시키기
	LockSession();
	map_Session.erase(TempSock);
	UnlockSession();
}



// ------------
// 리시브 관련 함수들
// ------------
// RecvProc 함수. 큐의 내용 체크 후 PacketProc으로 넘긴다.
void RecvProc(stSession* NowSession, DWORD cbTransferred)
{
	if (NowSession == nullptr)
		return;

	// -----------------
	// Recv 큐 관련 처리
	// -----------------
	// 1. 큐 락
	//NowSession->m_RecvBuff.EnterLOCK();		// 락 ----------------------

	// 2. 받은 바이트만큼 rear 이동
	NowSession->m_RecvBuff.MoveWritePos(cbTransferred);

	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
		stNETWORK_PACKET_HEADE HeaderBuff;

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (NowSession->m_RecvBuff.GetUseSize() < dfNETWORK_PACKET_HEADER_SIZE)
		{
			break;
		}

		// 2. 헤더를 Peek으로 확인한다.  Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 
		// 버퍼가 비어있으면 접속 끊음.
		if (NowSession->m_RecvBuff.Peek((char*)&HeaderBuff, dfNETWORK_PACKET_HEADER_SIZE) == -1)
		{
			fputs("WorkerThread. Recv->Header Peek : Buff Empty\n", stdout);
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			break;
		}

		// 3. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size + 완료코드)
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (NowSession->m_RecvBuff.GetUseSize() < (dfNETWORK_PACKET_HEADER_SIZE + HeaderBuff.bySize + 1))
		{
			break;
		}

		// 4. 헤더 코드 확인
		if (HeaderBuff.byCode != dfNETWORK_PACKET_CODE)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 PacketCode틀림. 접속 종료\n"));
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			break;
		}

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		NowSession->m_RecvBuff.RemoveData(dfNETWORK_PACKET_HEADER_SIZE);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.bySize;

		int DequeueSize = NowSession->m_RecvBuff.Dequeue(PayloadBuff.GetBufferPtr(), PayloadSize);

		// 버퍼가 비어있으면 접속 끊음
		if (DequeueSize == -1)
		{
			fputs("WorkerThread. Recv->Payload Dequeue : Buff Empty\n", stdout);
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			break;
		}
		PayloadBuff.MoveWritePos(DequeueSize);

		// 7. RecvBuff에서 엔드코드 1Byte뽑음.	(디큐이다. Peek 아님)
		BYTE EndCode;
		DequeueSize = NowSession->m_RecvBuff.Dequeue((char*)&EndCode, 1);
		if (DequeueSize == -1)
		{
			fputs("WorkerThread. Recv->EndCode Dequeue : Buff Empty\n", stdout);
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			break;
		}

		// 8. 엔드코드 확인
		if (EndCode != dfNETWORK_PACKET_END)
		{
			_tprintf(_T("RecvProc(). 완료패킷 엔드코드 처리 중 정상패킷 아님. 접속 종료\n"));
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			break;
		}


		// 9. 헤더에 들어있는 타입에 따라 분기처리. false가 리턴되면 접속 끊음.
		if (PacketProc(HeaderBuff.byType, NowSession, &PayloadBuff) == false)
		{
			shutdown(NowSession->m_Client_sock, SD_BOTH);
			break;
		}

	}

	// 3. 락 해제
	//NowSession->m_RecvBuff.LeaveLOCK();		// 락 해제 ----------------------

}

// RecvPost 함수. 비동기 입출력 시작
bool RecvPost(stSession* NowSession)
{
	// ------------------
	// 비동기 입출력 시작
	// ------------------
	// 1. WSABUF 선언 후 Recv링버퍼 락걸기
	WSABUF wsabuf[2];
	//NowSession->m_RecvBuff.EnterLOCK();  // 락 ---------------------------

	 // 2. recv 링버퍼 포인터 얻어오기.
	char* recvBuff = NowSession->m_RecvBuff.GetBufferPtr();

	// 3. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = NowSession->m_RecvBuff.GetNotBrokenPutSize();

	// 4. 1칸 이동한 rear 위치 알기(실제 rear 위치가 이동하지는 않음)	
	int* rear = (int*)NowSession->m_RecvBuff.GetWriteBufferPtr();
	int TempRear = NowSession->m_RecvBuff.NextIndex(*rear, 1);

	// 5-1. 만약 Size가 free사이즈보다 작다면, WSABUF를 2개를 셋팅한다.
	int wsabufCount = 0;
	int FreeSize = NowSession->m_RecvBuff.GetFreeSize();
	if (Size < FreeSize)
	{
		wsabuf[0].buf = &recvBuff[TempRear];
		wsabuf[0].len = Size;

		wsabuf[1].buf = &recvBuff[0];
		wsabuf[1].len = FreeSize - Size;
		wsabufCount = 2;
	}

	// 5-2. 그게 아니라면, WSABUF를 1개만 셋팅한다.
	else
	{
		wsabuf[0].buf = &recvBuff[TempRear];
		wsabuf[0].len = Size;

		wsabufCount = 1;
	}

	// 6. Overlapped 구조체 초기화
	ZeroMemory(&NowSession->m_RecvOverlapped, sizeof(NowSession->m_RecvOverlapped));

	// 7. WSARecv()
	DWORD recvBytes, flags = 0;
	InterlockedIncrement64((LONG64*)&NowSession->m_IOCount);
	int retval = WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_RecvOverlapped, NULL);

	//NowSession->m_RecvBuff.LeaveLOCK();   // 락 해제 ---------------------------

	 // 8. 리턴값이 0이고, 바이트도 0이면 정상종료.
	if (retval == 0 && recvBytes == 0)
	{
		// I/O카운트가 0이라면, 접속 종료
		int a = InterlockedDecrement64((LONG64*)&NowSession->m_IOCount);
		if (a == 0)
		{
			Disconnect(NowSession);
		}
	}

	// 9. 에러 처리
	if (retval == SOCKET_ERROR)
	{
		int Error = WSAGetLastError();

		// 비동기 입출력이 시작된게 아니라면
		if (Error != WSA_IO_PENDING)
		{
			// I/O카운트가 0이라면, 접속 종료
			int a = InterlockedDecrement64((LONG64*)&NowSession->m_IOCount);
			if (a == 0)
			{
				Disconnect(NowSession);
			}

			if(Error != WSAECONNRESET && Error != WSAESHUTDOWN)
				printf("Recv %d\n", Error);

			// 근데, 버퍼 부족이라면
			if (Error == WSAENOBUFS)
			{
				// 화면에 버퍼부족 출력 
				_tprintf(L"[TCP 서버] 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", NowSession->m_IP, NowSession->m_prot);
			}			

			return false;
		}
	}

	return true;
}

// 패킷 처리. RecvProc()에서 받은 패킷 처리.
bool PacketProc(BYTE PacketType, stSession* NowSession, CProtocolBuff* Payload)
{

	switch (PacketType)
	{
		// 문자열 에코 패킷
	case dfPACKET_CS_ECHO:
	{
		if (Recv_Packet_Echo(NowSession, Payload) == false)
			return false;
	}
	break;


	default:
		printf("알 수 없는 패킷!\n");
		break;
	}

	return true;	
}




// ------------
// 패킷 처리 함수들
// ------------
// 에코 패킷 처리 함수
bool Recv_Packet_Echo(stSession* NowSession, CProtocolBuff* Payload)
{
	// 1. 페이로드 사용중인 용량 얻기
	int Size = Payload->GetUseSize();

	// 2. 그 사이즈만큼 데이터 memcpy
	char* Text = new char[Size + 1];
	memcpy_s(Text, Size + 1, Payload->GetBufferPtr(), Size);

	// 3. 카피 한 만큼 payload의 front이동
	Payload->MoveReadPos(Size);

	// 4. 문자열 출력을 위해 마지막 위치에 NULL 넣기
	Text[Size] = '\0';

	// 5. 문자열 출력
	LockSession();
	_tprintf(L"[%s:%d] 받은 데이터(User : %zd) : ", NowSession->m_IP, NowSession->m_prot, map_Session.size());
	printf("%s\n", Text);
	UnlockSession();

	// 6. 받은 데이터 다시 샌드하기.
	// 에코 패킷 만들기
	CProtocolBuff Header;
	CProtocolBuff Payload_2;
	Send_Packet_Echo(&Header, &Payload_2, Text, Size);

	// 에코 패킷을 SendBuff에 넣기
	if (SendPacket(NowSession, &Header, &Payload_2) == false)
		return false;

	// 데이터 샌드하기. Send중이 아닐 경우에만...
	SendPost(NowSession);

	delete Text;
	return true;
}



// ------------
// 패킷 만들기 함수들
// ------------
// 헤더 만들기
void Send_Packet_Header(BYTE Type, BYTE PayloadSize, CProtocolBuff* Header)
{
	// 패킷 코드 (1Byte)
	*Header << dfNETWORK_PACKET_CODE;

	// 페이로드 사이즈 (1Byte)
	*Header << PayloadSize;

	// 패킷 타입 (1Byte)
	*Header << Type;

	// 사용안함 (1Byte)
	BYTE temp = 12;
	*Header << temp;

}

// 에코패킷 만들기
void Send_Packet_Echo(CProtocolBuff* header, CProtocolBuff* payload, char* RetrunText, int RetrunTextSize)
{
	// 1. 페이로드 제작
	// 에코패킷은, 인자로 받은 retrunText를 payload에 넣는 역활.
	memcpy_s(payload->GetBufferPtr(), payload->GetFreeSize(), RetrunText, RetrunTextSize);
	payload->MoveWritePos(RetrunTextSize);

	// 2. 헤더 제작
	Send_Packet_Header(dfPACKET_SC_ECHO, payload->GetUseSize(), header);
}





// ------------
// 샌드 관련 함수들
// ------------
// 샌드 버퍼에 데이터 넣기
bool SendPacket(stSession* NowSession, CProtocolBuff* header, CProtocolBuff* payload)
{
	// 1. 큐 락
	//NowSession->m_SendBuff.EnterLOCK();		// 락 ---------------------------

	// 2. 샌드 q에 헤더 넣기
	int Size = dfNETWORK_PACKET_HEADER_SIZE;
	int EnqueueCheck = NowSession->m_SendBuff.Enqueue(header->GetBufferPtr(), Size);
	if (EnqueueCheck == -1)
	{		
		fputs("SendPacket(). Header->Enqueue. Size Full\n", stdout);

		//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------
		return false;
	}

	// 3. 샌드 q에 페이로드 넣기
	int PayloadLen = payload->GetUseSize();
	EnqueueCheck = NowSession->m_SendBuff.Enqueue(payload->GetBufferPtr(), PayloadLen);
	if (EnqueueCheck == -1)
	{		
		fputs("SendPacket(). Payload->Enqueue. Size Full\n", stdout);

		//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------
		return false;
	}

	// 3. 샌드 q에 엔드코드 넣기
	char EndCode = dfNETWORK_PACKET_END;
	EnqueueCheck = NowSession->m_SendBuff.Enqueue(&EndCode, 1);
	if (EnqueueCheck == -1)
	{
		fputs("SendPacket(). EndCode->Enqueue. Size Full\n", stdout);

		//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------
		return false;
	}
	
	//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------

	return true;

}

// 샌드 버퍼의 데이터 WSASend() 하기
void SendPost(stSession* NowSession)
{
	// ------------------
	// send 가능 상태인지 체크
	// ------------------
	// SendFlag(1번인자)가 0(3번인자)과 같다면, SendFlag(1번인자)를 1(2번인자)으로 변경
	DWORD Dest = (DWORD)InterlockedCompareExchange64((LONG64*)&NowSession->m_SendFlag, TRUE, FALSE);

	// 값이 변경되지 않았다면, (즉, 여기서는 NowSession->m_SendFlag가 이미 1(샌드 중)이었다는 것) 리턴
	if (Dest  == NowSession->m_SendFlag)
		return;

	// ------------------
	// 가능 상태라면 send로직 탄다.
	// ------------------
	WSABUF wsabuf[2];

	// 1. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = NowSession->m_SendBuff.GetBufferPtr();

	// 2. SendBuff에 데이터가 있는지 확인
	if (NowSession->m_SendBuff.GetUseSize() == 0)
	{
		// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
		// SendFlag(1번인자)가 1(3번인자)과 같다면, SendFlag(1번인자)를 0(2번인자)으로 변경
		InterlockedCompareExchange64((LONG64*)&NowSession->m_SendFlag, FALSE, TRUE);
		return;
	}

	// 3 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
	int Size = NowSession->m_SendBuff.GetNotBrokenGetSize();

	// 4. 1칸 이동한 front 위치 알기(실제 front 위치가 이동하지는 않음)	
	int *front = (int*)NowSession->m_SendBuff.GetReadBufferPtr();
	int TempFront = NowSession->m_SendBuff.NextIndex(*front, 1);

	// 5-1. 만약 Size가 Use 사이즈보다 작다면, WSABUF를 2개를 셋팅한다.
	int wsabufCount = 0;
	int UseSize = NowSession->m_SendBuff.GetUseSize();
	if (Size < UseSize)
	{
		wsabuf[0].buf = &sendbuff[TempFront];
		wsabuf[0].len = Size;

		wsabuf[1].buf = &sendbuff[0];
		wsabuf[1].len = UseSize - Size;
		wsabufCount = 2;
	}

	// 5-2. 그게 아니라면, WSABUF를 1개만 셋팅한다.
	else
	{
		wsabuf[0].buf = &sendbuff[TempFront];
		wsabuf[0].len = Size;

		wsabufCount = 1;
	}

	// 6. Overlapped 구조체 초기화
	ZeroMemory(&NowSession->m_SendOverlapped, sizeof(NowSession->m_SendOverlapped));

	// 7. WSASend()
	DWORD SendBytes, flags = 0;
	InterlockedIncrement64((LONG64*)&NowSession->m_IOCount);
	int retval = WSASend(NowSession->m_Client_sock, wsabuf, wsabufCount, &SendBytes, flags, &NowSession->m_SendOverlapped, NULL);

	// 8. 에러 처리
	if (retval == SOCKET_ERROR)
	{
		int Error = WSAGetLastError();

		// 비동기 입출력이 시작된게 아니라면
		if (Error != WSA_IO_PENDING)
		{
			// I/O카운트 1 감소.
			// I/O카운트가 0이라면, 접속 종료
			int a = InterlockedDecrement64((LONG64*)&NowSession->m_IOCount);
			if (a == 0)
			{
				Disconnect(NowSession);
			}

			if(Error != WSAESHUTDOWN && Error != WSAECONNRESET)
				printf("Send %d\n", Error);

			// 근데, 버퍼 부족이라면
			if (Error == WSAENOBUFS)
			{
				// 화면에 버퍼부족 출력 
				_tprintf(L"[TCP 서버] WSASend() 중 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", NowSession->m_IP, NowSession->m_prot);
			}

			
		}
	}	
}