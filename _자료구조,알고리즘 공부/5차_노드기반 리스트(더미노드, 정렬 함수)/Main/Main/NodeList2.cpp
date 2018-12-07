#include "pch.h"
#include "NodeList2.h"


// -----------------------------------------
// 기본 삽입
void  NodeList2::NormalInsert(LData data)
{
	// 노드 생성 및 셋팅
	Node* NewNode = new Node;
	NewNode->m_Data = data;

	// 머리에 삽입
	NewNode->m_pNext = m_pHead->m_pNext;
	m_pHead->m_pNext = NewNode;

	m_iNodeCount++;
}

// 정렬 삽입
void  NodeList2::SortInsert(LData data)
{
	// 노드 생성 및 셋팅
	Node* NewNode = new Node;
	NewNode->m_Data = data;
	
	Node* pPred = m_pHead;

	// 헤더부터 모두 순회하며 위치 찾는다.
	// 정렬 함수를 이용해, 이번 노드가 들어가야 할 위치를 찾는다.
	// 정렬 함수에서 'true'가 리턴되면 내가 들어갈 위치이다.
	while (pPred->m_pNext != nullptr && 
		CompareFunc(data, pPred->m_pNext->m_Data) != true)
	{
		// 못찾았으면 pPred를 다음위치로 이동시킨다.
		pPred = pPred->m_pNext;
	}

	// 새로운 노드는 pPred의 오른쪽에 연결된다.
	NewNode->m_pNext = pPred->m_pNext;
	pPred->m_pNext = NewNode;

	m_iNodeCount++;
}
// -----------------------------------------


// 초기화
void NodeList2::Init()
{
	// 더미 노드 만들어두기
	m_pHead = new Node;
	m_pHead->m_pNext = nullptr;

	// 변수 초기화
	m_pCur = nullptr;
	m_pBefore = nullptr;
	CompareFunc = nullptr;
	m_iNodeCount = 0;	
}

// 삽입
void NodeList2::Insert(LData data)
{
	// 정렬 기준이 없다면 기본 삽입.
	if (CompareFunc == nullptr)
		NormalInsert(data);

	// 정렬 기준이 있다면 정렬 기준으로 삽입
	else
		SortInsert(data);
}

// 첫 노드 반환
bool NodeList2::LFirst(LData* pData)
{
	// 노드가 없다면 false 리턴
	if (m_pHead->m_pNext == nullptr)
		return false;

	// 노드가 있을 경우
	m_pBefore = m_pCur;
	m_pCur = m_pHead->m_pNext;
	*pData = m_pCur->m_Data;

	return true;
}

// 다음 노드 반환
bool NodeList2::LNext(LData* pData)
{
	// 끝 노드일 경우
	if (m_pCur->m_pNext == nullptr)
		return false;

	// 끝 노드가 아닐 경우
	m_pBefore = m_pCur;
	m_pCur = m_pCur->m_pNext;
	*pData = m_pCur->m_Data;

	return true;
}

// 삭제
NodeList2::LData NodeList2::LRemove()
{
	// 리턴할 데이터 받아두기
	LData ret = m_pCur->m_Data;

	// 삭제할 노드, 리스트에서 제외하기
	m_pBefore->m_pNext = m_pCur->m_pNext;

	// 노드 삭제 후, m_pCur 이동
	delete m_pCur;
	m_pCur = m_pBefore;

	m_iNodeCount--;

	return ret;
}

// 노드 수 반환
int NodeList2::Size()
{
	return m_iNodeCount;
}

// 정렬 함수 받기
void NodeList2::SetSortRule(bool(*comp)(LData C1, LData C2))
{
	CompareFunc = comp;
}