// ConsoleApplication1.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <initializer_list>
#include <string>
#include <vector>
#include <queue>
#include <bitset>
#include <algorithm>

using std::initializer_list;
using std::cout;
using std::endl;
using namespace std;

class MyLiteral
{
	const CHAR* mTagName;
	const WCHAR* mTagName_W;
	std::vector<CHAR*> mNewChar;	
	std::vector<WCHAR*> mNewWChar;

	int mTagstrSize;

private:
	MyLiteral()
		:mTagName("song "), mTagName_W(L"song "), mTagstrSize(5)
	{}

	auto CharCelar() ->void
	{
		size_t Size = mNewChar.size();
		for (; Size > 0; --Size)
		{
			delete[] mNewChar[Size - 1];
			mNewChar.pop_back();
		}

		Size = mNewWChar.size();
		for (; Size > 0; --Size)
		{
			delete[] mNewWChar[Size - 1];
			mNewWChar.pop_back();
		}
	}	

public:
	static auto Getinstance() ->MyLiteral&
	{		
		static MyLiteral mMy;
		return mMy;
	}

	virtual ~MyLiteral()
	{		
		CharCelar();
	}

	virtual auto NewChar(const char* str, size_t len) ->CHAR*
	{
		char* ret = new char[len + mTagstrSize];

		strcpy_s(ret, len + mTagstrSize, mTagName);
		strcat_s(ret + mTagstrSize, len + mTagstrSize, str);

		mNewChar.push_back(ret);

		return ret;
	}

	virtual auto NewChar(const WCHAR* str, size_t len) ->WCHAR*
	{
		WCHAR* ret = new WCHAR[(len*2) + (mTagstrSize*2)];

		wcscpy_s(ret, len + mTagstrSize, mTagName_W);
		wcscat_s(ret + mTagstrSize, len + mTagstrSize, str);

		mNewWChar.push_back(ret);

		return ret;
	}
};

MyLiteral& MyLit = MyLiteral::Getinstance();

auto operator"" _Mycustom(const char* str, size_t len) ->CHAR*
{
	return MyLit.NewChar(str, len);	
}

auto operator"" _Mycustom(const WCHAR* str, size_t len)	->WCHAR*
{
	return MyLit.NewChar(str, len);
}

class Test
{
public:
	Test() = default;
	Test(const Test& ref)
	{
		cout << "Copy constructor\n";
	}

	Test(Test&& ref)
	{
		cout << "Move constructor\n";
	}
};

auto Create() -> Test
{
	Test aa;
	return aa;
}


auto main()	-> int
{
	auto str1 = "Hello World"_Mycustom;
	auto str2 = "My C++"_Mycustom;
	auto str3 = "kkkkkkkkkk"_Mycustom;
	auto str4 = "aabbcc"_Mycustom;
	auto str5 = "ddeeff"_Mycustom;

	auto str6 = L"Hello World"_Mycustom;
	auto str7 = L"My C++"_Mycustom;
	auto str8 = L"kkkkkkkkkk"_Mycustom;
	auto str9 = L"aabbcc"_Mycustom;
	auto str10 = L"ddeeff"_Mycustom;

	//Test aa = Create();
	Test aa;
	Test bb = move(aa);


	return 0;
}

