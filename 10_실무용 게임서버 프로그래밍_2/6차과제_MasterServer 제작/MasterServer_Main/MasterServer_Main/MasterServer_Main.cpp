#include "pch.h"
#include "MasterServer/MasterServer.h"

using namespace Library_Jingyu;

int _tmain()
{
	UINT64 BattleServerNO = 145;
	int RoomNo = 12485;

	UINT64 RoomKey = ((BattleServerNO << 32) | RoomNo);

	printf("%lld\n", RoomKey);

	int TempB = RoomKey >> 32;
	printf("BattleServerNo : %d\n", TempB);

	int TempR = (int)RoomKey;
	printf("RoomNo : %d\n", TempR);



	//CMatchServer_Lan MServer;

	//if (MServer.ServerStart() == false)
	//	return 0;

	//while (1)
	//{
	//	Sleep(1000);

	//	// 화면 출력

	//}

	return 0;   
}
