// MatchTestClient_Main.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "Network_Func.h"

int _tmain()
{
	cMatchTestDummy Dummy;

	// 1더미 제작 (100명)
	// 미리 DB에 들어가 있어야 함.. 세션키도 동일하게.
	Dummy.CreateDummpy(100);

	while (1)
	{
		// 1. 매치메이킹의 주소 알아오기
		

		// 2. 매치메이킹 접속 후 로그인 패킷 보내기



		// 3. 접속 종료
	}


	return 0;	
}

