#include "pch.h"
#include "PDHCheck.h"

#include <strsafe.h>


namespace Library_Jingyu
{
	// 쿼리 스트링
	/*

	L"\\Process(이름)\\Thread Count"		// 스레드
	L"\\Process(이름)\\Working Set"		// 사용 메모리

	L"\\Memory\\Available MBytes"		// 사용가능

	L"\\Memory\\Pool Nonpaged Bytes"	// 논페이지드

	L"\\Network Interface(*)\\Bytes Received/sec"
	L"\\Network Interface(*)\\Bytes Sent/sec"

	*/


	// 생성자
	CPDH::CPDH()
	{
		// 쿼리 오픈
		PdhOpenQuery(NULL, NULL, &m_pdh_Query);

		// 초기 결과값 0으로 초기화
		m_pdh_value = 0;
	}


	// 쿼리 날리기
	// 결과는 GetResult() 함수로 얻기 가능
	//
	// Parameter : 쿼리(TCHARr*)
	// return : 없음
	void  CPDH::Query(const TCHAR* tQuery)
	{
		// 측정 카운터 핸들 변수
		PDH_HCOUNTER	pdh_Counter;

		// ---------- 카운터 등록
		PdhAddCounter(m_pdh_Query, tQuery, NULL, &pdh_Counter);

		// ---------- 데이터 갱신
		PdhCollectQueryData(m_pdh_Query);

		// ---------- 데이터 뽑기.
		PDH_FMT_COUNTERVALUE CounterValue;

		if(PdhGetFormattedCounterValue(pdh_Counter, PDH_FMT_DOUBLE, NULL, &CounterValue) == ERROR_SUCCESS)
			m_pdh_value = CounterValue.doubleValue;		
		
	}

	// 결과 얻기
	//
	// Parameter : 없음
	// return : 결과 값(double)
	double CPDH::GetResult()
	{
		return m_pdh_value;
	}
}