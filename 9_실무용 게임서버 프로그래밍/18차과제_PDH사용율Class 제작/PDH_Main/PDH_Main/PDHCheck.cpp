#include "pch.h"
#include "PDHCheck.h"

#include <strsafe.h>


namespace Library_Jingyu
{
	// 생성자
	CPDH::CPDH()
	{
		m_iQueryIndex = 0;

		// 쿼리 오픈
		PdhOpenQuery(NULL, NULL, &m_pdh_Query);
	}

	// 쿼리 셋팅하기.
	// 해당 클래스가 성능모니터에 날릴 쿼리 셋팅
	//
	// Parameter : 등록할 쿼리.
	// return : 등록 성공 시 true, 실패시 false
	bool CPDH::SetQuery(TCHAR* tQuery)
	{
		// 쿼리 복사
		HRESULT ret =  StringCbCopy(m_tcQuery[m_iQueryIndex], 256, tQuery);

		// 실패 시 false 리턴
		if (ret != S_OK)
		{
			printf("SetQuery(). PDH query Set Fail..\n");
			return false;
		}

		m_iQueryIndex++;

		return true;
	}

	// 등록된 모든 쿼리의 정보 갱신
	//
	void CPDH::UpdateInfo()
	{
		int i = 0;
		while (i< m_iQueryIndex)
		{
			// ---------- 카운터 등록
			PdhAddCounter(m_pdh_Query, m_tcQuery[i], NULL, &m_pdh_Counter[i]);

			// ---------- 데이터 갱신
			PdhCollectQueryData(m_pdh_Query);

			// ---------- 데이터 뽑기.
			PDH_STATUS Status;

			PDH_FMT_COUNTERVALUE CounterValue;

			Status = PdhGetFormattedCounterValue(m_pdh_Counter[i], PDH_FMT_DOUBLE, NULL, &CounterValue);
			if (Status == 0)
				m_pdh_value[i] = CounterValue.doubleValue;

			i++;
		}	
		
	}
}