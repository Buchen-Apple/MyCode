#ifndef __LIST_ARRAY_H__
#define __LIST_ARRAY_H__

template <typename T>
class listStack
{
	struct Node
	{
		T m_data;
		Node* m_pNext;
	};

	Node* m_pTop;
	int m_iSize;

public:
	// 초기화
	void Init();

	// 삽입
	void Push(T data);

	// 삭제
	bool Pop(T *pData);

	// Peek
	bool Peek(T* pData);

	// 비었는지 확인
	bool IsEmpty();

	// 노드 수
	int GetNodeSize();
};


// 초기화
template <typename T>
void listStack<T>::Init()
{
	m_pTop = nullptr;
	m_iSize = 0;
}

// 삽입
template <typename T>
void listStack<T>::Push(T data)
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
template <typename T>
bool listStack<T>::Pop(T *pData)
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
template <typename T>
bool listStack<T>::Peek(T* pData)
{
	// 노드가 없으면 return false
	if (m_pTop == nullptr)
		return false;

	// 리턴할 데이터 셋팅
	*pData = m_pTop->m_data;

	return true;
}

// 비었는지 확인
template <typename T>
bool listStack<T>::IsEmpty()
{
	if (m_pTop == nullptr)
		return true;

	return false;
}

// 노드 수
template <typename T>
int listStack<T>::GetNodeSize()
{
	return m_iSize;
}

#endif // !__LIST_ARRAY_H__
