// Netlib.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "NetworkLib\NetworkLib.h"

#include <conio.h>

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#define PORT	6000

using namespace Library_Jingyu;
using namespace std;

extern LONG g_llPacketAllocCount;




class MyLanServer : public CLanServer
{
private:
	// ------------
	// 패킷 처리 함수들
	// ------------
	// 에코패킷 만들기
	void Send_Packet_Echo(CProtocolBuff* Buff, char* RetrunText, int RetrunTextSize);

	// 에코 패킷 처리 함수
	bool Recv_Packet_Echo(ULONGLONG ClinetID, CProtocolBuff* Payload);
	
	SRWLOCK sl;


public:
	MyLanServer()
	{
		InitializeSRWLock(&sl);
	}
	~MyLanServer()
	{
		int abc = 10;
	}

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


// ------------
// 패킷 처리 함수들
// ------------
// 에코패킷 만들기
void MyLanServer::Send_Packet_Echo(CProtocolBuff* Buff, char* RetrunText, int RetrunTextSize)
{
	// 1. 페이로드 제작 후 넣기	
	Buff->PutData(RetrunText, RetrunTextSize);
}

// 에코 패킷 처리 함수
bool MyLanServer::Recv_Packet_Echo(ULONGLONG ClinetID, CProtocolBuff* Payload)
{
	// 1. 페이로드 사이즈 얻기
	int Size = Payload->GetUseSize();

	// 2. 그 사이즈만큼 데이터 memcpy
	char* Text = new char[Size];
	Payload->GetData(Text, Size);

	// 3. 에코 패킷 만들기. Buff안에는 페이로드만 있다.
	//CProtocolBuff* Buff = new CProtocolBuff;
	CProtocolBuff* Buff = CProtocolBuff::Alloc();
	InterlockedIncrement(&g_llPacketAllocCount);
	Send_Packet_Echo(Buff, Text, Size);

	delete[] Text;

	// 4. 에코 패킷을 SendBuff에 넣기
	bool Check = SendPacket(ClinetID, Buff);

	return Check;
}


// Accept 직후, 호출된다.
//
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
	// 데이터 페이로드에 만들기
	//CProtocolBuff* Payload = new CProtocolBuff;
	CProtocolBuff* Payload = CProtocolBuff::Alloc();
	InterlockedIncrement(&g_llPacketAllocCount);
	__int64 i64_Data = 0x7fffffffffffffff;

	*Payload << i64_Data;

	// SendBuff에 넣기
	bool Check = SendPacket(ClinetID, Payload);
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
	//LOG(L"OnError", CSystemLog::en_LogLevel::LEVEL_ERROR, L" [%s ---> %d, %d]", errorStr, error, WinGetLastError());
}





int _tmain()
{
	timeBeginPeriod(1);

	system("mode con: cols=120 lines=20");

	MyLanServer LanServer;

	TCHAR IP[30] = L"0,0,0,0";

	if (LanServer.Start(IP, PORT, 4, 1, true, 1000) == false)
	{
		// 윈도우 에러와 내 에러를 다 얻어본다.
		int WinError = LanServer.WinGetLastError();
		int MyError = LanServer.NetLibGetLastError();

		return 0;
	}


	int iLoopCount = 0;

	while (1)
	{
		//iLoopCount++;
		//_tprintf_s(L"서버 상태 : %s, 접속중 : %lld\n", LanServer.GetServerState() ? L"가동중" : L"가동중 아님",  LanServer.GetClientCount());

		//if (iLoopCount == 7)
		//{
		//	_tprintf(L"Q키 누름. 서버 종료\n");
		//	LanServer.Stop();

		//	iLoopCount = -1;

		//	Sleep(2000);

		//}

		//else if (iLoopCount == 0)
		//{
		//	_tprintf(L"S키 누름. 서버 시작\n");

		//	if (LanServer.Start(IP, PORT, 5, 1, true, 1000) == false)
		//	{
		//		// 윈도우 에러와 내 에러를 다 얻어본다.
		//		int WinError = LanServer.WinGetLastError();
		//		int MyError = LanServer.NetLibGetLastError();

		//		return 0;
		//	}			
		//}


		_tprintf_s(L"Stat : %s, JoinCount : %lld, PacketCount : [%d]\n", LanServer.GetServerState() ? L"Open" : L"Close", LanServer.GetClientCount(), g_llPacketAllocCount);
		// 서버 종료 체크
		if (_kbhit())
		{

			int Key = _getch();

			if (Key == 'q' || Key == 'Q')
			{

				_tprintf(L"Q Key. Server Exit\n");
				LanServer.Stop();

				break;
			}

			//else if (Key == 's' || Key == 'S')
			//{
			//	_tprintf(L"S Key. Server Start\n");

			//	if (LanServer.Start(IP, PORT, 5, 1, true, 1000) == false)
			//	{
			//		// 윈도우 에러와 내 에러를 다 얻어본다.
			//		int WinError = LanServer.WinGetLastError();
			//		int MyError = LanServer.NetLibGetLastError();

			//		return 0;
			//	}
			//}
		}

		Sleep(1000);
	}

	timeEndPeriod(1);


	return 0;
}