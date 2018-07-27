#pragma once
#ifndef __MEMORY_LEAK_LINKED_LIST_H__
#define __MEMORY_LEAK_LINKED_LIST_H__

#include <cstring>
#define MEM_ARRAY_MAX 300

// 할당 후, 메모리 누수를 체크하기 위한 구조체
struct Memory
{
	int size;
	char _name[300];
	int _line;
	void* _address;
};

typedef Memory Data;

struct Node
{
	Data* _data;
	Node* _prev;
	Node* _next;
};

struct LinkedList
{
	Node* _cur;
	Node* _head;
	Node* _tail;
};

// 리스트 초기화
void ListInit(LinkedList* list)
{
	// 헤드, 꼬리 더미 생성
	Node* headDumy = new Node;
	Node* tailDumy = new Node;

	// 헤드, 꼬리 더미가 서로를 가리킨다.
	headDumy->_prev = 0;
	headDumy->_next = tailDumy;

	tailDumy->_prev = headDumy;
	tailDumy->_next = 0;

	// 리스트의 헤드와 꼬리가 각각 더미를 가리킴
	list->_head = headDumy;
	list->_tail = tailDumy;
	list->_cur = list->_head;
}

// 리스트 추가 (꼬리에 추가)
void ListInsert(LinkedList* list, Data* data)
{
	// 새로운 노드 세팅
	Node* newNode = new Node;

	// 새로운 노드, _data에 data의 값을 넣는다.
	newNode->_data = data;

	// 새로운 노드, _data에 data의 값을 넣는다.

	// 해당 노드를 뒤에서부터 추가한다. 즉, 꼬리에서 추가. 
	// 첫 노드 추가일때랑 그 이후랑은 절차가 다르다.
	if (list->_tail->_prev == list->_head)	// 첫 노드라면
	{
		newNode->_prev = list->_head;
		newNode->_next = list->_tail;

		list->_head->_next = newNode;
		list->_tail->_prev = newNode;
	}

	else  // 첫 노드가 아니라면 
	{
		list->_tail->_prev->_next = newNode;
		newNode->_prev = list->_tail->_prev;

		newNode->_next = list->_tail;
		list->_tail->_prev = newNode;
	}

}

// 리스트 삭제. 원하는 주소(temp)가 없으면 false / 있으면 true 반환
bool ListDelete(LinkedList* list, void* temp)
{
	// temp(주소)를 체크할 노드를 헤드 위치로 이동
	list->_cur = list->_head->_next;

	// 헤드 위치부터 한칸씩 >>쪽으로 이동하면서 data._address 검사.
	while (1)
	{
		// 현재 노드가 꼬리더미라면, 가장 끝에 도착했는데도 원하는 주소를 못찾은거니 false 반환;
		if (list->_cur == list->_tail)
			return false;

		// 현재 노드의 주소가 temp와 같으면 원하는 주소를 찾은것이니 break;
		if (list->_cur->_data->_address == temp)
			break;

		// 위 상황이 둘 다 아니면 >>한칸 이동
		list->_cur = list->_cur->_next;
	}

	// 현재 노드의 이전과 현재 노드의 다음이 서로를 연결한다.
	list->_cur->_prev->_next = list->_cur->_next;
	list->_cur->_next->_prev = list->_cur->_prev;

	// 현재 노드의 _data를 할당 해제한다
	delete list->_cur->_data;

	// 현재 노드를 할당 해제한다.
	delete list->_cur;

	return true;
}

// 리스트 검색(First). 첫 노드가 있으면 true / 없으면 flase 반환
bool ListSearchFirst(LinkedList* list, Data* data)
{
	list->_cur = list->_head->_next;

	if (list->_cur == list->_tail)
		return false;

	memcpy(data, list->_cur->_data, sizeof(Data));

	return true;
}

// 리스트 검색(Next). 다음 노드가 있으면 true / 없으면 false 반환
bool ListSearchSecond(LinkedList* list, Data* data)
{
	list->_cur = list->_cur->_next;

	if (list->_cur == list->_tail)
		return false;

	memcpy(data, list->_cur->_data, sizeof(Data));

	return true;
}

#endif // !__MEMORY_LEAK_LINKED_LIST_H__

