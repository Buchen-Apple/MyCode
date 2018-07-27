#pragma once
#ifndef __ACTION_QUEUE_H__
#define __ACTION_QUEUE_H__

#include <stdio.h>
#define QUE_LEN 11

// 행동 로직, 공격 로직을 저장하는 큐
class Queue
{
public:
	int front;
	int rear;
	int Posx[QUE_LEN];
	int Posy[QUE_LEN];
};

void Init(Queue* pq);
int NextPos(int pos);
bool Enqueue(Queue* pq, int Posx, int Posy);
bool Dequeue(Queue* pq, int* Posx, int* Posy);

#endif // !__ACTION_QUEUE_H__

