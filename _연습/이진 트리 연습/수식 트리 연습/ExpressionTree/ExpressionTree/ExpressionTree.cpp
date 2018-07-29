#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ListBaseStack.h"
#include "ExpressionTree.h"

// 수식 트리 구성
BTreeNode* MakeExpTree(char exp[])
{
	Stack stack;
	BTreeNode* pnode;

	size_t explen = strlen(exp);
	int i;

	StackInit(&stack);

	for (i = 0; i < explen; ++i)
	{
		pnode = MakeBTreeNode();

		// 숫자라면
		if (isdigit(exp[i]))
			SetData(pnode, exp[i] - '0');

		// 연산자라면
		else
		{
			MakeRightSubTree(pnode, SPop(&stack));
			MakeLeftSubTree(pnode, SPop(&stack));
			SetData(pnode, exp[i]);
		}

		SPush(&stack, pnode);
	}

	return SPop(&stack);
}

// 수식 트리 계산
int EvaluateExpTree(BTreeNode* bt)
{
	int op1, op2;

	if (GetLeftSubTree(bt) == NULL && GetRightSubTree(bt) == NULL)
		return GetData(bt);

	op1 = EvaluateExpTree(GetLeftSubTree(bt));
	op2 = EvaluateExpTree(GetRightSubTree(bt));

	switch (GetData(bt))
	{
	case '+':
		return op1 + op2;
	case '-':
		return op1 - op2;
	case '*':
		return op1 * op2;
	case '/':
		return op1 / op2;
	}

	return 0;
}

// 값 출력함수
void ShowNodeData(int data)
{
	if (0 <= data && data <= 9)
		printf("%d ", data);
	else
		printf("%c ", data);
}

// 전위 표기법 기반 출력
void ShowPreFixTypeExp(BTreeNode* bt)
{
	FirstTraverse(bt, ShowNodeData);
}

// 중위 표기법 기반 출력
void ShowInFixTypeExp(BTreeNode* bt)
{	
	SecondTraverse(bt, ShowNodeData);	
}

// 후위 표기법 기반 출력
void ShowPostFixTypeExp(BTreeNode* bt)
{
	EndTraverse(bt, ShowNodeData);
}