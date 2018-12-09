#ifndef __BINARY_TREE_LIST_H__
#define __BINARY_TREE_LIST_H__

typedef int BTData;

// 이진 트리의 노드
class BTNode
{
	friend class BinaryTree_List;

	BTData m_data;
	BTNode* m_pLeft;
	BTNode* m_pRight;

	BTNode();	
};

// 트리 클래스
// 외부에서 던지는 노드를 이용하는 함수들이 정의되어 있다.
class BinaryTree_List
{
	typedef void Action(BTData data);

	// 삭제용. 후위순회
	void NodeDelete(BTNode* pNode);

public:
	// 이진트리의 노드 생성
	BTNode* MakeBTreeNode(void);

	// 인자로 받은 노드의 data 얻기
	BTData GetData(BTNode* pNode);

	// 인자로 받은 노드의 data 변경
	void SetData(BTNode* pNode, BTData Data);

	// 인자로 받은 노드의 Left 서브트리 루트 얻기
	BTNode* GetLeftSubTree(BTNode* pNode);

	// 인자로 받은 노드의 Right 서브트리 루트 얻기
	BTNode* GetRightSubTree(BTNode* pNode);

	// 1번 인자의 Left서브트리로, 2번인자 연결
	void SetLeftSubTree(BTNode* pMain, BTNode* pLeftSub);

	// 1번 인자의 Right서브트리로, 2번인자 연결
	void SetRightSubTree(BTNode* pMain, BTNode* pRightSub);

public:
	// ------------- 순회 ---------------

	// 전위 순회
	void PreorderTraverse(BTNode* pNode, Action function);

	// 중위 순회
	void InorderTraverse(BTNode* pNode, Action function);

	// 후위 순회
	void PostorderTraverse(BTNode* pNode, Action function);
};

#endif // !__BINARY_TREE_LIST_H__
