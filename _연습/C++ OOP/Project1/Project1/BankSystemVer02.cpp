// 2. 구조체를 클래스로 변경

#include <iostream>
#include "conio.h"
#include <cstring>

using namespace std;

#define ACCOUNT_COUNT 50

enum
{
	CREATE = 1, INSERT, OUT, ALL_SHOW, EXIT
};

class Account
{
	char* m_cName;
	int m_iID;
	int m_iMoney;
public:
	// 생성자
	Account(char* name, int ID, int Money)
		:m_iID(ID), m_iMoney(Money)
	{
		size_t len = strlen(name) + 1;
		m_cName = new char[len];
		strcpy_s(m_cName, len, name);
	}

	// ID 반환
	int GetID()
	{
		return m_iID;
	}

	// 입금
	void Insert(int iTempMoney)
	{
		m_iMoney += iTempMoney;
	}

	// 잔액 체크 및 출금
	bool Out(int iTempMoney)
	{
		// 현재 보유 돈이 iTempMoney보다 적음. 즉, 출금 불가.
		if (m_iMoney < iTempMoney)
			return false;
		
		// 보유액이 충분하면 출금 완료.
		m_iMoney -= iTempMoney;
		return true;
	}

	// 정보 보여주기
	void Show()
	{		
		cout << "계좌 ID : " << m_iID << endl;
		cout << "이름 : " << m_cName << endl;
		cout << "잔액 : " << m_iMoney << endl << endl;
	}

	~Account()
	{
		delete[] m_cName;
	}
};

// 계좌 정보를 저장하는 객체 포인터 배열
Account* arrArc[ACCOUNT_COUNT];

// 현재 계좌 수 카운트
int iAccountCurCount = 0;

// 계좌 개설 함수
void CreateFunc();

// 입금 함수
void InsertFunc();

// 출금 함수
void OutFunc();

// 계좌 정보 전체 출력 함수
void ShowFunc();

int main()
{
	int iInput;

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

		// 프로그램 종료 플래그
		if (iInput == EXIT)
			break;

		switch (iInput)
		{
		case CREATE:
			CreateFunc();
			_getch();

			break;
		case INSERT:
			InsertFunc();
			_getch();

			break;
		case OUT:
			OutFunc();
			_getch();

			break;
		case ALL_SHOW:
			ShowFunc();
			_getch();

			break;
		}

	}

	return 0;
}

//계좌 개설 함수
void CreateFunc()
{
	int iTempMoney;
	int iTempID;
	char cTempName[30];

	cout << "[계좌 개설]" << endl;
	cout << "계좌 ID : ";
	cin >> iTempID;

	cout << "이름 : ";
	cin >> cTempName;

	cout << "입금 액 : ";
	cin >> iTempMoney;

	arrArc[iAccountCurCount] = new Account(cTempName, iTempID, iTempMoney);

	iAccountCurCount++;

	cout << "계좌 개설 완료!" << endl;
}

// 입금 함수
void InsertFunc()
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

// 출금 함수
void OutFunc()
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

// 계좌 정보 전체 출력 함수
void ShowFunc()
{
	cout << "[계좌 정보 전체 출력]" << endl;

	for (int i = 0; i < iAccountCurCount; ++i)
	{
		cout << "-----------------------" << endl;
		arrArc[i]->Show();		
	}

	cout << "모두 출력 완료!" << endl;
}