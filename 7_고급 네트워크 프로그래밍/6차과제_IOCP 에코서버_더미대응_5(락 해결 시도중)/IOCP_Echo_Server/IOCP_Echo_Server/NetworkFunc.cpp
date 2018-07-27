#include "stdafx.h"
#include "NetworkFunc.h"


SRWLOCK g_Session_map_srwl;
map<SOCKET, stSession*> map_Session;

extern long IOCountMinusCount;
extern long JoinUser;
extern long SessionDeleteCount;
long TempCount = 0;


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
		//int UseSize = NowSession->m_RecvBuff.GetUseSize();
		if (NowSession->m_RecvBuff.GetUseSize() < dfNETWORK_PACKET_HEADER_SIZE)
		{
			break;
		}

		// 2. 헤더를 Peek으로 확인한다.  Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 
		// 버퍼가 비어있으면 접속 끊음.
		int PeekSize = NowSession->m_RecvBuff.Peek((char*)&Header_PaylaodSize, dfNETWORK_PACKET_HEADER_SIZE);
		if (PeekSize == -1)
		{
			fputs("WorkerThread. Recv->Header Peek : Buff Empty\n", stdout);
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			return false;
		}

		// 3. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size)
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (NowSession->m_RecvBuff.GetUseSize() < (dfNETWORK_PACKET_HEADER_SIZE + Header_PaylaodSize))
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
			fputs("WorkerThread. Recv->Payload Dequeue : Buff Empty\n", stdout);
			shutdown(NowSession->m_Client_sock, SD_BOTH);

			return false;
		}
		PayloadBuff.MoveWritePos(DequeueSize);

		// 9. 헤더에 들어있는 타입에 따라 분기처리. false가 리턴되면 접속 끊음.
		if (PacketProc(NowSession, &PayloadBuff) == false)
		{
			fputs("PacketProc. return false\n", stdout);			
			return false;
		}
	}

	return true;
}

// Accept용 RecvProc함수
bool RecvPost_Accept(stSession* NowSession)
{
	// ------------------
	// 비동기 입출력 시작
	// ------------------
	// 1. WSABUF 선언 후 Recv링버퍼 락걸기
	WSABUF wsabuf[2];

	// 2. recv 링버퍼 포인터 얻어오기.
	char* recvBuff = NowSession->m_RecvBuff.GetBufferPtr();

	// 3. WSABUF 셋팅.
	int wsabufCount = 0;

	int FreeSize = NowSession->m_RecvBuff.GetFreeSize();
	int Size = NowSession->m_RecvBuff.GetNotBrokenPutSize();

	if (Size < FreeSize)
	{
		wsabuf[0].buf = &recvBuff[NowSession->m_RecvBuff.GetWriteBufferPtr_Next()];
		wsabuf[0].len = Size;

		wsabuf[1].buf = &recvBuff[0];
		wsabuf[1].len = FreeSize - Size;
		wsabufCount = 2;

	}

	// 4. 그게 아니라면, WSABUF를 1개만 셋팅한다.
	else
	{
		wsabuf[0].buf = &recvBuff[NowSession->m_RecvBuff.GetWriteBufferPtr_Next()];
		wsabuf[0].len = Size;

		wsabufCount = 1;
	}

	// 5. Overlapped 구조체 초기화
	ZeroMemory(&NowSession->m_RecvOverlapped, sizeof(NowSession->m_RecvOverlapped));

	// 6. WSARecv()
	DWORD recvBytes = 0, flags = 0;
	InterlockedIncrement(&NowSession->m_IOCount);
	int retval = WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_RecvOverlapped, NULL);


	// 7. 에러 처리
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
				printf("Recv return false1111 %d\n", Error);

			// 근데, 버퍼 부족이라면
			else if (Error == WSAENOBUFS)
			{
				// 화면에 버퍼부족 출력 
				_tprintf(L"[TCP 서버] 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", NowSession->m_IP, NowSession->m_prot);
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
	// 1. WSABUF 선언 후 Recv링버퍼 락걸기
	WSABUF wsabuf[2];

	// 2. recv 링버퍼 포인터 얻어오기.
	char* recvBuff = NowSession->m_RecvBuff.GetBufferPtr();		

	// 3. WSABUF 셋팅.
	int wsabufCount = 0;	

	int FreeSize = NowSession->m_RecvBuff.GetFreeSize();
	int Size = NowSession->m_RecvBuff.GetNotBrokenPutSize();

	if (Size < FreeSize)
	{
		wsabuf[0].buf = &recvBuff[NowSession->m_RecvBuff.GetWriteBufferPtr_Next()];
		wsabuf[0].len = Size;

		wsabuf[1].buf = &recvBuff[0];
		wsabuf[1].len = FreeSize - Size;
		wsabufCount = 2;

	}

	// 4. 그게 아니라면, WSABUF를 1개만 셋팅한다.
	else
	{
		wsabuf[0].buf = &recvBuff[NowSession->m_RecvBuff.GetWriteBufferPtr_Next()];
		wsabuf[0].len = Size;

		wsabufCount = 1;
	}

	// 5. Overlapped 구조체 초기화
	ZeroMemory(&NowSession->m_RecvOverlapped, sizeof(NowSession->m_RecvOverlapped));

	// 6. WSARecv()
	DWORD recvBytes = 0 , flags = 0;
	InterlockedIncrement(&NowSession->m_IOCount);
	int retval = WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_RecvOverlapped, NULL);	++NowSession->m_IOR;
	
	// 7. 에러 처리
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
				printf("Recv return false1111 %d\n", Error);		

			// 근데, 버퍼 부족이라면
			else if (Error == WSAENOBUFS)
			{
				// 화면에 버퍼부족 출력 
				_tprintf(L"[TCP 서버] 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", NowSession->m_IP, NowSession->m_prot);
			}

			//printf("Recv return false2222, %d\n", Error);

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
	// 1. 페이로드 사용중인 용량 얻기
	int Size = Payload->GetUseSize();

	// 2. 그 사이즈만큼 데이터 memcpy
	char* Text = new char[Size];
	memcpy_s(Text, Size, Payload->GetBufferPtr(), Size);

	// 3. 카피 한 만큼 payload의 front이동
	Payload->MoveReadPos(Size);

	// 4. 에코 패킷 만들기
	/*CProtocolBuff Header;
	CProtocolBuff Payload_2;
	Send_Packet_Echo(&Header, &Payload_2, Text, Size);*/
	CProtocolBuff Buff;
	Send_Packet_Echo2(&Buff, Size, Text, Size);

	delete[] Text;

	// 5. 에코 패킷을 SendBuff에 넣기
	//if (SendPacket(NowSession, &Header, &Payload_2) == false)
	//	return false;	

	if (SendPacket2(NowSession, &Buff) == false)
		return false;

	// 6. 데이터 샌드하기.
	if (SendPost(NowSession, true) == false)
		return false;

	return true;
}



