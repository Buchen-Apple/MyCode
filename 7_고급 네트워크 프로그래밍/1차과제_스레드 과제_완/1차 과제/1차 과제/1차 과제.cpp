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

// 이벤트 핸들
HANDLE Event;

// 크리티컬 섹션 구조체
//CRITICAL_SECTION cs;

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

	// 크리티컬 섹션 초기화
	//InitializeCriticalSection(&cs);

	// SRW 락 초기화
	InitializeSRWLock(&sl);

	// 각종 변수 초기화
	DWORD	dwThreadID[THREAD_COUNT];
	HANDLE	hThread[THREAD_COUNT];
	DWORD	dwThreadCount = 0;	




	// ----------------------------
	// 쓰레드 생성
	// ---------------------------
	// 이벤트 생성 (자동 리셋 이벤트)
	Event = CreateEvent(NULL, FALSE, FALSE, NULL);

	// AcceptThread 쓰레드 생성
	hThread[dwThreadCount] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, NULL, 0, (UINT*)&dwThreadID[dwThreadCount]);
	dwThreadCount++;

	// DisconnectThread 쓰레드 생성
	hThread[dwThreadCount] = (HANDLE)_beginthreadex(NULL, 0, DisconnectThread, NULL, 0, (UINT*)&dwThreadID[dwThreadCount]);
	dwThreadCount++;

	// UpdateThread 쓰레드 생성
	for (; dwThreadCount < THREAD_COUNT; ++dwThreadCount)
		hThread[dwThreadCount] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, NULL, 0, (UINT*)&dwThreadID[dwThreadCount]);
	
	// 쓰레드 아무거나 하나 깨움
	SetEvent(Event);




	// ----------------------------
	// 20초 체크
	// ---------------------------	
	DWORD dwTime = timeGetTime();
	DWORD dwPrintfTime = timeGetTime();
	int i = 0;
	while (1)
	{
		if (timeGetTime() - dwPrintfTime >= 1000)
		{
			printf("%d g_Connect : %d\n", ++i, g_Connect);
			dwPrintfTime = timeGetTime();
		}

		if (timeGetTime() - dwTime >= 20000)
		{
			if ((timeGetTime() - dwPrintfTime) >= 1000)
				printf("%d g_Connect : %d\n", ++i, g_Connect);

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

	CloseHandle(Event);
	for (; dwThreadCount < THREAD_COUNT; ++dwThreadCount)
		CloseHandle(hThread[dwThreadCount]);

	//DeleteCriticalSection(&cs);
	timeEndPeriod(1);
    return 0;
}

// 100 ~ 1000ms 마다 랜덤하게 g_Connect 를 + 1
UINT	WINAPI	AcceptThread(LPVOID lpParam)
{
	DWORD dwTime = timeGetTime();	
	DWORD iRandTime = (rand() % 900) + 100;

	while (1)
	{		
		WaitForSingleObject(Event, INFINITE);
		if (g_Shutdown == true)
			break;

		// 현재 시간 얻기
		DWORD dwTempTime = timeGetTime() - dwTime;
		if (dwTempTime >= iRandTime)
		{
			// 인터락으로 증가
			InterlockedIncrement64((volatile LONG64*)&g_Connect);
			dwTime = timeGetTime();
			iRandTime = (rand() % 900) + 100;
		}	

		SetEvent(Event);
	}

	SetEvent(Event);
	return 0;	
}

// 100 ~ 1000ms 마다 랜덤하게 g_Connect 를 - 1
UINT	WINAPI	DisconnectThread(LPVOID lpParam)
{
	DWORD dwTime = timeGetTime();
	DWORD iRandTime = (rand() % 900) + 100;

	while (1)
	{		
		WaitForSingleObject(Event, INFINITE);
		if (g_Shutdown == true)
			break;

		// 현재 시간 얻기
		DWORD dwTempTime = timeGetTime() - dwTime;
		
		if (dwTempTime >= iRandTime)
		{
			// 인터락으로 감소
			InterlockedDecrement64((volatile LONG64*)&g_Connect);
			dwTime = timeGetTime();
			iRandTime = (rand() % 900) + 100;
		}
		SetEvent(Event);
	}

	SetEvent(Event);
	return 0;	
}

// 10ms 마다 g_Data 를 + 1. 그리고 g_Data 가 1000 단위가 될 때 마다 이를 출력
UINT	WINAPI	UpdateThread(LPVOID lpParam)
{
	DWORD dwTime = timeGetTime();	

	while (1)
	{	
		WaitForSingleObject(Event, INFINITE);
		if (g_Shutdown == true)
			break;

		// 임계영역 입장
		// -----------------------
		//EnterCriticalSection(&cs);
		AcquireSRWLockExclusive(&sl);

		// 현재 시간 얻기
		if ((timeGetTime() - dwTime) >= 10)
		{
			g_Data++;

			// g_Data 체크 후 1000단위면 출력한다.
			if ((g_Data % 1000) == 0)
				printf("g_Data : %d\n", g_Data);

			dwTime = timeGetTime();
		}		

		ReleaseSRWLockExclusive(&sl);
		//LeaveCriticalSection(&cs);
		// -----------------------
		// 임계영역 퇴장		

		SetEvent(Event);		
	}

	SetEvent(Event);
	return 0;	
}

