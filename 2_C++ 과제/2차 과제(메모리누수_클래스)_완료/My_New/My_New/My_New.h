#pragma once
#ifndef __MY_NEW_H__
#define __MY_NEW_H__

// 클래스 선언부
class MemoryCheck
{
private:
	size_t size;
	char _name[300];
	int _line;
	bool ArrayCheck;

public:
	void* _address;
	friend void* operator new (size_t size, const char* File, int Line); // operator new 전역함수. MemoryCheck와 frined
	friend void* operator new[](size_t size, const char* File, int Line); // operator new[] 전역함수. MemoryCheck와 frined
	friend void operator delete (void* ptr); // operator delete 전역함수. MemoryCheck와 frined
	friend void operator delete[](void* ptr); // operator delete[] 전역함수. MemoryCheck와 frined
	~MemoryCheck();
};

void* operator new (size_t size, const char* File, int Line);
void* operator new[](size_t size, const char* File, int Line);
void operator delete (void* ptr);
void operator delete[](void* ptr);
void operator delete (void* p, char* File, int Line) {}	// 에러방지용
void operator delete[](void* p, char* File, int Line) {} // 에러방지용

typedef MemoryCheck Data;

// 리스트의 노드 클래스
class Node	
{
public:
	Data* _data;
	Node* _prev;
	Node* _next;
};

// 리스트 클래스
class LinkedList 
{
	Node * _cur;
	Node* _head;
	Node* _tail;

public:	
	bool SearchFunc(LinkedList* list, void* temp); // 원하는 주소(temp)의 리스트를 찾는다. ListDelete / ListSearch에서 사용
	void ListInit(LinkedList* list); // 리스트 초기화
	void ListInsert(LinkedList* list, Data* data); // 리스트 추가 (꼬리에 추가)
	bool ListDelete(LinkedList* list, void* temp); // 리스트 삭제. 원하는(temp) 주소의 노드를 리스트에서 제외
	bool ListSearch(LinkedList* list, void* temp, Data* data); // 리스트 검색. 원하는(temp) 주소가 리스트에 있으면 해당 리스트의 _data를 data에 넣음. 원하는 주소의 리스트가 없으면 false 반환
	bool ListSearchFirst(LinkedList* list, Data* data); // 리스트 순회(First). 첫 노드가 있으면 true / 없으면 flase 반환
	bool ListSearchSecond(LinkedList* list, Data* data); // 리스트 순회(Next). 다음 노드가 있으면 true / 없으면 false 반환
};

// operator new함수를 호출하는 매크로
#define new new(__FILE__,__LINE__)

#endif // !__MY_NEW_H__



