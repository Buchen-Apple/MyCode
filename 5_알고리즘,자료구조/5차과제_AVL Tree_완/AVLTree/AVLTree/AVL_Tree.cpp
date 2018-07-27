#include <stdio.h>
#include <stdlib.h>
#include "AVL_Tree.h"

// 생성자
CAVLTree::CAVLTree()
{
	// RootNode를 기본 NULL로 생성
	m_pRootNode = NULL;
}

// 삽입 함수
void CAVLTree::Insert(BTreeNode** pNode, TreeData data)
{
	// RootNode가 NULL이라면, 최초 노드 등록이니, data로 루트노드를 셋팅하고 return한다.
	if (m_pRootNode == NULL)
	{
		// RootNode 셋팅
		m_pRootNode = (BTreeNode*)malloc(sizeof(BTreeNode));

		m_pRootNode->iNodeData = data;
		m_pRootNode->stpLeftChild = NULL;
		m_pRootNode->stpRightChild = NULL;

		*pNode = m_pRootNode;

		return;
	}

	// RootNode가 NULL이 아니라면, 절차대로 진행

	// 1. 끝 위치를 찾았으면 노드 생성 후 연결.
	// **pNode는 &(p->Left / p->Right)이다.
	// *pNode는 p->Left / p->Right이다.
	// pNode는 p이다.
	if (*pNode == NULL)
	{
		BTreeNode* pNewNode = (BTreeNode*)malloc(sizeof(BTreeNode));
		pNewNode->iNodeData = data;
		pNewNode->stpLeftChild = NULL;
		pNewNode->stpRightChild = NULL;
		*pNode = pNewNode;

		return;
	}
	
	// 2. 값이 작다면, 왼쪽으로 Insert(&노드->왼) 재귀.
	else if ((*pNode)->iNodeData > data)
	{
		Insert((&(*pNode)->stpLeftChild), data);

		// 그리고 리밸런싱
		*pNode = (BTreeNode*)Rebalance(*pNode);
	}

	// 3. 값이 크다면, 오른쪽으로 Insert(&노드->오른) 재귀.
	else if ((*pNode)->iNodeData < data)
	{
		Insert((&(*pNode)->stpRightChild), data);

		// 그리고 리밸런싱
		*pNode = (BTreeNode*)Rebalance(*pNode);
	}
}

