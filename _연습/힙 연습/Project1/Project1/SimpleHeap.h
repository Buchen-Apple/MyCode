#pragma once
#ifndef __SIMPLE_HEAP_H__
#define __SIMPLE_HEAP_H__

#define TRUE 1
#define FALSE 0

#define HEAP_LEN 100

typedef char* HData;
typedef int PriorityComp(HData d1, HData d2);


/////////////////////////////////////////////////////
// comp : 우선순위를 비교하는 함수의 이름을 저장할 함수포인터
//
// *********************기준**********************
// 1번 인자의 우선순위가 높으면 0보다 큰 값
// 2번 인자의 우선순위가 높으면 0보다 작은 값
// 두 인자의 우선순위가 동일하면 0 반환
/////////////////////////////////////////////////////

// numOfData : heapArry에 저장된 데이터가 있는 마지막 위치
// heapArry : HeapElem을 저장하는 배열
typedef struct _heap
{
	PriorityComp* comp;
	int numOfData;
	HData heapArry[HEAP_LEN];

}Heap;

void HeapInit(Heap* ph, PriorityComp pc);

int HIsEmpty(Heap* ph);

void HInsert(Heap* ph, HData data);

HData HDelete(Heap* ph);

#endif // !__SIMPLE_HEAP_H__
