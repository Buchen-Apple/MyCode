#ifndef __NODE_LIST2_H__
#define __NODE_LIST2_H__

#include "Point.h"

class NodeList2
{
	typedef Point* LData;

	struct Node
	{
		LData m_Data;
		Node* m_pNext;
	};

	Node* m_pHead;
	Node* m_pCur;
	Node* m_pBefore;
	int m_iNodeCount;

	bool(*CompareFunc)(LData C1, LData C2);


private:
	// 기본 삽입
	void NormalInsert(LData data);

	// 정렬 삽입
	void SortInsert(LData data);


public:
	// 초기화
	void Init();

	// 삽입
	void Insert(LData data);	

	// 첫 노드 반환
	bool LFirst(LData* pData);

	// 다음 노드 반환
	bool LNext(LData* pData);

	// 삭제
	LData LRemove();

	// 노드 수 반환
	int Size();

	// 정렬 함수 받기
	void SetSortRule(bool(*comp)(LData C1, LData C2));
};

#endif // !__NODE_LIST2_H__