// 삭제 함수
void CAVLTree::Delete(TreeData data)
{
	// 삭제할 노드를 찾는다.
	BTreeNode* pParentNode;
	BTreeNode* pNowNode = m_pRootNode;
	while (1)
	{
		// 1. 부모 노드 셋팅
		pParentNode = pNowNode;

		// 2. 내 위치 셋팅
		// 2-1. [부모 노드 data > 찾으려는 data]라면, 나는 부모의 왼쪽 노드가 된다.
		if (pParentNode->iNodeData > data)
			pNowNode = pParentNode->stpLeftChild;

		// 2-2. [부모 노드 data < 찾으려는 data]라면, 나는 부모의 오른쪽 노드가 된다.
		else if (pParentNode->iNodeData < data)
			pNowNode = pParentNode->stpRightChild;

		// 2-3. [부모 노드 data == 찾으려는 data]라면. 이 경우는, 루트노드를 삭제하는 경우만 해당.
		else
			break;

		// 종료 조건. pNowNode가 NULL이면, 리프 노드(잎사귀 노드)까지 모두 검색했는데도 원하는 값이 없는 것. 
		if (pNowNode == NULL)
			return;

		// 3. 내가 현재 삭제되어야 하는 노드인지 체크
		else if (pNowNode->iNodeData == data)
			break;
	}

	// 이제 이에 대응되는 실제 삭제 노드를 찾는다.
	BTreeNode* pBigNode;
	BTreeNode* pBigChildNode;
	BTreeNode* pBigParentNode = NULL;
	int iCaseCheck = 0;

	// case 1. 내 자식이 0명인 경우
	if (pNowNode->stpLeftChild == NULL && pNowNode->stpRightChild == NULL)
	{
		// 그냥 나를 삭제한다.
		pBigNode = pNowNode;

		iCaseCheck = 1;
	}

	// case 2. 양쪽 다 자식이 있는 경우
	// case 3. 내 왼쪽 자식이 있을 경우
	else if ((pNowNode->stpLeftChild != NULL && pNowNode->stpRightChild != NULL) || pNowNode->stpLeftChild != NULL)
	{
		// 왼쪽 노드의, 가장 큰 값을 찾는다.
		// 현재, pBigNode에는 pNowNode의 왼쪽 노드가 들어있다.
		pBigNode = pNowNode->stpLeftChild;
		pBigChildNode = pBigNode->stpRightChild;
		pBigParentNode = pNowNode;

		// 1. 왼쪽 자식 중 가장 값이 큰 노드 찾기.
		// 오른쪽 자식이 NULL이 나올 때 까지 찾는다.
		while (1)
		{
			// 오른쪽 자식이 Nil이면 위치를 찾은 것.
			if (pBigChildNode == NULL)
				break;

			pBigParentNode = pBigNode;
			pBigNode = pBigChildNode;
			pBigChildNode = pBigChildNode->stpRightChild;
		}
		iCaseCheck = 2;
	}

	// case 4. 내 오른쪽에 자식이 있을 경우
	else if (pNowNode->stpRightChild != NULL)
	{
		// 오른쪽 노드의, 가장 작은 값을 찾는다.
		// 현재, pBigNode에는 pNowNode의 오른쪽 노드가 들어있다.
		pBigNode = pNowNode->stpRightChild;
		pBigChildNode = pBigNode->stpLeftChild;
		pBigParentNode = pNowNode;

		// 1. 오른쪽 자식 중 가장 값이 작은 노드 찾기.
		// 왼쪽 자식이 NULL이 나올 때 까지 찾는다.
		while (1)
		{
			// 왼쪽 자식이 Nil이면 위치를 찾은 것.
			if (pBigChildNode == NULL)
				break;

			pBigParentNode = pBigNode;
			pBigNode = pBigChildNode;
			pBigChildNode = pBigChildNode->stpLeftChild;
		}

		iCaseCheck = 4;
	}

	// 실제 삭제, 삭제 후 주변 노드 연결, 그리고 리밸런싱
	int iTempData = pBigNode->iNodeData;
	DeleteSupport(&m_pRootNode, pBigParentNode, pBigNode->iNodeData, iCaseCheck);
}

// 삭제, 노드 연결 후, 리밸런싱하는 재귀함수
void CAVLTree::DeleteSupport(BTreeNode** pNode, BTreeNode* pSubTree, TreeData data, int iCaseCheck)
{
	// 1. data 위치를 찾았으면 케이스(iCaseCheck)에 따라 절차 진행.
	// 인자의 의미.
	// **pNode는 &(p->Left / p->Right)이다.
	// *pNode는 p->Left / p->Right이다.
	// pNode는 p이다.
	if ((*pNode)->iNodeData == data)
	{
		switch (iCaseCheck)
		{
			// case 1, 2, 3, 4는 Delete함수 참조
			case 1:
			{
				BTreeNode* DeleteNode = *pNode;

				// 1. 삭제하려는 노드가 루트노드라면 루트노드를 NULL로 변경
				// 그리고 값 대입 안함
				if (DeleteNode == m_pRootNode)
					m_pRootNode = NULL;

				// 2. 루트노드가 아니라면 내 부모가 나를 가리키던 방향을(Left or Right) NULL로 만든다.
				else
					*pNode = NULL;

				// 3. 나를 동적해제.
				free(DeleteNode);
			}
			break;

			case 2:
			case 3:
			{				
				BTreeNode* DeleteNode = *pNode;

				// 1. 내 오른쪽 자식이 NULL이 아니라면, 부모는 내 오른쪽 자식을 가리킴.
				if (DeleteNode->stpRightChild != NULL)
					*pNode = DeleteNode->stpRightChild;

				// 2. 내 오른쪽 자식이 NULL이라면, 부모는 내 왼쪽 자식을 가리킴
				else
					*pNode = DeleteNode->stpLeftChild;				

				// 3. 값 대입
				pSubTree->iNodeData = data;

				// 4. 나를 동적해제.
				free(DeleteNode);				
			}

			break;

			case 4:
			{
				BTreeNode* DeleteNode = *pNode;

				// 1. 내 왼쪽 자식이 NULL이 아니라면, 부모는 내 왼쪽 자식을 가리킴.
				if (DeleteNode->stpLeftChild != NULL)
					*pNode = DeleteNode->stpLeftChild;

				// 2. 내 왼쪽 자식이 NULL이라면, 부모는 내 오른쪽 자식을 가리킴
				else
					*pNode = DeleteNode->stpRightChild;

				// 3. 값 대입
				pSubTree->iNodeData = data;				

				// 4. 나를 동적해제.
				free(DeleteNode);				
			}
			break;

		}

		return;
	}

	// 2. 값이 작다면, 왼쪽으로 Insert(&노드->왼) 재귀.
	else if ((*pNode)->iNodeData > data)
	{
		DeleteSupport( &(*pNode)->stpLeftChild, pSubTree, data, iCaseCheck);

		// 그리고 리밸런싱
		*pNode = (BTreeNode*)Rebalance(*pNode);
	}

	// 3. 값이 크다면, 오른쪽으로 Insert(&노드->오른) 재귀.
	else if ((*pNode)->iNodeData < data)
	{
		DeleteSupport( &(*pNode)->stpRightChild, pSubTree, data, iCaseCheck);

		// 그리고 리밸런싱
		*pNode = (BTreeNode*)Rebalance(*pNode);
	}

}

