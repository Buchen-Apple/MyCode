// PDH_Main.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "PDHClass\PDHCheck.h"

#include <Pdh.h>
#pragma comment(lib, "Pdh.lib")

using namespace Library_Jingyu;

int _tmain()
{
	// 쿼리 스트링
	/*

	L"\\Process(이름)\\Thread Count"		// 스레드
	L"\\Process(이름)\\Working Set"		// 사용 메모리

	L"\\Memory\\Available MBytes"		// 사용가능

	L"\\Memory\\Pool Nonpaged Bytes"	// 논페이지드

	L"\\Network Interface(*)\\Bytes Received/sec"
	L"\\Network Interface(*)\\Bytes Sent/sec"

	*/

	// 선언
	CPDH cPdh;

	// 논 페이지드 풀 알아오기
	cPdh.Query(L"\\Memory\\Pool Nonpaged Bytes\0");
	double dResult = cPdh.GetResult();
	printf("Pool Nonpaged Bytes : %.1f\n", dResult);

	// 사용가능 메모리 알아오기
	cPdh.Query(L"\\Memory\\Available MBytes\0");
	dResult = cPdh.GetResult();
	printf("Available MBytes : %.1f\n", dResult);



	return 0;
}

