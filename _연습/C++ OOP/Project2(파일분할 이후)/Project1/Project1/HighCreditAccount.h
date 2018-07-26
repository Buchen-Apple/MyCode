/*
* 업데이트 정보 : [2018년 1월 13일] 버전 0.7
*/

#pragma once
#ifndef __HIGH_CREDIT_ACCOUNT_H__
#define __HIGH_CREDIT_ACCOUNT_H__

#include "NormalAccount.h"
#include "BankingCommonDecl.h"

// 신용 계좌
class HighCreditAccount :public NormalAccount
{
	int CreditRare;
	CREDIT CreditRareenum;

public:
	// 생성자
	HighCreditAccount(char* name, int ID, int Money, int Bonus, int Rare);

	// 신용 계좌 입금
	virtual void Insert(int iTempMoney);

	// 정보 보여주기
	virtual void Show() const;
};


// ---------------------
// HighCreditAccount의 멤버함수 (엔터티 클래스) (NormalAccount 상속)
// ---------------------

//HighCreditAccount 안의 생성자
HighCreditAccount::HighCreditAccount(char* name, int ID, int Money, int Bonus, int Rare)
	:NormalAccount(name, ID, Money, Bonus), CreditRareenum((CREDIT)Rare)
{
	if (Rare == 1)
		CreditRare = 7;
	else if (Rare == 2)
		CreditRare = 4;
	else if (Rare == 3)
		CreditRare = 2;
}

// HighCreditAccount 안의 신용 계좌 입금 (가상함수)
void HighCreditAccount::Insert(int iTempMoney)
{
	NormalAccount::Insert(iTempMoney);	// 원금 및 이자 추가
	Account::Insert(iTempMoney * (CreditRare / 100.0));		// 신용 등급에 따른 추가
}

// HighCreditAccount 안의 정보 보여주기 (가상함수)
void HighCreditAccount::Show() const
{
	NormalAccount::Show();
	cout << "신용등급 : " << CreditRareenum << "등급" << endl;
}

#endif // !__HIGH_CREDIT_ACCOUNT_H__