// Main의 Left노드 변경
void CAVLTree::ChangeLeftSubTree(BTreeNode* pMain, BTreeNode* pChangeNode)
{
	pMain->stpLeftChild = pChangeNode;
}

// Main의 Right노드 변경
void CAVLTree::ChangeRightSubTree(BTreeNode* pMain, BTreeNode* pChangeNode)
{
	pMain->stpRightChild = pChangeNode;
}

// LL회전 (우회전)
char* CAVLTree::LLRotation(BTreeNode* pRotateNode)
{
	// pRotateNode를 나로 해서, 한칸 좌측 아래의 노드 기준 회전
	BTreeNode* pNode = pRotateNode;
	BTreeNode* pChildNode = pNode->stpLeftChild;

	// 1. 만약, 나 자신이나, 내 좌측 노드가 NULL이라면 회전 불가능.
	if (pNode == NULL || pChildNode == NULL)
		return nullptr;

	// 2. 내가 왼쪽으로, 내 왼쪽 자식의, 오른쪽 자식을 가리킨다.
	ChangeLeftSubTree(pNode, pChildNode->stpRightChild);

	// 3. 내 왼쪽 자식이 오른쪽으로, 나를 가리킨다.
	ChangeRightSubTree(pChildNode, pNode);

	// 4. 만약, 내가(pRotateNode)가 루트였으면, 나의 좌측자식을 새롭게 루트로 지정
	if (pRotateNode == m_pRootNode)
		m_pRootNode = pChildNode;

	// 5. 회전 후 변경된 루트 노드의 주소값 반환
	return (char*)pChildNode;
}

// LR회전 (부분 우회전 후 -> 좌회전)
char* CAVLTree::LRRotation(BTreeNode* pRotateNode)
{
	// 부분 우회전 후 좌회전.
	// pRotateNode를 부모로 해서, 한칸 좌측 아래의 노드 기준 회전
	BTreeNode* pNode = pRotateNode;
	BTreeNode* pChildNode = pNode->stpLeftChild;

	// 1. 만약, 나 자신이나, 내 좌측 노드가 NULL이라면 회전 불가능.
	if (pNode == NULL || pChildNode == NULL)
		return nullptr;

	// 2. 부분 우회전.
	ChangeLeftSubTree(pNode, (BTreeNode*)RRRotation(pChildNode));

	// 3. 좌회전
	return LLRotation(pNode);	
}

