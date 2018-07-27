#ifndef __BINARY_SEARCH_TREE_H__
#define __BINARY_SEARCH_TREE_H__

typedef int BData;

class CBST
{
	struct BTreeNode
	{
		BData iNodeData;
		BTreeNode* stpLeftChild;
		BTreeNode* stpRightChild;
	};

	BTreeNode* m_pRootNode;

public:
	// 생성자
	CBST(BData data = 10);

	// 삽입 함수
	bool Insert(BData data);

	// 삭제 함수
	bool Delete(BData data);

	// 순회하며 모든 값 찍는 함수
	void Traversal();

	// 중위 검색
	void InorderTraversal(BTreeNode* pSearchNode);

	// 루트 노드의 값 반환
	BData GetRootNodeData();


};

#endif // !__BINARY_SEARCH_TREE_H__

