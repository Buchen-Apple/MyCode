#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "Red_Black_Tree.h"

// 생성자
CRBT::CRBT()
{
	// 닐 노드 셋팅
	m_pNilNode.eNodeColor = BLACK;
	m_pNilNode.stpLeftChild = NULL;
	m_pNilNode.stpRightChild = NULL;
	m_pNilNode.stpParent = NULL;

	// RootNode는 기본 NULL로 셋팅
	m_pRootNode = NULL;
}

// 삽입 함수
bool CRBT::Insert(BData data)
{
	// RootNode가 NULL이라면, 최초 노드 등록이니, data로 루트노드를 셋팅하고 return한다.
	if (m_pRootNode == NULL)
	{
		// RootNode 셋팅
		m_pRootNode = (BTreeNode*)malloc(sizeof(BTreeNode));

		m_pRootNode->iNodeData = data;
		m_pRootNode->stpLeftChild = &m_pNilNode;
		m_pRootNode->stpRightChild = &m_pNilNode;
		m_pRootNode->stpParent = NULL;
		m_pRootNode->eNodeColor = BLACK;

		return true;
	}

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
			if (pSerchNode == &m_pNilNode)
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
			if (pSerchNode == &m_pNilNode)
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
	pSerchNode->stpLeftChild = &m_pNilNode;
	pSerchNode->stpRightChild = &m_pNilNode;
	pSerchNode->stpParent = pParentNode;
	pSerchNode->eNodeColor = NODE_COLOR::RED;

	// 5. 부모와 연결
	if (bPostionFalg == false)
		pParentNode->stpLeftChild = pSerchNode;
	else
		pParentNode->stpRightChild = pSerchNode;

	// 6. 새로 추가한 노드의 부모가 Red라면, 문제 해결을 위한 함수 호출
	if (pSerchNode->stpParent->eNodeColor == RED)
		InsertColorFixup(pSerchNode);

	return true;
}

