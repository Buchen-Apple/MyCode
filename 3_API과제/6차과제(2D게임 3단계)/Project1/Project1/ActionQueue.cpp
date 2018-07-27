#include "stdafx.h"
#include "ActionQueue.h"

void Init(Queue* pq)
{
	pq->front = 0;
	pq->rear = 0;
}

int NextPos(int pos)
{
	if (pos == QUE_LEN - 1)
		return 0;
	else
		return pos + 1;
}

bool Enqueue(Queue* pq, int Posx, int Posy)
{
	if (NextPos(pq->rear) == pq->front)
		return false;

	pq->rear = NextPos(pq->rear);
	pq->Posx[pq->rear] = Posx;
	pq->Posy[pq->rear] = Posy;

	return true;
}

bool Dequeue(Queue* pq, int* Posx, int* Posy)
{
	if (pq->front == pq->rear)
		return false;

	pq->front = NextPos(pq->front);

	*Posx = pq->Posx[pq->front];
	*Posy = pq->Posy[pq->front];

	return true;
}