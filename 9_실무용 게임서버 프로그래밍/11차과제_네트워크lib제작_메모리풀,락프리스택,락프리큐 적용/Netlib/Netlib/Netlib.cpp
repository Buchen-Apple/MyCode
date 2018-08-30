// Netlib.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "NetworkLib\NetworkLib.h"

#include <conio.h>

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#include "profiling\Profiling_Class.h"

#define PORT	6000

using namespace Library_Jingyu;
using namespace std;

#define CREATE_WORKER_COUNT	4
#define ACTIVE_WORKER_COUNT	2
#define CREATE_ACCEPT_COUNT	1

//ULONGLONG g_ullAllocNodeCount;
//ULONGLONG g_ullFreeNodeCount;



class MyLanServer : public CLanServer
{

public:
	virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

	virtual void OnClientJoin(ULONGLONG ClinetID);

	virtual void OnClientLeave(ULONGLONG ClinetID);

	virtual void OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload);

	virtual void OnSend(ULONGLONG ClinetID, DWORD SendSize);

	virtual void OnWorkerThreadBegin();

	virtual void OnWorkerThreadEnd();

	virtual void OnError(int error, const TCHAR* errorStr);
};


bool MyLanServer::OnConnectionRequest(TCHAR* IP, USHORT port)
{
	//printf("OnConnectionRequest\n");
	return true;
}

void MyLanServer::OnClientJoin(ULONGLONG ClinetID)
{
	// 데이터 페이로드에 만들기
	BEGIN("Test");
	CProtocolBuff_Lan* Payload = CProtocolBuff_Lan::Alloc();

	__int64 i64_Data = 0x7fffffffffffffff;

	Payload->PutData((char*)&i64_Data, 8);

	// SendBuff에 넣기
	SendPacket(ClinetID, Payload);
}

void MyLanServer::OnClientLeave(ULONGLONG ClinetID)
{
	//printf("OnClientLeave : %lld\n", ClinetID);
}

void MyLanServer::OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload)
{
	// 1. 페이로드 사이즈 얻기
	int Size = Payload->GetUseSize();

	// 2. 그 사이즈만큼 데이터 memcpy
	char* Text = new char[Size];
	Payload->GetData(Text, Size);

	// 3. 에코 패킷 만들기. Buff안에는 페이로드만 있다.
	CProtocolBuff_Lan* Buff = CProtocolBuff_Lan::Alloc();
	Buff->PutData(Text, Size);

	delete[] Text;

	// 4. 에코 패킷을 SendBuff에 넣기.
	SendPacket(ClinetID, Buff);
}


void MyLanServer::OnSend(ULONGLONG ClinetID, DWORD SendSize)
{
	//printf("OnSend  --->   ");
	//printf("id : %lld, Byte : %d\n", ClinetID, SendSize);
}

void MyLanServer::OnWorkerThreadBegin()
{
	//printf("OnWorkerThreadBegin\n");
}

void MyLanServer::OnWorkerThreadEnd()
{
	//printf("OnWorkerThreadEnd\n");
}

void MyLanServer::OnError(int error, const TCHAR* errorStr)
{
	//LOG(L"OnError", CSystemLog::en_LogLevel::LEVEL_ERROR, L" [%s ---> %d, %d]", errorStr, error, WinGetLastError());
}






int _tmain()
{
	timeBeginPeriod(1);

	system("mode con: cols=120 lines=8");

	MyLanServer LanServer;

	TCHAR IP[30] = L"0,0,0,0";

	if (LanServer.Start(IP, PORT, CREATE_WORKER_COUNT, ACTIVE_WORKER_COUNT, CREATE_ACCEPT_COUNT, false, 200) == false)
	{
		// 윈도우 에러와 내 에러를 다 얻어본다.
		int WinError = LanServer.WinGetLastError();
		int MyError = LanServer.NetLibGetLastError();

		return 0;
	}	

	while (1)
	{
		if (_kbhit())
		{
			int Key = _getch();

			if (Key == 'q' || Key == 'Q')
			{
				break;
			}
		}

		_tprintf_s(L"====================================================================================\n"
			"WorkerThread : %d, ActiveWorkerThread : %d, AcceptThread : %d\n\n"
			"JoinCount : %lld\n"
			"====================================================================================\n\n",
			CREATE_WORKER_COUNT, ACTIVE_WORKER_COUNT, CREATE_ACCEPT_COUNT,
			LanServer.GetClientCount() );

		Sleep(1000);
	}	

	if(LanServer.GetServerState() == true)
		LanServer.Stop();

	timeEndPeriod(1);


	return 0;
}