// RR회전 (좌회전)
char* CAVLTree::RRRotation(BTreeNode* pRotateNode)
{
	// pRotateNode를 부모로 해서, 한칸 우측 아래의 노드 기준 회전
	BTreeNode* pNode = pRotateNode;
	BTreeNode* pChildNode = pNode->stpRightChild;

	// 1. 만약, 나 자신이나, 내 우측 노드가 NULL이라면 회전 불가능.
	if (pNode == NULL || pChildNode == NULL)
		return nullptr;

	// 2. 내가 오른쪽으로, 내 오른쪽 자식의, 왼쪽 자식을 가리킨다.
	ChangeRightSubTree(pNode, pChildNode->stpLeftChild);	

	// 3. 내 오른쪽 자식이 왼쪽으로, 나를 가리킨다.
	ChangeLeftSubTree(pChildNode, pNode);

	// 4. 만약, 내가(pRotateNode)가 루트였으면, 나의 우측자식을 새롭게 루트로 지정
	if (pRotateNode == m_pRootNode)
		m_pRootNode = pChildNode;

	// 5. 회전 후 변경된 루트 노드의 주소값 반환
	return (char*)pChildNode;
}

// RL회전 (부분 좌회전 후 -> 우회전)
char* CAVLTree::RLRotation(BTreeNode* pRotateNode)
{
	// 부분 좌회전 후 우회전.
	// pRotateNode를 부모로 해서, 한칸 우측 아래의 노드 기준 회전
	BTreeNode* pNode = pRotateNode;
	BTreeNode* pChildNode = pNode->stpRightChild;

	// 1. 만약, 나 자신이나, 내 우측 노드가 NULL이라면 회전 불가능.
	if (pNode == NULL || pChildNode == NULL)
		return nullptr;

	// 2. 부분 좌회전.
	ChangeRightSubTree(pNode, (BTreeNode*)LLRotation(pChildNode));

	// 3. 우회전
	return RRRotation(pNode);
}

// 높이 구하기
int CAVLTree::GetHeight(BTreeNode* pHeightNode)
{
	if (pHeightNode == NULL)
		return 0;

	int leftH = GetHeight(pHeightNode->stpLeftChild);
	int rightH = GetHeight(pHeightNode->stpRightChild);

	// 큰 값의 높이 반환.
	return 1 + (leftH > rightH ? leftH : rightH);
}

// 균형 인수 구하기
int CAVLTree::GetHeightDiff(BTreeNode* pDiffNode)
{
	if (pDiffNode == NULL)
		return 0;

	int leftSubH = GetHeight(pDiffNode->stpLeftChild);
	int rightSubH = GetHeight(pDiffNode->stpRightChild);

	// 균형 인수 계산 결과 반환.
	return leftSubH - rightSubH;
}

// 리밸런싱 하기
char* CAVLTree::Rebalance(BTreeNode* pbalanceNode)
{
	// 균형 인수 계산
	int iDiff = GetHeightDiff(pbalanceNode);

	// 균형 인수가 +2 이상이면 LL상태 또는 LR 상태이다.
	if (iDiff > 1)
	{
		// LL상태라면
		if (GetHeightDiff(pbalanceNode->stpLeftChild) > 0)
			pbalanceNode = (BTreeNode*)LLRotation(pbalanceNode);

		// LR상태라면
		else
			pbalanceNode = (BTreeNode*)LRRotation(pbalanceNode);
	}

	// 균형 인수가 -2 이하라면 RR상태 또는 RL 상태이다.
	if (iDiff < -1)
	{
		// RR상태라면
		if (GetHeightDiff(pbalanceNode->stpRightChild) < 0)
			pbalanceNode = (BTreeNode*)RRRotation(pbalanceNode);

		// RL상태라면
		else
			pbalanceNode = (BTreeNode*)RLRotation(pbalanceNode);
	}

	return (char*)pbalanceNode;
}

// 중위 순회
void CAVLTree::InorderTraversal(BTreeNode* RootNode)
{
	if (RootNode == NULL)
		return;

	InorderTraversal(RootNode->stpLeftChild);

	if(RootNode == m_pRootNode)
		printf("(rt %d) ", RootNode->iNodeData);

	else
		printf("%d ", RootNode->iNodeData);

	InorderTraversal(RootNode->stpRightChild);
}


