#ifndef __DEQUE_H__
#define __DEQUE_H__

class Deque
{
	typedef int Data;

	struct Node
	{
		Data m_Data;
		Node* m_pNext;
		Node* m_pPrev;
	};

	Node* m_pHead;
	Node* m_pTail;
	int m_iSize;

public:
	// 초기화
	void Init();

	// 앞으로 삽입
	void Enqueue_Head(Data data);

	// 앞에 데이터 빼기
	bool Dequeue_Head(Data* pData);

	// 뒤로 삽입
	void Enqueue_Tail(Data data);

	// 뒤의 데이터 빼기
	bool Dequeue_Tail(Data* pData);

	// 앞의 데이터 참조
	bool Peek_Head(Data* pData);

	// 뒤의 데이터 참조
	bool Peek_Tail(Data* pData);

	// 내부 사이즈
	int Size();
};

#endif // !__DEQUE_H__
