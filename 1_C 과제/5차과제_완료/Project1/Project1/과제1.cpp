// 1. 진정한 랜덤

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


void Gatcha()
{
	srand((unsigned int)time(NULL));
	// 1. 전체 아이템들의 비율 총 합을 구함.
	int iTotal = 0;
	int iRandSave;
	int iPrev = 0;
	int iNext;
	int i;
	int iCount = 1;

	for (i = 0; i < ITEM_COUNT; ++i)
	{
		iTotal += g_Gatcha[i].m_Rate;
	}		

	while (1)
	{
		// 2. rand() 함수로 확률을 구함
		// 여기서 확률은 1/100 이 아니며, 1/총합비율 임.
		iRandSave = (rand() % iTotal) + 1;

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

	Gatcha();

	return 0;
}