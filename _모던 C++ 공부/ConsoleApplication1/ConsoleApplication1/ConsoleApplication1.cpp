// ConsoleApplication1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <string>

using std::cout;
using std::wcout;
using std::endl;
using std::string;
using std::wstring;


int main()
{
	wstring str1 = L"Hello";
	wstring str2 = L"World";

	wstring str3 = str1 + L" " + str2;

	if (str3 == L"Hello World")
	{
		wcout << L"OK!!\n";
	}
	else
	{
		wcout << L"Fail...\n";
	}
		 
	return 0;
}

