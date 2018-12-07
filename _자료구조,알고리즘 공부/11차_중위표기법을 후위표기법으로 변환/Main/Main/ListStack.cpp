#include "pch.h"
#include "ListStack.h"

// 초기화
void listStack::Init()
{
	m_pTop = nullptr;
	m_iSize = 0;
}

// 삽입
void listStack::Push(Data data)
{
	// 새 노드 생성
	Node* NewNode = new Node;
	NewNode->m_data = data;

	// Next 연결
	if (m_pTop == nullptr)
		NewNode->m_pNext = nullptr;
	else
		NewNode->m_pNext = m_pTop;

	// Top 갱신
	m_pTop = NewNode;

	m_iSize++;
}

// 삭제
bool listStack::Pop(Data *pData)
{
	// 노드가 없으면 return false
	if (m_pTop == nullptr)
		return false;

	// 삭제할 노드 받아두기
	Node* DeleteNode = m_pTop;

	// Top을 Next로 이동
	m_pTop = m_pTop->m_pNext;

	// 리턴할 데이터 셋팅
	*pData = DeleteNode->m_data;

	// 노드 메모리 해제 및 true 리턴
	delete DeleteNode;

	m_iSize--;

	return true;
}

// Peek
bool listStack::Peek(Data* pData)
{
	// 노드가 없으면 return false
	if (m_pTop == nullptr)
		return false;

	// 리턴할 데이터 셋팅
	*pData = m_pTop->m_data;

	return true;
}

// 비었는지 확인
bool listStack::IsEmpty()
{
	if (m_pTop == nullptr)
		return true;

	return false;
}

// 노드 수
int listStack::GetNodeSize()
{
	return m_iSize;
}