// 삭제 함수
bool CRBT::Delete(BData data)
{
	BTreeNode* pNowNode = m_pRootNode;
	BTreeNode* pParentNode;
	BTreeNode* CheckNode = &m_pNilNode;
	bool bPostionFalg = false;
	// bPostionFalg == false : pNowNode는 pParentNode의 왼쪽 자식이다.
	// bPostionFalg == true	 :  pNowNode는 pParentNode의 오른쪽 자식이다.

	bool bDeleteColor = false;
	// bDeleteColor == false : 이번에 삭제한 노드가 Red컬러이다.
	// bDeleteColor == true	 : 이번에 삭제한 노드가 Black컬러이다.

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
		if (pNowNode == &m_pNilNode)
			return false;


		// 3. 내가 현재 삭제되어야 하는 노드인지 체크
		else if (pNowNode->iNodeData == data)
			break;
	}

	// 삭제할 노드를 찾음. (pNowNode)
	// 실제 삭제 절차 진행	

	// 1-1. 내 자식이 0명이라면, 그냥 삭제
	if (pNowNode->stpLeftChild == &m_pNilNode && pNowNode->stpRightChild == &m_pNilNode)
	{
		// 1-1-1. 내가 부모의 왼쪽 자식이라면, 부모의 왼쪽을 Nil로 만듬.
		if (bPostionFalg == false)
			pParentNode->stpLeftChild = &m_pNilNode;

		// 1-1-2. 내가 부모의 오른쪽 자식이라면, 부모의 오른쪽을 Nil로 만듬.
		else
			pParentNode->stpRightChild = &m_pNilNode;

		// 삭제하려는 노드가 루트노드라면 트리가 완전히 소멸된 것.
		if (pNowNode == m_pRootNode)
			m_pRootNode = NULL;

		// 삭제할 노드에 따라 bDeleteColor변수를 셋팅한다. 이 값 가지고, 색상맞추기 함수를 호출해야하는지 체크
		if (pNowNode->eNodeColor == BLACK)
			bDeleteColor = true;

		// 2. 그리고, 나를 동적해제
		free(pNowNode);

		// 실제 삭제한 노드의 자식 노드를 셋팅. 컬러 색을 맞추기 위해!
		// 근데 자식이 0명인 것은, 색 맞출것도 없다. 그리고 CheckNode의 값 셋팅 안함.
	}

	// 1-2. 내 양쪽에 모두 자식이 있다면, 자식 중 1개를 선정한다.
	// 1-3. 내 왼쪽 자식이 있을 경우, 자식 중 1개를 선정한다.
	// 둘이 같이 해결
	//
	// 내 왼쪽 자식 중 가장 값이 큰 놈을 찾는다.
	else if ((pNowNode->stpLeftChild != &m_pNilNode && pNowNode->stpRightChild != &m_pNilNode) || pNowNode->stpLeftChild != &m_pNilNode)
	{
		// 변수 선언 시점에, pBigNode에는 pNowNode의 왼쪽 노드가 들어있다.
		BTreeNode* pBigNode = pNowNode->stpLeftChild;
		BTreeNode* pBigRightNode = pBigNode->stpRightChild;
		BTreeNode* pBigParentNode = pNowNode;

		// 1. 왼쪽 자식 중 가장 값이 큰 노드 찾기.
		// 오른쪽 자식이 NULL이 나올 때 까지 찾는다.
		while (1)
		{
			// 오른쪽 자식이 Nil이면 위치를 찾은 것.
			if (pBigRightNode == &m_pNilNode)
				break;

			pBigParentNode = pBigNode;
			pBigNode = pBigRightNode;
			pBigRightNode = pBigRightNode->stpRightChild;
		}

		// pBigNode에 실제 삭제하려는 노드가 들어있다.
		// 삭제할 노드에 따라 bDeleteColor변수를 셋팅한다. 이 값 가지고, 색상맞추기 함수를 호출해야하는지 체크
		if (pBigNode->eNodeColor == BLACK)
			bDeleteColor = true;

		// 2. pBigParentNode와 pBigNode의 연결을 끊는다.
		// 2-1. pBigNode가 부모의 왼쪽 자식인 경우 (즉, 삭제하려는 노드의 왼쪽에 자식이 1개밖에 없는 경우)
		// pBigParentNode가 왼쪽으로, pBigNode의 왼쪽 자식을 가리킨다.
		if (pBigParentNode->stpLeftChild == pBigNode)
		{
			//pBigParentNode->stpLeftChild = &m_pNilNode;
			pBigParentNode->stpLeftChild = pBigNode->stpLeftChild;

			// 실제 삭제한 노드의 자식 노드를 셋팅. 컬러 색을 맞추기 위해!
			CheckNode = pBigNode->stpLeftChild;
			CheckNode->stpParent = pBigParentNode;
		}

		// 2-2. pBigNode가 부모의 오른쪽 자식인 경우 (대부분의 경우)
		// pBigParentNode가 오른쪽으로, pBigNode의 오른쪽 자식을 가리킨다.
		else if (pBigParentNode->stpRightChild == pBigNode)
		{
			// 근데, pBigNode의 오른쪽 자식이 Nil일 경우, pBigNode의 왼쪽 자식을 가리킨다.
			if (pBigNode->stpRightChild == &m_pNilNode)
			{
				pBigParentNode->stpRightChild = pBigNode->stpLeftChild;

				// 실제 삭제한 노드의 자식 노드를 셋팅. 컬러 색을 맞추기 위해!
				CheckNode = pBigNode->stpLeftChild;
				CheckNode->stpParent = pBigParentNode;
			}

			// 그게 아니라면, pBigNode의 오른쪽 자식을 가리킨다.
			else
			{
				pBigParentNode->stpRightChild = pBigNode->stpRightChild;

				// 실제 삭제한 노드의 자식 노드를 셋팅. 컬러 색을 맞추기 위해!
				CheckNode = pBigNode->stpRightChild;
				CheckNode->stpParent = pBigParentNode;
			}			
		}		

		// 3. pBigNode에는 pNowNode의 왼쪽 노드에서 가장 큰 값이 들어있는 노드가 위치한다.
		// 이제 pBigNode의 정보를 pNowNode로 대입. (데이터)
		pNowNode->iNodeData = pBigNode->iNodeData;		

		// 4. 그리고, pBigNode를 동적해제
		free(pBigNode);
	}	

	// 1-4. 내 오른쪽에 자식이 있다면, 부모와 내 오른쪽 자식을 연결
	// 내 오른쪽 노드 중 가장 값이 작은 노드를 찾는다.
	else if (pNowNode->stpRightChild != &m_pNilNode)
	{
		// 변수 선언 시점에, pBigNode에는 pNowNode의 오른쪽 노드가 들어있다.
		BTreeNode* pBigNode = pNowNode->stpRightChild;
		BTreeNode* pBigLeftNode = pBigNode->stpLeftChild;
		BTreeNode* pBigParentNode = pNowNode;

		// 1. 오른쪽 자식 중 가장 값이 작은 노드 찾기.
		// 왼쪽 자식이 Nil이 나올 때 까지 찾는다.
		while (1)
		{
			// 왼쪽 자식이 Nil이면 위치를 찾은 것.
			if (pBigLeftNode == &m_pNilNode)
				break;

			pBigParentNode = pBigNode;
			pBigNode = pBigLeftNode;
			pBigLeftNode = pBigLeftNode->stpLeftChild;
		}

		// pBigNode에 실제 삭제하려는 노드가 들어있다.
		// 삭제할 노드에 따라 bDeleteColor변수를 셋팅한다. 이 값 가지고, 색상맞추기 함수를 호출해야하는지 체크
		if (pBigNode->eNodeColor == BLACK)
			bDeleteColor = true;

		// 2. pBigParentNode와 pBigNode의 연결을 끊는다.
		// 2-1. pBigNode가 부모의 오른쪽 자식인 경우 (즉, 삭제하려는 노드의 오른쪽에 자식이 1개밖에 없는 경우)
		// pBigParentNode가 오른쪽으로, pBigNode의 오른쪽 자식을 가리킨다.
		if (pBigParentNode->stpRightChild == pBigNode)
		{
			pBigParentNode->stpRightChild = pBigNode->stpRightChild;

			// 실제 삭제한 노드의 자식 노드를 셋팅. 컬러 색을 맞추기 위해!
			CheckNode = pBigNode->stpRightChild;
			CheckNode->stpParent = pBigParentNode;
		}

		// 2-2. pBigNode가 부모의 왼쪽 자식인 경우 (대부분의 경우)
		// pBigParentNode가 왼쪽으로, pBigNode의 왼쪽 자식을 가리킨다.
		else if (pBigParentNode->stpLeftChild == pBigNode)
		{
			// 근데, pBigNode의 왼쪽 자식이 Nil일 경우, pBigNode의 오른쪽 자식을 가리킨다.
			if (pBigNode->stpLeftChild == &m_pNilNode)
			{
				pBigParentNode->stpLeftChild = pBigNode->stpRightChild;

				// 실제 삭제한 노드의 자식 노드를 셋팅. 컬러 색을 맞추기 위해!
				CheckNode = pBigNode->stpRightChild;
				CheckNode->stpParent = pBigParentNode;
			}

			// 그게 아니라면, pBigNode의 왼쪽 자식을 가리킨다.
			else
			{
				pBigParentNode->stpLeftChild = pBigNode->stpLeftChild;

				// 실제 삭제한 노드의 자식 노드를 셋팅. 컬러 색을 맞추기 위해!
				CheckNode = pBigNode->stpLeftChild;
				CheckNode->stpParent = pBigParentNode;
			}
		}

		// 3. pBigNode에는 pNowNode의 왼쪽 노드에서 가장 큰 값이 들어있는 노드가 위치한다.
		// 이제 pBigNode의 정보를 pNowNode로 대입. (데이터)
		pNowNode->iNodeData = pBigNode->iNodeData;

		// 4. 그리고, pBigNode를 동적해제
		free(pBigNode);	
	}

	// 삭제가 완료되었으면, 색 체크 함수 호출.
	// 실제 삭제한 노드의, 자식 노드를 기준으로 색 체크.

	// CheckNode가 닐노드가 아니고, 실제 삭제한 노드의 색이 BLACK일 때만 함수 호출
	if(CheckNode != &m_pNilNode && bDeleteColor == true)
		DeleteColorFixup(CheckNode);

	return true;
}

