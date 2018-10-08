// MMOServer_Main.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"

#include "MMOGameServer\CGameServer.h"
#include <conio.h>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace Library_Jingyu;

int _tmain()
{
	timeBeginPeriod(1);

	system("mode con: cols=60 lines=23");

	CGameServer GameS;

	GameS.GameServerStart();

	while (1)
	{
		Sleep(1000);

		//if (_kbhit())
		//{
		//	int Key = _getch();

		//	// q를 누르면 게임서버 종료
		//	if (Key == 'Q' || Key == 'q')
		//	{
		//		GameS.GameServerStop();
		//		break;
		//	}

		//}

		GameS.ShowPrintf();
	}

	timeEndPeriod(1);

	return 0;
}

