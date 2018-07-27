// 버블정렬
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int iCheck = 0;
	int i;
	int iTemp;
	int iWhileExit = 9;

	int Arr[] = { 10, 1, 9, 2, 8, 3, 7, 4, 6, 5 };
	

	while (iWhileExit > 0)
	{
		for (iCheck = 0; iCheck < iWhileExit; iCheck++)
		{
			// 값 체크
			if (Arr[iCheck] > Arr[iCheck + 1])
			{
				iTemp = Arr[iCheck];
				Arr[iCheck] = Arr[iCheck + 1];
				Arr[iCheck + 1] = iTemp;
			}

			// 모든 배열의 값 표시
			for (i = 0; i < 10; ++i)
			{
				if (i == iCheck)
				{
					printf("[%d][%d]", Arr[i], Arr[i + 1]);
					i++;
				}
				else
					printf(" %d ", Arr[i]);
			}
			puts("\n");
			system("pause");

		}
		fputs("\n", stdout);
		iWhileExit--;

	}

	puts("정렬 완료");	

	return 0;
}