// 순회 함수
void CRBT::Traversal(HDC hdc, RECT rt)
{
	// 순회하면서 원과 글자, 선을 찍는다.
	// 원을 찍을 위치를 잡기 위해 일단 루트의 위치 설정.
	int x = rt.right /2;
	int y = rt.top + 30;

	// 전위 순회하며 프린트하기.
	if(m_pRootNode != NULL)
		preorderTraversal(m_pRootNode, hdc, x, y);
}

// 전위 순회하며 프린트 하기
void CRBT::preorderTraversal(BTreeNode* pSearchNode, HDC hdc, double x, double y)
{
	// 리턴 조건. 현재 노드가 닐이거나 널노드면 리턴.
	if (pSearchNode == &m_pNilNode)
		return;

	// 1. 원 도형의 색 지정.
	HBRUSH MyBrush, OldBrush;
	if (pSearchNode->eNodeColor == BLACK)
	{
		MyBrush = CreateSolidBrush(RGB(0, 0, 0));
		OldBrush = (HBRUSH)SelectObject(hdc, MyBrush);
	}
	else
	{
		MyBrush = CreateSolidBrush(RGB(255, 0, 0));
		OldBrush = (HBRUSH)SelectObject(hdc, MyBrush);
	}

	// 2. 인자로 받은 x,y 기준으로 원과 숫자 찍기
	Ellipse(hdc, x - 20, y - 20, x + 20, y + 20);
	TCHAR string[5];
	_itow_s(pSearchNode->iNodeData, string, _countof(string), 10);
	TextOut(hdc, x - 5, y - 5, string, _tcslen(string));
	SelectObject(hdc, OldBrush);
	DeleteObject(MyBrush);
	
	// 3. 왼쪽 노드의, 오른쪽 뎁스가 얼마나 깊은지 체크
	// 내 왼쪽 노드가 닐노드가 아니고, 
	// 왼쪽 노드의, 오른쪽 노드도 닐노드가 아니라면 왼쪽, 오른쪽 노드의 높이 체크 시작

	// 뎁스 저장용 변수.
	int LeftRightDepth = 1;

	// 높이 저장용 변수
	int LeftRightHeight;
	if (pSearchNode->stpLeftChild != &m_pNilNode && pSearchNode->stpLeftChild->stpRightChild != &m_pNilNode)
	{
		LeftRightHeight = SetHeight(pSearchNode->stpLeftChild->stpRightChild);

		for (int i = 0; i < LeftRightHeight; ++i)
			LeftRightDepth *= 2;
	}	

	// 4. 나(pSearchNode) 부터 왼쪽으로 선긋기. 내 왼쪽이 Nil노드라면 선 안긋는다.
	double dX = x;
	double dY = y + 20;
	if (pSearchNode->stpLeftChild != &m_pNilNode)
	{
		MoveToEx(hdc, x, dY, NULL);

		double yMul50 = 50 / LeftRightDepth;

		for (int i = 0; i < LeftRightDepth; ++i)
		{
			dX -= 50;
			dY += yMul50;
			LineTo(hdc, dX, dY);
		}

	}
	preorderTraversal(pSearchNode->stpLeftChild, hdc, dX, dY);

	// 5. 오른쪽 노드의, 왼쪽 뎁스가 얼마나 깊은지 체크
	// 내 오른쪽 노드가 닐노드가 아니고, 
	// 오른쪽 노드의, 왼쪽 노드도 닐노드가 아니라면 뎁스 체크 시작

	// 뎁스 저장용 변수.
	int RightLeftDepth = 1;

	// 높이 저장용 변수
	int RIghtLeftHeight;
	if (pSearchNode->stpRightChild != &m_pNilNode && pSearchNode->stpRightChild->stpLeftChild != &m_pNilNode)
	{
		RIghtLeftHeight = SetHeight(pSearchNode->stpRightChild->stpLeftChild);

		for (int i = 0; i < RIghtLeftHeight; ++i)
			RightLeftDepth *= 2;
	}

	// 6. 나(pSearchNode) 부터 오른쪽으로 선긋기. 내 오른쪽이 Nil노드라면 선 안긋는다.
	dX = x;
	dY = y + 20;

	if (pSearchNode->stpRightChild != &m_pNilNode)
	{
		MoveToEx(hdc, x, dY, NULL);

		double yMul50 = 50 / RightLeftDepth;

		for (int i = 0; i < RightLeftDepth; ++i)
		{
			dX += 50;
			dY += yMul50;
			LineTo(hdc, dX, dY);
		}
	}
	
	preorderTraversal(pSearchNode->stpRightChild, hdc, dX, dY);
}

