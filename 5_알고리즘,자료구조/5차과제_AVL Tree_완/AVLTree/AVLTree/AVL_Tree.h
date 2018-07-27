#ifndef __AVL_TREE_H__
#define __AVL_TREE_H__

typedef int	TreeData;

struct BTreeNode
{
	TreeData iNodeData;
	BTreeNode* stpLeftChild;
	BTreeNode* stpRightChild;
};

class CAVLTree
{
	/*struct BTreeNode
	{
		TreeData iNodeData;
		BTreeNode* stpLeftChild;
		BTreeNode* stpRightChild;
	};*/

	BTreeNode* m_pRootNode;

public:
	// 생성자
	CAVLTree();

	BTreeNode* GetRootNode()
	{
		return m_pRootNode;
	}

	// 삽입 함수
	void Insert(BTreeNode** pNode, TreeData data);

	// 삭제 함수
	void Delete(TreeData data);

	// 해당 노드의 값 반환.
	TreeData GetData(BTreeNode* pNode)
	{
		return pNode->iNodeData;
	}

	// 해당 노드의 왼쪽노드 주소 반환
	BTreeNode* GetLeftNode(BTreeNode* pNode)
	{
		return pNode->stpLeftChild;
	}

	//  해당 노드의 오른쪽노드 주소 반환
	BTreeNode* GetRightNode(BTreeNode* pNode)
	{
		return pNode->stpRightChild;
	}

	// 중위 순회
	void InorderTraversal(BTreeNode* RootNode);

private:
	// 삭제하며 리밸런싱하는 재귀함수
	void DeleteSupport(BTreeNode** pNode, BTreeNode* pSubTree, TreeData data, int iCaseCheck);

	// Main의 Left노드 변경
	void ChangeLeftSubTree(BTreeNode* pMain, BTreeNode* pChangeNode);

	// Main의 Right노드 변경
	void ChangeRightSubTree(BTreeNode* pMain, BTreeNode* pChangeNode);

	// LL회전 (우회전)
	char* LLRotation(BTreeNode* pRotateNode);

	// LR회전 (우회전 후 -> 좌회전)
	char* LRRotation(BTreeNode* pRotateNode);

	// RR회전 (좌회전)
	char* RRRotation(BTreeNode* pRotateNode);

	// RL회전 (좌회전 후 -> 우회전)
	char* RLRotation(BTreeNode* pRotateNode);

	// 높이 구하기
	int GetHeight(BTreeNode* pHeightNode);

	// 균형 인수 구하기
	int GetHeightDiff(BTreeNode* pDiffNode);

	// 리밸런싱 하기
	char* Rebalance(BTreeNode* pbalanceNode);
};

#endif // !__AVL_TREE_H__
