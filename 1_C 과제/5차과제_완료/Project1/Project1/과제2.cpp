//2. 테이블 지정 방법
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "string.h"
#include "conio.h"

#define ITEM_COUNT 9

struct st_ITEM
{
	char	Name[30];
	int	Rate;		// 일반 랜덤 뽑기와 같은 용도
	int	WinTime;	// 이 아이템이 나올 뽑기 회차.
					// 0 이면 일반 아이템
					// 0 이 아니면 그 회차에만 나옴
};

st_ITEM g_Gatcha[] = {
	{ "칼",						 20, 0 },
	{ "방패",					 20, 0 },
	{ "신발",					 20, 0 },
	{ "물약",					 20, 0 },
	{ "초강력레어무기",			  5, 0 },
	{ "초강력방패",				  5, 0 },
	{ "초강력레어레어레어신발1", 1, 50 },
	{ "초강력레어레어레어신발2", 1, 51 },
	{ "초강력레어레어레어신발3", 1, 10 }

// 마지막 3개의 아이템은 일반 확률로는 나오지 않으며
// 뒤에 입력된 WinTime 회차때만 100% 로 나옴.
};


void Gatcha()
{
	srand((unsigned int)time(NULL));

	int i;
	int iTotal = 0;
	int iRandSave;
	int iCount = 0;
	int iPrev;
	int iNext;
	bool bSeedCheck = false;

	// 1. 전체 아이템들의 비율 총 합을 구함.
	// 단, WinTime 이 정해진 아이템은 확률의 대상 자체가 아니기 때문에 제외.
	for (i = 0; i < ITEM_COUNT; ++i)
	{
		if (g_Gatcha[i].WinTime == 0)
			iTotal += g_Gatcha[i].Rate;
	}

	while (1)
	{
		_getch();
		iNext = 0;
		iPrev = 0;
		bSeedCheck = false;

		while (1)
		{
			// 뽑기 회차 증가. (이는 전역적으로 계산 되어야 함)
			iCount++;

			// 2. 본 뽑기 회차에 대한 지정 아이템이 있는지 확인
			// WinTime 과 iCount 가 같은 아이템을 찾는다.
			// 있다면.. 그 아이템을 뽑고 중단.
			for (i = 0; i < ITEM_COUNT; ++i)
			{
				if (g_Gatcha[i].WinTime == iCount)
				{
					bSeedCheck = true;
					break;
				}
			}

			if (bSeedCheck == true)
				break;

			// 3. rand() 함수로 확률을 구함
			// 여기서 확률은 1/100 이 아니며, 1/총합비율 임.
			iRandSave = (rand() % iTotal) + 1;

			// 4. 전체 아이템 테이블을 돌면서
			// 위에서 구한 Rand 값에 해당 지점의 아이템을 찾는다.
			for (i = 0; i < ITEM_COUNT; ++i)
			{
				iNext += g_Gatcha[i].Rate;

				if (iPrev < iRandSave && iRandSave <= iNext)
					break;

				iPrev += g_Gatcha[i].Rate;
			}


			// 5. 뽑기 회차를 초기화 해야할지 판단하여 초기화.
			if (iCount == 100)
				iCount = 0;

			break;
		}
		printf("%02d. %s (%d / %d)\n", iCount, g_Gatcha[i].Name, iTotal, g_Gatcha[i].Rate);
	}
}

int main(){

	Gatcha();

	return 0;
}

