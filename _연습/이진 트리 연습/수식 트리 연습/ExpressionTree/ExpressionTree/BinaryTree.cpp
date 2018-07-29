#include "BinaryTree.h"
#include <stdlib.h>
#include <stdio.h>


////////////////////////////////////
// 이진트리 구조체 생성 함수
////////////////////////////////////
BTreeNode* MakeBTreeNode(void)
{
	BTreeNode * Node = (BTreeNode*)malloc(sizeof(BTreeNode));
	Node->m_Left = NULL;
	Node->m_Right = NULL;

	return Node;
}

////////////////////////////////////
// 이진트리 구조체의 데이터 얻기 함수
////////////////////////////////////
BTData GetData(BTreeNode* bt)
{
	return bt->m_data;
}

////////////////////////////////////
// 이진트리 구조체의 데이터 변경 함수
////////////////////////////////////
void SetData(BTreeNode* bt, BTData data)
{
	bt->m_data = data;
}

////////////////////////////////////
// 전달된 이진트리의, 왼쪽 서브트리의 루트노드를 얻는 함수
////////////////////////////////////
BTreeNode* GetLeftSubTree(BTreeNode* bt)
{
	return bt->m_Left;
}

////////////////////////////////////
// 전달된 이진트리의, 오른쪽 서브트리의 루트노드를 얻는 함수
////////////////////////////////////
BTreeNode* GetRightSubTree(BTreeNode* bt)
{
	return bt->m_Right;
}

////////////////////////////////////
// 왼쪽에 이진트리 연결 함수.
// main의 왼쪽에 sub 연결
////////////////////////////////////
void MakeLeftSubTree(BTreeNode* main, BTreeNode* sub)
{
	if (main->m_Left != NULL)
		DeleteTree(main->m_Left);

	main->m_Left = sub;
}

////////////////////////////////////
// 오른쪽에 이진트리 연결 함수.
// main의 오른쪽에 sub 연결
////////////////////////////////////
void MakeRightSubTree(BTreeNode* main, BTreeNode* sub)
{
	if (main->m_Right != NULL)
		DeleteTree(main->m_Right);

	main->m_Right = sub;
}

////////////////////////////////////
// 전위 순회 함수
////////////////////////////////////
void FirstTraverse(BTreeNode* bt, VisitFuncPtr action)
{
	if (bt == NULL)
		return;

	action(GetData(bt));
	FirstTraverse(bt->m_Left, action);
	FirstTraverse(bt->m_Right, action);
}


////////////////////////////////////
// 중위 순회 함수
////////////////////////////////////
void SecondTraverse(BTreeNode* bt, VisitFuncPtr action)
{
	if (bt == NULL)
		return;

	if(bt->m_Left != NULL && bt->m_Right != NULL)
		printf(" ( ");

	SecondTraverse(bt->m_Left, action);		
	action(GetData(bt));
	SecondTraverse(bt->m_Right, action);	

	if (bt->m_Left != NULL && bt->m_Right != NULL)
		printf(" ) ");
}

////////////////////////////////////
// 후위 순회 함수
////////////////////////////////////
void EndTraverse(BTreeNode* bt, VisitFuncPtr action)
{
	if (bt == NULL)
		return;

	EndTraverse(bt->m_Left, action);
	EndTraverse(bt->m_Right, action);
	action(GetData(bt));
}

////////////////////////////////////
// 트리 소멸 함수
// 전달된 루트 노드의 좌,우 모든 트리를 소멸시킨다.
// 후위 순회로 해야 모든 트리가 소멸된다.
////////////////////////////////////
void DeleteTree(BTreeNode* bt)
{
	if (bt == NULL)
		return;

	DeleteTree(bt->m_Left);
	DeleteTree(bt->m_Right);
	free(bt);
}

