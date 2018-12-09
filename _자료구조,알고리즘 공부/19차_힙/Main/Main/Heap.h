#ifndef __HEAP_H__
#define __HEAP_H__

#define HEAP_LEN	100

template <typename T>
class Heap
{
	struct Node
	{
		T m_data;
		int m_iPr;	//우선순위
	};

	Node m_HeapArray[HEAP_LEN];
	int m_iSize;

private:
	// 부모의 인덱스
	int GetParentIDX(int idx)
	{	return idx / 2;	}

	// 왼쪽 자식의 인덱스
	int GetLeftChildIDX(int idx)
	{	return idx * 2; 	}

	// 오른쪽 자식의 인덱스
	int GetRightChildIDX(int idx)
	{	return idx * 2 + 1; 	}

	// 인자 2개의 우선순위 체크
	

public:
	// 초기화
	void Init()
	{  
		m_iSize = 0; 
	}

	// 삽입
	bool Insert(T data, int Pr);

	// 삭제
	bool Delete(T* pData);

	// 사이즈 리턴
	int Size()
	{  return m_iSize;  }
};


// 삽입
template <typename T>
bool Heap<T>::Insert(T data, int Pr)
{
	// 꽉찼으면 더 이상 못넣음
	if (m_iSize == HEAP_LEN)
		return false;

	// 임시 위치에 넣어둠
	int NowIndex = m_iSize + 1;

	Node Temp;
	Temp.m_data = data;
	Temp.m_iPr = Pr;	

	// 현재 인덱스가 1이아니고 (즉 루트가 아니고)
	while (NowIndex != 1)
	{
		// 부모 인덱스 구함
		int ParnetIndex = GetParentIDX(NowIndex);

		// 부모의 우선순위가 나보다 낮다면 값 교체 (우선순위의 값이 작을수록 우선순위가 높다고 가정) 
		if (Pr < m_HeapArray[ParnetIndex].m_iPr)
		{
			// 부모를 내 위치로 이동(현재 내 위치는 비어있기 때문에 바로 이동시켜도 됨)
			m_HeapArray[NowIndex] = m_HeapArray[ParnetIndex];

			// 부모를 나로 취급
			NowIndex = ParnetIndex;

			// 부모 인덱스 다시 구함
			ParnetIndex = GetParentIDX(NowIndex);
		}

		else
			break;
	}

	// 찾은 위치에 나를 저장
	m_HeapArray[NowIndex] = Temp;
	m_iSize++;

	return true;
}

// 삭제
template <typename T>
bool Heap<T>::Delete(T* pData)
{
	// 데이터가 없으면 더 이상 못뺌
	if (m_iSize == 0)
		return false;

	// 리턴할 데이터 받아두기
	// 힙은 무조건 가장 앞의 데이터 리턴.
	*pData = m_HeapArray[1].m_data;

	// 가장 마지막 위치의 값을 받아옴
	Node Temp = m_HeapArray[m_iSize];

	// Temp가 루트에 들어갔다고 가정. 자식으로 타고 내려가며 우선순위를 체크한다.
	int NowIndex = 1;

	// 자식노드 존재여부 체크
	// 나의 왼쪽 자식이 없으면 자식이 하나도 없는것. 왼쪽자식의 Index가 더 작기때문에.
	while (1)
	{
		int CheckIndex = 0;
		int LChildIndex = GetLeftChildIDX(NowIndex);

		// 자식노드가 0명일 경우
		if (LChildIndex > m_iSize)
			break;		

		// 자식노드가 왼쪽만 있을 경우
		else if (LChildIndex == m_iSize)
		{
			CheckIndex = LChildIndex;
		}

		// 자식노드가 둘 다 있을 경우
		else
		{
			int RChildIndex = GetRightChildIDX(NowIndex);

			// 오른쪽 자식의 우선순위가 높을 경우
			if (m_HeapArray[RChildIndex].m_iPr < m_HeapArray[LChildIndex].m_iPr)
			{
				CheckIndex = RChildIndex;
			}

			// 왼쪽 자식의 우선순위가 높을 경우
			else
				CheckIndex = LChildIndex;
		}	

		// 마지막 노드의 우선순위가 높다면 break;
		if (Temp.m_iPr <= m_HeapArray[CheckIndex].m_iPr)
			break;

		// 찾아온 자식노드의 우선순위가 더 높다면, 값 교체
		m_HeapArray[NowIndex] = m_HeapArray[CheckIndex];
		NowIndex = CheckIndex;
	}

	// 찾은 위치에 Temp를 넣는다.
	m_HeapArray[NowIndex] = Temp;
	m_iSize--;

	return true;
}


#endif // !__HEAP_H__
