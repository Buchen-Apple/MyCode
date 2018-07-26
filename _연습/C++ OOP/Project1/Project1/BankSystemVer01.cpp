// 1단계 : 틀 구성

#include <iostream>
#include "conio.h"

using namespace std;

#define ACCOUNT_COUNT 50

enum 
{
	CREATE=1, INSERT, OUT, ALL_SHOW, EXIT
};

typedef struct
{
	char m_cName[30];
	int m_iID;
	int m_iMoney;

} Account;

// 계좌를 저장하는 구조체 배열
Account arrArc[ACCOUNT_COUNT];

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

	cout << "[계좌 개설]" << endl;
	cout << "계좌 ID : ";
	cin >> arrArc[iAccountCurCount].m_iID;

	cout << "이름 : ";
	cin >> arrArc[iAccountCurCount].m_cName;

	cout << "입금 액 : ";
	cin >> iTempMoney;
	arrArc[iAccountCurCount].m_iMoney += iTempMoney;

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
		if (arrArc[i].m_iID == iTempID)
			break;
	}

	if (i >= iAccountCurCount)
	{
		cout << "없는 계좌입니다." << endl;
		return;
	}

	cout << "입금 액 : ";
	cin >> iTempMoney;
	arrArc[i].m_iMoney += iTempMoney;

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
		if (arrArc[i].m_iID == iTempID)
			break;
	}

	if (i >= iAccountCurCount)
	{
		cout << "없는 계좌입니다." << endl;
		return;
	}

	cout << "출금 액 : ";
	cin >> iTempMoney;

	if (arrArc[i].m_iMoney < iTempMoney)
	{
		cout << "잔액이 부족합니다." << endl;
		return;
	}

	arrArc[i].m_iMoney -= iTempMoney;

	cout << "출금 완료!" << endl;

}

// 계좌 정보 전체 출력 함수
void ShowFunc()
{
	cout << "[계좌 정보 전체 출력]" << endl;

	for (int i = 0; i < iAccountCurCount; ++i)
	{
		cout << "-----------------------" << endl;
		cout << "계좌 ID : " << arrArc[i].m_iID << endl;
		cout << "이름 : " << arrArc[i].m_cName << endl;
		cout << "잔액 : " << arrArc[i].m_iMoney << endl << endl;
	}

	cout << "모두 출력 완료!" << endl;
}