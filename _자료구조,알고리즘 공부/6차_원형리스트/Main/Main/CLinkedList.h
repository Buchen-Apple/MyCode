#ifndef __C_LINKED_LIST_H__
#define __C_LINKED_LIST_H__

struct Employee
{
	int m_iNumber;
	char m_cName[20];
};

class CLinkedList
{
	typedef Employee* LData;

	struct Node
	{
		LData m_data;
		Node* m_pNext;
	};

	Node* m_pTail;
	Node* m_pCur;
	Node* m_pBefore;

	int m_iNodeCount;

public:
	// 초기화
	void Init();

	// 머리에 삽입
	void Insert(LData Data);

	// 꼬리에 삽입
	void Insert_Tail(LData Data);

	// 첫 노드 반환
	bool First(LData *pData);

	// 이후 노드 반환
	bool Next(LData *pData);

	// 삭제
	LData Remove();

	// 노드 수 반환
	int Size();
};

#endif // !__C_LINKED_LIST_H__
