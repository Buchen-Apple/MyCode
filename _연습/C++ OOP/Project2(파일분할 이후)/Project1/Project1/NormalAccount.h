/*
* 업데이트 정보 : [2018년 1월 13일] 버전 0.7
*/

// 선언과 정의가 같이 있다.
#pragma once
#ifndef __NORMAL_ACCOUNT_H__
#define _NORMAL_ACCOUNT_H__

#include "Account.h"

// 보통 계좌
class NormalAccount :public Account
{
	int BonusPercent;

public:
	// 생성자
	NormalAccount(char* name, int ID, int Money, int Bonus);

	// 보통 계좌 입금
	virtual void Insert(int iTempMoney);

	// 정보 보여주기
	virtual void Show() const;
};

// ---------------------
// NormalAccount의 멤버함수 (엔터티 클래스)  (Account 상속)
// ---------------------

// NormalAccount안의 생성자
NormalAccount::NormalAccount(char* name, int ID, int Money, int Bonus)
	:Account(name, ID, Money), BonusPercent(Bonus)
{}

// NormalAccount안의 보통 계좌 입금 (가상함수)
void NormalAccount::Insert(int iTempMoney)
{
	Account::Insert(iTempMoney); // 원금 추가
	Account::Insert(iTempMoney * (BonusPercent / 100.0));	// 이자 추가
}

// NormalAccount안의 정보 보여주기 (가상함수)
void NormalAccount::Show() const
{
	Account::Show();
	cout << "이자율 : " << BonusPercent << "%" << endl;
}


#endif // !__NORMAL_ACCOUNT_H__

