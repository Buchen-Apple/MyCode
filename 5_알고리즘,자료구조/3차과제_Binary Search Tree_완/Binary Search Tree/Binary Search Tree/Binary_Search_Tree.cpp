#include <stdio.h>
#include <stdlib.h>
#include "Binary_Search_Tree.h"

// 생성자
CBST::CBST(BData data)
{
	m_pRootNode = (BTreeNode*)malloc(sizeof(BTreeNode));

	m_pRootNode->iNodeData = data;
	m_pRootNode->stpLeftChild = NULL;
	m_pRootNode->stpRightChild = NULL;
}

// 삽입 함수
bool CBST::Insert(BData data)
{
	BTreeNode* pSerchNode = m_pRootNode;
	BTreeNode* pParentNode;
	bool bPostionFalg = false;
	// bPostionFalg == false : 왼쪽 자식에 연결
	// bPostionFalg == true	 : 오른쪽 자식에 연결

	// 들어갈 위치 찾기
	while (1)
	{
		// 1. pParentNode에 pSerchNode를 넣어서 부모 위치 저장.
		pParentNode = pSerchNode;

		// 2-1. [넣으려는 data < 부모의 data]라면, pSerchNode에 부모의 왼쪽 자식 위치를 pSerchNode에 셋팅
		if (pParentNode->iNodeData > data)
		{
			pSerchNode = pParentNode->stpLeftChild;
			// 3. 자식이 NULL이라면 들어갈 위치를 찾은 것
			if (pSerchNode == NULL)
			{
				// bPostionFalg를 false로 하고 나간다.
				bPostionFalg = false;
				break;
			}
		}

		// 2-2. [넣으려는 data > 부모의 data]라면, pSerchNode에 부모의 오른쪽 자식 위치를 pSerchNode에 셋팅
		else if (pParentNode->iNodeData < data)
		{
			pSerchNode = pParentNode->stpRightChild;

			// 3. 자식이 NULL이라면 들어갈 위치를 찾은 것
			if (pSerchNode == NULL)
			{
				// bPostionFalg를 true로 하고 나간다.
				bPostionFalg = true;
				break;
			}
		}

		// 2-3. [부모의 data == 넣으려는 data]라면, 트리에 못넣음.
		else
		{
			printf("Insert(). 왼쪽 / 중복된 데이터입니다. 시도한 값 : %d\n", data);
			return false;
		}
	}

	// 4. 위치를 찾았으면, 데이터 셋팅
	pSerchNode = (BTreeNode*)malloc(sizeof(BTreeNode));
	pSerchNode->iNodeData = data;
	pSerchNode->stpLeftChild = NULL;
	pSerchNode->stpRightChild = NULL;

	// 5. 부모와 연결
	if (bPostionFalg == false)
		pParentNode->stpLeftChild = pSerchNode;
	else
		pParentNode->stpRightChild = pSerchNode;

	return true;
}

