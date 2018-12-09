#include "pch.h"
#include "BinaryTree_List.h"

// 이진트리의 노드
BTNode::BTNode()
{
	m_pLeft = nullptr;
	m_pRight = nullptr;
}



// 삭제용. 후위순회
void BinaryTree_List::NodeDelete(BTNode* pNode)
{
	// 더 이상 노드가 없으면 리턴
	if (pNode == nullptr)
		return;

	// 왼쪽 모두 삭제
	NodeDelete(pNode->m_pLeft);

	// 오른쪽 모두 삭제
	NodeDelete(pNode->m_pRight);

	// 현재 노드 삭제
	delete pNode;
}


// 이진트리의 노드 생성
BTNode* BinaryTree_List::MakeBTreeNode(void)
{
	return new BTNode;
}

// 인자로 받은 노드의 data 얻기
BTData BinaryTree_List::GetData(BTNode* pNode)
{
	return pNode->m_data;
}

// 인자로 받은 노드의 data 변경
void BinaryTree_List::SetData(BTNode* pNode, BTData Data)
{
	pNode->m_data = Data;
}

// 인자로 받은 노드의 Left 서브트리 루트 얻기
BTNode* BinaryTree_List::GetLeftSubTree(BTNode* pNode)
{
	return pNode->m_pLeft;
}

// 인자로 받은 노드의 Right 서브트리 루트 얻기
BTNode* BinaryTree_List::GetRightSubTree(BTNode* pNode)
{
	return pNode->m_pRight;
}

// 1번 인자의 Left서브트리로, 2번인자 연결
void BinaryTree_List::SetLeftSubTree(BTNode* pMain, BTNode* pLeftSub)
{
	if (pMain->m_pLeft != nullptr)
		NodeDelete(pMain->m_pLeft);

	pMain->m_pLeft = pLeftSub;
}

// 1번 인자의 Right서브트리로, 2번인자 연결
void BinaryTree_List::SetRightSubTree(BTNode* pMain, BTNode* pRightSub)
{
	if (pMain->m_pRight != nullptr)
		NodeDelete(pMain->m_pRight);

	pMain->m_pRight = pRightSub;
}

// 전위 순회
void BinaryTree_List::PreorderTraverse(BTNode* pNode, Action function)
{
	// 탈출 조건. 노드가 nullptr이면 끝에 도착한것.
	if (pNode == nullptr)
		return;

	// 루트 보고
	function(pNode->m_data);

	// 왼쪽 보고
	PreorderTraverse(pNode->m_pLeft, function);

	// 오른쪽 보고
	PreorderTraverse(pNode->m_pRight, function);
}

// 중위 순회
void BinaryTree_List::InorderTraverse(BTNode* pNode, Action function)
{
	// 탈출 조건. 노드가 nullptr이면 끝에 도착한것.
	if (pNode == nullptr)
		return;

	// 왼쪽 보고
	InorderTraverse(pNode->m_pLeft, function);

	// 루트 보고
	function(pNode->m_data);

	// 오른쪽 보고
	InorderTraverse(pNode->m_pRight, function);
}

// 후위 순회
void BinaryTree_List::PostorderTraverse(BTNode* pNode, Action function)
{
	// 탈출 조건. 노드가 nullptr이면 끝에 도착한것.
	if (pNode == nullptr)
		return;

	// 왼쪽 보고
	PostorderTraverse(pNode->m_pLeft, function);

	// 오른쪽 보고
	PostorderTraverse(pNode->m_pRight, function);

	// 루트 보고
	function(pNode->m_data);
}