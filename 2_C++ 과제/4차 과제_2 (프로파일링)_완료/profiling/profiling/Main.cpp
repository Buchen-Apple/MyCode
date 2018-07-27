#include <iostream>
#include <windows.h>
//#include "Profiling_Class.h"

// Profiling_Class.h가 선언되어 있지 않다면, 아래 매크로들을 공백으로 만듦. 
#ifndef __PROFILING_CLASS_H__
#define BEGIN(STR) 
#define END(STR)
#define FREQUENCY_SET()
#define PROFILING_SHOW()
#define PROFILING_FILE_SAVE()
#define RESET()
#else
#define BEGIN(STR)				BEGIN(STR)
#define END(STR)				END(STR)
#define FREQUENCY_SET()			FREQUENCY_SET()
#define PROFILING_SHOW()		PROFILING_SHOW()
#define PROFILING_FILE_SAVE()	PROFILING_FILE_SAVE()
#define RESET()					RESET()

#endif // !__PROFILING_CLASS_H__

using namespace std;

// 테스트용 함수
void Func1();
void Func2();
void Func3();
void Func4();
void Func5();

int main()
{	
	FREQUENCY_SET(); // 주파수 구하기	
	PROFILING_SHOW(); // 프로파일링 전체보기
	cout << "1차 끝" << endl;

	for (int i = 0; i < 1257; ++i)
	{
		Func1();
		Func2();
		Func3();
		Func4();
		Func5();
		if (i == 3)
		{
			RESET(); // 프로파일링 리셋
		}
	}
	PROFILING_SHOW(); // 프로파일링 전체보기

	PROFILING_FILE_SAVE(); // 프로파일링 파일 저장
	return 0;
}

// 테스트용 함수
void Func1()
{	
	BEGIN("Func1");
	puts("Func1()\n");
	END("Func1");
}
void Func2()
{
	BEGIN("Func2");
	printf("Func2()\n");
	END("Func2");
}
void Func3()
{
	BEGIN("Func3");
	cout << "Func3()" << endl;
	END("Func3");
}
void Func4()
{
	BEGIN("Func4");
	Sleep(1);
	END("Func4");
}
void Func5()
{
	BEGIN("Func5");
	char* test = new char;
	delete test;
	END("Func5");
}