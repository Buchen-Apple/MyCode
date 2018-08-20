// LockFree_Test_02.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "LockFree_Queue\LockFree_Queue.h"
#include "CrashDump\CrashDump.h"

#include <process.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace Library_Jingyu;

// 각 스레드마다 노드를 Alloc하는 수
#define ALLOC_COUNT	2

// 각 스레드 수
#define CREATE_THREAD_COUNT	2

// 총 노드의 수
#define TOTAL_NODE_COUNT	CREATE_THREAD_COUNT*2*ALLOC_COUNT


class CTest
{
public:
	ULONGLONG Addr = 0x0000000055555555;
	LONG Count = 0;
};

// TestThread
// 기존처럼 값 체크하는 스레드
UINT	WINAPI	TestThread(LPVOID lParam);

// TestThread2
// Alloc,Free만 하는 스레드
UINT	WINAPI	TestThread2(LPVOID lParam);



CCrashDump* g_CDump = CCrashDump::GetInstance();
CLF_Queue<CTest> LFQ(0);

LONG g_UseCount;

ULONGLONG ThreadCount[CREATE_THREAD_COUNT*2] = { 0, };


int _tmain()
{
	timeBeginPeriod(1);

	//system("mode con: cols=120 lines=8");

	// 1. 시작 전에 TOTAL_NODE_COUNT 개를 만들어서 Enqueue한다.
	for (int i = 0; i < TOTAL_NODE_COUNT; ++i)
	{
		CTest Test;
		LFQ.Enqueue(Test);
	}

	// 2. 스레드 생성
	// TOTAL_THREAD개의 스레드. 스레드당 THREAD_ALLOC_COUNT개의 블럭 사용
	HANDLE hThread[10];
	int i = 0;

	// TestThread. CREATE_THREAD_COUNT만큼 만들기
	for (; i < CREATE_THREAD_COUNT; ++i)
	{
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, TestThread, (VOID*)i, 0, NULL);
	}

	// TestThread2. CREATE_THREAD_COUNT만큼 만들기
	for (; i < CREATE_THREAD_COUNT * 2; ++i)
	{
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, TestThread2, (VOID*)i, 0, NULL);
	}

	// 3. 화면에 출력
	// [유저가 사용중인 블록 수 / 총 블록 수] 
	while (1)
	{
		Sleep(1000);
		printf("=================================================================\n"
				"OutNodeCount : %d	InNodeCount : %d		TotalNodeCount : %d\n"
				"TestThreadLoop : %lld / %lld\n"
				"A/F ThreadLoop : %lld / %lld\n"
				"=================================================================\n\n"
				, g_UseCount, LFQ.GetInNode(), TOTAL_NODE_COUNT,
				ThreadCount[0], ThreadCount[1], ThreadCount[2], ThreadCount[3]);
	}


	timeEndPeriod(1);
	return 0;
 
}

// TestThread
// 기존처럼 값 체크하는 스레드
UINT	WINAPI	TestThread(LPVOID lParam)
{
	CTest cTestArray[ALLOC_COUNT];
	ULONGLONG TempAddr[ALLOC_COUNT];
	LONG TempCount[ALLOC_COUNT];

	int ID = (int)lParam;

	while (1)
	{
		cTestArray[0].Addr = NULL;
		cTestArray[0].Count = 0;	
		
		// 1. Dequeue
		for (int i = 0; i < ALLOC_COUNT; ++i)
		{
			int retval = LFQ.Dequeue(cTestArray[i]);
			if (retval == -1)
				g_CDump->Crash();

			InterlockedIncrement(&g_UseCount);
		}

		// 2. Addr 비교 (0x0000000055555555가 맞는지)
		for (int i = 0; i < ALLOC_COUNT; ++i)
		{
			if (cTestArray[i].Addr != 0x0000000055555555)
				g_CDump->Crash();

			// 3. 인터락으로 Addr, Count ++
			TempAddr[i] = InterlockedIncrement(&cTestArray[i].Addr);
			TempCount[i] = InterlockedIncrement(&cTestArray[i].Count);
		}

		// 3. 잠시 대기 (Sleep)
		//Sleep(0);

		// 4. Addr과 Count 다시 비교
		for (int i = 0; i < ALLOC_COUNT; ++i)
		{
			if (cTestArray[i].Addr != TempAddr[i])
				g_CDump->Crash();

			else if (cTestArray[i].Count != TempCount[i])
				g_CDump->Crash();

			// 6. 인터락으로 Addr, Count --
			TempAddr[i] = InterlockedDecrement(&cTestArray[i].Addr);
			TempCount[i] = InterlockedDecrement(&cTestArray[i].Count);
		}


		// 5. 잠시 대기 (Sleep)
		//Sleep(0);

		// 6. Addr과 Count가 0x0000000055555555, 0이 맞는지 확인
		for (int i = 0; i < ALLOC_COUNT; ++i)
		{
			if (cTestArray[i].Addr != TempAddr[i])
				g_CDump->Crash();

			else if (cTestArray[i].Count != TempCount[i])
				g_CDump->Crash();
		}

		// 7. Enqueue
		for (int i = 0; i < ALLOC_COUNT; ++i)
		{
			LFQ.Enqueue(cTestArray[i]);
			InterlockedDecrement(&g_UseCount);
		}

		ThreadCount[ID]++;
	}
	return 0;
}

// TestThread
// Alloc,Free만 하는 스레드
UINT	WINAPI	TestThread2(LPVOID lParam)
{
	CTest cTestArray[ALLOC_COUNT];

	int ID = (int)lParam;

	while (1)
	{
		cTestArray[0].Addr = NULL;
		cTestArray[0].Count = 0;

		// 1. Dequeue
		for (int i = 0; i < ALLOC_COUNT; ++i)
		{
			int retval = LFQ.Dequeue(cTestArray[i]);
			if (retval == -1)
				g_CDump->Crash();

			InterlockedIncrement(&g_UseCount);
		}				

		// 2. 잠시 대기 (Sleep)
		//Sleep(0);

		// 3. 잠시 대기 (Sleep)
		//Sleep(0);

		// 4. Enqueue
		for (int i = 0; i < ALLOC_COUNT; ++i)
		{
			LFQ.Enqueue(cTestArray[i]);
			InterlockedDecrement(&g_UseCount);
		}		

		ThreadCount[ID]++;

	}
	return 0;
}

