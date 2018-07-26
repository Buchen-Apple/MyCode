/*
* 업데이트 정보 : [2018년 1월 13일] 버전 0.7
*/

// 7. 파일 분할

#include "BankingCommonDecl.h"  
/*
아래 헤더파일들은 AccountHandler.cpp에 선언되어 있다. 그리고 AccountHandler.cpp는 AccountHandler.h를 인크루드 한다. 
때문에, 해당 BankingSystemMain.cpp 가 실행될 때는 이미 아래 헤더파일들이 다 인크루드 된 상태이다.
#include "Account.h"
#include "NormalAccount.h"
#include "HighCreditAccount.h"
*/
#include "AccountHandler.h"

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