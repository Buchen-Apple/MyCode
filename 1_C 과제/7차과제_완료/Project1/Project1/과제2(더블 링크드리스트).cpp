#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <mmsystem.h>
#include <conio.h>

#define UNIT_COUNT 4

int g_iGlobalUnitCount = 0;

enum 
{
	UNIT_CREATE = 49, EXIT
};

typedef struct _node
{
	_node* pPrev;
	_node* pNext;
	ULONGLONG m_Tick;

}unit;

typedef struct
{
	_node* pHead;
	_node* pTail;
	_node* pCur;

}LinkedList;

void ListInit(LinkedList* pList);
bool ListCreate(LinkedList* pList, ULONGLONG Tick);
bool ListFirstPeek(LinkedList* pList, ULONGLONG* Tick);
bool ListNextPeek(LinkedList* pList, ULONGLONG* Tick);
bool ListDelete(LinkedList* pList);


int main()
{
	int Key;
	int icurShowTime;
	ULONGLONG tick;
	ULONGLONG curtick;
	bool bExitCheck = true;
	bool bCompleteCheck = false;
	bool bUnitMaxTextShow = false;

	LinkedList list;
	ListInit(&list);

	timeBeginPeriod(1);
	while (1)
	{		
		Key = 0;
		// 키보드 영역		
		if (_kbhit())
		{
			Key = _getch();
			tick = GetTickCount64();
		}

		// 로직
		switch (Key)
		{
		case UNIT_CREATE:
			// 현재 생성중인 유닛이 0~3개라면 1개씩 추가. 4개라면 더 이상 생성 불가능. 			
			if (!ListCreate(&list, tick))
				bUnitMaxTextShow = true;
			break;
		case EXIT:
			// 프로그램 종료
			bExitCheck = false;
			break;

		}
		// EXIT를 눌렀으면 프로그램 종료
		if (bExitCheck == false)
			break;

		// 랜더링 영역
		system("cls");

		puts("[ 1 : 유닛 생성 요청 / 2 : 종료 ]\n");
		puts("-- CONTROL TOWER UNIT CREATE --");
		puts("#-------------------------------------------------------------------------");
		printf("# ");
		// 첫 번째 리스트 정보를 가져온다. 첫 번쨰 리스트가 있는 경우에만 가져온다. 즉, 생성 요청한 유닛이 0개면 이거 실행 안함.
		if (ListFirstPeek(&list, &tick))
		{
			// 현재 시간을 받아온다.
			curtick = GetTickCount64();

			// 여기서는 10초(10000밀리세컨드)면 유닛1개가 완성된다. 때문에 아래 공식 적용
			// 아래 공식은, 현재 시간을 유닛 생성 요청한 시간+10000이랑 비교해 비율을 계산한 것이다.
			// 1초에 10%씩 증가한다. 
			icurShowTime = (int)(100 - (((tick + 10000) - curtick) / 100));

			// 계산 결과, 100%보다 큰 값이 나오면 그냥 100%로 맞춘다.
			if (icurShowTime > 100)
				icurShowTime = 100;

			// 100%가 됐다면(유닛 생성 완료)
			if (icurShowTime == 100)
			{
				// 생성 완료된 유닛을 뺀다.
				ListDelete(&list);
				// 아래에서 완료되었다는 것을 보여주기 위한 플래그를 on 시킨다.
				bCompleteCheck = true;
			}
			printf("유닛생산중..[%02d%%] ", icurShowTime);

			// 첫 번째 이후 리스트 정보가 있을 경우에만 가져온다.
			while (ListNextPeek(&list,&tick))
			{
				curtick = GetTickCount64();
				icurShowTime = (int)(100 - (((tick + 10000) - curtick) / 100));
				if (icurShowTime > 100)
					icurShowTime = 100;
				if (icurShowTime == 100)
				{
					ListDelete(&list);
					bCompleteCheck = true;
				}
				printf("유닛생산중..[%02d%%] ", icurShowTime);		
			}
		}
		puts("\n#-------------------------------------------------------------------------");

		// 랜더링 후 텍스트 표시
		// 랜더링 중, 생성이 완료된 유닛이 있으면 유닛 생성 완료 텍스트 보여줌.
		if (bCompleteCheck == true)
		{
			puts("##유닛 생성 완료!");
			bCompleteCheck = false;
		}

		// 로직 중, 유닛 대기창이 가득찼으면 대기창이 가득찼다고 텍스트 보여줌.
		if (bUnitMaxTextShow == true)
		{
			puts("##유닛 대기창이 가득 찼습니다!");
			bUnitMaxTextShow = false;
		}


		Sleep(600);		
	}
	timeEndPeriod(1);

	return 0;
}


