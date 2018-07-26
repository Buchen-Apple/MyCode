// http_Request.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Http_Exchange\HTTP_Exchange.h"
#include <time.h>

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

using namespace rapidjson;
using namespace Library_Jingyu;

// 제이슨으로 각 컨텐츠 파싱하기
bool LoginFunc(TCHAR* RequestBody, UINT64* AccountNo, TCHAR* SessionKey, int KeySize);
bool SessionKeyFunc(TCHAR* RequestBody, UINT64* AccountNo, TCHAR* SessionKey, int KeySize);
bool ScoreGetFunc(TCHAR* RequestBody, UINT64* AccountNo, UINT64* Score);
bool ScoreUpdateFunc(TCHAR* RequestBody, UINT64* AccountNo, UINT64* New_Score);

#define WEB_SERVER_PORT		80

int _tmain()
{
	system("mode con: cols=180 lines=50");
	
	TCHAR RequestBody[2000];
	
	TCHAR Body[1000];			

	srand((unsigned)time(NULL));

	TCHAR ID[] = L"song";
	TCHAR Pass[] = L"1234";
	TCHAR Nick[] = L"SongTest";
	TCHAR Host[] = L"127.0.0.1";
	TCHAR* Path;


	// http 클래스 선언.
	HTTP_Exchange http_ex(Host, WEB_SERVER_PORT, false);
	


	// 1. 회원 가입
	ZeroMemory(RequestBody, sizeof(RequestBody));

	Path = (TCHAR*)L"4_Test/4-1MemCreate.php";
	swprintf_s(Body, _Mycountof(Body), L"0.1\r\n0.5\r\n0\r\n{\"id\":\"%s\", \"pass\":\"%s\", \"nick\":\"%s\"}", ID, Pass, Nick);

	if (http_ex.HTTP_ReqANDRes(Path, Body, RequestBody) == false)
		return 0;

	_tprintf(L"--------\nMemCreate!!\n\n%s\n\n\n", RequestBody);



	// 2. 로그인
	ZeroMemory(RequestBody, sizeof(RequestBody));

	Path = (TCHAR*)L"4_Test/4-1Login.php";
	swprintf_s(Body, _Mycountof(Body), L"0.1\r\n0.5\r\n0\r\n{\"id\":\"%s\", \"pass\":\"%s\"}", ID, Pass);

	if (http_ex.HTTP_ReqANDRes(Path, Body, RequestBody) == false)
		return 0;

	_tprintf(L"--------\nLogin\n\n%s\n", RequestBody);
	UINT64 AccountNo = 0;
	TCHAR SessionKey[70];
	if (LoginFunc(RequestBody, &AccountNo, SessionKey, 70) == false)
		return 0;

	_tprintf(L"-Account No : %lld\n-SessionKey : %s\n\n\n", AccountNo, SessionKey);



	// 3. 세션키 갱신
	ZeroMemory(RequestBody, sizeof(RequestBody));

	Path = (TCHAR*)L"4_Test/4-1SessionKey.php";
	swprintf_s(Body, _Mycountof(Body), L"0.1\r\n0.5\r\n1\r\n{\"no\":\"%lld\", \"key\":\"%s\"}", AccountNo, SessionKey);

	if (http_ex.HTTP_ReqANDRes(Path, Body, RequestBody) == false)
		return 0;
	

	_tprintf(L"--------\nReset_SessionKey\n\n%s\n", RequestBody);

	AccountNo = 0;
	SessionKey[0] = { 0, };
	if (SessionKeyFunc(RequestBody, &AccountNo, SessionKey, 70) == false)
		return 0;

	_tprintf(L"-Account No : %lld\n-New_SessionKey : %s\n\n\n", AccountNo, SessionKey);



	// 4. 스코어 확인
	ZeroMemory(RequestBody, sizeof(RequestBody));

	Path = (TCHAR*)L"/4_Test/4-1ScoreGet.php";
	swprintf_s(Body, _Mycountof(Body), L"0.1\r\n0.5\r\n1\r\n{\"no\":\"%lld\", \"key\":\"%s\"}", AccountNo, SessionKey);

	if (http_ex.HTTP_ReqANDRes(Path, Body, RequestBody) == false)
		return 0;

	_tprintf(L"--------\nGet_Score\n\n%s\n", RequestBody);

	AccountNo = 0;
	UINT64 Score = 0;
	if (ScoreGetFunc(RequestBody, &AccountNo, &Score) == false)
		return 0;

	_tprintf(L"-Account No : %lld\n-Score : %lld\n\n\n", AccountNo, Score);


	// 5. 스코어 갱신
	ZeroMemory(RequestBody, sizeof(RequestBody));

	Path = (TCHAR*)L"4_Test/4-1ScoreUpdate.php";
	swprintf_s(Body, _Mycountof(Body), L"0.1\r\n0.5\r\n1\r\n{\"no\":\"%lld\", \"key\":\"%s\", \"new_score\":\"%d\"}", AccountNo, SessionKey, rand() % 1024);

	if (http_ex.HTTP_ReqANDRes(Path, Body, RequestBody) == false)
		return 0;

	_tprintf(L"--------\nUpdate_Score\n\n%s\n", RequestBody);

	AccountNo = 0;
	UINT64 Newe_Score = 0;
	if (ScoreUpdateFunc(RequestBody, &AccountNo, &Newe_Score) == false)
		return 0;

	_tprintf(L"-Account No : %lld\n-New_Score : %lld\n\n\n", AccountNo, Newe_Score);


	// 6. 스코어 확인
	ZeroMemory(RequestBody, sizeof(RequestBody));

	Path = (TCHAR*)L"4_Test/4-1ScoreGet.php";
	swprintf_s(Body, _Mycountof(Body), L"0.1\r\n0.5\r\n1\r\n{\"no\":\"%lld\", \"key\":\"%s\"}", AccountNo, SessionKey);

	if (http_ex.HTTP_ReqANDRes(Path, Body, RequestBody) == false)
		return 0;

	_tprintf(L"--------\nGet_Score\n\n%s\n", RequestBody);

	AccountNo = 0;
	Score = 0;
	if (ScoreGetFunc(RequestBody, &AccountNo, &Score) == false)
		return 0;

	_tprintf(L"-Account No : %lld\n-Score : %lld\n\n\n", AccountNo, Score);
	

    return 0;
}

