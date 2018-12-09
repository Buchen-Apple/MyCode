#include "pch.h"
#include "ExpressionTree.h"
#include "windows.h"
#include <iostream>
#include <stdio.h>

using namespace std;

// 출력용 함수
void ShowAction(BTData data)
{
	// 피연산자와 연산자는 다르게 출력되어야 한다.
	if (0 <= data && data <= 9)
		cout << data << " ";

	else
		printf("%c ", data);
}


// 수식트리 제작
BTNode* ExpressionTree::CreateExpTree(char exp[])
{
	// 인자로 받은 exp를 후위 표기법으로 변환
	m_ConvClass.ConvToRPNExp(exp);

	// 후위 표기법으로 변환된 문자를 수식트리로 변환
	listStack<BTNode*> stack;
	stack.Init();

	int expLen = strlen(exp);
	int TempIndex = 0;
	int i = 0;

	// 하나씩 빼면서 확인한다.
	while (i < expLen)
	{
		// 새로운 노드 생성
		BTNode* NewNode = m_BTtool.MakeBTreeNode();

		// 피연산자인지 확인
		if (isdigit(exp[i]))
		{
			// 피 연산자라면 문자를 정수로 변경해 노드에 저장
			m_BTtool.SetData(NewNode, exp[i] - '0');
		}

		// 연산자일 경우
		else
		{
			BTNode* PopNode;

			// 스택에 있는 2개의 피연산자를 꺼내온 후, 새로운 노드의 좌/우에 연결
			// 먼저 튀어나온 것이 오른쪽에 연결
			stack.Pop(&PopNode);
			m_BTtool.SetRightSubTree(NewNode, PopNode);

			stack.Pop(&PopNode);
			m_BTtool.SetLeftSubTree(NewNode, PopNode);

			m_BTtool.SetData(NewNode, exp[i]);
		}

		// 새로운 노드를 스택에 Push
		stack.Push(NewNode);

		++i;
	}

	// 루트 반환
	BTNode* Root;

	stack.Pop(&Root);
	return Root;
}

// 수식트리 계산
int ExpressionTree::ExpTreeResult(BTNode* Root)
{
	// 더 이상 노드가 없을 경우 data 리턴
	if (m_BTtool.GetLeftSubTree(Root) == nullptr &&
		m_BTtool.GetRightSubTree(Root) == nullptr)
	{
		return m_BTtool.GetData(Root);
	}

	int Result = 0;

	// 해당 노드의 왼쪽, 오른쪽을 알아온다.
	int op1 = ExpTreeResult(m_BTtool.GetLeftSubTree(Root));
	int op2 = ExpTreeResult(m_BTtool.GetRightSubTree(Root));

	// 현재 노드의 연산에 따라 계산
	switch (m_BTtool.GetData(Root))
	{
	case '+':
		Result = op1 + op2;
		break;

	case '-':
		Result = op1 - op2;
		break;

	case '*':
		Result = op1 * op2;
		break;

	case '/':
		Result = op1 / op2;
		break;
	}

	return Result;
}

// 전위 표기법 기반 출력
void ExpressionTree::ShowPreOrder(BTNode* Root)
{
	m_BTtool.PreorderTraverse(Root, ShowAction);
}

// 중위 표기법 기반 출력
void ExpressionTree::ShowInOrder(BTNode* Root)
{
	// 탈출 조건. 노드가 nullptr이면 끝에 도착한것.
	if (Root == nullptr)
		return;	

	int RootData = m_BTtool.GetData(Root);

	// 연산자라면 괄호 친다
	if ('*' <= RootData && RootData <= '/')
		printf("(");

	// 왼쪽 보고
	ShowInOrder(m_BTtool.GetLeftSubTree(Root));	

	// 루트 보고
	ShowAction(m_BTtool.GetData(Root));	

	// 오른쪽 보고
	ShowInOrder(m_BTtool.GetRightSubTree(Root));

	// 연산자라면 괄호 친다
	if ('*' <= RootData && RootData <= '/')
		printf(") ");
}

// 후위 표기법 기반 출력
void ExpressionTree::ShowPostOrder(BTNode* Root)
{
	m_BTtool.PostorderTraverse(Root, ShowAction);
}

