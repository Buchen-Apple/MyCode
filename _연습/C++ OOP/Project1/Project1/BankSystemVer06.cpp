// 6. 계좌를 보통계좌 , 신용계좌로 나눔. 이에 따른 상속 및 가상함수 적용

#include <iostream>
#include "conio.h"
#include <cstring>
#include <windows.h>

using namespace std;

#define ACCOUNT_COUNT 50

enum
{
	CREATE = 1, INSERT, OUT_MONEY, ALL_SHOW, EXIT
};

enum CREDIT
{
	A = 7, B = 4, C = 2
};

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

// 핸들러
class AccountHandler
{
	Account* arrArc[ACCOUNT_COUNT]; // 계좌 정보를 저장하는 객체 포인터 배열
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

int main()
{
	int iInput;
	AccountHandler AcHandler;

	while (1)
	{
		system("cls");

		cout << "----Menu----" << endl;
		cout << "1. 계좌 개설" << endl;
		cout << "2. 입 금" << endl;
		cout << "3. 출 금" << endl;
		cout << "4. 계좌정보 전체 출력" << endl;
		cout << "5. 프로그램 종료" << endl;
		cin >> iInput;

		switch (iInput)
		{
		case CREATE:
			AcHandler.CreateAccount();
			_getch();

			break;
		case INSERT:
			AcHandler.InsertFunc();
			_getch();

			break;
		case OUT_MONEY:
			AcHandler.OutFunc();
			_getch();

			break;
		case ALL_SHOW:
			AcHandler.ShowFunc();
			_getch();

			break;
		case EXIT:
			return 0;

		default:
			cout << "없는 명령입니다." << endl;
			_getch();

			break;
		}

	}

	return 0;
}

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
	cout << "신용등급 : " << CreditRareenum << "등급" <<  endl;
}


// ---------------------
// AccountHandler의 멤버함수 (핸들러 클래스)
// ---------------------

// AccountHandler 안의 계좌 개설 함수
void AccountHandler::CreateAccount()
{
	int iInput2;
	cout << "[계좌종류선택]" << endl;
	cout << "1. 보통예금계좌  2. 신용신뢰계좌" << endl;
	cin >> iInput2;
	if (iInput2 == 1)
		CreateFunc1();
	else
		CreateFunc2();
}

// AccountHandler 안의 보통 계좌 개설 함수
void AccountHandler::CreateFunc1()
{
	int iTempMoney;
	int iTempID;
	char cTempName[30];
	int Percent;

	cout << "[계좌 개설]" << endl;
	cout << "계좌 ID : ";
	cin >> iTempID;

	cout << "이름 : ";
	cin >> cTempName;

	cout << "입금 액 : ";
	cin >> iTempMoney;

	cout << "이자율 : ";
	cin >> Percent;

	arrArc[iAccountCurCount] = new NormalAccount(cTempName, iTempID, iTempMoney, Percent);

	iAccountCurCount++;

	cout << "계좌 개설 완료!" << endl;
}

// AccountHandler 안의 신용 계좌 개설 함수
void AccountHandler::CreateFunc2()
{
	int iTempMoney;
	int iTempID;
	char cTempName[30];
	int Percent;
	int Rare;


	cout << "[계좌 개설]" << endl;
	cout << "계좌 ID : ";
	cin >> iTempID;

	cout << "이름 : ";
	cin >> cTempName;

	cout << "입금 액 : ";
	cin >> iTempMoney;

	cout << "이자율 : ";
	cin >> Percent;

	cout << "신용 등급(1toA, 2toB, 3toC) : ";
	cin >> Rare;

	arrArc[iAccountCurCount] = new HighCreditAccount(cTempName, iTempID, iTempMoney, Percent, Rare);

	iAccountCurCount++;

	cout << "계좌 개설 완료!" << endl;
}

// AccountHandler 안의 입금 함수
void AccountHandler::InsertFunc()
{
	int iTempID;
	int iTempMoney;
	int i;

	cout << "[입 금]" << endl;
	cout << "계좌 ID : ";
	cin >> iTempID;

	for (i = 0; i < iAccountCurCount; ++i)
	{
		if (arrArc[i]->GetID() == iTempID)
			break;
	}

	if (i >= iAccountCurCount)
	{
		cout << "없는 계좌입니다." << endl;
		return;
	}

	cout << "입금 액 : ";
	cin >> iTempMoney;
	arrArc[i]->Insert(iTempMoney);

	cout << "입금 완료!" << endl;
}

// AccountHandler 안의 출금 함수
void AccountHandler::OutFunc()
{
	int iTempID;
	int iTempMoney;
	int i;

	cout << "[출 금]" << endl;
	cout << "계좌 ID : ";
	cin >> iTempID;

	for (i = 0; i < iAccountCurCount; ++i)
	{
		if (arrArc[i]->GetID() == iTempID)
			break;
	}

	if (i >= iAccountCurCount)
	{
		cout << "없는 계좌입니다." << endl;
		return;
	}

	cout << "출금 액 : ";
	cin >> iTempMoney;

	if (arrArc[i]->Out(iTempMoney) == false)
	{
		cout << "잔액이 부족합니다." << endl;
		return;
	}
	else
	{
		cout << "출금 완료!" << endl;
	}

}

// AccountHandler 안의 계좌 정보 전체 출력 함수
void AccountHandler::ShowFunc()
{
	cout << "[계좌 정보 전체 출력]" << endl;

	for (int i = 0; i < iAccountCurCount; ++i)
	{
		cout << "-----------------------" << endl;
		arrArc[i]->Show();
		cout << endl;
	}

	cout << "모두 출력 완료!" << endl;
}
