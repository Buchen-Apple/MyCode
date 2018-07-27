#ifndef __RED_BLAKC_TREE_H__
#define __RED_BLAKC_TREE_H__

typedef int BData;

class CRBT
{
	enum NODE_COLOR
	{
		BLACK = 0, RED
	};

	struct BTreeNode
	{
		BData iNodeData;
		BTreeNode* stpLeftChild;
		BTreeNode* stpRightChild;
		BTreeNode* stpParent;
		NODE_COLOR eNodeColor;
	};

	BTreeNode* m_pRootNode;
	BTreeNode m_pNilNode;
	int m_iHeight;

public:
	// 생성자
	CRBT();

	// 삽입 함수
	bool Insert(BData data);

	// 삭제 함수
	bool Delete(BData data);

	// 순회하며 모든 값 찍는 함수
	void Traversal(HDC hdc, RECT rt);

private:
	// 전위 순회하며 원,숫자, 선 긋기
	void preorderTraversal(BTreeNode* pSearchNode, HDC hdc, double x, double y);

	// 좌회전
	void LeftRotation(BTreeNode* pRotateNode);

	// 우회전
	void RightRotation(BTreeNode* pRotateNode);

	// Insert 후 색 맞추기 처리
	void InsertColorFixup(BTreeNode* pZNode);

	// 높이 구하는 함수
	int SetHeight(BTreeNode* pHeightNode);
};

#endif // !__RED_BLAKC_TREE_H__

