// 1차 과제.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"

#include <windows.h>
#include <process.h>
#include <time.h>

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#define THREAD_COUNT	5


int g_Data = 0;					// 가상의 데이터
int g_Connect = 0;				// 가상의 접속자 수
bool g_Shutdown = false;		// true면 모든 스레드 종료.

// SRW 락 구조체
SRWLOCK sl;

// 100 ~ 1000ms 마다 랜덤하게 g_Connect 를 + 1
UINT	WINAPI	AcceptThread(LPVOID lpParam);

// 100 ~ 1000ms 마다 랜덤하게 g_Connect 를 + 1
UINT	WINAPI	DisconnectThread(LPVOID lpParam);

// 10ms 마다 g_Data 를 + 1. 그리고 g_Data 가 1000 단위가 될 때 마다 이를 출력
UINT	WINAPI	UpdateThread(LPVOID lpParam);


int _tmain()
{
	// ----------------------------
	// 셋팅
	// ---------------------------
	timeBeginPeriod(1);

	// SRW 락 초기화
	InitializeSRWLock(&sl);

	// 각종 변수 초기화
	DWORD	dwThreadID[THREAD_COUNT];
	HANDLE	hThread[THREAD_COUNT];
	DWORD	dwThreadCount = 0;	




	// ----------------------------
	// 쓰레드 생성
	// ---------------------------
	// AcceptThread 쓰레드 생성
	hThread[dwThreadCount] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, NULL, 0, (UINT*)&dwThreadID[dwThreadCount]);
	dwThreadCount++;

	// DisconnectThread 쓰레드 생성
	hThread[dwThreadCount] = (HANDLE)_beginthreadex(NULL, 0, DisconnectThread, NULL, 0, (UINT*)&dwThreadID[dwThreadCount]);
	dwThreadCount++;

	// UpdateThread 쓰레드 생성
	for (; dwThreadCount < THREAD_COUNT; ++dwThreadCount)
		hThread[dwThreadCount] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, (UINT*)&dwThreadID[dwThreadCount]);




	// ----------------------------
	// 20초 체크
	// ---------------------------	
	int i = 0;
	while (1)
	{
		Sleep(1000);
		printf("%d g_Connect : %d\n", ++i, g_Connect);

		if (i == 20)
		{
			g_Shutdown = true;
			break;
		}
	}




	// ----------------------------
	// 20초가 됐으면 종료 대기
	// ---------------------------	
	WaitForMultipleObjects(THREAD_COUNT, hThread, TRUE, INFINITE);
	


	// ----------------------------
	// 삭제
	// ---------------------------
	dwThreadCount = 0;

	for (; dwThreadCount < THREAD_COUNT; ++dwThreadCount)
		CloseHandle(hThread[dwThreadCount]);

	timeEndPeriod(1);
    return 0;
}

// 100 ~ 1000ms 마다 랜덤하게 g_Connect 를 + 1
UINT	WINAPI	AcceptThread(LPVOID lpParam)
{
	DWORD RareTest;

	srand((unsigned)(time(NULL) + &RareTest));

	DWORD iRandTime = (rand() % 900) + 100;

	while (1)
	{		
		if (g_Shutdown == true)
			break;

		Sleep(iRandTime);

		InterlockedIncrement64((volatile LONG64*)&g_Connect);
		iRandTime = (rand() % 900) + 100;		
	}

	return 0;	
}

// 100 ~ 1000ms 마다 랜덤하게 g_Connect 를 - 1
UINT	WINAPI	DisconnectThread(LPVOID lpParam)
{
	DWORD RareTest;

	srand((unsigned)(time(NULL) + &RareTest));
	DWORD iRandTime = (rand() % 900) + 100;

	while (1)
	{		
		if (g_Shutdown == true)
			break;

		Sleep(iRandTime);

		// 인터락으로 감소
		InterlockedDecrement64((volatile LONG64*)&g_Connect);
		iRandTime = (rand() % 900) + 100;		
	}

	return 0;	
}

// 10ms 마다 g_Data 를 + 1. 그리고 g_Data 가 1000 단위가 될 때 마다 이를 출력
UINT	WINAPI	UpdateThread(LPVOID lpParam)
{
	while (1)
	{			
		if (g_Shutdown == true)
			break;

		Sleep(10);

		// 임계영역 입장
		// -----------------------
		AcquireSRWLockExclusive(&sl);

		g_Data++;

		// g_Data 체크 후 1000단위면 출력한다.
		if ((g_Data % 1000) == 0)
			printf("g_Data : %d\n", g_Data);					

		ReleaseSRWLockExclusive(&sl);
		// -----------------------
		// 임계영역 퇴장		
	}

	return 0;	
}

