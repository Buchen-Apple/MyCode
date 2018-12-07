#ifndef __NODE_LIST_H__
#define __NODE_LIST_H__

// 노드 기반 리스트
class NodeList
{
	typedef int LData;

	// 노드
	struct Node
	{
		LData m_Data;
		Node* m_pNext;
	};

	Node* m_pHead;
	Node* m_pTail;
	Node* m_pCur;
	Node* m_pBefore;

	int m_iNodeCount;


public:
	// 초기화
	void ListInit();

	// 머리에 삽입
	void ListInsert(LData data);
	
	// 첫 데이터 반환
	bool ListFirst(LData* pData);

	// 두 번째 이후부터 데이터 반환
	bool ListNext(LData* pData);

	// 현재 Cur이 가리키는 데이터 삭제
	LData ListRemove();

	// 내부 노드 수 반환
	int ListCount();
};

#endif // !__NODE_LIST_H__
