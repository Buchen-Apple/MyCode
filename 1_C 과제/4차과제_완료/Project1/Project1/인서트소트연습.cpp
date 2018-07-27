/*
인서트 소트
1. 1번 인덱스(0번의 한칸 오른쪽)부터 시작
2. 한칸씩 오른쪽으로 이동하면서 왼쪽에 내가 들어갈 장소가 있을지 찾는다
3. 찾는다면 그 위치에 들어가고, 그 위치 뒤의 있는 데이터는 모두 한칸씩 뒤로 밀린다.
4. 즉, 내가 있을곳을 찾아서 그곳에 인서트한다는 컨셉
*/

#include <stdio.h>
#define ARR_LEN 10

void print(int);
void printLine(int);

// 1. 배열 값 세팅
int Arr[ARR_LEN] = { 8, 6, 13, 50, 1, 3, 7, 9, 20, 15 };

int main()
{
	// 2. Check값이 1번 인덱스를 가르키도록 한다.
	int iCheck = 1;
	int iTemp;

	puts("최초 값");
	for (int i = 0; i < ARR_LEN; ++i)
	{
		printf(" %02d ", Arr[i]);
	}
	fputs("\n\n\n", stdout);

	while (iCheck < ARR_LEN)
	{
		
		// 3. Arr[Check]가 Arr[Check-1]보다 작은지 체크. 작을 시 아래 로직 시작. 크다면, 로직 안하고 Check ++;
		if (Arr[iCheck] < Arr[iCheck - 1])
		{
			print(iCheck);
			printLine(iCheck);
			for (int i = 0; i < iCheck; ++i)
			{
				// 4.순회 중, Arr[iCheck]보다 큰 값을 찾으면 정지.
				if (Arr[iCheck] < Arr[i])
				{
					// 5. 정지 후, Arr[iCheck]의 값을 저장해 둔 후 값을 오른쪽으로 한칸씩 당긴다. iCheck -1이 iCheck에 들어갈 때 까지.
					iTemp = Arr[iCheck];
					for (int j = iCheck; j > i; --j)
					{
						Arr[j] = Arr[j - 1];

					}
					// 6. 모두 당겼으면 아까 찾은 큰 값의 1칸 앞에(좌측에) 값을 넣는다.
					Arr[i] = iTemp;
				}
				
			}
			
		}		
		iCheck++;
	}	
	// 3 ~ 6을 반복하다가 마지막 숫자까지 모두 체크하면 break.


	// 7. 정렬 완료된 배열 보여줌
	puts("\n\n정렬 완료");
	for (int i = 0; i < ARR_LEN; ++i)
	{
		printf(" %02d ", Arr[i]);
	}
	fputs("\n", stdout);
	
	return 0;
}

void print(int iCheck)
{
	for (int i = 0; i < ARR_LEN; ++i)
	{
		if(i == iCheck)
			printf("[%02d]", Arr[i]);
		else
			printf(" %02d ", Arr[i]);
	}
	fputs("\n", stdout);
}

void printLine(int iCheck)
{
	for (int i = 0; i < iCheck; ++i)
	{
		fputs("----", stdout);
	}
	fputs("\n", stdout);
}