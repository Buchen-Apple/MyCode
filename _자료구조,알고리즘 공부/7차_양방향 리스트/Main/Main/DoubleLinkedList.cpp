#include "pch.h"
#include "DoubleLinkedList.h"

// 초기화
void DLinkedList::Init()
{
	m_pHead = nullptr;
	m_pCur = nullptr;
	m_iNodeCount = 0;
}

// 머리에 삽입
void DLinkedList::Insert(LData data)
{
	// 새로운 노드 제작
	Node* NewNode = new Node;
	NewNode->m_Data = data;
	NewNode->m_pPrev = nullptr;
	

	// 최초 노드인 경우
	if (m_pHead == nullptr)
	{
		// 새로운 노드의 Next가 nullptr이 된다
		NewNode->m_pNext = nullptr;			
	}

	// 최초 노드가 아닌 경우
	else
	{
		// 새로운 노드의 Next가 헤더를 가리킨다.
		NewNode->m_pNext = m_pHead;

		// 헤더의 Prev가 새로운 노드를 가리킨다.
		m_pHead->m_pPrev = NewNode;
	}

	// 새로운 노드가 헤더가 된다.
	m_pHead = NewNode;

	// 노드 카운트 증가
	m_iNodeCount++;
}

// 첫 노드 얻기
bool DLinkedList::First(LData* pData)
{
	// 노드가 없을 경우
	if (m_pHead == nullptr)
		return false;

	// 노드가 있을 경우
	m_pCur = m_pHead;
	*pData = m_pCur->m_Data;

	return true;
}

// 그 다음 노드 얻기
bool DLinkedList::Next(LData* pData)
{
	// 끝에 도달한 경우
	if (m_pCur->m_pNext == nullptr)
		return false;

	// 끝이 아닌 경우
	m_pCur = m_pCur->m_pNext;
	*pData = m_pCur->m_Data;

	return true;
}

// 삭제
DLinkedList::LData DLinkedList::Remove()
{
	// 리턴할 데이터 얻어두기
	LData ret = m_pCur->m_Data;

	// 삭제할 노드 얻어두기
	Node* DeleteNode = m_pCur;

	// 삭제하고자 하는 노드가 Head인 경우
	if (DeleteNode == m_pHead)
	{
		// 노드가 1개라면
		if (m_iNodeCount == 1)
		{
			// head가 null이된다.
			m_pHead = nullptr;		
		}

		// 1개가 아니라면
		else
		{
			// Head를 Head->Next로 이동
			m_pHead = m_pHead->m_pNext;

			// Head의 Prev를 nullptr로
			m_pHead->m_pPrev = nullptr;

			// m_pCur을 Next로 이동
			m_pCur = m_pCur->m_pNext;
		}		
	}

	// 삭제하고자 하는 노드가 Head가 아닌 경우
	else
	{
		// 삭제할 노드를, 리스트에서 제외
		m_pCur->m_pPrev->m_pNext = m_pCur->m_pNext;

		// pCur의 Next가 null이 아닐 경우에만 아래 작업 진행
		// 가장 마지막 노드 삭제 시, 아래 작업을 안할것이다.
		if(m_pCur->m_pNext != nullptr)
			m_pCur->m_pNext->m_pPrev = m_pCur->m_pPrev;

		// m_pCur을 Prev로 이동
		m_pCur = m_pCur->m_pPrev;
	}

	// 노드 반환
	delete DeleteNode;

	// 노드 카운트 감소
	m_iNodeCount--;

	return ret;
}

// 노드 수 반환
int DLinkedList::Size()
{
	return m_iNodeCount;
}