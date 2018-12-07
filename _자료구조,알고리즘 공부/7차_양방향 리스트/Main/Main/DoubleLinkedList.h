#ifndef __DOUBLE_LINKED_LIST_H__
#define __DOUBLE_LINKED_LIST_H__

class DLinkedList
{
	typedef int LData;

	struct Node
	{
		LData m_Data;
		Node* m_pNext;
		Node* m_pPrev;
	};

	Node* m_pHead;
	Node* m_pCur;
	int m_iNodeCount;

public:

	// 초기화
	void Init();

	// 머리에 삽입
	void Insert(LData data);

	// 첫 노드 얻기
	bool First(LData* pData);

	// 그 다음 노드 얻기
	bool Next(LData* pData);

	// 삭제
	LData Remove();

	// 노드 수 반환
	int Size();
};

#endif // !__DOUBLE_LINKED_LIST_H__
