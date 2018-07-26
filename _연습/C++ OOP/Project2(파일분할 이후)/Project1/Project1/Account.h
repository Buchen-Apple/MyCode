#pragma once
#ifndef __ACCOUNT_H__
#define __ACCOUNT_H__

/*
 * 업데이트 정보 : [2018년 1월 13일] 버전 0.7
 */

// 계좌 기본 정보
class Account
{
	char* m_cName;
	int m_iID;
	int m_iMoney;
public:
	// 생성자
	Account(char* name, int ID, int Money);

	// 복사 생성자
	Account(const Account& ref);

	// 대입 연산자
	Account& operator=(const Account&);

	// ID 반환
	int GetID() const;

	// 입금
	virtual void Insert(int iTempMoney);

	// 잔액 체크 및 출금
	bool Out(int iTempMoney);

	// 정보 보여주기
	virtual void Show() const;

	// 소멸자
	~Account();
};
#endif // !__ACCOUNT_H__

