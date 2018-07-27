#include <iostream>
#include <cstring>
#include <stdlib.h>
#include "Memory_Leak_Linked_List.h"

using namespace std;

// MallocFunc함수를 호출하는 매크로 함수
#define MallocMacro(type, size) MallocFunc<type>(size, __FILE__, __LINE__);

// 전역변수 모음
int AllocSize = 0;
int AllocCount = 0;

// 메모리 정보 저장 리스트 선언
LinkedList list;

// MallocFunc 함수 템플릿
template <class T>
T* MallocFunc(int, const char*, int);

// FreeFunc 함수 템플릿
template <typename T>
void FreeFunc(T*);

// 현재 메모리 현황 호출
void ShowMomoryFunc();

int main()
{
	// 리스트 초기화
	ListInit(&list);

	// 뉴 동적할당
	char *c1 = MallocMacro(char, 40);
	short *s1 = MallocMacro(short, 4);
	int *p1 = MallocMacro(int, 4);
	long *l1 = MallocMacro(long, 4);
	long long *dl1 = MallocMacro(long long, 4);
	float *f1 = MallocMacro(float, 4);
	double *d1 = MallocMacro(double, 4);

	// 딜리트
	//FreeFunc(c1);
	FreeFunc(s1);
	//FreeFunc(p1);
	FreeFunc(l1);
	FreeFunc(dl1);
	FreeFunc(f1);
	//FreeFunc(d1);

	// 메모리 누수 현황 확인
	ShowMomoryFunc();

	return 0;
}

// MallocFunc 함수 템플릿
template <class T>
T* MallocFunc(int size, const char* name, int line)
{
	Memory* mem = new Memory;

	T* temp = new T[size];

	mem->size = size * sizeof(T);
	strcpy_s(mem->_name, strlen(name) + 1, name);
	mem->_line = line;
	mem->_address = temp;

	ListInsert(&list, mem);

	AllocSize += mem->size;
	AllocCount++;

	return temp;
}

// FreeFunc 함수 템플릿
template <typename T>
void FreeFunc(T* ptr)
{
	bool bCheck = ListDelete(&list, ptr);

	if (!bCheck)
	{
		cout << "동적 할당하지 않은 주소는 delete 할 수 없습니다." << endl;
		return;
	}

	delete ptr;
}

// 현재 메모리 현황 호출
void ShowMomoryFunc()
{
	cout << "-----------메모리 누수 현황-----------" << endl;
	cout << "총 동적할당 사이즈 : " << AllocSize << " Byte" << endl;
	cout << "총 동적할당 횟수 : " << AllocCount << endl << endl;;

	// 반복문을 돌면서, 모든 리스트를 검사. 리스트에 남아있는 노드들이 해제되지 않은 메모리들이다.
	Memory mem;

	if (ListSearchFirst(&list, &mem))	// 노드를 찾았을 경우, mem에다가 찾은 노드의 정보를 넣는다.
	{
		cout << "해제되지 않은 메모리 : [0x" << mem._address << "] / " << mem.size << " Byte" << endl;
		cout << "파일 : " << mem._name << " / " << mem._line << "Line" << endl << endl;

		while (ListSearchSecond(&list, &mem))
		{
			cout << "해제되지 않은 메모리 : [0x" << mem._address << "] / " << mem.size << " Byte" << endl;
			cout << "파일 : " << mem._name << " / " << mem._line << "Line" << endl << endl;
		}
	}
	else
	{
		cout << "해제되지 않은 메모리 없음" << endl;
	}
}