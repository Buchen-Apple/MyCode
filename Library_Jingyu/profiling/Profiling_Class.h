
#ifndef __PROFILING_CLASS_H__
#define __PROFILING_CLASS_H__

#include <Windows.h>

namespace Library_Jingyu
{
	class Profiling
	{
	private:
		char m_Name[30];
		int m_CallCount;
		double m_StartTime;

		double m_TotalTime;
		double m_Average;
		double m_MaxTime[2];
		double m_MInTime[2];

		LARGE_INTEGER m_StartCount;						// BEGIN에서 사용. QueryPerformanceCounter로 시작시간을 구할 때 사용
		LARGE_INTEGER m_EndCount;						// END에서 사용. QueryPerformanceCounter로 시작시간을 구할 때 사용

	public:
		Profiling();
		friend void BEGIN(const char*);
		friend void END(const char*);
		friend void RESET();
		friend void PROFILING_SHOW();
		friend void PROFILING_FILE_SAVE();	// 인자로, 스레드들의 ID가 저장된 배열과 Count받아옴.
	};

	// 주파수 구하기	
	void FREQUENCY_SET();

	// 프로파일링 시작. Profiling와 friend
	void BEGIN(const char* str);

	// 프로파일링 종료. Profiling와 friend
	void END(const char* str);

	// 프로파일링 전체 리셋. Profiling와 friend
	void RESET();

	// 프로파일링 정보 보기. Profiling와 friend
	void PROFILING_SHOW();

	// 프로파일링을 파일로 출력. Profiling와 friend
	void PROFILING_FILE_SAVE();


#endif // !__PROFILING_CLASS_H__
}