// 좌회전
void CRBT::LeftRotation(BTreeNode* pRotateNode)
{
	// 인자로 전달받은 노드를 기준으로 좌회전 시킨다.
	// 그 전에, 만약, 나의 오른쪽 노드가 Nil 노드라면 좌회전 불가
	if (pRotateNode->stpRightChild == &m_pNilNode)
		return;

	BTreeNode* pRotateNode_Parent = pRotateNode->stpParent;
	BTreeNode* pRotateNode_Right = pRotateNode->stpRightChild;
	BTreeNode* pRotateNode_Righ_Left = pRotateNode_Right->stpLeftChild;

	// 1. pRotateNode가 오른쪽으로, pRotateNode의 오른쪽의 왼쪽 자식을 가리킴.
	pRotateNode->stpRightChild = pRotateNode_Righ_Left;

	// 2. pRotateNode의 오른쪽의 왼쪽 자식이 부모로, pRotateNode를 가리킴
	pRotateNode_Righ_Left->stpParent = pRotateNode;

	// 3. pRotateNode의 오른쪽 자식이 왼쪽으로, pRotateNode를 가리킴
	pRotateNode_Right->stpLeftChild = pRotateNode;

	// 4. pRotateNode의 오른쪽 자식이 부모로, pRotateNode의 부모를 가키림
	pRotateNode_Right->stpParent = pRotateNode->stpParent;

	// 5. 부모가 NULL이 아니라면, 부모와 pRotateNode의 자식 간 연결 진행
	if (pRotateNode_Parent != NULL)
	{
		// 5-1. 만약, pRotateNode가 부모의 왼쪽 자식이면, pRotateNode의 부모가 왼쪽으로, pRotateNode의 오른쪽 자식을 가리킴
		if (pRotateNode == pRotateNode_Parent->stpLeftChild)
			pRotateNode_Parent->stpLeftChild = pRotateNode_Right;

		// 5-2. 만약, pRotateNode가 부모의 오른쪽 자식이면, pRotateNode의 부모가 오른쪽으로, pRotateNode의 오른쪽 자식을 가리킴
		else if (pRotateNode == pRotateNode_Parent->stpRightChild)
			pRotateNode_Parent->stpRightChild = pRotateNode_Right;
	}

	// 6. 부모가 NULL이라면, 회전하는 노드가 루트노드라는것. 이 경우는, 루트 노드를 다시 설정해야한다. 회전으로 인해 루트노드가 변경됨
	else
		m_pRootNode = pRotateNode_Right;

	// 7. pRotateNode가 부모로,  pRotateNode의 오른쪽 자식을 가키림
	pRotateNode->stpParent = pRotateNode_Right;

}

