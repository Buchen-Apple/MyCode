#include "pch.h"
#include "ArrayQueue.h"

// 다음 위치 확인
int arrayQueue::NextPos(int Pos)
{
	// 큐 위치가 끝이라면 0번인덱스 반환
	if (Pos == ARRAY_SIZE - 1)
		return 0;

	return Pos + 1;
}

// 초기화
void arrayQueue::Init()
{
	m_iFront = 0;
	m_iRear = 0;
	m_iSize = 0;
}

// 인큐
bool arrayQueue::Enqueue(Data data)
{
	// 큐가 꽉 찼나 확인
	int TempRear = NextPos(m_iRear);
	if (TempRear == m_iFront)
		return false;

	// 큐에 공간이 있을 경우, 현재 rear 자리에 쓰고 움직인다.
	m_Array[m_iRear] = data;
	m_iRear = TempRear;

	return true;
}

// 디큐
bool arrayQueue::Dequeue(Data *pData)
{
	// 큐가 비었나 확인.
	if (m_iRear == m_iFront)
		return false;

	// 큐에 데이터가 있을 경우, 현재 Front자리에 있는 데이터를 읽고 움직인다.
	*pData = m_Array[m_iFront];
	m_iFront = NextPos(m_iFront);
	
	return true;
}

// Peek
bool arrayQueue::Peek(Data *pData)
{
	// 큐가 비었나 확인.
	if (m_iRear == m_iFront)
		return false;

	// 큐에 데이터가 있을 경우, 현재 Front자리에 있는 데이터를 읽는다.
	*pData = m_Array[m_iFront];
	return true;
}

// 사이즈
int arrayQueue::GetNodeSize()
{
	return m_iSize;
}