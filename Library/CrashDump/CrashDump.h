#ifndef __CRASH_DUMP_H__
#define __CRASH_DUMP_H__

#include <Windows.h>

namespace Library_Jingyu
{

	class CCrashDump
	{
		// 싱글톤을 위해 생성자는 Private
		// 생성자
		CCrashDump(void);

	public:
		static long _DumpCount;

	public:
		// 싱글톤 얻기
		static CCrashDump* GetInstance();


		// -----------------
		// static 함수들
		// -----------------
		// 강제로 Crash 내기
		static void Crash(void);

		// 덤프파일 만들기
		static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer);

		// C 런타임 라이브러리 내부의 예외 핸들러 등록을 막고, 해당 함수가 호출되도록 하기 위한것.
		// 근데 2017에서는 안먹힘..
		// static LONG WINAPI RedirectedSetUnhandlerExceptionFilter(EXCEPTION_POINTERS *exceptionInfo);

		// 덤프파일 제작하는 함수 등록하기.
		static void SetHandlerDump();


		// --------------
		// 에러 핸들링 되었을 때 호출되는 함수들
		// -------------
		// CRT 함수에 인자를 잘못넣었을 때 (nullptr등..) 호출 (CRT는 C RungTime의 약자)
		static void myInvalidParameterHandler(const TCHAR* expression, const TCHAR* function, const TCHAR* filfile, UINT line, UINT_PTR pReserved);

		// 그 외 CRT 함수 에러 발생 시 호출
		static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue);

		// pure virtual function called 에러 발생 시 호출
		static void myPurecallHandler(void);

		// 각종 시그널 발생 시 호출.
		static void signalHandler(int Error);
	};
	

}



#endif // !__CRASH_DUMP_H__