// 우회전
void CRBT::RightRotation(BTreeNode* pRotateNode)
{
	// 인자로 전달받은 노드를 기준으로 우회전 시킨다.
	// 그 전에, 만약, 나의 왼쪽 노드가 Nil 노드라면 우회전 불가
	if (pRotateNode->stpLeftChild == &m_pNilNode)
		return;

	BTreeNode* pRotateNode_Parent = pRotateNode->stpParent;
	BTreeNode* pRotateNode_Left = pRotateNode->stpLeftChild;
	BTreeNode* pRotateNode_Left_Right = pRotateNode_Left->stpRightChild;

	// 1. pRotateNode가 왼쪽으로, pRotateNode의 왼쪽의 오른쪽 자식을 가키림.
	pRotateNode->stpLeftChild = pRotateNode_Left_Right;

	// 2. pRotateNode의 왼쪽의 오른쪽 자식이 부모로, pRotateNode를 가키림
	pRotateNode_Left_Right->stpParent = pRotateNode;

	// 3. pRotateNode의 왼쪽 자식이 오른쪽으로, pRotateNode를 가키림
	pRotateNode_Left->stpRightChild = pRotateNode;

	// 4. pRotateNode의 왼쪽 자식이 부모로, pRotateNode의 부모를 가키림
	pRotateNode_Left->stpParent = pRotateNode->stpParent;

	// 5. 부모가 NULL이 아니라면, 부모와 pRotateNode의 자식 간 연결 진행
	if (pRotateNode_Parent != NULL)
	{
		// 5-1. 만약, pRotateNode가 부모의 왼쪽 자식이면, pRotateNode의 부모가 왼쪽으로, pRotateNode의 왼쪽 자식을 가리킴
		if (pRotateNode == pRotateNode_Parent->stpLeftChild)
			pRotateNode_Parent->stpLeftChild = pRotateNode_Left;

		// 5-2. 만약, pRotateNode가 부모의 왼쪽 자식이면, pRotateNode의 부모가 오른쪽으로, pRotateNode의 왼쪽 자식을 가리킴
		else if (pRotateNode == pRotateNode_Parent->stpRightChild)
			pRotateNode_Parent->stpRightChild = pRotateNode_Left;
	}

	// 6. 부모가 NULL이라면, 회전하는 노드가 루트노드라는것. 이 경우는, 루트 노드를 다시 설정해야한다. 회전으로 인해 루트노드가 변경됨
	else
		m_pRootNode = pRotateNode_Left;	

	// 7. pRotateNode가 부모로,  pRotateNode의 왼쪽 자식을 가키림
	pRotateNode->stpParent = pRotateNode_Left;

}