// 삭제 함수
bool CBST::Delete(BData data)
{	
	BTreeNode* pNowNode = m_pRootNode;
	BTreeNode* pParentNode;
	bool bPostionFalg = false;
	// bPostionFalg == false : pNowNode는 pParentNode의 왼쪽 자식이다.
	// bPostionFalg == true	 :  pNowNode는 pParentNode의 오른쪽 자식이다.

	// 삭제하려는 노드 찾기.
	while (1)	
	{	
		// 1. 부모 노드 셋팅
		pParentNode = pNowNode;

		// 2. 내 위치 셋팅
		// 2-1. [부모 노드 data > 찾으려는 data]라면, 나는 부모의 왼쪽 노드가 된다.
		if (pParentNode->iNodeData > data)
		{
			pNowNode = pParentNode->stpLeftChild;
			bPostionFalg = false;
		}

		// 2-2. [부모 노드 data < 찾으려는 data]라면, 나는 부모의 오른쪽 노드가 된다.
		else if (pParentNode->iNodeData < data)
		{
			pNowNode = pParentNode->stpRightChild;
			bPostionFalg = true;
		}

		// 2-3. [부모 노드 data == 찾으려는 data]라면. 이 경우는, 루트노드를 삭제하는 경우만 해당.
		else
			break;

		// 종료 조건. pNowNode가 NULL이면, 리프 노드(잎사귀 노드)까지 모두 검색했는데도 원하는 값이 없는 것. 
		if (pNowNode == NULL)
			return false;


		// 3. 내가 현재 삭제되어야 하는 노드인지 체크
		else if (pNowNode->iNodeData == data)
			break;
	}

	// 삭제할 노드를 찾음. (pNowNode)
	// 실제 삭제 절차 진행	

	// 1-1. 내 자식이 0명이라면, 그냥 삭제
	if (pNowNode->stpLeftChild == NULL && pNowNode->stpRightChild == NULL)
	{
		// 1-1-1. 내가 부모의 왼쪽 자식이라면, 부모의 왼쪽을 NULL로 만듬.
		if (bPostionFalg == false)
			pParentNode->stpLeftChild = NULL;

		// 1-1-2. 내가 부모의 오른쪽 자식이라면, 부모의 오른쪽을 NULL로 만듬.
		else
			pParentNode->stpRightChild = NULL;

		// 내 자식이 0명인데, 삭제한 노드가 루트노드라면 트리가 완전히 소멸된 것.
		if (pParentNode == m_pRootNode)
			m_pRootNode->iNodeData = NULL;

		// 2. 그리고, 나를 동적해제
		free(pNowNode);
	}

	// 1-2. 내 양쪽에 모두 자식이 있다면, 자식 중 1개를 선정한다.
	// 선정 기준 : 내 모든 왼쪽 자식 중 가장 값이 큰 놈 or 내 모든 오른쪽 자식 중 가장 값이 작은 놈
	// 이 중 나는, 왼쪽 자식 중 가장 값이 큰 놈으로 하기로 함.
	else if (pNowNode->stpLeftChild != NULL && pNowNode->stpRightChild != NULL)
	{
		// 변수 선언 시점에, pBigNode에는 pNowNode의 왼쪽 노드가 들어있다.
		BTreeNode* pBigNode = pNowNode->stpLeftChild;
		BTreeNode* pBigRightNode = pBigNode->stpRightChild;
		BTreeNode* pBigParentNode = pNowNode;

		// 1. 왼쪽 자식 중 가장 값이 큰 노드 찾기.
		// 오른쪽 자식이 NULL이 나올 때 까지 찾는다.
		while (1)
		{
			// 오른쪽 자식이 NULL이면 위치를 찾은 것.
			if (pBigRightNode == NULL)
				break;

			pBigParentNode = pBigNode;
			pBigNode = pBigRightNode;
			pBigRightNode = pBigRightNode->stpRightChild;
		}

		// 2. pBigParentNode와 pBigNode의 연결을 끊는다.
		// 2-1. pBigNode가 부모의 왼쪽 자식인 경우 (즉, 삭제하려는 노드의 왼쪽에 자식이 1개밖에 없는 경우)
		if (pBigParentNode->stpLeftChild == pBigNode)
		{
			pBigParentNode->stpLeftChild = NULL;
		}

		// 2-2. pBigNode가 부모의 오른쪽 자식인 경우 (대부분의 경우)
		else if (pBigParentNode->stpRightChild == pBigNode)
		{
			pBigParentNode->stpRightChild = NULL;
		}

		// 3. pBigNode에는 pNowNode의 왼쪽 노드에서 가장 큰 값이 들어있는 노드가 위치한다.
		// 이제 pBigNode의 값을 pNowNode의 값으로 대입.
		pNowNode->iNodeData = pBigNode->iNodeData;

		// 4. 그리고, pBigNode를 동적해제
		free(pBigNode);
	}

	// 1-3. 내 왼쪽에 자식이 있다면, 부모와 내 왼쪽 자식을 연결
	else if (pNowNode->stpLeftChild != NULL)
	{	
		// 1-3-1. 내가(삭제하려는 노드)가 Root노드라면.		
		if (pNowNode == m_pRootNode)
		{
			BTreeNode* pNowNodeLeft = pNowNode->stpLeftChild;

			// 내 왼쪽 자식의 값을 루트노드에 저장하고 
			pNowNode->iNodeData = pNowNodeLeft->iNodeData;

			// 루트노드의 왼쪽이, 왼쪽 자식의 왼쪽 자식을 가리킨다.
			pNowNode->stpLeftChild = pNowNodeLeft->stpLeftChild;

			// 2. 그리고, 루트노드의 바로 왼쪽 노드를 동적해제
			free(pNowNodeLeft);
		}

		// 1-3-2. 내가 부모의 왼쪽 자식이라면, 부모의 왼쪽에 내 왼쪽 자식 연결
		else if (bPostionFalg == false)
		{
			pParentNode->stpLeftChild = pNowNode->stpLeftChild;

			// 2. 그리고, 나를 동적해제
			free(pNowNode);
		}

		// 1-3-3. 내가 부모의 오른쪽 자식이라면, 부모의 오른쪽에 내 왼쪽 자식 연결
		else if (bPostionFalg == true)
		{
			pParentNode->stpRightChild = pNowNode->stpLeftChild;

			// 2. 그리고, 나를 동적해제
			free(pNowNode);
		}
	}

	// 1-4. 내 오른쪽에 자식이 있다면, 부모와 내 오른쪽 자식을 연결
	else if (pNowNode->stpRightChild != NULL)
	{
		// 1-4-1. 내가(삭제하려는 노드)가 Root노드라면.		
		if (pNowNode == m_pRootNode)
		{
			BTreeNode* pNowNodeRight = pNowNode->stpRightChild;

			// 내 오른쪽 자식의 값을 루트노드에 저장하고 
			pNowNode->iNodeData = pNowNodeRight->iNodeData;

			// 루트노드의 오른쪽이, 오른쪽 자식의 오른쪽 자식을 가리킨다.
			pNowNode->stpRightChild = pNowNodeRight->stpRightChild;

			// 2. 그리고, 루트노드의 바로 오른쪽 노드를 동적해제
			free(pNowNodeRight);
		}

		// 1-4-1. 내가 부모의 왼쪽 자식이라면, 부모의 왼쪽에 내 오른쪽 자식 연결
		else if (bPostionFalg == false)
			pParentNode->stpLeftChild = pNowNode->stpRightChild;

		// 1-4-2. 내가 부모의 오른쪽 자식이라면, 부모의 오른쪽에 내 오른쪽 자식 연결
		else if (bPostionFalg == true)
			pParentNode->stpRightChild = pNowNode->stpRightChild;

		// 2. 그리고, 나를 동적해제
		free(pNowNode);
	}

	return true;
}

// 순회 함수
void CBST::Traversal()
{
	// 중위 순회
	InorderTraversal(m_pRootNode);

}

// 중위 검색
void CBST::InorderTraversal(BTreeNode* pSearchNode)
{
	if (pSearchNode == NULL)
		return;

	InorderTraversal(pSearchNode->stpLeftChild);
	printf("%d ", pSearchNode->iNodeData);
	InorderTraversal(pSearchNode->stpRightChild);
}

// 루트 노드의 값 반환
BData CBST::GetRootNodeData()
{
	return m_pRootNode->iNodeData;
}