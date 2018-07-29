#include <stdio.h>
#include <string.h>
#include "SimpleHeap.h"

// 우선순위 비교 함수
/////////////////////////////////////////////////////
// *********************기준**********************
// 1번 인자의 우선순위가 높으면 0보다 큰 값
// 2번 인자의 우선순위가 높으면 0보다 작은 값
// 두 인자의 우선순위가 동일하면 0 반환
/////////////////////////////////////////////////////
int DataPriorityComp(HData ch1, HData ch2);

int main()
{
	Heap heap;
	HeapInit(&heap, DataPriorityComp);

	// 힙에 데이터 추가
	HInsert(&heap, "abc");
	HInsert(&heap, "abcde");
	HInsert(&heap, "abcdefg");
	printf("%s \n", HDelete(&heap));

	// 힙에 데이터 다시 추가
	HInsert(&heap, "abcdefgh");
	HInsert(&heap, "abcdefghij");
	HInsert(&heap, "abcdefghijklnm");
	printf("%s \n", HDelete(&heap));

	// 나머지 모두 디큐하기
	while (!HIsEmpty(&heap))
		printf("%s \n", HDelete(&heap));

	return 0;
}

// 우선순위 비교 함수
int DataPriorityComp(HData ch1, HData ch2)
{
	// 1번 인자의 우선순위가 높으면 0보다 큰 값
	// 2번 인자의 우선순위가 높으면 0보다 작은 값
	// 두 인자의 우선순위가 동일하면 0 반환
	//return ch2 - ch1;

	return strlen(ch2) - strlen(ch1);

}