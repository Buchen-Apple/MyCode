#ifndef __LIST_QUEUE_H__
#define __LIST_QUEUE_H__

class listQueue
{
	typedef int Data;

	struct Node
	{
		Data m_Data;
		Node* m_pNext;
	};

	Node* m_pFront;
	Node* m_pRear;
	int m_iNodeCount;

public:
	// 초기화
	void Init();

	// 삽입
	void Enqueue(Data data);

	// 삭제
	bool Dequeue(Data* pData);

	// 픽
	bool Peek(Data* pData);

	// 노드 수 확인
	int Size();
};


#endif // !__LIST_QUEUE_H__
