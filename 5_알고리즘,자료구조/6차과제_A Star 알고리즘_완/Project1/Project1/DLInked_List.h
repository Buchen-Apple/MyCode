#ifndef __D_LINKED_LIST_H__
#define __D_LINKED_LIST_H__

template <typename T>
class DLinkedList
{
	struct stListNode
	{
		T m_FindSearchNode;
		stListNode* m_pPrev;
		stListNode* m_pNext;
	};

	int m_iCount;
	stListNode* m_Head;
	stListNode* m_Tail;
	stListNode* m_pCur;
	stListNode* m_pReturnSaveNode;

public:
	// 생성자
	DLinkedList();

	// 검색 후, 해당 노드 반환 (삭제 아님)
	T Search(int x, int y);

	// 삽입
	void Insert(T InsertNode);

	// 가장 왼쪽 노드 1개 반환하고 리스트에서 제외시키기.
	T GetListNode();

	// 리스트 내의 노드 수 반환
	int GetCount();

	// 정렬
	void Sort();

	// 리스트내 노드 모두 삭제
	void Clear();
};


// 생성자
template <typename T>
DLinkedList<T>::DLinkedList()
{
	// 가장 왼쪽노드를 리턴할 때, 값을 저장할 노드 생성
	m_pReturnSaveNode = new stListNode[sizeof(stListNode)];

	// 시작, 끝 더미 1개씩 생성
	m_Head = new stListNode[sizeof(stListNode)];
	m_Tail = new stListNode[sizeof(stListNode)];

	// 더미 셋팅
	m_Head->m_pNext = m_Tail;
	m_Head->m_pPrev = nullptr;

	m_Tail->m_pNext = nullptr;
	m_Tail->m_pPrev = m_Head;

	m_iCount = 0;
}

// 검색 후, 해당 노드 반환 (삭제 아님)
template <typename T>
T DLinkedList<T>::Search(int x, int y)
{
	// 인자로 받은 x,y를 가진 노드가 리스트 내에 있는지 검색
	m_pCur = m_Head->m_pNext;

	if (m_pCur == m_Tail)
		return nullptr;

	for (int i = 0; i < m_iCount; ++i)
	{
		if (m_pCur->m_FindSearchNode->m_iX == x && m_pCur->m_FindSearchNode->m_iY == y)
			return m_pCur->m_FindSearchNode;

		m_pCur = m_pCur->m_pNext;
	}

	return nullptr;
}

// 삽입
template <typename T>
void DLinkedList<T>::Insert(T InsertNode)
{
	// 1. 인자로 받은 stFindSearchNode형 포인터를 가리키는 노드 생성
	stListNode* newNode =  new stListNode[sizeof(stListNode)];
	newNode->m_FindSearchNode = InsertNode;

	// 2. 머리쪽으로 삽입
	newNode->m_pPrev = m_Head;
	newNode->m_pNext = m_Head->m_pNext;

	m_Head->m_pNext->m_pPrev = newNode;
	m_Head->m_pNext = newNode;

	m_iCount++;

	// 2. Sort()
	Sort();
}

// 정렬
template <typename T>
void DLinkedList<T>::Sort()
{
	m_pCur = m_Head->m_pNext;
	stListNode* CurNext = m_pCur->m_pNext;

	// 1. 머리부터 꼬리쪽으로 가면서, 버블정렬. 작은 값이 머리쪽으로 오도록.
	for (int i = 0; i < m_iCount; ++i)
	{
		for (int j = 0; j < m_iCount - i; ++j)
		{
			if (CurNext == m_Tail)
				break;

			// 현재 노드가, 현재 노드의 다음 노드보다 값이 크다면, 위치 변경
			if (m_pCur->m_FindSearchNode->m_fF > CurNext->m_FindSearchNode->m_fF)
			{
				stListNode* Temp = m_pCur;

				Temp->m_pPrev->m_pNext = Temp->m_pNext;
				Temp->m_pNext->m_pPrev = Temp->m_pPrev;

				Temp->m_pPrev = Temp->m_pNext;
				Temp->m_pNext = Temp->m_pNext->m_pNext;
				Temp->m_pNext->m_pPrev = Temp;
				Temp->m_pPrev->m_pNext = Temp;
			}

			m_pCur = CurNext;
			CurNext = m_pCur->m_pNext;
		}

		if (CurNext == m_Tail)
			break;
	}

}

// 가장 왼쪽 노드 1개 반환하고 노드를 리스트에서 제외시키기.
template <typename T>
T DLinkedList<T>::GetListNode()
{
	stListNode* returnNode = m_Head->m_pNext;

	m_Head->m_pNext = returnNode->m_pNext;
	returnNode->m_pNext->m_pPrev = m_Head;	

	m_pReturnSaveNode->m_FindSearchNode = returnNode->m_FindSearchNode;

	delete[] returnNode;
	m_iCount--;

	return m_pReturnSaveNode->m_FindSearchNode;
}

// 리스트 내의 노드 수 반환
template <typename T>
int DLinkedList<T>::GetCount()
{
	return m_iCount;
}

// 리스트내 노드 모두 삭제
template <typename T>
void DLinkedList<T>::Clear()
{
	stListNode* Before = m_Head;
	m_pCur = Before->m_pNext;

	while (1)
	{
		if (m_pCur == m_Tail)
			break;

		// 재연결
		Before->m_pNext = m_pCur->m_pNext;
		m_pCur->m_pNext->m_pPrev = Before;

		m_iCount--;

		// 동적 소멸
		delete[] m_pCur->m_FindSearchNode;
		delete m_pCur;
		
		m_pCur = Before->m_pNext;
	}
}


#endif // !__D_LINKED_LIST_H__
