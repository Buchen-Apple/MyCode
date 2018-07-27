// 3. 랜덤 테이블
// 첫번째 진짜 랜덤과 뽑기 부분은 같으나 rand() 함수를 대체할 우리만의 랜덤을 만든다.
// 우리가 만드는 랜덤은 지정된 범위 안에서 중복값이 절대 나오지 않는 랜덤 함수.

#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "string.h"
#include "conio.h"

#define ITEM_COUNT 7

struct st_ITEM
{
	char m_Name[30];
	int	 m_Rate;		// 아이템이 뽑힐 비율
};

st_ITEM g_Gatcha[] =
{
	{ "칼",						20 },
	{ "방패",					20 },
	{ "신발",					20 },
	{ "물약",					20 },
	{ "초강력레어무기",			 5 },
	{ "초강력방패",				 5 },
	{ "초강력레어레어레어신발",  1 }
};


int RandTable[1000];		// 랜덤 범위를 최대 1000 개로 잡음.

int TableRand(int Max)
{
	
	srand((unsigned int)time(NULL));

	int iCount = 0;
	static int iUseIndexCount = 0;

	int TempIndex1;
	int TempIndex2;
	int Temp;

	// 랜덤 범위 배열을 모두 사용하면, iUseIndexCount의 값을 0으로 만든다. 이는 다시 셔플하기 위함이다.
	if (iUseIndexCount >= 1000)
		iUseIndexCount = 0;

	// iUseIndexCount가 0이면 값을 다시 세팅한 후 셔플한다.
	if(iUseIndexCount == 0)
	{	// 1000개의 배열에 1~Max까지의 값을 넣음.
		// 값이 Max보다 커지면 다시 1부터 넣는다.
		// 만약 Max가 170이라면, 1000개의 배열에는 1...169,170,1,2,3...의 값이 반복해서 들어간다.
		for (int i = 0; i < 1000; ++i)
		{
			RandTable[i] = iCount + 1;
			if (iCount + 1 == Max)
				iCount = -1;

			iCount++;
		}

		// 100번정도 RandTable 안의 값들을 셔플한다.
		for (int i = 0; i < 100; ++i)
		{
			TempIndex1 = rand() % 1000;
			TempIndex2 = rand() % 1000;

			Temp = RandTable[TempIndex1];
			RandTable[TempIndex1] = RandTable[TempIndex2];
			RandTable[TempIndex2] = Temp;
		}
	}

	return RandTable[iUseIndexCount++];
}


void Gatcha()
{
	srand((unsigned int)time(NULL));
	// 1. 전체 아이템들의 비율 총 합을 구함.
	int iTotal = 0;
	int iRandSave;
	int iPrev = 0;
	int iNext = 0;
	int i;
	int iCount = 1;

	for (i = 0; i < ITEM_COUNT; ++i)
	{
		iTotal += g_Gatcha[i].m_Rate;
	}

	while (1)
	{
		// 2. 값 랜덤섞기 함수를 호출한 후, 그 값을 저장한다.
		iRandSave = TableRand(iTotal);

		// 3. 전체 아이템 테이블을 돌면서
		// 위에서 구한 Rand 값에 해당 지점의 아이템을 찾는다.
		for (i = 0; i < ITEM_COUNT; ++i)
		{
			iNext += g_Gatcha[i].m_Rate;

			if (iPrev < iRandSave && iRandSave <= iNext)
				break;

			iPrev += g_Gatcha[i].m_Rate;
		}

		printf("%d. %s (%d / %d)\n", iCount, g_Gatcha[i].m_Name, iTotal, g_Gatcha[i].m_Rate);
		iCount++;
		iNext = 0;
		iPrev = 0;

		_getch();
	}
}

int main()
{

	// 가차 함수 호출
	Gatcha();

	return 0;
}