#ifndef __LIST_ARRAY_H__
#define __LIST_ARRAY_H__

class listStack
{
	typedef char Data;

	struct Node
	{
		Data m_data;
		Node* m_pNext;
	};

	Node* m_pTop;
	int m_iSize;

public:
	// 초기화
	void Init();

	// 삽입
	void Push(Data data);

	// 삭제
	bool Pop(Data *pData);

	// Peek
	bool Peek(Data* pData);

	// 비었는지 확인
	bool IsEmpty();

	// 노드 수
	int GetNodeSize();
};

#endif // !__LIST_ARRAY_H__
