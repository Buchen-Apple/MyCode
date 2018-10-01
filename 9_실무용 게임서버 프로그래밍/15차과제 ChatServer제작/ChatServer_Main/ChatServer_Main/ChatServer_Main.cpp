// NetServer_Main.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "ChatServer\ChatServer.h"

#include <conio.h>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace Library_Jingyu;



int _tmain()
{
	timeBeginPeriod(1);

	system("mode con: cols=80 lines=44");

	srand((UINT)time(NULL));

	// 채팅서버
	CChatServer ChatS;

	// 채팅서버 시작
	if (ChatS.ServerStart() == false)
	{
		printf("Server OpenFail...\n");
		return 0;
	}

	while (1)
	{
		Sleep(1000);

		//if (_kbhit())
		//{
		//	int Key = _getch();

		//	// q를 누르면 채팅서버 종료
		//	if (Key == 'Q' || Key == 'q')
		//	{
		//		ChatS.ServerStop();
		//		break;
		//	}

		//}

		ChatS.ShowPrintf();

	}

	

	timeEndPeriod(1);

	return 0;
}