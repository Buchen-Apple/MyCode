#include "stdafx.h"
#include "NetworkFunc.h"

CRingBuff RecvBUff(100);
CRingBuff SendBuff(100);


#define STRING_BUFF_SIZE	512

SOCKET sock;

// ture면 샌드 중, false면 샌드중 아님
bool SendFlag;

// 이번에 슬립해야하는지 체크.
// true면 슬립 해야함. false면 슬립 안해야함
//bool SleepFlag;

// ------------
// 리시브 관련 함수들
// ------------
// RecvPost함수. 실제 Recv 호출
int RecvPost()
{
	int a = 10;

	// 1. recv 링버퍼 포인터 얻어오기.
	char* recvbuff = RecvBUff.GetBufferPtr();

	// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = RecvBUff.GetNotBrokenPutSize();

	// 3. 한 번에 쓸 수 있는 공간이 0이라면
	if (Size == 0)
	{
		// 여유공간이 있을 경우, Size를 1로 변경
		if (RecvBUff.GetFreeSize() > 0)
			Size = 1;
	}

	// 4. 1칸 이동한 rear 위치 알기(실제 rear 위치가 이동하지는 않음)	
	int* rear = (int*)RecvBUff.GetWriteBufferPtr();
	int TempRear = RecvBUff.NextIndex(*rear, 1);

	// 5. recv()
	int retval = recv(sock, &recvbuff[TempRear], Size, 0);

	// 6. 리시브 에러체크
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
			_tprintf(L"RecvProc(). retval 후 우드블럭이 아닌 에러가 뜸(%d). 접속 종료\n", WSAGetLastError());		
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// Size로 0을 넣은게 아닌데, 0이 리턴되는 것은 정상종료.
	else if (retval == 0 && Size != 0)
		return false;

	// 7. 여기까지 왔으면 Rear를 이동시킨다.
	RecvBUff.MoveWritePos(retval);

	if (RecvProc() == false)
		return false;

	return retval;
}

// RecvProc 함수. 리시브 큐의 내용을 분석
bool RecvProc()
{
	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
		stNETWORK_PACKET_HEADE HeaderBuff;

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (RecvBUff.GetUseSize() < dfNETWORK_PACKET_HEADER_SIZE)
		{
			break;
		}

		// 2. 헤더를 Peek으로 확인한다.  Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 
		// 버퍼가 비어있으면 접속 끊음.
		int SIze = RecvBUff.Peek((char*)&HeaderBuff, dfNETWORK_PACKET_HEADER_SIZE);

		if (SIze == -1)
		{
			fputs("WorkerThread. Recv->Header Peek : Buff Empty\n", stdout);
			return false;
		}

		// 3. 완성된 패킷이 있는지 확인. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size + 완료코드)
		// 계산 결과, 완성 패킷 사이즈가 안되면 다음에 확인.
		int UseSize = RecvBUff.GetUseSize();
		if (UseSize < (dfNETWORK_PACKET_HEADER_SIZE + HeaderBuff.bySize + 1))
		{
			//SleepFlag = false;
			return true;
		}

		// 4. 헤더 코드 확인
		if (HeaderBuff.byCode != dfNETWORK_PACKET_CODE)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 PacketCode틀림. 접속 종료\n"));
			return false;
		}

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		RecvBUff.RemoveData(dfNETWORK_PACKET_HEADER_SIZE);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.bySize;

		int DequeueSize = RecvBUff.Dequeue(PayloadBuff.GetBufferPtr(), PayloadSize);

		// 버퍼가 비어있으면 접속 끊음
		if (DequeueSize == -1)
		{
			fputs("WorkerThread. Recv->Payload Dequeue : Buff Empty\n", stdout);
			return false;
		}
		PayloadBuff.MoveWritePos(DequeueSize);

		// 7. RecvBuff에서 엔드코드 1Byte뽑음.	(디큐이다. Peek 아님)
		BYTE EndCode;
		DequeueSize = RecvBUff.Dequeue((char*)&EndCode, 1);
		if (DequeueSize == -1)
		{
			fputs("WorkerThread. Recv->EndCode Dequeue : Buff Empty\n", stdout);
			return false;
		}

		// 8. 엔드코드 확인
		if (EndCode != dfNETWORK_PACKET_END)
		{
			_tprintf(_T("RecvProc(). 완료패킷 엔드코드 처리 중 정상패킷 아님. 접속 종료\n"));			
			return false;
		}

		// 9. 헤더에 들어있는 타입에 따라 분기처리. false가 리턴되면 접속 끊음.
		if (PacketProc(HeaderBuff.byType, &PayloadBuff) == false)
		{
			return false;
		}

		Sleep(300);

	}

	return true;
}

