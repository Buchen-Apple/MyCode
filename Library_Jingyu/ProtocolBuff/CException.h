#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <Windows.h>
#include <tchar.h>

namespace Library_Jingyu
{

#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// Recv()패킷 처리 중, 예외 발생 시 던지는 예외클래스이다.
	class CException
	{
	private:
		wchar_t ExceptionText[100];

	public:
		// 생성자
		CException(const wchar_t* str)
		{
			_tcscpy_s(ExceptionText, _MyCountof(ExceptionText), str);
		}

		// 예외 텍스트 포인터 반환
		char* GetExceptionText()
		{
			return (char*)&ExceptionText;
		}
	};	
}


#endif // !__EXCEPTION_H__

