#include "stdafx.h"
#include "ExceptionClass.h"

// 생성자
CException::CException(const wchar_t* str)
{
	_tcscpy_s(ExceptionText, _countof(ExceptionText), str);
}

// 예외 텍스트의 주소 반환
char* CException::GetExceptionText()
{
	return (char*)&ExceptionText;
}