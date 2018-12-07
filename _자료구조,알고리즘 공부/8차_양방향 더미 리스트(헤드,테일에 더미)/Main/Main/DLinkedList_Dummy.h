#ifndef __D_LINKED_LIST_H__
#define __D_LINKED_LIST_H__

template <typename T>
class DLinkedList_Dummy
{
	struct Node
	{
		T m_Data;
		Node* m_pPrev;
		Node* m_pNext;
	};

	Node* m_pHead;
	Node* m_pTail;
	Node* m_pCur;
	int m_iNodeCount;

public:
	// 초기화
	void Init();

	// 꼬리에 삽입
	void Insert_back(T data);

	// 첫 노드 반환
	bool First(T *pData);

	// 다음 노드 반환
	bool Next(T *pData);

	// 삭제
	T Remove();

	// 노드 카운트 반환
	int Size();
};


// 초기화
template <typename T>
void DLinkedList_Dummy<T>::Init()
{
	// 헤더와 더미에 각각 더미노드 제작
	m_pHead = new Node;
	m_pTail = new Node;

	// 헤더와 테일이 각각 서로를 가리킨다.
	m_pHead->m_pNext = m_pTail;
	m_pHead->m_pPrev = nullptr;
	
	m_pTail->m_pNext = nullptr;
	m_pTail->m_pPrev = m_pHead;

	// 그 외 변수 초기화
	m_pCur = nullptr;
	m_iNodeCount = 0;
}

// 꼬리에 삽입
template <typename T>
void DLinkedList_Dummy<T>::Insert_back(T data)
{
	// 새 노드 제작
	Node* NewNode = new Node;
	NewNode->m_Data = data;

	// 새로운 노드의 Next가 Tail을 가리킨다.
	NewNode->m_pNext = m_pTail;

	// 새로운 노드의 Prev가 Tail의 Prev를 가리킨다.
	NewNode->m_pPrev = m_pTail->m_pPrev;

	// Tail의 Prev가 Next로, 새로운 노드를 가리킨다.
	m_pTail->m_pPrev->m_pNext = NewNode;

	// Tail이 Prev로, 새로운 노드를 가리킨다.
	m_pTail->m_pPrev = NewNode;

	m_iNodeCount++;
}

// 첫 노드 반환
template <typename T>
bool DLinkedList_Dummy<T>::First(T *pData)
{
	// 노드가 없을 경우
	if (m_iNodeCount == 0)
		return false;

	// 노드가 있을 경우
	m_pCur = m_pHead->m_pNext;
	*pData = m_pCur->m_Data;

	return true;
}

// 다음 노드 반환
template <typename T>
bool DLinkedList_Dummy<T>::Next(T *pData)
{
	// 끝에 도착했을 경우
	if (m_pCur->m_pNext == m_pTail)
		return false;

	// 아직 끝이 아닐 경우
	m_pCur = m_pCur->m_pNext;
	*pData = m_pCur->m_Data;

	return true;
}

// 삭제
template <typename T>
T DLinkedList_Dummy<T>::Remove()
{
	// 삭제할 데이터 받아두기
	T ret = m_pCur->m_Data;

	// 삭제할 노드 받아두기
	Node* DeleteNode = m_pCur;

	// 삭제할 노드를, 리스트에서 제외
	m_pCur->m_pPrev->m_pNext = m_pCur->m_pNext;
	m_pCur->m_pNext->m_pPrev = m_pCur->m_pPrev;

	// pCur을 Prev로 이동
	m_pCur = m_pCur->m_pPrev;

	// 메모리 반환
	delete DeleteNode;

	m_iNodeCount--;

	return ret;
}

// 노드 카운트 반환
template <typename T>
int DLinkedList_Dummy<T>::Size()
{
	return m_iNodeCount;
}

#endif // !__D_LINKED_LIST_H__
