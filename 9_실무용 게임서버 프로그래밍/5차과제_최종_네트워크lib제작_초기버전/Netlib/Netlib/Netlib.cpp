// Netlib.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "NetworkLib.h"

#define PORT	6000

class MyLanServer : public CLanServer
{
private:
	// ------------
	// 패킷 처리 함수들
	// ------------
	// 에코패킷 만들기
	void Send_Packet_Echo(CProtocolBuff* Buff, WORD PayloadSize, char* RetrunText, int RetrunTextSize);

	// 에코 패킷 처리 함수
	bool Recv_Packet_Echo(ULONGLONG ClinetID, CProtocolBuff* Payload);

	
public:
	// Accept 직후, 호출된다.
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

	// Accept 후 접속처리 완료 후 호출
	virtual void OnClientJoin(ULONGLONG ClinetID);

	// Disconnect 후 호출된다.
	virtual void OnClientLeave(ULONGLONG ClinetID);

	// 패킷 수신 완료 후 호출되는 함수.
	virtual void OnRecv(ULONGLONG ClinetID, CProtocolBuff* Payload);

	// 패킷 송신 완료 후 호출되는 함수
	virtual void OnSend(ULONGLONG ClinetID, DWORD SendSize);

	// 워커 스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadBegin();

	// 워커스레드 1루푸 종료 후 호출되는 함수.
	virtual void OnWorkerThreadEnd();

	// 에러 발생 시 호출되는 함수.
	virtual void OnError(int error, const TCHAR* errorStr);
};


// Cpp부분
// ------------
// 패킷 처리 함수들
// ------------
// 에코패킷 만들기
void MyLanServer::Send_Packet_Echo(CProtocolBuff* Buff, WORD PayloadSize, char* RetrunText, int RetrunTextSize)
{
	// 1. 헤더 제작
	// 지금 제작할게 없음. 인자로받은 PayloadSize가 곧 헤더임

	// 2. 헤더 넣기
	*Buff << PayloadSize;

	// 3. 페이로드 제작 후 넣기
	memcpy_s(Buff->GetBufferPtr() + Buff->GetUseSize(), Buff->GetFreeSize(), RetrunText, RetrunTextSize);
	Buff->MoveWritePos(RetrunTextSize);
}

// 에코 패킷 처리 함수
bool MyLanServer::Recv_Packet_Echo(ULONGLONG ClinetID, CProtocolBuff* Payload)
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
	if (SendPacket(ClinetID, &Buff) == false)
		return false;

	return true;
}


// Accept 직후, 호출된다.
// return false : 클라이언트 접속 거부
// return true : 접속 허용
bool MyLanServer::OnConnectionRequest(TCHAR* IP, USHORT port)
{
	//printf("OnConnectionRequest\n");
	return true;
}

// Accept 후 접속처리 완료 후 호출
void MyLanServer::OnClientJoin(ULONGLONG ClinetID)
{
	//printf("OnClientJoin\n");
}

// Disconnect 후 호출된다.
void MyLanServer::OnClientLeave(ULONGLONG ClinetID)
{
	//printf("OnClientLeave : %lld\n", ClinetID);
}

// 패킷 수신 완료 후 호출되는 함수.
void MyLanServer::OnRecv(ULONGLONG ClinetID, CProtocolBuff* Payload)
{
	//printf("OnRecv\n");

	Recv_Packet_Echo(ClinetID, Payload);

}
// 패킷 송신 완료 후 호출되는 함수
void MyLanServer::OnSend(ULONGLONG ClinetID, DWORD SendSize)
{
	//printf("OnSend  --->   ");
	//printf("id : %lld, Byte : %d\n", ClinetID, SendSize);
}
// 워커 스레드 GQCS 바로 하단에서 호출
void MyLanServer::OnWorkerThreadBegin()
{
	//printf("OnWorkerThreadBegin\n");
}

// 워커스레드 1루프 종료 후 호출되는 함수.
void MyLanServer::OnWorkerThreadEnd()
{
	//printf("OnWorkerThreadEnd\n");
}

// 에러 발생 시 호출되는 함수.
void MyLanServer::OnError(int error, const TCHAR* errorStr)
{
	_tprintf_s(L"OnError : %s(%d)\n", errorStr, error);
}





int _tmain()
{
	MyLanServer LanServer;

	TCHAR IP[30] = L"0,0,0,0";

	LanServer.Start(IP, PORT, 2, false, 1000);

	Sleep(INFINITE);
    return 0;
}

