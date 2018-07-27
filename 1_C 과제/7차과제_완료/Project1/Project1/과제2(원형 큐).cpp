#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <mmsystem.h>
#include <conio.h>

#define UNIT_COUNT 5

enum
{
	UNIT_CREATE = 49, EXIT
};

typedef struct _queue
{
	int front;
	int cur;
	int rear;
	ULONGLONG m_Tick[UNIT_COUNT];
}cQueue;

void QueueInit(cQueue* pq);
int NextPos(int pos);
bool Enqueue(cQueue* pq, ULONGLONG tick);
bool Dequeue(cQueue* pq);
bool Qpeek(cQueue* pq, ULONGLONG* tick);


int main()
{
	int Key;
	int icurShowTime;
	ULONGLONG tick;
	ULONGLONG curtick;
	bool bExitCheck = true;
	bool bCompleteCheck = false;
	bool bUnitMaxTextShow = false;
	
	// 큐 선언 및 초기화
	cQueue queue;
	QueueInit(&queue);

	// 타이머 인터럽트를 1m/s로 조절
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
			if (!Enqueue(&queue, tick))
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

		// 큐를 처음부터 순회한다.
		while (Qpeek(&queue, &tick))
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
				Dequeue(&queue);
				// 아래에서 완료되었다는 것을 보여주기 위한 플래그를 on 시킨다.
				bCompleteCheck = true;
			}
			printf("유닛생산중..[%02d%%] ", icurShowTime);

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
	// 타이머 인터럽트를 원복
	timeEndPeriod(1);	

	return 0;
}


void QueueInit(cQueue* pq)
{
	pq->front = 0;
	pq->rear = 0;
	pq->cur = pq->front;
}

int NextPos(int pos)
{
	if (pos == UNIT_COUNT - 1)
		return 0;
	else
		pos += 1;
}

bool Enqueue(cQueue* pq, ULONGLONG tick)
{
	// 큐의 끝에 도착했는지 체크
	if (NextPos(pq->rear) == pq->front)
		return false;

	pq->rear = NextPos(pq->rear);
	pq->m_Tick[pq->rear] = tick;
	return true;
}

bool Dequeue(cQueue* pq)
{
	// 큐가 텅 빈 상태인지 체크
	if (pq->front == pq->rear)
		return false;
		
	pq->front = NextPos(pq->front);
	pq->cur = pq->front;
	pq->m_Tick[pq->front] = 0;	
	return true;
}

bool Qpeek(cQueue* pq, ULONGLONG* tick)
{
	// 텅 빈 상황인지 체크
	// 끝까지 순회했다면, 다시 cur를 front로 이동
	if (pq->cur == pq->rear)
	{
		pq->cur = pq->front;
		return false;
	}

	// 텅 빈 상황이 아니면, 현재 위치를 한칸 이동한 후, 값을 tick에 넣는다.
	pq->cur = NextPos(pq->cur);
	*tick = pq->m_Tick[pq->cur];
	
	return true;
}