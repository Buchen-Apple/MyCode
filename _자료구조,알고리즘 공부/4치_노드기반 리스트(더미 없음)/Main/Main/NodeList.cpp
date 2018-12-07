#include "pch.h"
#include "NodeList.h"
#include <iostream>

using namespace std;

// 초기화
void NodeList::ListInit()
{
	m_pCur = nullptr;
	m_pTail = nullptr;
	m_pHead = nullptr;
	m_pBefore = nullptr;

	m_iNodeCount = 0;
}

// 머리에 삽입
void NodeList::ListInsert(LData data)
{
	// 새로운 노드 생성
	Node* NewNode = new Node;
	NewNode->m_Data = data;	

	// 첫 노드일 경우
	if (m_pHead == nullptr)
	{
		NewNode->m_pNext = nullptr;
		m_pHead = NewNode;
		m_pTail = NewNode;
	}

	// 첫 노드가 아닐 경우
	else
	{
		// 새로운 노드가 Next로 헤더를 가리킨다.
		NewNode->m_pNext = m_pHead;

		// 새로운 노드가 헤더가 됨
		m_pHead = NewNode;
	}
	m_iNodeCount++;
}

// 첫 데이터 반환
bool NodeList::ListFirst(LData* pData)
{
	// 노드가 없을 경우
	if (m_pHead == nullptr)
		return false;

	// 노드가 있을 경우
	m_pBefore = m_pHead;
	m_pCur = m_pHead;
	*pData = m_pCur->m_Data;

	return true;
}

// 두 번째 이후부터 데이터 반환
bool NodeList::ListNext(LData* pData)
{
	// 끝에 도착했을 경우
	if (m_pCur->m_pNext == nullptr)
		return false;

	// 끝이 아닐 경우	
	m_pBefore = m_pCur;
	m_pCur = m_pCur->m_pNext;
	*pData = m_pCur->m_Data;	

	return true;
}

// 현재 Cur이 가리키는 데이터 삭제
NodeList::LData NodeList::ListRemove()
{
	// 삭제할 노드의 Next 받아두기
	Node* DeleteNext = m_pCur->m_pNext;

	// 리턴할 데이터 받아두기
	LData ret = m_pCur->m_Data;

	// 삭제할 노드가 첫 노드라면
	if (m_pCur == m_pHead)
	{
		// 헤더가 DeleteNext를 가리킨다.
		m_pHead = DeleteNext;	

		// 기존 노드 삭제
		delete m_pCur;
	}

	// 첫 노드가 아니라면
	else
	{
		// Before의 Next가 DeleteNext를 가리킨다
		m_pBefore->m_pNext = DeleteNext;

		// 기존 노드 삭제
		delete m_pCur;

		// m_pCur이 m_pBefore를 가리킨다. (한칸 뒤로 이동)
		m_pCur = m_pBefore;
	}	

	// 노드 수 감소
	m_iNodeCount--;

	// 리턴
	return ret;
}

// 내부 노드 수 반환
int NodeList::ListCount()
{
	return m_iNodeCount;
}