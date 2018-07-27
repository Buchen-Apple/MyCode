// 2차과제.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#include <list>
#include <time.h>

#include <conio.h>

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")


using namespace std;

#define THREAD_COUNT	6

// 1초마다 List 의 데이터를 출력
UINT WINAPI PrintThread(LPVOID lParam);

// 30ms 마다 List 의 끝 데이터를 삭제
UINT WINAPI	DeleteThread(LPVOID lParam);


// 메인스레드의 이벤트에 의해 깨어나며
// 깨어나면 List 의 내용을 파일 TXT 로 저장한다.
//(2회 이상 저장시 기존 파일을 지우고 저장한다)
UINT WINAPI	SaveThread(LPVOID lParam);

// 1초마다 List 에 rand 값 삽입.
UINT WINAPI	WorkerThread(LPVOID lParam);



list<int> list_TestList;
HANDLE Event;
HANDLE SaveEvent;

bool g_ExitFlag = false;

SRWLOCK sl;


int _tmain()
{
	// ----------------------------
	// 셋팅
	// ---------------------------
	timeBeginPeriod(1);

	HANDLE	hThread[THREAD_COUNT];
	DWORD	dwThreadID[THREAD_COUNT];
	DWORD	ThreadCount = 0;

	// SRW 락 초기화
	InitializeSRWLock(&sl);




	// --------------------------------
	// 이벤트 생성하기 (수동, Non-Signaled)
	// --------------------------------	
	Event = CreateEvent(NULL, TRUE, FALSE, NULL);

	// --------------------------------
	// 이벤트 생성하기 (자동, Non-Signaled)
	// --------------------------------	
	SaveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);





	// --------------------------------
	// 스레드 생성하기.
	// print/ deelte / Save / worker x 3 총 6개 생성
	// --------------------------------	
	ThreadCount = 0;

	// PrintThread 스레드 생성
	hThread[ThreadCount] = (HANDLE)_beginthreadex(NULL, 0, PrintThread, NULL, 0, (UINT*)&dwThreadID[ThreadCount]);
	ThreadCount++;

	// DeleteThread 스레드 생성
	hThread[ThreadCount] = (HANDLE)_beginthreadex(NULL, 0, DeleteThread, NULL, 0, (UINT*)&dwThreadID[ThreadCount]);
	ThreadCount++;

	// SaveThread 스레드 생성
	hThread[ThreadCount] = (HANDLE)_beginthreadex(NULL, 0, SaveThread, NULL, 0, (UINT*)&dwThreadID[ThreadCount]);
	ThreadCount++;

	// WorkerThread 스레드 생성
	for(; ThreadCount<THREAD_COUNT; ++ThreadCount)
		hThread[ThreadCount] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, NULL, 0, (UINT*)&dwThreadID[ThreadCount]);




	// -------------------------------
	// kb_hit 체크
	// -------------------------------
	int Key;
	while (1)
	{
		WaitForSingleObject(Event, 1);

		if (_kbhit())
		{
			Key = _getch();

			// s키를 눌렀다면
			if (Key == 's' || Key == 'S')
			{
				// SaveThread를 깨운다.
				fputs("'s'Key Click\n", stdout);
				SetEvent(SaveEvent);

			}

			// e 키를 눌렀다면
			if (Key == 'e' || Key == 'E')
			{
				// 모든 스레드 종료
				fputs("'e'Key Click\n", stdout);
				g_ExitFlag = true;
				SetEvent(SaveEvent);
				break;

			}

		}
	}




	// -------------------------------
	// 모든 스레드 종료 체크
	// -------------------------------
	WaitForMultipleObjects(THREAD_COUNT, hThread, TRUE, INFINITE);
	fputs("All Thread Exit OK!\n", stdout);




	// -------------------------------
	// 삭제
	// -------------------------------
	// 이벤트 삭제
	CloseHandle(Event);

	// 스레드 삭제
	for (ThreadCount = 0; ThreadCount < THREAD_COUNT; ++ThreadCount)
		CloseHandle(hThread[ThreadCount]);

	fputs("All Thread Close OK\n", stdout);
	
	timeEndPeriod(1);
    return 0;
}

// 1초마다 List 의 데이터를 출력
UINT WINAPI PrintThread(LPVOID lParam)
{
	while (1)
	{
		WaitForSingleObject(Event, 1000);

		if (g_ExitFlag == true)
			break;

		// 공유모드 입장
		// -----------------------
		AcquireSRWLockShared(&sl);

		list<int>::iterator itor = list_TestList.begin();

		for (; itor != list_TestList.end(); ++itor)
			printf("%d ", *itor);

		fputs("\n-------------------------\n", stdout);

		ReleaseSRWLockShared(&sl);
		// -----------------------
		// 공유모드 퇴장

	}

	return 0;	
}

// 30ms 마다 List 의 끝 데이터를 삭제
UINT WINAPI	DeleteThread(LPVOID lParam)
{
	while (1)
	{
		WaitForSingleObject(Event, 30);

		if (g_ExitFlag == true)
			break;

		// 임계영역 입장
		// -----------------------
		AcquireSRWLockExclusive(&sl);

		if(list_TestList.size() != 0)
			list_TestList.pop_back();

		ReleaseSRWLockExclusive(&sl);
		// -----------------------
		// 임계영역 퇴장	
	}

	return 0;
}

// 메인스레드의 이벤트에 의해 깨어나며
// 깨어나면 List 의 내용을 파일 TXT 로 저장한다.
//(2회 이상 저장시 기존 파일을 지우고 저장한다)
UINT WINAPI	SaveThread(LPVOID lParam)
{
	while (1)
	{
		WaitForSingleObject(SaveEvent, INFINITE);
		if (g_ExitFlag == true)
			break;

		FILE *fp;

		fopen_s(&fp, "Test.txt", "wt");


		// 공유모드 입장
		// -----------------------
		AcquireSRWLockShared(&sl);

		list<int>::iterator itor = list_TestList.begin();

		for (; itor != list_TestList.end(); ++itor)
		{
			int Save = *itor;
			fprintf_s(fp, "%d ", Save);
		}

		ReleaseSRWLockShared(&sl);
		// -----------------------
		// 공유모드 퇴장

		fclose(fp);

		fputs("File Save Complite!\n", stdout);
	}	

	return 0;
}

// 1초마다 List 에 rand 값 삽입.
UINT WINAPI	WorkerThread(LPVOID lParam)
{
	DWORD Rand;
	srand((unsigned)(time(NULL) + &Rand));

	while (1)
	{
		WaitForSingleObject(Event, 1000);

		if (g_ExitFlag == true)
			break;

		// 임계영역 입장
		// -----------------------
		AcquireSRWLockExclusive(&sl);

		list_TestList.push_front(rand() % 1000);

		ReleaseSRWLockExclusive(&sl);
		// -----------------------
		// 임계영역 퇴장	

	}
	return 0;
}

