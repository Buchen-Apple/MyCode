#include "pch.h"
#include "MasterServer/MasterServer.h"

#include <conio.h>

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace Library_Jingyu;

int _tmain()
{
	timeBeginPeriod(1);

	system("mode con: cols=80 lines=37");

	CMatchServer_Lan MServer;

	if (MServer.ServerStart() == false)
		return 0;

	while (1)
	{
		Sleep(1000);

		//if (_kbhit())
		//{
		//	int Key = _getch();

		//	// q를 누르면 채팅서버 종료
		//	if (Key == 'Q' || Key == 'q')
		//	{
		//		MServer.ServerStop();
		//		break;
		//	}

		//}

		MServer.ShowPrintf();
	}

	timeEndPeriod(1);
	return 0;   
}
