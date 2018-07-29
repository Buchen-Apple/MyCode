#include <stdio.h>
#include "BinaryTree.h"

// 순회 시 하는 행동 함수
void ShowIntData(int data);

int main()
{
	// 테스트 노드 4개 생성
	BTreeNode* Tree1 = MakeBTreeNode();
	BTreeNode* Tree2 = MakeBTreeNode();
	BTreeNode* Tree3 = MakeBTreeNode();
	BTreeNode* Tree4 = MakeBTreeNode();

	// 테스트 노드에 데이터 세팅
	SetData(Tree1, 1);
	SetData(Tree2, 2);
	SetData(Tree3, 3);
	SetData(Tree4, 4);

	// 연결하기
	MakeLeftSubTree(Tree1, Tree2);
	MakeRightSubTree(Tree1, Tree3);
	MakeLeftSubTree(Tree2, Tree4);	

	// 중위 순회 하기
	SecondTraverse(Tree1, ShowIntData);
	fputc('\n', stdout);

	// 새로운 노드 만들어서 Tree2의 왼쪽에붙이기
	BTreeNode* Tree5 = MakeBTreeNode();
	SetData(Tree5, 10);

	// 기존의 Tree4(숫자 4 들어있음)가 잘 삭제되는지 확인.
	MakeLeftSubTree(GetLeftSubTree(Tree1), Tree5);
	SecondTraverse(Tree1, ShowIntData);
	fputc('\n', stdout);

	return 0;
}

// 순회 시 하는 행동 함수
void ShowIntData(int data)
{
	printf("%d ", data);
}