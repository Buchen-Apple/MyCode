/*
* 업데이트 정보 : [2018년 1월 13일] 버전 0.7
*/

#include "BankingCommonDecl.h"
#include "Account.h"

// ---------------------
// Account의 멤버함수
// ---------------------

// Account 안의 생성자 (엔터티 클래스)
Account::Account(char* name, int ID, int Money)
	:m_iID(ID), m_iMoney(Money)
{
	size_t len = strlen(name) + 1;
	m_cName = new char[len];
	strcpy_s(m_cName, len, name);
}

// Account 안의 복사 생성자
Account::Account(const Account& ref)
	:m_iID(ref.m_iID), m_iMoney(ref.m_iMoney)
{
	size_t len = strlen(ref.m_cName) + 1;
	m_cName = new char[len];
	strcpy_s(m_cName, len, ref.m_cName);
}

// Account 안의 대입 연산자
Account& Account::operator=(const Account& ref)
{
	m_iID = ref.m_iID;
	m_iMoney = ref.m_iMoney;

	delete[] m_cName;
	m_cName = new char[strlen(ref.m_cName) + 1];
	strcpy_s(m_cName, strlen(ref.m_cName) + 1, ref.m_cName);

	return *this;
}

// Account 안의 ID 반환
int Account::GetID() const
{
	return m_iID;
}

// Account 안의 입금
void Account::Insert(int iTempMoney)
{
	m_iMoney += iTempMoney;
}

// Account 안의 잔액 체크 및 출금
bool Account::Out(int iTempMoney)
{
	// 현재 보유 돈이 iTempMoney보다 적음. 즉, 출금 불가.
	if (m_iMoney < iTempMoney)
		return false;

	// 보유액이 충분하면 출금 완료.
	m_iMoney -= iTempMoney;
	return true;
}

// Account 안의 정보 보여주기
void Account::Show() const
{
	cout << "계좌 ID : " << m_iID << endl;
	cout << "이름 : " << m_cName << endl;
	cout << "잔액 : " << m_iMoney << endl;
}

// Account 안의 소멸자
Account::~Account()
{
	delete[] m_cName;
}