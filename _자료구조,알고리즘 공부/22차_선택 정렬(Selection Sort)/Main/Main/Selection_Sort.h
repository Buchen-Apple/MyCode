#ifndef __SELECTION_SORT_H__
#define __SELECTION_SORT_H__

template <class T>
class SelectionSort
{
	T* m_DataArray;
	int m_iNowSize;
	int m_iMaxSIze;

public:
	// 생성자
	SelectionSort(int MaxSize)
	{
		m_iNowSize = 0;
		m_iMaxSIze = MaxSize;
		m_DataArray = new T[MaxSize];
	}

	// 소멸자
	~SelectionSort()
	{
		delete[] m_DataArray;
	}	

	// 삽입
	bool Insert(T Data)
	{
		if (m_iNowSize == m_iMaxSIze)
			return false;

		m_DataArray[m_iNowSize] = Data;
		m_iNowSize++;

		return true;
	}

	// 정렬
	void Sort()
	{
		int LoopCount = m_iNowSize-1;
		int EmptyIndex = 0;	// 이번에 데이터를 넣을 위치. 

		// LoopCount만큼 돌면서 데이터 정렬
		while (LoopCount > 0)
		{
			int SearchIndex = EmptyIndex;
			int TempIndex = EmptyIndex;

			// 가장 작은 값이 있는 인덱스를 찾는다.
			while (SearchIndex < m_iNowSize)
			{				
				if (m_DataArray[SearchIndex] < m_DataArray[TempIndex])
				{
					TempIndex = SearchIndex;
				}

				SearchIndex++;
			}

			// 교환한다.
			T Temp = m_DataArray[EmptyIndex];
			m_DataArray[EmptyIndex] = m_DataArray[TempIndex];
			m_DataArray[TempIndex] = Temp;

			LoopCount--;
			EmptyIndex++;
		}
	}


};

#endif // !__SELECTION_SORT_H__
