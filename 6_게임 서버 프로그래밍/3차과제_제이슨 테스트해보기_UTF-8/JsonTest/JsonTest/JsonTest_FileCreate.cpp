// JsonTest.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"
#include <iostream>
#include "Header.h"

using namespace rapidjson;

// 파일 생성 후 데이터 쓰기 함수
bool FileCreate_UTF8(const char* FileName, const char* pJson, size_t StringSize);

int main()
{
	StringBuffer StringJson;
	Writer< StringBuffer, UTF16<> > Writer(StringJson);

	stUser* UserSave[2];

	stUser* NewUser1 = new stUser;
	NewUser1->m_AccountID = 1;
	TCHAR Nick1[NICK_MAX_LEN] = L"송진규1";
	_tcscpy_s(NewUser1->m_NickName, _countof(Nick1), Nick1);

	stUser* NewUser2 = new stUser;
	NewUser2->m_AccountID = 1;
	TCHAR Nick2[NICK_MAX_LEN] = L"송진규규규규2";
	_tcscpy_s(NewUser2->m_NickName, _countof(Nick2), Nick2);

	UserSave[0] = NewUser1;
	UserSave[1] = NewUser2;

	Writer.StartObject();
	Writer.String(L"Account");
	Writer.StartArray();
	for(int i=0; i<2; ++i)
	{
		Writer.StartObject();
		Writer.String(L"AccountNo");
		Writer.Uint64(UserSave[i]->m_AccountID);
		Writer.String(L"NickName");
		Writer.String(UserSave[i]->m_NickName);
		Writer.EndObject();
	}
	Writer.EndArray();
	Writer.EndObject();

	// pJson에는 UTF-8의 형태로 저장된다.
	const char* pJson = StringJson.GetString();
	size_t Size = StringJson.GetSize();	
	
	// 파일 생성 후 데이터 생성 및 쓰기
	if(FileCreate_UTF8("json_test.txt", pJson, Size) == true)
		fputs("Json 파일 생성 성공!\n", stdout);

	return 0;
}

// 파일 생성 후 데이터 쓰기 함수
bool FileCreate_UTF8(const char* FileName, const char* pJson, size_t StringSize)
{
	FILE* fp;			// 파일 스트림
	size_t iFileCheck;	// 파일 오픈 여부 체크, 파일의 사이즈 저장. 총 2개의 옹도

	// 파일 생성
	iFileCheck = _tfopen_s(&fp, _T("json_test.txt"), _T("wt"));
	//iFileCheck = fopen_s(&fp, FileName, "wb, ccs=UTF-8");
	if (iFileCheck != 0)
	{
		fputs("fopen 에러발생!\n", stdout);
		return false;
	}

	// 파일에 데이터 쓰기
	iFileCheck = fwrite(pJson, 1, StringSize, fp);
	if (iFileCheck != StringSize)
	{
		fputs("fwrite 에러발생!\n", stdout);
		return false;
	}

	fclose(fp);
	return true;
}


