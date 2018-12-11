#ifndef __HEAP_H__
#define __HEAP_H__

#define LEN	100

template <typename T>
class Heap
{
	T m_DataArray[LEN];
	int m_iSize;

	// 최대 힙 여부. true면 최대 힙. false면 최소 힙
	bool m_MaxHeapFlag;

private:
	// Max_Heapify
	// Heapify를 시작할 루트의 인덱스와 힙의 최대 사이즈를 던진다
	void Max_Heapify(int Idx, int Size);

	// Min_Heapify
	// Heapify를 시작할 루트의 인덱스를 던진다.
	void Min_Heapify(int Idx, int Size);

	// 부모 인덱스
	int ParentIdx(int Idx)
	{
		return Idx / 2;
	}

	// 왼쪽자식 인덱스
	int LChildIdx(int Idx)
	{
		return Idx * 2;
	}

	// 오른쪽 자식 인덱스
	int RChildIdx(int Idx)
	{
		return Idx * 2 + 1;
	}

	// Max_Heap으로 변경
	void MaxHeap();

	// Min_Heap으로 변경
	void MinHeap();

public:
	// 초기화
	// 디폴트 : 최대 힙
	void Init(bool MaxHeapFlag = true)
	{
		m_MaxHeapFlag = MaxHeapFlag;
		m_iSize = 0;
	}

	// 삽입
	bool Insert(T Data);	

	// 삭제
	bool Delete(T* pData);	

	// 오름차순 정렬
	void Sort_asce();

	// 내림차순 정렬
	void Sort_desc();
};


// 삽입
template <typename T>
bool Heap<T>::Insert(T Data)
{
	if (m_iSize == LEN - 1)
		return false;
	
	m_iSize++;
	m_DataArray[m_iSize] = Data;

	if (m_MaxHeapFlag == true)
		MaxHeap();
	else
		MinHeap();

	return true;
}

// 삭제
template <typename T>
bool Heap<T>::Delete(T* pData)
{
	if (m_iSize == 0)
		return false;

	*pData = m_DataArray[1];
	m_DataArray[1] = m_DataArray[m_iSize];
	m_iSize--;

	if (m_MaxHeapFlag == true)
		MaxHeap();
	else
		MinHeap();

	return true;
}

// Max_Heapify
template <typename T>
void Heap<T>::Max_Heapify(int Idx, int Size)
{
	int LChild = LChildIdx(Idx);
	int RChild = RChildIdx(Idx);

	// 자식이 있을 경우
	while (LChild <= Size)
	{
		// 왼쪽 자식만 있는 경우
		if (LChild == Size)
		{
			// 내가 왼쪽자식보다 크거나 같다면 정렬 끝
			if (m_DataArray[LChild] <= m_DataArray[Idx])
				break;

			// 왼쪽 자식과 나의 정보를 Swap
			T Temp = m_DataArray[Idx];
			m_DataArray[Idx] = m_DataArray[LChild];
			m_DataArray[LChild] = Temp;
			Idx = LChild;
		}

		// 둘 다 있는 경우
		else
		{
			// 2명의 자식 중 더 큰 자식의 인덱스를 알아온다.
			int BigChild;
			if (m_DataArray[LChild] > m_DataArray[RChild])
				BigChild = LChild;

			else
				BigChild = RChild;

			// 그 자식보다 내가 크거나 같다면 정렬 끝
			if (m_DataArray[BigChild] <= m_DataArray[Idx])
				break;		

			// 그게 아니라면 더 큰 자식과 나를 swap
			T Temp = m_DataArray[Idx];
			m_DataArray[Idx] = m_DataArray[BigChild];
			m_DataArray[BigChild] = Temp;
			Idx = BigChild;
		}

		LChild = LChildIdx(Idx);
		RChild = RChildIdx(Idx);
	}	
}