void ListInit(LinkedList* pList)
{
	//헤드 더미 생성
	unit* HeadNode = (unit*)malloc(sizeof(unit));
	HeadNode->m_Tick = NULL;
	HeadNode->pPrev = NULL;

	//꼬리 더미 생성
	unit* TailNode = (unit*)malloc(sizeof(unit));
	TailNode->m_Tick = NULL;
	TailNode->pNext = NULL;

	// 헤드 더미와 꼬리 더미를 서로 연결
	HeadNode->pNext = TailNode;
	TailNode->pPrev = HeadNode;

	// 헤드가 헤드더미를, 꼬리가 꼬리더미를 가리킨다. 
	pList->pHead = HeadNode;
	pList->pTail = TailNode;
	pList->pCur = NULL;
}

bool ListCreate(LinkedList* pList, ULONGLONG Tick)
{
	if (g_iGlobalUnitCount == UNIT_COUNT)
		return false;

	// 새로운 노드는 꼬리에서 추가된다.
	// 새로운 노드 세팅
	unit* newNode = (unit*)malloc(sizeof(unit));
	newNode->m_Tick = Tick;

	// 새로운 노드의 다음이 꼬리를, 새로운 노드의 이전이 꼬리의 이전을 가리킨다.
	newNode->pNext = pList->pTail;
	newNode->pPrev = pList->pTail->pPrev;
	pList->pCur = NULL;

	// 꼬리의 이전의 다음이 새로운 노드를, 꼬리의 이전이 새로운 노드를 가리킨다.
	pList->pTail->pPrev->pNext = newNode;
	pList->pTail->pPrev = newNode;	

	g_iGlobalUnitCount++;

	return true;
}

bool ListFirstPeek(LinkedList* pList, ULONGLONG* Tick)
{
	// 첫 노드를 반환한다 (헤드 더미 다음부터 체크)
	// 1. 일단, pCur포인터가 더미 다음 노드를 가리킨다.
	pList->pCur = pList->pHead->pNext;

	// 2. 만약, 현재 가리키는게 꼬리 더미일 시, 하나도 추가된게 없다는 것이니 false를 반환한다.
	if (pList->pCur->pNext == NULL)
		return false;

	// 3. 여기까지 왔으면 더미가 아니라는 것이니, Tick에다가 값을 넣고 true를 반환한다.
	(*Tick) = pList->pCur->m_Tick;

	return true;
}

bool ListNextPeek(LinkedList* pList, ULONGLONG* Tick)
{
	// 다음 노드 반환
	// 1. 일단 pCur포인터가 pCur->pNext를 가리킨다.
	pList->pCur = pList->pCur->pNext;

	// 2. 만약 현재 가리키는게 꼬리더미라면, 마지막에 도착했다는 것이니 false를 반환
	if (pList->pCur->pNext == NULL)
		return false;

	// 3. 여기까지 왔으면 더미가 아니라는 것이니, Tick에다가 값을 넣고 true를 반환한다.
	(*Tick) = pList->pCur->m_Tick;
	return true;
}

bool ListDelete(LinkedList* pList)
{
	// 현재 가리키고 있는 리스트를 삭제한다.
	// 1. 임시 포인터가 현재를 가리킨다.
	unit* pTempCur = pList->pCur;

	if (pTempCur->pPrev == NULL || pTempCur->pNext == NULL)
		return false;

	// 2. 현재의 이전이 다음으로 현재의 다음을 가리킨다.
	pList->pCur->pPrev->pNext = pList->pCur->pNext;

	// 3. 현재의 다음이 이전으로 현재의 이전을 가리킨다.
	pList->pCur->pNext->pPrev = pList->pCur->pPrev;

	// 4. 현재가 현재의 이전을 가리킨다.
	pList->pCur = pList->pCur->pPrev;

	// 5. 현재 소멸
	free(pTempCur);

	g_iGlobalUnitCount--;
	return true;
}