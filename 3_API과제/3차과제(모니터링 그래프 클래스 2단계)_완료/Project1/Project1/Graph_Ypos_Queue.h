#pragma once
#ifndef __GRAPH_YPOS_QUEUE_H__
#define __GRAPH_YPOS_QUEUE_H__

#define QUEUE_LEN 100
// 큐 모음
typedef struct
{
	int front;
	int rear;
	int Peek;
	int queArr[QUEUE_LEN];
}Queue;

// 초기화
void Init(Queue* pq);
// 다음 위치 확인
int NextPos(int pos);
// 디큐
int Dequeue(Queue* pq);
// 인큐
void Enqueue(Queue* pq, int y);
// 첫 큐 Peek
bool FirstPeek(Queue* pq, int* Data);
// 다음 큐 Peek
bool NextPeek(Queue* pq, int* Data);

#endif // !__GRAPH_YPOS_QUEUE_H__

