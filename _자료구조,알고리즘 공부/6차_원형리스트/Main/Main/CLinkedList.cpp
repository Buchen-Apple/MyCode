#include "pch.h"
#include "CLinkedList.h"

// 초기화
void CLinkedList::Init()
{
	m_pTail = nullptr;
	m_pCur = nullptr;
	m_pBefore = nullptr;

	m_iNodeCount = 0;
}

// 머리에 삽입
void CLinkedList::Insert(LData Data)
{
	// 새로운 노드 생성
	Node* NewNode = new Node;
	NewNode->m_data = Data;

	// 첫 노드일 경우
	if (m_pTail == nullptr)
	{
		// 테일이 새로운 노드를 가리킴
		m_pTail = NewNode;

		// 테일의 Next도 새로운 노드를 가리킴
		m_pTail->m_pNext = NewNode;
	}

	// 첫 노드가 아닐 경우
	else
	{
		// 새로운 노드의 Next가 테일의 Next를 가리킴
		NewNode->m_pNext = m_pTail->m_pNext;

		// 테일의 Next가 새로운 노드를 가리킴
		m_pTail->m_pNext = NewNode;
	}

	m_iNodeCount++;
}

// 꼬리에 삽입
void CLinkedList::Insert_Tail(LData Data)
{
	// 새로운 노드 생성
	Node* NewNode = new Node;
	NewNode->m_data = Data;

	// 첫 노드일 경우
	if (m_pTail == nullptr)
	{
		// 테일이 새로운 노드를 가리킴
		m_pTail = NewNode;

		// 테일의 Next도 새로운 노드를 가리킴
		m_pTail->m_pNext = NewNode;
	}

	// 첫 노드가 아닐 경우
	else
	{
		// 새로운 노드의 Next가 테일의 Next를 가리킴
		NewNode->m_pNext = m_pTail->m_pNext;

		// 테일의 Next가 새로운 노드를 가리킴
		m_pTail->m_pNext = NewNode;

		// 테일이 새로운 노드를 가리킴
		m_pTail = NewNode;
	}

	m_iNodeCount++;
}

// 첫 노드 반환
bool CLinkedList::First(LData *pData)
{
	// 노드가 없으면 return false
	if (m_pTail == nullptr)
		return false;

	// 노드가 있을 경우, 헤더부터 순회한다 (헤더는 Tail의 Next이다)
	m_pBefore = m_pTail;
	m_pCur = m_pTail->m_pNext;

	*pData = m_pCur->m_data;

	return true;
}

// 이후 노드 반환
bool CLinkedList::Next(LData *pData)
{
	// 노드가 없으면 return false
	if (m_pTail == nullptr)
		return false;

	// 노드가 있을 경우 끝없이 순환하게된다.
	m_pBefore = m_pCur;
	m_pCur = m_pCur->m_pNext;

	*pData = m_pCur->m_data;

	return true;	
}

// 삭제
CLinkedList::LData CLinkedList::Remove()
{
	// 리턴할 데이터 받아두기
	LData ret = m_pCur->m_data;

	// 삭제할 노드 받아두기
	Node* DeleteNode = m_pCur;

	// 삭제될 노드가 tail인 경우
	if (DeleteNode == m_pTail)
	{
		// 만약, 남아있는 노드가 1개라면, Tail은 nullptr을 가리킨다.
		if (m_iNodeCount == 1)
			m_pTail = nullptr;

		// 남아있는 노드가 1개 이상이라면, Tail을 Before로 이동시킨다.
		else
			m_pTail = m_pBefore;
	}

	// 삭제될 노드, 리스트에서 제외
	m_pBefore->m_pNext = m_pCur->m_pNext;

	// m_pCur위치 이동 후 노드 메모리반환.
	m_pCur = m_pBefore;
	delete DeleteNode;

	m_iNodeCount--;

	return ret;
}

// 노드 수 반환
int CLinkedList::Size()
{
	return m_iNodeCount;
}