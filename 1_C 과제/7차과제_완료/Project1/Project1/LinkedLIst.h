#pragma once
#ifndef __LINKED_LIST_H__
#include <stdlib.h>
#include <string.h>

#define __LINKED_LIST_H__

typedef struct node
{
	int Key;
	char Data[100];
	node* pPrev;
	node* pNext;

}Node;

typedef struct linkedlist
{
	Node* pHead;
	Node* pTail;


}LinkedList;

// 리스트 초기화
void Init(LinkedList* pList)
{
	// 더미 2개 생성
	Node* headDumy = (Node*)malloc(sizeof(Node));
	headDumy->Key = -1;
	strcpy_s(headDumy->Data, 5, "Dumy");

	Node* tailDumy = (Node*)malloc(sizeof(Node));
	tailDumy->Key = -1;
	strcpy_s(tailDumy->Data, 5, "Dumy");

	// 더미 2개를 서로 연결
	headDumy->pPrev = NULL;
	headDumy->pNext = tailDumy;

	tailDumy->pPrev = headDumy;
	tailDumy->pNext = NULL;	

	// 세팅된 더미를 리스트가 가리킨다.
	pList->pHead = headDumy;
	pList->pTail = tailDumy;
}

// 머리에 리스트 추가
void ListInsert_Head(LinkedList* pList, int Key, char* Data)
{
	// 새로운 노드의 값 세팅
	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->Key = Key;
	strcpy_s(newNode->Data, strlen(Data)+1, Data);

	// 새로운 노드의 이전, 이후 연결
	newNode->pPrev = pList->pHead;
	newNode->pNext = pList->pHead->pNext;

	// 헤드의 다음이 새로운 노드가 되고, 헤드의 다음의 이전이 새로운 노드가 된다.
	pList->pHead->pNext->pPrev = newNode;
	pList->pHead->pNext = newNode;	
}

bool ListShow(LinkedList* pList, Node* returnNode, int pos)
{
	// 헤드를 기준으로 pos만큼 >>로 움직인 노드를 반환한다.
	// 먼저, 반환할 노드를 가르킬 포인터 선언
	LinkedList returnList;

	// returnList가 pList의 헤드(더미)를 가리킨다.
	returnList.pHead = pList->pHead;	

	// pList를 pos만큼 움직이고 그, 값을 returnList가 가리킨다.
	for (int i = 0; i < pos; ++i)
		returnList.pHead = returnList.pHead->pNext;

	// 만약, pos만큼 움직인 후, 다음 노드가 더미라면 false 반환
	if (returnList.pHead->pNext == NULL)
		return false;

	// pos만큼 움직인 노드의 값을 returnNode에 넣는다.
	returnNode->Key = returnList.pHead->Key;
	strcpy_s(returnNode->Data, strlen(returnList.pHead->Data) + 1, returnList.pHead->Data);

	return true;
}

bool ListPeek(LinkedList* pList, char Data[100], int SearchKey)
{
	// 헤드를 기준으로 >>로 움직이면서, key값이 동일한 노드를 찾는다.
	// 먼저, 반환할 노드를 가르킬 포인터를 선언한다.
	LinkedList SearchList;

	// SearchList가 pList의 헤드의 다음을 가리킨다.
	SearchList.pHead = pList->pHead->pNext;

	// 한칸씩 옆으로 움직이면서 key를 비교한다.
	while (1)
	{
		// 만약, 내가 꼬리(더미)라면 유저가 키가 없는거니 false 반환
		if (SearchList.pHead->pNext == NULL)
			return false;

		// 내가 원하는 키를 찾았어도 반복문 종료
		if (SearchList.pHead->Key == SearchKey)
			break;

		SearchList.pHead = SearchList.pHead->pNext;

	}	

	// 여기까지 왔으면 원하는 키를 찾은거니 해당 노드의 Data를 저장한다.
	strcpy_s(Data, sizeof(SearchList.pHead->Data), SearchList.pHead->Data);
	return true;
}

bool ListDelete(LinkedList* pList, int Key)
{
	// 헤드부터 >>로 움직이면서 Key를 찾는다. Key를 찾으면 삭제후 true 반환 / 못찾았으면 false반환
	// 먼저, 순회할 포인터를 생성한다.
	LinkedList DeleteList;

	// DeleteList가 pList의 헤드 다음을 가리킨다.
	DeleteList.pHead = pList->pHead->pNext;

	while (1)
	{
		// 만약, 내가 현재 꼬리(더미)라면, 마지막까지 순회한 것이니 false 반환
		if (DeleteList.pHead->pNext == NULL)
			return false;

		// 내가 원하는 것 Key를 찾았으면 반복문 종료
		if (DeleteList.pHead->Key == Key)
			break;

		DeleteList.pHead = DeleteList.pHead->pNext;
	}

	// 여기까지 왔으면 원하는 키를 찾은거니, 삭제 절차를 진행한다.
	DeleteList.pHead->pPrev->pNext = DeleteList.pHead->pNext;
	DeleteList.pHead->pNext->pPrev = DeleteList.pHead->pPrev;
	free(DeleteList.pHead);

	return true;
}


#endif // !__LINKED_LIST_H__