// RequestBody를 파싱해서 Key와 accountNo를 넣어준다.
bool LoginFunc(TCHAR* RequestBody, UINT64 *AccountNo, TCHAR* SessionKey, int KeySize)
{	
	// Json데이터 파싱하기 (이미 UTF-16을 넣는중)
	GenericDocument<UTF16<>> Doc;
	Doc.Parse(RequestBody);

	// code, msg, no, key 순서대로 빼온다.
	UINT64 ResultCode;
	const TCHAR* ResultMsg;
	const TCHAR* ResultNo;
	const TCHAR* ResultKey;

	ResultCode = Doc[_T("resultCode")].GetInt64();
	ResultMsg = Doc[_T("resultMsg")].GetString();
	ResultNo = Doc[_T("accountNo")].GetString();
	ResultKey = Doc[_T("sessionKey")].GetString();

	// ResultCode가 1이 아니면 에러임.
	if (ResultCode != 1)
	{
		_tprintf(L"LoginFunc() Error : %s(%lld)\n", ResultMsg, ResultCode);
		return false;
	}

	*AccountNo = _ttoi(ResultNo);
	_tcscpy_s(SessionKey, KeySize, ResultKey);

	return true;
}

// RequestBody를 파싱해서 Key와 accountNo를 넣어준다.
bool SessionKeyFunc(TCHAR* RequestBody, UINT64* AccountNo, TCHAR* SessionKey, int KeySize)
{
	// Json데이터 파싱하기 (이미 UTF-16을 넣는중)
	GenericDocument<UTF16<>> Doc;
	Doc.Parse(RequestBody);

	// code, msg, no, key 순서대로 빼온다.
	UINT64 ResultCode;
	const TCHAR* ResultMsg;
	const TCHAR* ResultNo;
	const TCHAR* ResultKey;

	ResultCode = Doc[_T("resultCode")].GetInt64();
	ResultMsg = Doc[_T("resultMsg")].GetString();
	ResultNo = Doc[_T("accountNo")].GetString();
	ResultKey = Doc[_T("sessionKey")].GetString();

	// ResultCode가 1이 아니면 에러임.
	if (ResultCode != 1)
	{
		_tprintf(L"SessionKeyFunc() Error : %s(%lld)\n", ResultMsg, ResultCode);
		return false;
	}

	*AccountNo = _ttoi(ResultNo);
	_tcscpy_s(SessionKey, KeySize, ResultKey);

	return true;

}

// RequestBody를 파싱해서 accountNo와 현재 스코어를 넣어준다.
bool ScoreGetFunc(TCHAR* RequestBody, UINT64* AccountNo, UINT64* Score)
{
	// Json데이터 파싱하기 (이미 UTF-16을 넣는중)
	GenericDocument<UTF16<>> Doc;
	Doc.Parse(RequestBody);

	// code, msg, no, Score순서대로 빼온다.
	UINT64 ResultCode;
	const TCHAR* ResultMsg;
	const TCHAR* ResultNo;
	const TCHAR* ResultScore;

	ResultCode = Doc[_T("resultCode")].GetInt64();
	ResultMsg = Doc[_T("resultMsg")].GetString();
	ResultNo = Doc[_T("accountNo")].GetString();
	ResultScore = Doc[_T("Score")].GetString();

	// ResultCode가 1이 아니면 에러임.
	if (ResultCode != 1)
	{
		_tprintf(L"ScoreGetFunc() Error : %s(%lld)\n", ResultMsg, ResultCode);
		return false;
	}

	*AccountNo = _ttoi(ResultNo);
	*Score = _ttoi(ResultScore);

	return true;

}

// RequestBody를 파싱해서 accountNo와 갱신된 스코어를 넣어준다.
bool ScoreUpdateFunc(TCHAR* RequestBody, UINT64* AccountNo, UINT64* New_Score)
{
	// Json데이터 파싱하기 (이미 UTF-16을 넣는중)
	GenericDocument<UTF16<>> Doc;
	Doc.Parse(RequestBody);

	// code, msg, no, New_Score순서대로 빼온다.
	UINT64 ResultCode;
	const TCHAR* ResultMsg;
	const TCHAR* ResultNo;
	const TCHAR* ResultScore;

	ResultCode = Doc[_T("resultCode")].GetInt64();
	ResultMsg = Doc[_T("resultMsg")].GetString();
	ResultNo = Doc[_T("accountNo")].GetString();
	ResultScore = Doc[_T("new_Score")].GetString();

	// ResultCode가 1이 아니면 에러임.
	if (ResultCode != 1)
	{
		_tprintf(L"ScoreGetFunc() Error : %s(%lld)\n", ResultMsg, ResultCode);
		return false;
	}

	*AccountNo = _ttoi(ResultNo);
	*New_Score = _ttoi(ResultScore);

	return true;

}