// Insert 후 색 맞추기 처리
void CRBT::InsertColorFixup(BTreeNode* pZNode)
{
	// Insert 후 해당 함수가 호출되었으면, pZNode의 부모가 Red인 상태.
	// 케이스에 따라 처리한다.
	
	while (1)
	{
		// 종료조건 1. pZNode가 루트 노드가 될 경우 while문 종료.
		if (pZNode == m_pRootNode)
			break;

		// 종료조건 1. pZNode의 부모가 Red가 아니라면 종료.
		if (pZNode->stpParent->eNodeColor != RED)
			break;

		// 케이스 1,2,3은, 내 부모가 할아버지의 왼쪽 자식일 경우.
		if (pZNode->stpParent == pZNode->stpParent->stpParent->stpLeftChild)
		{
			// 내 부모 셋팅
			BTreeNode* pZNode_stpParent = pZNode->stpParent;

			// 할아버지 셋팅
			BTreeNode* pZNode_grandpa = pZNode_stpParent->stpParent;

			// 삼촌은, 내 할아버지의 오른쪽 자식이다.
			BTreeNode* pZNode_uncle = pZNode_grandpa->stpRightChild;

			// case 1. pZNode의 삼촌이 Red인 경우.
			// 부모와 삼촌을 black으로 변경. 그리고 할아버지를 Red로 변경한 후, 할아버지를 zNode로 지정한다.
			// 아직 문제 해결 안됨. 다시 while문을 돌면서 케이스 체크
			if (pZNode_uncle->eNodeColor == RED)
			{
				pZNode_stpParent->eNodeColor = BLACK;
				pZNode_uncle->eNodeColor = BLACK;

				pZNode_grandpa->eNodeColor = RED;
				pZNode = pZNode_grandpa;
			}

			// pZNode의 삼촌이 Black인 경우
			else if (pZNode_uncle->eNodeColor == BLACK)
			{
				// case 2. 삼촌이 Black이면서, 내가(pZNode) 부모의 오른쪽 자식일 경우
				// 내 부모를 기준으로 Left로테이션. 그리고 부모를 zNode로 설정한다. 그러면 바로 Case3이된다.
				if (pZNode == pZNode_stpParent->stpRightChild)
				{
					LeftRotation(pZNode_stpParent);
					pZNode = pZNode_stpParent;
				}					

				// case 3. 삼촌이 Black이면서, 내가(pZNode) 부모의 왼쪽 자식일 경우
				// 부모를 Black, 할아버지를 Red로 변경.
				// 할아버지를 기준으로 Right로테이션. 그러면 문제 해결!
				pZNode->stpParent->eNodeColor = BLACK;
				pZNode->stpParent->stpParent->eNodeColor = RED;
				RightRotation(pZNode->stpParent->stpParent);

				break;
			}

		}

		// 케이스 4,5,6은, 내 부모가 할아버지의 오른쪽 자식일 경우.
		else if (pZNode->stpParent == pZNode->stpParent->stpParent->stpRightChild)
		{
			// 내 부모 셋팅
			BTreeNode* pZNode_stpParent = pZNode->stpParent;

			// 할아버지 셋팅
			BTreeNode* pZNode_grandpa = pZNode_stpParent->stpParent;

			// 삼촌은, 내 할아버지의 왼쪽 자식이다.
			BTreeNode* pZNode_uncle = pZNode_grandpa->stpLeftChild;

			// case 4. pZNode의 삼촌이 Red인 경우.
			// 부모와 삼촌을 black으로 변경. 그리고 할아버지를 Red로 변경한 후, 할아버지를 zNode로 지정한다.
			// 아직 문제 해결 안됨. 다시 while문을 돌면서 케이스 체크
			if (pZNode_uncle->eNodeColor == RED)
			{
				pZNode_stpParent->eNodeColor = BLACK;
				pZNode_uncle->eNodeColor = BLACK;

				pZNode_grandpa->eNodeColor = RED;
				pZNode = pZNode_grandpa;
			}

			// pZNode의 삼촌이 Black인 경우
			else if (pZNode_uncle->eNodeColor == BLACK)
			{
				// case 5. 삼촌이 Black이면서, 내가(pZNode) 부모의 왼쪽 자식일 경우
				// 내 부모를 기준으로 Right로테이션. 그리고 부모를 zNode로 설정한다. 그러면 바로 Case3이된다.
				if (pZNode == pZNode_stpParent->stpLeftChild)
				{
					RightRotation(pZNode_stpParent);
					pZNode = pZNode_stpParent;
				}

				// case 6. 삼촌이 Black이면서, 내가(pZNode) 부모의 오른쪽 자식일 경우
				// 부모를 Black, 할아버지를 Red로 변경.
				// 할아버지를 기준으로 LEft로테이션. 그러면 문제 해결!
				pZNode->stpParent->eNodeColor = BLACK;
				pZNode->stpParent->stpParent->eNodeColor = RED;
				LeftRotation(pZNode->stpParent->stpParent);

				break;
			}
		}
	}

	// 케이스 처리 후, 무조건 루트노드를 BLACK으로 변경.
	m_pRootNode->eNodeColor = BLACK;
}

