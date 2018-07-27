#pragma once
#ifndef __MOVE_QUEUE_H__
#define __MOVE_QUEUE_H__

#include <stdio.h>
#define QUE_LEN 11

// 행동 로직을 저장하는 큐
typedef struct
{
	int front;
	int rear;
	int Posx[QUE_LEN];
	int Posy[QUE_LEN];
}Queue;


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

#endif // !__MOVE_QUEUE_H__

