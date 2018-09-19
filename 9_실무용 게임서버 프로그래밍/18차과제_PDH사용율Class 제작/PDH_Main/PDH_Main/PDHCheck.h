#ifndef __PDH_CHECK_H__
#define __PDH_CHECK_H__

#include <Windows.h>

#include <Pdh.h>
#pragma comment(lib, "Pdh.lib")

// ---------------------
// 성능 모니터에서 정보 빼오는 클래스
// PDH 이용
// ---------------------
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

	class CPDH
	{
		// -----------------
		// 멤버 변수
		// -----------------

		// PDH에 전송할 쿼리. 카운트와 매칭
		PDH_HQUERY	m_pdh_Query;

		// 측정 데이터 별 카운터 핸들 변수
		//PDH_HCOUNTER	m_pdh_Counter_NonPaged;
		PDH_HCOUNTER	m_pdh_Counter[30];

		// 측정 데이터 저장용 변수
		//double		m_pdh_value_NonPaged;
		double			m_pdh_value[30];

		// 쿼리 저장소. 최대 30개의 쿼리 저장 가능.
		// 1개의 쿼리당 최대 256글자
		TCHAR m_tcQuery[30][256];

		// 쿼리 저장소(배열)에 몇 개의 쿼리가 들어있는지.
		int m_iQueryIndex;

	public:
		// 생성자
		CPDH();

		// 쿼리 셋팅하기.
		// 해당 클래스가 성능모니터에 날릴 쿼리 셋팅
		//
		// Parameter : 등록할 쿼리.
		// return : 등록 성공 시 true, 실패시 false
		bool SetQuery(TCHAR* tQuery);

		// 등록된 모든 쿼리의 정보 갱신
		//
		void UpdateInfo();


	};
}

#endif // !__PDH_CHECK_H__
