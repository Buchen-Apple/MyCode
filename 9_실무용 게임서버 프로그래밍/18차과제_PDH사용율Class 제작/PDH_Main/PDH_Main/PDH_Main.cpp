#include "pch.h"
#include "PDHClass\PDHCheck.h"

#include <strsafe.h>

using namespace Library_Jingyu;

int _tmain()
{
	CPDH Test;
	

	while (1)
	{
		Sleep(1000);

		Test.SetInfo();

		// 사용가능 메모리
		printf("AVA Mem : %.1f MByte\n", Test.Get_AVA_Mem());

		// 논 페이지드 메모리
		printf("Non Paged Mem : %.1f MByte\n", Test.Get_NonPaged_Mem() / 1024 / 1024);

		// 리시브
		printf("Recv : %.1f Byte\n", Test.Get_Net_Recv());

		// 샌드
		printf("Send : %.1f Byte\n", Test.Get_Net_Send());

		// 유저 커밋 메모리
		printf("User Commit : %.1f KByte\n\n\n", Test.Get_UserCommit() / 1024);


	}

	return 0;
}