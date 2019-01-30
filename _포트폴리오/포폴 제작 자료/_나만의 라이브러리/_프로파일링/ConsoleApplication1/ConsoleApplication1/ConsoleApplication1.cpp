// ConsoleApplication1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <process.h>

#include "profiling/Profiling_Class.h"

using namespace Library_Jingyu;
using namespace std;

auto WINAPI	PrintfThread(LPVOID lParam) -> UINT
{
	// printf를 10만회 호출
	for (int i = 0; i < 100000; ++i)
	{
		BEGIN("printf");
		printf("Printf()\n");
		END("printf");
	}

	return 0;
}

auto WINAPI	CountThread(LPVOID lParam) -> UINT
{
	// cout을 10만회 호출
	for (int i = 0; i < 100000; ++i)
	{
		BEGIN("cout");
		cout << "cout\n";
		END("cout");
	}

	return 0;
}

auto main()	->int
{
	Profiling pro;		// 프로파일링 객체 생성
	FREQUENCY_SET();	// QueryPerformanceCounter 주파수 구하기	

	// PrintfTest, CountTest를 호출할 스레드 생성
	HANDLE hThread[2];
	hThread[0] = (HANDLE)_beginthreadex(NULL, NULL, PrintfThread, NULL, NULL, NULL);
	hThread[1] = (HANDLE)_beginthreadex(NULL, NULL, CountThread, NULL, NULL, NULL);

	// 모든 스레드 종료 대기
	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);	

	// 화면에 출력
	PROFILING_SHOW();
	
	cout << "File Save Start\n";
	PROFILING_FILE_SAVE();		// 파일로 저장하기
	cout << "File Save End\n";	

	return 0;
}