// ------------
// 패킷 만들기 함수들
// ------------
// 헤더 만들기

//void Send_Packet_Header(WORD PayloadSize, CProtocolBuff* Header)
//{
//	*Header << PayloadSize;
//}
//
//// 에코패킷 만들기
//void Send_Packet_Echo(CProtocolBuff* header, CProtocolBuff* payload, char* RetrunText, int RetrunTextSize)
//{
//	// 1. 페이로드 제작
//	// 에코패킷은, 인자로 받은 retrunText를 payload에 넣는 역활.
//	memcpy_s(payload->GetBufferPtr(), payload->GetFreeSize(), RetrunText, RetrunTextSize);
//	payload->MoveWritePos(RetrunTextSize);
//
//	// 2. 헤더 제작
//	Send_Packet_Header(payload->GetUseSize(), header);
//}

// 에코패킷 만들기2
void Send_Packet_Echo2(CProtocolBuff* Buff, WORD PayloadSize, char* RetrunText, int RetrunTextSize)
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
//
//bool SendPacket(stSession* NowSession, CProtocolBuff* header, CProtocolBuff* payload)
//{
//	// 1. 큐 락
//	//NowSession->m_SendBuff.EnterLOCK();		// 락 ---------------------------	
//
//	// 2. 샌드 q에 헤더 넣기
//	int Size = dfNETWORK_PACKET_HEADER_SIZE;
//	int EnqueueCheck = NowSession->m_SendBuff.Enqueue(header->GetBufferPtr(), Size);	
//	if (EnqueueCheck == -1)
//	{		
//		fputs("SendPacket(). Header->Enqueue. Size Full\n", stdout);
//
//		//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------
//		return false;
//	}
//
//	// 3. 샌드 q에 페이로드 넣기
//	int PayloadLen = payload->GetUseSize();
//	EnqueueCheck = NowSession->m_SendBuff.Enqueue(payload->GetBufferPtr(), PayloadLen);	
//	if (EnqueueCheck == -1)
//	{		
//		fputs("SendPacket(). Payload->Enqueue. Size Full\n", stdout);
//
//		//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------
//		return false;
//	}
//	
//	//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------
//
//	return true;
//}

// 샌드 버퍼에 데이터 넣기2
bool SendPacket2(stSession* NowSession, CProtocolBuff* Buff)
{
	// 1. 큐 락
	

	// 데이터 넣기
	int Size = Buff->GetUseSize();

	//NowSession->m_SendBuff.EnterLOCK();		// 락 ---------------------------	
	int EnqueueCheck = NowSession->m_SendBuff.Enqueue(Buff->GetBufferPtr(), Size);
	if (EnqueueCheck == -1)
	{
		fputs("SendPacket(). Enqueue. Size Full\n", stdout);

		//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------
		return false;
	}	

	//NowSession->m_SendBuff.LeaveLOCK();	 // 락해제 ---------------------------

	return true;
}

