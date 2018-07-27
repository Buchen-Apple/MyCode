// 퀵정렬
/*
1. 피봇과 L,R잡기
2. Left를 ->로 이동. Left > 피봇이거나 Left인덱스 == Right인덱스 일 시 정지
3. Right를 <-로 이동. Right < 피봇이거나, Right인덱스 == Pivot인덱스 일 시 정지

4. 2, 3이 완료된 후
-> Right인덱스 > Left인덱스면 L,R의 값 스왑 후 2,3 이어서 진행
-> Right인덱스 <= Left인덱스면 R,P 스왑 후, while문 종료. 이후 재귀로 좌,우 실행
*/

#include <stdio.h>
#include <windows.h>
#define QUICK_SORT_LEN 12

int Arr[QUICK_SORT_LEN] = { 5, 1, 8, 20, 2, 3, 6, 11, 4, 7, 12, 19};

void QuickSort(int, int);
void Print(int, int);

int main()
{
	// 정렬 시작 전 숫자 보여줌.
	for (int i = 0; i < QUICK_SORT_LEN; ++i)
	{
		if (Arr[i] >= 10)
			printf(" %d ", Arr[i]);
		else
		{
			fputs(" 0", stdout);
			printf("%d ", Arr[i]);
		}			
	}
	fputs("\n", stdout);

	// 시작 전에 정렬 공간 보여줌
	fputs("\n", stdout);
	for (int i = 0; i < QUICK_SORT_LEN; ++i)
	{
		fputs("^^^^", stdout);
	}
	fputs("\n", stdout);
	
	// 퀵정렬 시작
	QuickSort(1, QUICK_SORT_LEN-1);

	// 정렬 완료 후 다시한번 숫자 보여줌
	for (int i = 0; i < QUICK_SORT_LEN; ++i)
	{
		if (Arr[i] >= 10)
			printf(" %d ", Arr[i]);
		else
		{
			fputs(" 0", stdout);
			printf("%d ", Arr[i]);
		}
	}
	fputs("\n", stdout);

	return 0;
}

// 출력 함수
void Print(int Left, int RIght)
{
	for (int i = 0; i < QUICK_SORT_LEN; ++i)
	{
		if (Arr[i] >= 10)
		{
			if (i == Left || i == RIght)
				printf("[%d]", Arr[i]);
			else
				printf(" %d ", Arr[i]);
		}
		else
		{
			if (i == Left || i == RIght)
			{
				fputs("[0", stdout);
				printf("%d]", Arr[i]);
			}				
			else
			{
				fputs(" 0", stdout);
				printf("%d ", Arr[i]);
			}
		}
		
	}
	fputs("\n", stdout);
}

void QuickSort(int Left, int Right)
{
	// 1. 피봇과 L,R 잡기
	int iPivot = Left-1;
	int iTemp;
	int iOriginalLeft = Left;
	int iOriginalRight = Right;

	// 탈출조건. <<끝이나 >>끝이면 탈출
	if (Left > iOriginalRight || Right < iOriginalLeft)
		return;
			
	while (1)
	{		
		while (1)
		{
			// 2. Left를->로 이동.Left > 피봇이거나 Left인덱스 == Right인덱스 일 시 정지. 그게 아니라면 Left인덱스 증가.			
			if (Arr[iPivot] < Arr[Left] || Left == Right)
				break;
			Left++;
		}
		while (1)
		{
			// 3. Right를 <-로 이동.Right < 피봇이거나, Right인덱스 == Pivot인덱스 일 시 정지. 그게 아니라면 Right인덱스 감소.
			if (Arr[iPivot] > Arr[Right] || iPivot == Right)
				break;
			Right--;;
		}
		
		/*
		4. 2, 3이 완료된 후
			->Right인덱스 > Left인덱스면 L, R의 값 스왑 후 2, 3 이어서 진행
			->Right인덱스 <= Left인덱스면 R, P 스왑 후, while문 종료.이후 재귀로 좌, 우 실행
		*/
		if (Right > Left)
		{ 
			iTemp = Arr[Right];
			Arr[Right] = Arr[Left];
			Arr[Left] = iTemp;	
		}
		
		else if (Right <= Left)
		{
			iTemp = Arr[Right];
			Arr[Right] = Arr[iPivot];
			Arr[iPivot] = iTemp;	
			Print(Left, Right);
			break;
		}
		Print(Left, Right);
	}	

	// 다음 정렬 공간 보여줌.
	fputs("\n", stdout);
	for (int i = 0; i < QUICK_SORT_LEN; ++i)
	{
		if (iPivot <= i && i <= Right)
			fputs("^^^^", stdout);
		else
			fputs("    ", stdout);
	}
	fputs("\n", stdout);
	
	// 현재 피벗의 좌측
	QuickSort(iOriginalLeft, Right-1);

	// 현재 피벗의 우측
	QuickSort(Right+2, iOriginalRight);
}