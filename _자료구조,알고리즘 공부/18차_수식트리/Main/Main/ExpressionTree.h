#ifndef __EXPRESSION_TREE_H__
#define __EXPRESSION_TREE_H__

#include "Convertor_Cal.h"
#include "BinaryTree_List.h"

// 출력용 함수
void ShowAction(BTData data);

class ExpressionTree
{
	// 표기법 변환 클래스
	Convertor_Cal m_ConvClass;

	// 이진트리 도구
	BinaryTree_List m_BTtool;

public:
	// 수식트리 제작
	BTNode* CreateExpTree(char exp[]);

	// 수식트리 계산
	int ExpTreeResult(BTNode* Root);

	// 전위 표기법 기반 출력
	void ShowPreOrder(BTNode* Root);

	// 중위 표기법 기반 출력
	void ShowInOrder(BTNode* Root);

	// 후위 표기법 기반 출력
	void ShowPostOrder(BTNode* Root);
};



#endif // !__EXPRESSION_TREE_H__


