#include "pch.h"
#include "ListQueue.h"

// 초기화
void listQueue::Init()
{
	m_pFront = nullptr;
	m_pRear = nullptr;
	m_iNodeCount = 0;
}

// 삽입
void listQueue::Enqueue(Data data)
{
	Node* NewNode = new Node;
	NewNode->m_Data = data;
	NewNode->m_pNext = nullptr;

	// 최초 노드일 경우
	if (m_pFront == nullptr)
	{
		m_pFront = NewNode;
		m_pRear = NewNode;
	}

	// 최초 노드가 아닐 경우
	else
	{
		m_pRear->m_pNext = NewNode;
		m_pRear = NewNode;
	}

	m_iNodeCount++;
}

// 삭제
bool listQueue::Dequeue(Data* pData)
{
	// 노드가 하나도 없을 경우
	if (m_pFront == nullptr)
		return false;

	// 노드가 있을 경우
	Node* DeleteNode = m_pFront;
	*pData = m_pFront->m_Data;

	m_pFront = m_pFront->m_pNext;
	delete DeleteNode;

	return true;
}

// 픽
bool listQueue::Peek(Data* pData)
{
	// 노드가 하나도 없을 경우
	if (m_pFront == nullptr)
		return false;

	// 노드가 있을 경우
	*pData = m_pFront->m_Data;
	return true;
}

// 노드 수 확인
int listQueue::Size()
{
	return m_iNodeCount;
}