// Min_Heapify
// Heapify를 시작할 루트의 인덱스를 던진다.
template <typename T>
void Heap<T>::Min_Heapify(int Idx, int Size)
{
	int LChild = LChildIdx(Idx);
	int RChild = RChildIdx(Idx);

	// 자식이 있을 경우
	while (LChild <= Size)
	{
		// 왼쪽 자식만 있는 경우
		if (LChild == Size)
		{
			// 내가 왼쪽자식보다 작거나 같다면 정렬 끝
			if (m_DataArray[LChild] >= m_DataArray[Idx])
				break;

			// 왼쪽 자식과 나의 정보를 Swap
			T Temp = m_DataArray[Idx];
			m_DataArray[Idx] = m_DataArray[LChild];
			m_DataArray[LChild] = Temp;
			Idx = LChild;
		}

		// 둘 다 있는 경우
		else
		{
			// 2명의 자식 중 더 작은 자식의 인덱스를 알아온다.
			int SmalChild;
			if (m_DataArray[LChild] < m_DataArray[RChild])
				SmalChild = LChild;

			else
				SmalChild = RChild;

			// 자식보다 내가 작거나 같다면 정렬 끝
			if (m_DataArray[SmalChild] >= m_DataArray[Idx])
				break;

			// 그게 아니라면 더 작은 자식과 나를 swap
			T Temp = m_DataArray[Idx];
			m_DataArray[Idx] = m_DataArray[SmalChild];
			m_DataArray[SmalChild] = Temp;
			Idx = SmalChild;
		}

		LChild = LChildIdx(Idx);
		RChild = RChildIdx(Idx);
	}
}

// Max_Heap으로 변경
template <typename T>
void Heap<T>::MaxHeap()
{
	// 최초로 자식이 있는 노드는, 2/n이다
	int StartIdx = m_iSize / 2;

	// 역순으로 돌아가며, 1이될때 까지 간다.
	while (StartIdx >= 1)
	{
		Max_Heapify(StartIdx, m_iSize);
		--StartIdx;
	}
}

// Min_Heap으로 변경
template <typename T>
void Heap<T>::MinHeap()
{
	// 최초로 자식이 있는 노드는, 2/n이다
	int StartIdx = m_iSize / 2;

	// 역순으로 돌아가며, 1이될때 까지 간다.
	while (StartIdx >= 1)
	{
		Min_Heapify(StartIdx, m_iSize);
		--StartIdx;
	}
}

// 오름차순 정렬
template <typename T>
void Heap<T>::Sort_asce()
{
	// 최소 힙일 경우, 최대 힙으로 변경 후 오름차순 진행
	if (m_MaxHeapFlag == false)
		MaxHeap();

	// 최대 힙 상태에서 오름차순 정렬
	int TempSize = m_iSize;
	while (TempSize > 1)
	{
		// 1. 루트와, 가장 마지막 배열의 값을 Exchange
		T Temp = m_DataArray[1];
		m_DataArray[1] = m_DataArray[TempSize];
		m_DataArray[TempSize] = Temp;

		// 2. 힙 사이즈가 1 줄어든것으로 간주
		TempSize--;

		// 3. 루트를 기준으로 Max_Heapify
		Max_Heapify(1, TempSize);
	}	
}

// 내림차순 정렬
template <typename T>
void Heap<T>::Sort_desc()
{
	// 최대 힙일 경우, 최소 힙으로 변경 후 내림차순 진행
	if (m_MaxHeapFlag == false)
		MinHeap();

	// 최소 힙 상태에서 내림차순 정렬
	int TempSize = m_iSize;
	while (TempSize > 1)
	{
		// 1. 루트와, 가장 마지막 배열의 값을 Exchange
		T Temp = m_DataArray[1];
		m_DataArray[1] = m_DataArray[TempSize];
		m_DataArray[TempSize] = Temp;

		// 2. 힙 사이즈가 1 줄어든것으로 간주
		TempSize--;

		// 3. 루트를 기준으로 Min_Heapify
		Min_Heapify(1, TempSize);
	}

}

#endif // !__HEAP_H__
