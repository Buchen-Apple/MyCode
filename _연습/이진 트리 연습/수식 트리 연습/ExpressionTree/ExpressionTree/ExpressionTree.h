#pragma once
#ifndef __EXPRESSION_TREE_H__
#define __EXPRESSION_TREE_H__

#include "BinaryTree.h"

// 수식 트리 구성
BTreeNode* MakeExpTree(char exp[]);

// 수식 트리 계산
int EvaluateExpTree(BTreeNode* bt);

// 전위 표기법 기반 출력
void ShowPreFixTypeExp(BTreeNode* bt);

// 중위 표기법 기반 출력
void ShowInFixTypeExp(BTreeNode* bt);

// 후위 표기법 기반 출력
void ShowPostFixTypeExp(BTreeNode* bt);

#endif // !__EXPRESSION_TREE_H__
