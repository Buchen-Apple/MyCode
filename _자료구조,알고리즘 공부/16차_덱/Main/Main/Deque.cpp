#include "pch.h"
#include "Deque.h"

// 초기화
void Deque::Init()
{
	m_pHead = nullptr;
	m_pTail = nullptr;
	m_iSize = 0;
}

// 앞으로 삽입
void Deque::Enqueue_Head(Data data)
{
	// 새로운 노드 생성
	Node* NewNode = new Node;
	NewNode->m_Data = data;
	NewNode->m_pPrev = nullptr;

	// 최초 노드일 경우
	if (m_pHead == nullptr)
	{		
		NewNode->m_pNext = nullptr;
		m_pHead = NewNode;
		m_pTail = NewNode;
	}

	// 최초 노드가 아닐 경우
	else
	{
		NewNode->m_pNext = m_pHead;
		m_pHead->m_pPrev = NewNode;
		m_pHead = NewNode;
	}

	m_iSize++;
}

// 앞의 데이터 빼기
bool Deque::Dequeue_Head(Data* pData)
{
	// 데이터가 없을 경우
	if (m_pHead == nullptr)
		return false;

	// 데이터가 있을 경우
	Node* DeleteNode = m_pHead;
	*pData = m_pHead->m_Data;

	// 마지막 노드라면
	if (m_pHead->m_pNext == nullptr)
	{
		m_pHead = nullptr;
		m_pTail = nullptr;
	}
	
	// 마지막 노드가 아니라면
	else
	{
		m_pHead = m_pHead->m_pNext;
		m_pHead->m_pPrev = nullptr;
	}
	
	delete DeleteNode;

	m_iSize--;

	return true;
}

// 뒤로 삽입
void Deque::Enqueue_Tail(Data data)
{
	// 새로운 노드 생성
	Node* NewNode = new Node;
	NewNode->m_Data = data;
	NewNode->m_pNext = nullptr;	

	// 최초 노드일 경우
	if (m_pTail == nullptr)
	{
		NewNode->m_pPrev = nullptr;
		m_pHead = NewNode;
		m_pTail = NewNode;
	}

	// 최초 노드가 아닐 경우
	else
	{
		NewNode->m_pPrev = m_pTail;
		m_pTail->m_pNext = NewNode;
		m_pTail = NewNode;
	}

	m_iSize++;

}

// 뒤의 데이터 빼기
bool Deque::Dequeue_Tail(Data* pData)
{
	// 데이터가 없을 경우
	if (m_pTail == nullptr)
		return false;

	// 데이터가 있을 경우
	Node* DeleteNode = m_pTail;
	*pData = m_pTail->m_Data;

	// 마지막 노드일 경우
	if (m_pTail->m_pPrev == nullptr)
	{
		m_pHead = nullptr;
		m_pTail = nullptr;
	}

	// 마지막 노드가 아닐 경우
	else
	{
		m_pTail = m_pTail->m_pPrev;
		m_pTail->m_pNext = nullptr;
	}

	delete DeleteNode;

	m_iSize--;

	return true;
}

// 앞의 데이터 참조
bool Deque::Peek_Head(Data* pData)
{
	// 데이터가 없을 경우
	if (m_pHead == nullptr)
		return false;

	// 데이터가 있을 경우
	*pData = m_pHead->m_Data;	

	return true;
}

// 뒤의 데이터 참조
bool Deque::Peek_Tail(Data* pData)
{
	// 데이터가 없을 경우
	if (m_pTail == nullptr)
		return false;

	// 데이터가 있을 경우
	*pData = m_pTail->m_Data;	

	return true;
}

// 내부 사이즈
int Deque::Size()
{
	return m_iSize;
}