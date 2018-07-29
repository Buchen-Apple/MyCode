#include <stdio.h>
#include <stdlib.h>
#include "ListBaseStack.h"


// 스택초기화
void StackInit(Stack* pstack)
{
	pstack->head = NULL;
}

// 스택 비었는지 확인
int SIsEmpty(Stack* pstack)
{
	if (pstack->head == NULL)
		return TRUE;
	else
		return FALSE;
}

// Push
void SPush(Stack* pstack, Data data)
{
	Node* newNode = (Node*)malloc(sizeof(Node));

	newNode->data = data;
	newNode->next = pstack->head;

	pstack->head = newNode;
}

//Pop
Data SPop(Stack* pstack)
{
	Data rdata;
	Node* rnode;

	if (SIsEmpty(pstack) == TRUE)
	{
		printf("Stack Memory Error!\n");
		exit(-1);
	}

	rdata = pstack->head->data;
	rnode = pstack->head;

	pstack->head = rnode->next;
	free(rnode);

	return rdata;
}

//Peek
Data Peek(Stack* pstack)
{
	if (SIsEmpty(pstack) == TRUE)
	{
		printf("Stack Memory Error!\n");
		exit(-1);
	}

	return pstack->head->data;
}