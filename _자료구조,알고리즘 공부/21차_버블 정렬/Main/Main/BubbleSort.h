#ifndef __BUBBLE_SORT_H__
#define __BUBBLE_SORT_H__

template <class T>
class BubbleSort
{
	typedef void (*ActionFunc) (T Data);

	// 데이터 배열
	T* m_DataArray;

	int m_iMaxSize;
	int m_iNowSize;

	// Action 함수 내부에서 호출되는 함수.
	// 함수 포인터
	ActionFunc m_ActionFunc;

public:
	// 생성자
	BubbleSort(int ArraySize, ActionFunc fp)
	{
		m_iNowSize = 0;
		m_iMaxSize = ArraySize;
		m_DataArray = new T[ArraySize];
		m_ActionFunc = fp;
	}

	// 소멸자
	~BubbleSort()
	{
		delete m_DataArray;
	}


	// 삽입
	bool Insert(T Data)
	{
		if (m_iNowSize == m_iMaxSize)
			return false;

		m_DataArray[m_iNowSize] = Data;
		m_iNowSize++;

		return true;
	}

	// 정렬
	void Sort()
	{
		int LoopCount = m_iNowSize;		

		// n-1 만큼 루프.
		// 가장 마지막 데이터는 그 자리가 자신이 가야 할 위치이기 때문에 n-1.
		while (LoopCount > 1)
		{
			// StartIndex부터 LastIndex까지 Loop
			int StartIndex = 0;
			int LastIndex = LoopCount - 1;

			while (StartIndex < LastIndex)
			{
				int NextIndex = StartIndex + 1;

				// StartIndex가 NextIndex의 값 보다 크다면, 둘이 스왑.
				if (m_DataArray[StartIndex] > m_DataArray[NextIndex])
				{
					T Temp = m_DataArray[StartIndex];
					m_DataArray[StartIndex] = m_DataArray[NextIndex];
					m_DataArray[NextIndex] = Temp;
				}
				StartIndex++;
			}

			LoopCount--;
		}			
	}

	// 액션 함수
	void Action()
	{
		// 순회하며 출력
		int TempSize = 0;
		while (TempSize < m_iNowSize)
		{
			// 외부에서 던진 함수 호출
			m_ActionFunc(m_DataArray[TempSize]);
			TempSize++;
		}		
	}
};

#endif // !__BUBBLE_SORT_H__
