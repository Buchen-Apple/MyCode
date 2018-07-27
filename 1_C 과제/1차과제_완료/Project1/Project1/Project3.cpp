// Project3.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"

int main()
{
	unsigned short val = 0;

	int input;
	int OnOffinput;
	int i;


	while (1)
	{
		fputs("비트 위치 : ",stdout);
		scanf("%d", &input);

		fputs("OFF/ON [0,1] : ", stdout);
		scanf("%d", &OnOffinput);

		// 1을 입력했다면, OR비트연산을 통해 하나라도 1이 있으면 모두 1로 모두 바꾼다.
		if (OnOffinput == 1)
			val = val | (OnOffinput << (input - 1));

		// 0을 입력했다면
		// 1. << 연산으로 위치로 이동
		// 2. ~ 연산으로 반전
		// 3. AND연산으로 둘 다 1인것만 1로 바꿈
		else
		{
			OnOffinput = 1;
			val = val & (~(OnOffinput << (input - 1)));
		}

		
		for (i = 15; i >= 0; --i)
		{
			printf("%d 번 Bit : ", i + 1);
			if ((val >> i) & 1)
				fputs("ON", stdout);
			else
				fputs("OFF", stdout);
			printf("\n");
		}


		printf("\n");
	}

	return 0;
}

