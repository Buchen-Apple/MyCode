/*
* 업데이트 정보 : [2018년 1월 13일] 버전 0.7
*/

#include "BankingCommonDecl.h"

#include "AccountHandler.h"
#include "Account.h"
#include "NormalAccount.h"
#include "HighCreditAccount.h"

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
