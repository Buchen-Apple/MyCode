// Project1.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <windows.h>
#include <math.h>

int main()
{
	double dRadian = 0;
	double Check = 1;
	int iAngle = 5;
	int iFlag = 1;

	while (1)
	{
		// 라인 변경될 때 마다 라디안 재계산
		dRadian = iAngle * 3.14 / 180;

		// sin값과 비교할 Check변수 초기화. Check를 이용해 라인이 바뀔 때 마다 새로 체크한다.
		Check = 0;

		// Check의 값이 sine보다 작다면, Check값을 0.03씩 증가시키면서 별을 그린다.
		// 즉, Sine값 0.015마다 별이 2개
		for (; Check < sin(dRadian); Check += 0.03)
		{
			printf("**");
			Sleep(1);
		}

		fputs("\n", stdout);

		// angle이 90이 되면, 플래그를 off시킨다. 플래그가 off(0)되면 angle이 감소한다. 플래그가 on(1)일 때는 값이 증가한다.
		// 디폴트 on(1)로 시작한다.
		if (iAngle >= 90)
			iFlag = 0;
		else if (iAngle <= 0)
			iFlag = 1;

		// 플래그를 체크해 angle값을 증/감 시킨다.
		if (iFlag == 0)
			iAngle -= 5;
		else
			iAngle += 5;
	}

	return 0;
}