// Login_Main.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include <Windows.h>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include "LoginServer\LoginServer.h"


using namespace Library_Jingyu;

int _tmain()
{
	timeBeginPeriod(1);

	system("mode con: cols=80 lines=26");

	srand((UINT)time(NULL));

	CLogin_NetServer LoginS;	

	if (LoginS.ServerStart() == false)
		return 0;


	while (1)
	{
		Sleep(1000);

		LoginS.ShowPrintf();
		
	}

	timeEndPeriod(1);
	

	return 0;
}