// 높이 구하는 함수
int  CRBT::SetHeight(BTreeNode* pHeightNode)
{
	if (pHeightNode == &m_pNilNode)
		return 0;

	else
	{
		int left_h = SetHeight(pHeightNode->stpLeftChild);
		int right_h = SetHeight(pHeightNode->stpRightChild);
		return 1 + (left_h > right_h ? left_h : right_h);
	}
	
}

// Delete 후 색 맞추기 처리
void CRBT::DeleteColorFixup(BTreeNode* pZNode)
{
	// pZNode노드에는, 실제로 [삭제한 노드의, 자식 노드]가 들어있다.
	// Nil이 올 가능성도 있다.
	// 이 때, [삭제한 노드의, 자식노드] 즉, pZNode는 double black 상태이다.
	
	while (1)
	{
		// case 번외. 내가 Red라면 나를 Black으로 바꾸고 끝
		if (pZNode->eNodeColor == RED)
		{
			pZNode->eNodeColor = BLACK;
			break;
		}

		// case 번외. 내가 Root노드라면, 더 이상 할 것도 없음.
		else if (pZNode == m_pRootNode)
			break;

		// case 번외를 지나오면, [나는 무조건 Black]이다.

		// pZNode가 부모의 왼쪽 자식인 경우, 케이스 1,2,3,4 체크
		if (pZNode == pZNode->stpParent->stpLeftChild)
		{
			// 내 형제를 SibilingNode에 설정한다.
			// 여기에 왔다는 것은, 내가 부모의 왼쪽 자식이라는 것이니, 부모의 오른쪽 자식을 형제로 설정.
			BTreeNode* SibilingNode = pZNode->stpParent->stpRightChild;			

			// case 1. 내가 Black이고, 내 형제가 Red인 경우
			// 부모를 Red, 내 형제를 Black으로 변경.
			// 그리고 부모를 기준으로 Left Lotation
			if (SibilingNode->eNodeColor == RED)
			{
				pZNode->stpParent->eNodeColor = RED;
				SibilingNode->eNodeColor = BLACK;
				LeftRotation(pZNode->stpParent);
			}

			// case 1을 지나오면 [내 형제는 무조건 Black]이다.

			// case 2. 내 형제의 모든 자식이 BLACK인 경우.
			// 나와 내 형제의 Black을 하나씩 뺏어서 부모에게 주는 개념.
			// 일단 형제는 Red로 만들고, 내 부모는 [이미 블랙이었으면 double black, Red였으면 Black이 되고 삭제 절차 끝]이 된다.
			else if (SibilingNode->stpLeftChild->eNodeColor == BLACK && SibilingNode->stpRightChild->eNodeColor == BLACK)
			{
				SibilingNode->eNodeColor = RED;

				// 부모의 컬러가 Red라면
				if (pZNode->stpParent->eNodeColor == RED)
				{
					pZNode->stpParent->eNodeColor = BLACK;
					break;
				}

				// 부모의 컬러가 Black이라면, 부모를 새로운 pZNode로 설정.
				pZNode = pZNode->stpParent;
			}

			// case 3. 내 형제의 좌측 자식이 Red인 경우
			// 형제를 Red로, 형제의 좌측 자식을 Black으로 만든다.
			// 그리고 형제 기준, Right Rotation
			// 이렇게되면 바로 케이스 4가된다.
			else if (SibilingNode->stpLeftChild->eNodeColor == RED)
			{
				SibilingNode->eNodeColor = RED;
				SibilingNode->stpLeftChild->eNodeColor = BLACK;

				RightRotation(SibilingNode);
			}

			// case 4. 내 형제의 우측 자식이 Red인 경우
			// 형제를 부모의 색으로, 부모/형제의 우측 자식을 BLACK으로 변경.
			// 부모 기준 Left Rotation.
			// 그리고 삭제절차 끝.
			else if (SibilingNode->stpRightChild->eNodeColor == RED)
			{
				SibilingNode->eNodeColor = pZNode->stpParent->eNodeColor;
				pZNode->stpParent->eNodeColor = SibilingNode->stpRightChild->eNodeColor = BLACK;

				LeftRotation(pZNode->stpParent);
				break;
			}
		}

		// pZNode가 부모의 오른쪽 자식인 경우, 케이스 5,6,7,8 체크
		else if (pZNode == pZNode->stpParent->stpRightChild)
		{
			// 내 형제를 SibilingNode에 설정한다.
			// 여기에 왔다는 것은, 내가 부모의 오른쪽 자식이라는 것이니, 부모의 왼쪽 자식을 형제로 설정.
			BTreeNode* SibilingNode = pZNode->stpParent->stpLeftChild;

			// case 5. 내 형제가 Red인 경우
			// 부모를 Red, 내 형제를 Black으로 변경.
			// 그리고 부모를 기준으로 Right Lotation
			if (SibilingNode->eNodeColor == RED)
			{
				pZNode->stpParent->eNodeColor = RED;
				SibilingNode->eNodeColor = BLACK;
				RightRotation(pZNode->stpParent);
			}

			// case 5을 지나오면 [내 형제는 무조건 Black]이다.

			// case 6. 내 형제의 모든 자식이 BLACK인 경우.
			// 나와 내 형제의 Black을 하나씩 뺏어서 부모에게 주는 개념.
			// 일단 형제는 Red로 만들고, 내 부모는 [이미 블랙이었으면 double black, Red였으면 Black이 되고 삭제 절차 끝]이 된다.
			else if (SibilingNode->stpLeftChild->eNodeColor == BLACK && SibilingNode->stpRightChild->eNodeColor == BLACK)
			{
				SibilingNode->eNodeColor = RED;

				// 부모의 컬러가 Red라면
				if (pZNode->stpParent->eNodeColor == RED)
				{
					pZNode->stpParent->eNodeColor = BLACK;
					break;
				}

				// 부모의 컬러가 Black이라면, 부모를 새로운 pZNode로 설정.
				pZNode = pZNode->stpParent;
			}

			// case 7. 내 형제의 우측 자식이 Red인 경우
			// 형제를 Red로, 형제의 우측 자식을 Black으로 만든다.
			// 그리고 형제 기준, Left Rotation
			// 이렇게되면 바로 케이스 4가된다.
			else if (SibilingNode->stpRightChild->eNodeColor == RED)
			{
				SibilingNode->eNodeColor = RED;
				SibilingNode->stpRightChild->eNodeColor = BLACK;

				LeftRotation(SibilingNode);
			}

			// case 8. 내 형제의 좌측 자식이 Red인 경우
			// 형제를 부모의 색으로, 부모/형제의 좌측 자식을 BLACK으로 변경.
			// 부모 기준 Right Rotation.
			// 그리고 삭제절차 끝.
			else if (SibilingNode->stpLeftChild->eNodeColor == RED)
			{
				SibilingNode->eNodeColor = pZNode->stpParent->eNodeColor;
				pZNode->stpParent->eNodeColor = SibilingNode->stpLeftChild->eNodeColor = BLACK;

				RightRotation(pZNode->stpParent);
				break;
			}

		}
	}

	// 삭제가 다 완료되면, 루트노드를 무조건 BLACK으로 변경
	m_pRootNode->eNodeColor = BLACK;

}