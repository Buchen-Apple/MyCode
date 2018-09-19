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

	// 쿼리 1개마다 쿼리날리기, 결과얻기를 반복해야 한다.
	class CPDH
	{
		// -----------------
		// 멤버 변수
		// -----------------

		// PDH에 전송할 쿼리. 카운트와 매칭
		PDH_HQUERY	m_pdh_Query;		

		// 측정 데이터 저장용 변수
		double			m_pdh_value;

	public:
		// 생성자
		CPDH();

		// 쿼리 날리기
		// 결과는 GetResult() 함수로 얻기 가능
		//
		// Parameter : 쿼리(TCHARr*)
		// return : 없음
		void Query(const TCHAR* tQuery);

		// 결과 얻기
		//
		// Parameter : 없음
		// return : 결과 값(double)
		double GetResult();

	};
}

#endif // !__PDH_CHECK_H__