// 패킷 처리. RecvProc()에서 받은 패킷 처리.
bool PacketProc(BYTE PacketType, CProtocolBuff* Payload)
{

	switch (PacketType)
	{
		// 문자열 에코 패킷
	case dfPACKET_SC_ECHO:
	{
		if (Recv_Packet_Echo(Payload) == false)
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
// 패킷 만들기
// ------------
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

// 에코 패킷 만들기
bool Network_Send_Echo(CProtocolBuff *header, CProtocolBuff* payload)
{
	char buf[STRING_BUFF_SIZE + 1];

	int test = (rand() % 20) + 3;

	for (int i = 0; i < test - 2; ++i)
		buf[i] = 'a';

	buf[test - 2] = 'b';
	buf[test - 1] = '\0';

	int len = (int)strlen(buf);

	if (len == 0)
	{
		printf("(strlen(buf) : %d, test : %d\n", len, test);
		return false;
	}

	printf("%s\n", buf);
	
	// 페이로드에 넣기
	for(int i=0; i<len; ++i)
		*payload << buf[i];

	// 헤더 만들기
	CreateHeader(header, payload->GetUseSize(), dfPACKET_CS_ECHO);

	return true;
}


// ------------
// 패킷 처리 함수들
// ------------
// 에코 패킷 처리 함수
bool Recv_Packet_Echo(CProtocolBuff* Payload)
{
	// 1. 페이로드 사용중인 용량 얻기
	int Size = Payload->GetUseSize();

	// 2. 그 사이즈만큼 데이터 memcpy
	char* Text = new char[Size + 1];
	memcpy_s(Text, Size + 1, Payload->GetBufferPtr(), Size);

	// 3. 카피 한 만큼 payload의 front이동
	Payload->MoveWritePos(Size);

	// 4. 문자열 출력을 위해 마지막 위치에 NULL 넣기
	Text[Size] = '\0';

	// 5. 문자열 출력
	printf("[받은 데이터] : %s\n", Text);	

	_tprintf(L"[TCP 클라] %d 바이트 받음.\n\n", Size);

	delete Text;

	// 6. 보낸거 다 받았으니, 다시 보내기 위해, 보냄 상태로 만든다.
	SendFlag = false;
	//SleepFlag = true;


	return true;
}




// ------------
// 샌드 관련 함수들
// ------------
// 샌드 버퍼에 데이터 넣기
bool SendPacket(CProtocolBuff* header, CProtocolBuff* payload)
{	

	// 1. 샌드 q에 헤더 넣기
	int Size = dfNETWORK_PACKET_HEADER_SIZE;
	int EnqueueCheck = SendBuff.Enqueue(header->GetBufferPtr(), Size);
	if (EnqueueCheck == -1)
	{
		fputs("SendPacket(). Header->Enqueue. Size Full\n", stdout);
		return false;
	}

	// 3. 샌드 q에 페이로드 넣기
	int PayloadLen = payload->GetUseSize();
	EnqueueCheck = SendBuff.Enqueue(payload->GetBufferPtr(), PayloadLen);
	if (EnqueueCheck == -1)
	{
		fputs("SendPacket(). Payload->Enqueue. Size Full\n", stdout);
		return false;
	}

	// 3. 샌드 q에 엔드코드 넣기
	char EndCode = dfNETWORK_PACKET_END;
	EnqueueCheck = SendBuff.Enqueue(&EndCode, 1);
	if (EnqueueCheck == -1)
	{
		fputs("SendPacket(). EndCode->Enqueue. Size Full\n", stdout);
		return false;
	}	

	return true;

}

// 샌드 버퍼의 데이터 Send() 하기
int SendPost()
{
	// 1. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = SendBuff.GetBufferPtr();

	int TotalSize = 0;

	// 2. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 3. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (SendBuff.GetUseSize() == 0)
			break;

		// 4. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = SendBuff.GetNotBrokenGetSize();

		// 5. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 6. front 포인터 얻어옴
		int *front = (int*)SendBuff.GetReadBufferPtr();

		// 7. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = SendBuff.NextIndex(*front, 1);

		// 8. Send()
		int SendSize = send(sock, &sendbuff[BuffArrayCheck], Size, 0);

		// 9. 에러 체크. 우드블럭이면 더 이상 못보내니 while문 종료. 다음 Select때 다시 보냄
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

		// 10. 보낸 사이즈가 나왔으면, 그 만큼 remove
		SendBuff.RemoveData(SendSize);

		TotalSize += SendSize;
	}

	_tprintf(L"[TCP 클라] %d 바이트 보냄.\n", TotalSize - 5);

	if (TotalSize != 0)
		SendFlag = true;

	return TotalSize;
}