// 샌드 버퍼의 데이터 WSASend() 하기
bool SendPost(stSession* NowSession, bool _bR)
{
	while (1)
	{
		// ------------------
		// send 가능 상태인지 체크
		// ------------------
		// SendFlag(1번인자)가 0(3번인자)과 같다면, SendFlag(1번인자)를 1(2번인자)으로 변경
		// 여기서 TRUE가 리턴되는 것은, 이미 NowSession->m_SendFlag가 1(샌드 중)이었다는 것.
		if (InterlockedCompareExchange(&NowSession->m_SendFlag, TRUE, FALSE) == TRUE)
		{
			//NowSession->m_SendBuff.LeaveLOCK();
			return true;
		}

		// ------------------
		// 가능 상태라면 send로직 탄다.
		// ------------------
		//NowSession->m_SendBuff.EnterLOCK();     // 락 ----------------------------------		

		// 1. SendBuff에 데이터가 있는지 확인
		if (NowSession->m_SendBuff.GetUseSize() == 0)
		{
			// WSASend 안걸었기 때문에, 샌드 가능 상태로 다시 돌림.
			// SendFlag(1번인자)가 1(3번인자)과 같다면, SendFlag(1번인자)를 0(2번인자)으로 변경
			//NowSession->m_SendFlag = 0;
			//InterlockedExchange(&NowSession->m_SendFlag, FALSE);
			InterlockedCompareExchange(&NowSession->m_SendFlag, FALSE, TRUE);


			// 여기서 사이즈 한번 더 체크한다. 그래서 사이즈 있으면 goto문으로 위로 올라가서 한번 더 한다.
			if (NowSession->m_SendBuff.GetUseSize() > 0)
			{
				//NowSession->m_SendBuff.LeaveLOCK();
				continue;
			}

			//NowSession->m_SendBuff.LeaveLOCK();		// 락 해제 ----------------------------------
			break;
		}

		// 2. 샌드 링버퍼의 실질적인 버퍼 가져옴
		char* sendbuff = NowSession->m_SendBuff.GetBufferPtr();
		WSABUF wsabuf[2];
		int wsabufCount = 0;

		// 3. WSABUF를 2개를 셋팅한다.	

		//NowSession->m_SendBuff.EnterLOCK();//-----------------

		int UseSize = NowSession->m_SendBuff.GetUseSize();
		int BrokenSize = NowSession->m_SendBuff.GetNotBrokenGetSize();

		//NowSession->m_SendBuff.LeaveLOCK();//-----------------

		int iValue;
		int iIdx = 0;
		if (BrokenSize <  UseSize)
		{
			iIdx = NowSession->m_SendBuff.GetReadBufferPtr_Next();
			wsabuf[0].buf = &sendbuff[iIdx];
			wsabuf[0].len = BrokenSize;

			iValue = UseSize - BrokenSize;
			wsabuf[1].buf = &sendbuff[0];
			wsabuf[1].len = iValue;// UseSize - Size;
			wsabufCount = 2;
		}

		// 4-2. 그게 아니라면, WSABUF를 1개만 셋팅한다.
		else
		{
			iIdx = NowSession->m_SendBuff.GetReadBufferPtr_Next();
			wsabuf[0].buf = &sendbuff[iIdx];
			wsabuf[0].len = BrokenSize;

			wsabufCount = 1;
		}

		// 5. Overlapped 구조체 초기화
		ZeroMemory(&NowSession->m_SendOverlapped, sizeof(NowSession->m_SendOverlapped));

		// 7. WSASend()
		DWORD SendBytes = 0, flags = 0;
		InterlockedIncrement(&NowSession->m_IOCount);
		//wsabuf[0].len = -10;
		int retval = WSASend(NowSession->m_Client_sock, wsabuf, wsabufCount, &SendBytes, flags, &NowSession->m_SendOverlapped, NULL);

		//NowSession->m_SendBuff.LeaveLOCK();		// 락 해제 ---------------------------	

		// 8. 에러 처리
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
					printf("Send %d\n", Error);

				// 버퍼 부족이라면
				else if (Error == WSAENOBUFS)
				{
					// 화면에 버퍼부족 출력 
					_tprintf(L"[TCP 서버] WSASend() 중 버퍼 사이즈 부족 : IP 주소=%s, 포트=%d\n", NowSession->m_IP, NowSession->m_prot);
				}

				//printf("Send return false %d\n", Error);	

				return false;
			}
		}
		break;
	}

	return true;
}