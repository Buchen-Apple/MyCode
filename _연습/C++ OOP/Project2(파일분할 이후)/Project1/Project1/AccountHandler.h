/*
* 업데이트 정보 : [2018년 1월 13일] 버전 0.7
*/

#pragma once
#ifndef __ACCOUNT__HANDLER_H__
#define __ACCOUNT__HANDLER_H__

#include "Account.h"
#include "BankingCommonDecl.h"
#include "AccountArray.h"


// 핸들러
class AccountHandler
{
	ArrayBoundCheck* arrArc[ACCOUNT_COUNT]; // 계좌 정보를 저장하는 객체 포인터 배열
	int iAccountCurCount;	// 계좌 수 저장 변수

public:
	// 생성자
	AccountHandler()
		: iAccountCurCount(0) {}

protected:
	// 보통 계좌 개설
	void CreateFunc1();

	// 신용 계좌 개설
	void CreateFunc2();

public:
	// 계좌 개설 함수
	void CreateAccount();

	// 입금 함수
	void InsertFunc();

	// 출금 함수
	void OutFunc();

	// 계좌 정보 전체 출력 함수
	void ShowFunc();

	// 소멸자
	~AccountHandler()
	{
		for (int i = 0; i < iAccountCurCount; ++i)
		{
			delete arrArc[i];
		}
	}
};

#endif // !__ACCOUNT__HANDLER_H__

