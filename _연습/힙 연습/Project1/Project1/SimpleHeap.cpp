#include "SimpleHeap.h"

// 힙 초기화
void HeapInit(Heap* ph, PriorityComp pc)
{
	ph->numOfData = 0;
	ph->comp = pc;
}

// 힙 비었는지 확인
int HIsEmpty(Heap* ph)
{
	if (ph->numOfData == 0)
		return TRUE;
	else
		return FALSE;
}


// 부모의 ID 리턴
int GetParentID(int idx)
{
	return idx / 2;
}

// 왼쪽 자식의 ID 리턴
int GetLeftChildID(int idx)
{
	return idx * 2;
}

// 오른쪽 자식의 ID 리턴
int GetRightChildID(int idx)
{
	return GetLeftChildID(idx) + 1;
}

// 두개의 자식 중 높은 우선순위의 자식 노드 인덱스값 반환
int GetHiPriChildID(Heap* ph, int idx)
{
	// 왼쪽 자식이, 마지막 노드라면, 비교할것도 없이 왼쪽노드 바로 반환
	if (GetLeftChildID(idx) == ph->numOfData)
		return GetLeftChildID(idx);

	// 왼쪽 자식의 idx가 마지막 노드의 idx보다 크다면, 없는 노드이다. 
	// 즉, 현재 idx의 노드는 단말노드이다.
	else if (GetLeftChildID(idx) > ph->numOfData)
		return 0;

	// 자식노드가 둘 다 있다면 
	else
	{
		// comp()에 저장된 함수는 HData, 즉 값을 입력받는다. 값을 기준으로 우선순위 설정.
		// 1번 인자의 우선순위가 높으면 0보다 큰 값
		// 2번 인자의 우선순위가 높으면 0보다 작은 값
		// 두 인자의 우선순위가 동일하면 0 반환

		// 오른쪽 자식노드의 값의 우선순위가 높다면 (우선순위는 적은 값이 높은것)
		if(ph->comp(ph->heapArry[GetLeftChildID(idx)], 
			ph->heapArry[GetRightChildID(idx)]) < 0)
			return GetRightChildID(idx);

		// 왼쪽 자식의 우선순위가 높으면
		else
			return GetLeftChildID(idx);
	}
}

void HInsert(Heap* ph, HData data)
{
	// 새 노드의 인덱스 설정
	int idx = ph->numOfData + 1;

	// 새 노드가 저장될 위치가 루트노드의 위치가 아니라면, while문 반복
	while (idx != 1)
	{
		// 새 노드의 부모 노드의 우선순위 비교

		// 값을 기준으로, 새 노드의 우선순위가 높다면
		if (ph->comp(data, ph->heapArry[GetParentID(idx)]) > 0)
		{
			// 부모를 새 노드의 위치로 이동. 실제 반영
			ph->heapArry[idx] = ph->heapArry[GetParentID(idx)];

			// 새 노드의 위치를 부모 노드의 위치로 이동. idx만 변경
			idx = GetParentID(idx);
		}

		// 부모 노드의 우선순위가 높다면, 위치를 찾은 것이니 빠져나간다.
		else
			break;
	}

	// 위치를 찾았으니 idx에 값을 넣는다.
	ph->heapArry[idx] = data;

	// 배열 수 1 증가
	ph->numOfData += 1;
}

HData HDelete(Heap* ph)
{
	// 삭제할 데이터 저장. 이 값은 반환된다.
	HData retData = ph->heapArry[1];

	// 힙의 마지막 데이터 지정
	HData lastData = ph->heapArry[ph->numOfData];

	// 마지막 노드의 Index 설정. 일단 1부터 시작
	int parentID = 1;

	// 자식의 Index를 저장할 변수
	int childID;


	// 루트 노드의 우선순위가 높은 자식 노드를 시작으로 반복문 시작
	while (childID = GetHiPriChildID(ph, parentID))
	{
		// GetHiPriChildID()로 왼쪽 or 오른쪽 자식의 Index를 얻어온 상태

		// 마지막 노드와 우선순위 비교.
		// lastElem.pr(마지막 노드의 우선순위)가 ph->heapArry[childID].pr(자식의 우선순위)보다 높다면(값이 작다면) 자리를 찾은것이니 break;
		if(ph->comp(lastData, ph->heapArry[childID]) > 0)
			break;

		// 자식의 우선순위가 더 높다면, 자식의 data 값을 부모로 이동시키고, 마지막 노드의 ID를 자식의 ID로 변경. 
		// 즉 이게 스왑절차
		ph->heapArry[parentID] = ph->heapArry[childID];
		parentID = childID;

	}

	// 자리를 찾았으니 자리에 넣는다.
	ph->heapArry[parentID] = lastData;

	// 1개 삭제했으니 값 1감소
	ph->numOfData -= 1;

	// 삭제된 값 반환.
	return retData;
}
