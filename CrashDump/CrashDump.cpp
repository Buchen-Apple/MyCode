#include "stdafx.h"
#include "CrashDump.h"

#pragma comment(lib, "DbgHelp.lib")
#include <DbgHelp.h>
#include <crtdbg.h>
#include <csignal>

namespace Library_Jingyu
{
	long CCrashDump::_DumpCount;

	// 싱글톤을 위해 생성자는 Private
	// 생성자
	CCrashDump::CCrashDump(void)
	{
		_DumpCount = 0;

		// --------------
		// CRT 에러 핸들링
		// --------------
		_invalid_parameter_handler oldHandler, newHandler;
		newHandler = myInvalidParameterHandler;

		// CRT 함수에 nullptr 등을 넣었을 때 
		oldHandler = _set_invalid_parameter_handler(newHandler);

		// CRT 오류 메시지 출력 중단. 바로 덤프로 남긴다.
		_CrtSetReportMode(_CRT_WARN, 0);	// 경고 메시지 (Warning) 출력 중단.
		_CrtSetReportMode(_CRT_ASSERT, 0);	// 어설션 오류 출력 중단 (어셜션 오류 : 출력이 false인 경우 에러 발생시킴)
		_CrtSetReportMode(_CRT_ERROR, 0);	// Error 메시지 출력 중단.

											// CRT 에러가 발생하면, 후킹할 함수 등록(함수포인터)
		_CrtSetReportHook(_custom_Report_hook);


		// --------------
		// pure virtual function called 에러 핸들러를 사용자 정의 함수로 우회
		// 해당 에러는, 생성자/소멸자에서 virtual 함수를 호출했을 때 뭔가 꼬여서 발생하는 문제.
		// --------------
		_set_purecall_handler(myPurecallHandler);

		// abort() 함수를 호출했을 때 오류창이 뜨지 않도록 한다.
		// abort() : 해당 함수가 호출되면, 비정상 종료를 야기하고, 호스트 환경에 제어를 리턴. exit()와 마찬가지로 프로그램 종료 전, 버퍼 삭제하고 열린 파일을 닫음.
		// 그리고, Signal 신호를 발생시킨다.
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

		// 지정된 신호(1번 인자)가 올 시, 지정된 함수(2번 인자)가 호출된다.
		signal(SIGABRT, signalHandler);		// 비정상 종료
		signal(SIGINT, signalHandler);		// Ctrl+C 신호
		signal(SIGILL, signalHandler);		// 잘못된 명령
		signal(SIGFPE, signalHandler);		// 부동 소수점 오류
		signal(SIGSEGV, signalHandler);		// 잘못된 저장소에 엑세스
		signal(SIGTERM, signalHandler);		// 종료 요청.


		SetHandlerDump();
	}


	// 싱글톤 얻기
	CCrashDump* CCrashDump::GetInstance()
	{
		static CCrashDump cCrashDump;
		return &cCrashDump;
	}
	

	// -----------------
	// static 함수들
	// -----------------
	// 강제로 Crash 내기
	void CCrashDump::Crash(void)
	{
		fputs("Crush!!\n", stdout);
		int *p = nullptr;
		*p = 0;
	}

	// 덤프파일 만들기
	LONG WINAPI CCrashDump::MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		// -----------------
		// 파일에 카운트를 붙이기 위한 DumpCount 증가.
		// -----------------		
		long DumpCount = InterlockedIncrement(&_DumpCount);

		// -----------------
		// 현재 날짜와 시간을 구해온다.
		// -----------------
		SYSTEMTIME	stNowTime;
		GetLocalTime(&stNowTime);

		// -----------------
		// 덤프파일 이름 만들기
		// -----------------
		TCHAR tcFileName[MAX_PATH];
		swprintf_s(tcFileName, MAX_PATH, _T("Dump_%d%02d%02d_%02d.%02d.%02d._%d.dmp"), stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay,
			stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, DumpCount);

		// -----------------
		// 덤프 파일 생성중이라고 화면에 출력하기
		// -----------------
		_tprintf_s(_T("\n\n\n!!! Crash Error !!!  %d.%d.%d / %d:%d:%d \n"), stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay,
			stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);

		_tprintf_s(_T("Now Save dump File...\n"));

		// -----------------
		// 덤프파일 생성하기
		// -----------------
		// 파일 생성
		HANDLE hDumpFile = CreateFile(tcFileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		// 정상적으로 생성됐으면 덤프정보 저장하기
		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			// 덤프 정보 셋팅
			_MINIDUMP_EXCEPTION_INFORMATION DumpExceptionInfo;
			DumpExceptionInfo.ThreadId = GetCurrentThreadId();
			DumpExceptionInfo.ExceptionPointers = pExceptionPointer;
			DumpExceptionInfo.ClientPointers = TRUE;

			// 저장
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithFullMemory, &DumpExceptionInfo, NULL, NULL);

			// 핸들 닫기
			CloseHandle(hDumpFile);

			// 저장 잘 됐다고 출력
			_tprintf_s(_T("CrashDump Save Finish ! \n"));
		}

		// 해당 값을 리턴해야 프로그램이 즉시 종료된다.
		return EXCEPTION_EXECUTE_HANDLER;

	}

	// 덤프파일 제작하는 함수 등록하기.
	void CCrashDump::SetHandlerDump()
	{
		// 응용 프로그램이, 프로세스의 각 스레드의 최상위 예외 처리기를 대체할 수 있다.
		// 이 함수를 호출한 후, 디버깅 되지 않는 프로세스에서 예외가 발생하고, 처리되지 않은 예외 필터에서 예외가 발생하면
		// 해당 필터는 인자로 전달된 'MyExceptionFilter' 필터 함수를 호출한다.
		// MyExceptionFilter()함수는 현재, 덤프파일 제작한다.
		SetUnhandledExceptionFilter(MyExceptionFilter);

		// C 런타임 라이브러리 내부의 예외핸들러 등록을 막기 위해 API 후킹
		// 근데 vs 2017에서는 이거 안먹힘...
		//static CAPIHook apiHook("kernel32.dll", "SetUnhandledExceptionFilter", (PROC)RedirectedSetUnhandlerExceptionFilter, true);
	}

	


	// --------------
	// 에러 핸들링 되었을 때 호출되는 함수들
	// -------------
	// CRT 함수에 인자를 잘못넣었을 때 (nullptr등..) 호출 (CRT는 C RungTime의 약자)
	void CCrashDump::myInvalidParameterHandler(const TCHAR* expression, const TCHAR* function, const TCHAR* filfile, UINT line, UINT_PTR pReserved)
	{
		fputs("myInvalidParameterHandler()!!\n", stdout);
		Crash();
	}

	// 그 외 CRT 함수 에러 발생 시 호출
	int CCrashDump::_custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		fputs("_custom_Report_hook()!!\n", stdout);
		Crash();
		return true;
	}

	// pure virtual function called 에러 발생 시 호출
	void CCrashDump::myPurecallHandler(void)
	{
		fputs("myPurecallHandler()!!\n", stdout);
		Crash();
	}
	
	// 각종 시그널 발생 시 호출.
	void CCrashDump::signalHandler(int Error)
	{
		printf("signalHandler()!! %d\n", Error);
		Crash();
	}
}