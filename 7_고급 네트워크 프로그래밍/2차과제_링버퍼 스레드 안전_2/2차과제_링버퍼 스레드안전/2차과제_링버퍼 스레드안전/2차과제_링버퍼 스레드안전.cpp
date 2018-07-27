// 2차과제_링버퍼 스레드안전.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#include <locale.h>

#include "RingBuff\RingBuff.h"

using namespace Library_Jingyu;

// 읽기 스레드
UINT	WINAPI	ReadThread(LPVOID lParam);
// 쓰기 스레드
UINT	WINAPI	WriteThread(LPVOID lParam);

HANDLE Event;
char g_TestString[] = "songjingyu 123456789 procademy testtest iLoveCPrograming hahahahahahahahah 987654321 test123test 123456test kkkkkkEND";
int g_TestStringLen;
CRingBuff RingBuff(1000);

int _tmain()
{
	system("mode con: cols=117 lines=20");

	setlocale(LC_ALL, "korean");

	g_TestStringLen = (int)(strlen(g_TestString)+1);

	HANDLE	hThread[2];
	DWORD	dwThreadID[2];

	// 이벤트 생성
	Event = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 쓰레드 2개 생성
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0, ReadThread, NULL, 0, (UINT*)&dwThreadID[0]);
	hThread[1] = (HANDLE)_beginthreadex(NULL, 0, WriteThread, NULL, 0, (UINT*)&dwThreadID[1]);

	// 1초 대기 후 Signaled로 변경해서 모든 스레드 깨움
	Sleep(1000);
	SetEvent(Event);

	// Main스레드는 무한대기
	Sleep(INFINITE);	

    return 0;
}

// 읽기 스레드
UINT	WINAPI	ReadThread(LPVOID lParam)
{
	WaitForSingleObject(Event, INFINITE);	

	while (1)
	{
		char ReadBuff[118];
		ZeroMemory(ReadBuff, 118);

		// 링버퍼에서 데이터 읽어오기			
		int Check = RingBuff.Dequeue(ReadBuff, g_TestStringLen);

		if (Check == -1)
		{
			//fputs("Dequeue(). 큐가 비었음\n", stdout);
			//return 0;
		}

		else if (Check != g_TestStringLen)
		{
			fputs("Dequeue(). 원했던 바이트 디큐하지 않음\n", stdout);
			return 0;
		}

		if(Check > 0)
			printf("%s", ReadBuff);
		
	}

	return 0;
}

// 쓰기 스레드
UINT	WINAPI	WriteThread(LPVOID lParam)
{
	WaitForSingleObject(Event, INFINITE);	

	while (1)
	{
		// 링버퍼에 데이터 쓰기
		int Check = RingBuff.Enqueue(g_TestString, g_TestStringLen);
		if (Check == -1)
		{
			//fputs("Enqueue(). 큐가 꽉 참\n", stdout);
			//return 0;
		}
		else if (Check != g_TestStringLen)
		{
			fputs("Enqueue(). 원했던 바이트 인큐하지 않음\n", stdout);
			return 0;
		}
	}

	return 0;
}

