// Project4.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"

int main()
{
	unsigned int val = 0;
	unsigned int mask;
	unsigned int numInput;
	int input;
	int i;
	int j;
	int sum;
	int square;

	while (1)
	{
		fputs("위치 <1~4> : ", stdout);
		scanf("%d", &input);

		fputs("값 [0~255] : ", stdout);
		scanf("%d", &numInput);
				
		//일단, 넣으려는 바이트의 값을 0으로 초기화
		// 1. 가장 우측의 1을 위치로 이동
		// 2. 반전
		// 3. and연산으로 해당 바이트의 값을 순서대로 0으로 만듦.
		for (i = 8; i > 0; --i)
		{			
			val = val & (~(1 << ((input * 8) - i)));
		}

		// val의 해당 바이트에다가 값 넣기.
		for (i = 8; i > 0; --i)
		{
			// numInput의 가장 우측 값 1개를 mask에 저장. (아래는 예시)
			// 0000 0000 0000 0000 0000 0000 1010 1101 numInput
			// 0000 0000 0000 0000 0000 0000 0000 0001 1
			// ------------------------------------------ and연산
			// 0000 0000 0000 0000 0000 0000 0000 0001 mask
			mask = numInput & 1;

			// 1비트 위치의 정보를 각 바이트의 첫 위치와 or연산.
			val = val | (mask << ((input * 8) -i));

			// numInput을 우측으로 한칸 밀어서 계산된 비트는 사라지게 함. 그래야 다음 비트(좌측에 있던 비트)를 계산할 수 있으니까.
			numInput = (numInput >> 1);
			
		}

		// 1바이트 당 크기 표시
		// 가장 우측 바이트(1번 바이트)부터 순차적으로 접근.
		for (i = 1; i < 5; ++i)
		{
			sum = 0;
			square = 0;
			for (j = 8; j > 0; --j)
			{
				if (j == 8)
					square = 1;
				else
				{
					square *= 2;
				}

				if (val & (1 << ((i * 8) -j)))
					sum += square;
				
			}
			printf("%d 번째 바이트 값 : %d\n", i, sum);
		}
		fputs("\n", stdout);


		//전체 바이트 크기 표시
		//가장 좌측(4번) 바이트부터 비트단위로 접근해 16진수의 1자리수 씩 표시
		fputs("전체 바이트 크기 : 0x", stdout);
		for (i = 8; i > 0; --i)
		{
			sum = 0;
			square = 0;
			for (j = 4; j > 0; --j)
			{
				if (j == 4)
					square = 1;
				else
				{
					square *= 2;
				}

				if (val & (1 << ((i * 4) - j)))
					sum += square;
			}

			printf("%x", sum);
		}
		fputs("\n\n", stdout);
	}

	return 0;
}

