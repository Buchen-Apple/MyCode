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
bool FileCreate_UTF16(const TCHAR* FileName, const TCHAR* pJson, size_t StringSize);

int main()
{
	GenericStringBuffer<UTF16<>> StringJson;
	Writer< GenericStringBuffer<UTF16<>>, UTF16<>, UTF16<> > Writer(StringJson);

	stUser* UserSave[2];

	stUser* NewUser1 = new stUser;
	NewUser1->m_AccountID = 1;
	TCHAR Nick1[NICK_MAX_LEN] = L"송진규1";
	_tcscpy_s(NewUser1->m_NickName, _countof(Nick1), Nick1);

	stUser* NewUser2 = new stUser;
	NewUser2->m_AccountID = 2;
	TCHAR Nick2[NICK_MAX_LEN] = L"송진규규규규2";
	_tcscpy_s(NewUser2->m_NickName, _countof(Nick2), Nick2);

	UserSave[0] = NewUser1;
	UserSave[1] = NewUser2;

	Writer.StartObject();
	Writer.String(L"Account");
	Writer.StartArray();
	for (int i = 0; i<2; ++i)
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

	// tpJson에는 UTF-16의 형태로 저장된다.
	const TCHAR* tpJson = StringJson.GetString();
	size_t Size = StringJson.GetSize();

	// 파일 생성 후 데이터 생성 및 쓰기
	if (FileCreate_UTF16(_T("json_test.txt"), tpJson, Size) == true)
		fputs("Json 파일 생성 성공!\n", stdout);

	return 0;


	//StringBuffer StringJson;
	//Writer< StringBuffer, UTF16<> > Writer(StringJson);

	//stUser* UserSave[2];

	//stUser* NewUser1 = new stUser;
	//NewUser1->m_AccountID = 1;
	//TCHAR Nick1[NICK_MAX_LEN] = L"송진규1";
	//_tcscpy_s(NewUser1->m_NickName, _countof(Nick1), Nick1);

	//stUser* NewUser2 = new stUser;
	//NewUser2->m_AccountID = 1;
	//TCHAR Nick2[NICK_MAX_LEN] = L"송진규규규규2";
	//_tcscpy_s(NewUser2->m_NickName, _countof(Nick2), Nick2);

	//UserSave[0] = NewUser1;
	//UserSave[1] = NewUser2;

	//Writer.StartObject();
	//Writer.String(L"Account");
	//Writer.StartArray();
	//for(int i=0; i<2; ++i)
	//{
	//	Writer.StartObject();
	//	Writer.String(L"AccountNo");
	//	Writer.Uint64(UserSave[i]->m_AccountID);
	//	Writer.String(L"NickName");
	//	Writer.String(UserSave[i]->m_NickName);
	//	Writer.EndObject();
	//}
	//Writer.EndArray();
	//Writer.EndObject();

	//// pJson에는 UTF-8의 형태로 저장된다.
	//const char* pJson = StringJson.GetString();
	//
	//// pJson의 UTF-8을 UTF-16으로 변환.
	//StringStream source(pJson);
	//GenericStringBuffer<UTF16<>> target;

	//bool hasError = true;
	//while (source.Peek() != '\0')
	//{
	//	if (!Transcoder< UTF8<>, UTF16<> >::Transcode(source, target))
	//	{
	//		hasError = false;
	//		break;
	//	}
	//}
	//
	//if (!hasError)
	//{
	//	fputs("UTF-8을 UTF-16으로 변환 중 문제 발생\n", stdout);
	//	return 0;
	//}

	//const TCHAR* tpJson = target.GetString();

	//size_t Size = target.GetSize();
	//
	//// 파일 생성 후 데이터 생성 및 쓰기
	//if(FileCreate_UTF16(_T("json_test.txt"), tpJson, Size) == true)
	//	fputs("Json 파일 생성 성공!\n", stdout);

	//return 0;
}

// 파일 생성 후 데이터 쓰기 함수
bool FileCreate_UTF16(const TCHAR* FileName, const TCHAR* tpJson, size_t StringSize)
{
	FILE* fp;			// 파일 스트림
	size_t iFileCheck;	// 파일 오픈 여부 체크, 파일의 사이즈 저장. 총 2개의 옹도
	
	// 파일 생성 (UTF-16 리틀엔디안)
	iFileCheck = _tfopen_s(&fp, FileName, _T("wt, ccs=UTF-16LE"));
	if (iFileCheck != 0)
	{
		fputs("fopen 에러발생!\n", stdout);
		return false;
	}

	// 파일에 데이터 쓰기
	iFileCheck = fwrite(tpJson, 1, StringSize, fp);
	if (iFileCheck != StringSize)
	{
		fputs("fwrite 에러발생!\n", stdout);
		return false;
	}

	fclose(fp);
	return true;
}


