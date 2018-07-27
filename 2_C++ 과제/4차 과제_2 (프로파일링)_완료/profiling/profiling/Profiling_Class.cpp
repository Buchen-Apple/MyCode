#include "Profiling_Class.h"
#include <Windows.h>

namespace Library_Jingyu
{

	Profiling profile[PROFILING_SIZE];			// 프로파일링 객체
	LARGE_INTEGER Frequency;					// 주파수 저장 변수
	LARGE_INTEGER count1;						// 시간 저장 변수 1
	LARGE_INTEGER count2;						// 시간 저장 변수 2

	int g_NowProfiling_Size = -1;					// 프로파일링 객체 갯수 체크
#define MIN_SET 100000000

	Profiling::Profiling()
		:m_CallCount(0), m_StartTime(0), m_TotalTime(0), m_Average(0)
	{
		m_MaxTime[0] = 0;
		m_MaxTime[1] = 0;

		m_MInTime[0] = MIN_SET;
		m_MInTime[1] = MIN_SET;
	}

	// 주파수 구하기	
	void FREQUENCY_SET()
	{
		// 주파수 구하기	
		QueryPerformanceFrequency(&Frequency);
	}

	// 프로파일링 시작
	void BEGIN(const char* str)
	{
		QueryPerformanceCounter(&count1);
		int i = 0;
		bool bFlag = false;

		for (i = 0; i <= g_NowProfiling_Size; ++i)
		{
			if (strcmp(profile[i].m_Name, str) == 0)
			{
				bFlag = true;
				break;
			}
		}

		// 현재 할당된 프로파일링만큼 검색했지만 이름이 없다면, 추가로 현재까지 할당된 사이즈가 PROFILING_SIZE보다 작다면
		// 새로운 프로파일링 생성
		if (bFlag == false && g_NowProfiling_Size < PROFILING_SIZE)
		{
			g_NowProfiling_Size++;
			strcpy_s(profile[g_NowProfiling_Size].m_Name, sizeof(profile[g_NowProfiling_Size].m_Name), str);	// 이름 저장
			profile[g_NowProfiling_Size].m_CallCount = 1;							// 해당 함수(BEGIN 말고, 현재 BEGIN을 호출한 함수)호출 횟수 저장
			profile[g_NowProfiling_Size].m_StartTime = (double)count1.QuadPart;		// 시작 시간 저장.

		}

		// 검색된 이름이 있다면, 해당 프로파일링에 시작 시간 저장
		else if (bFlag == true)
		{
			profile[i].m_CallCount += 1;									// 해당 함수(BEGIN 말고, 현재 BEGIN을 호출한 함수)호출 횟수 저장
			profile[i].m_StartTime = (double)count1.QuadPart;				// 시작 시간 저장.
		}
	}

	// 프로파일링 종료
	void END(const char* str)
	{
		QueryPerformanceCounter(&count2);
		int i;
		bool bFlag = true;

		for (i = 0; strcmp(profile[i].m_Name, str) != 0; ++i)
		{
			if (i > g_NowProfiling_Size)
			{
				bFlag = false;
				break;
			}
		}

		// 검색된 이름이 있고, 그 프로파일링의 시작 시간이 0이 아니라면, 정보 저장
		if (bFlag == true && profile[i].m_StartTime != 0)
		{
			bool bCallmulCheck = false;
			bool bTimeMulCheck = false;
			// BEGIN부터 END까지의 시간을 마이크로 세컨드 단위로 저장.
			double dSecInterval = ((double)(count2.QuadPart - profile[i].m_StartTime) / Frequency.QuadPart) * 1000000;

			// 총 시간 저장
			profile[i].m_TotalTime += dSecInterval;

			// dSecInterval가 최소에 들어가야하는지 최대에 들어가야하는지 찾는다.
			// 버려지는 배열(최소) 체크.
			// 버려지는 배열에 들어가는 경우. 
			if (profile[i].m_MInTime[0] > dSecInterval)
			{
				if (profile[i].m_MInTime[0] == MIN_SET)	// 만약, 최초 호출이라면
				{
					profile[i].m_CallCount--;			// 콜 수를 1 감소. 교체가 아니라 아예 새로 넣는것이기 때문에 1 감소해야 함.	
					bCallmulCheck = true;				// 아래 최대의 버려지는 배열에도 현재 dSecInterval값이 들어갈 수 있기 때문에 콜을 뺏다는 것을 체크. 중복 빼기 방지.
				}
				else		                            // 최초 호출이 아니라면
				{
					profile[i].m_TotalTime -= dSecInterval;				// 최초 호출이 아니면, 총 시간에서 현재 시간(dSecInterval)을 다시 뺀다. 최초 호출 시에는 시간을 뺼 필요도 없음. 애초에 추가된 적이 없기 때문에.
					profile[i].m_TotalTime += profile[i].m_MInTime[0];	// 기존의 [0] 위치(버려지는 배열)의 시간을 총 시간에 더한다. 이걸 이제 최소로 쓸 것이기 때문에.
					profile[i].m_MInTime[1] = profile[i].m_MInTime[0];	// 기존 [0]의 시간을 [1]로 옮긴다.
					bTimeMulCheck = true;								// 아래 최대의 버려지는 배열에도 현재 dSecInterval값이 들어갈 수 있기 때문에 시간을 뺏다는 것을 체크. 중복 빼기 방지
				}

				profile[i].m_MInTime[0] = dSecInterval;		// 최초 호출 여부랑 상관없이, 현재 시간(dSecInterval)을 [0]에 넣는다. 이건 안쓰는 값이 됨.	

			}

			// 버려지지 않는 배열에 들어가는 경우, 그 시간을 1번 배열에 저장한다.
			else if (profile[i].m_MInTime[1] >= dSecInterval)
				profile[i].m_MInTime[1] = dSecInterval;

			// 이제 최대 체크
			// 버려지는 배열에 들어가는 경우, 시간을 빼고 호출횟수 감소. 그리고 버려진 시간을 0번 배열에 저장.
			if (profile[i].m_MaxTime[0] < dSecInterval)
			{
				if (profile[i].m_MaxTime[0] == 0)
				{
					if (bCallmulCheck == false)
					{
						profile[i].m_CallCount--;
					}
				}
				else
				{
					if (bTimeMulCheck == false)
						profile[i].m_TotalTime -= dSecInterval;
					profile[i].m_TotalTime += profile[i].m_MaxTime[0];
					profile[i].m_MaxTime[1] = profile[i].m_MaxTime[0];
				}

				profile[i].m_MaxTime[0] = dSecInterval;
			}

			// 버려지지 않는 배열에 들어가는 경우, 그 시간을 1번 배열에 저장한다.
			else if (profile[i].m_MaxTime[1] <= dSecInterval)
				profile[i].m_MaxTime[1] = dSecInterval;

			profile[i].m_StartTime = 0;	// StartTime을 0으로 만들어서, 해당 BEGIN이 끝났다는것 알려줌.
		}
		// 검색된 이름이 없다.
		else if (bFlag == false)
		{
			printf("END(). %s 존재하지 않는 함수 이름입니다\n", str);
			exit(1);
		}
		// 검색된 이름은 있는데, BEGIN중이 아니라면
		else if (profile[i].m_StartTime == 0)
		{
			printf("END(). %s BEGIN중이지 않은 함수입니다.\n", str);
			exit(1);

		}

	}

	// 프로파일링 전체 리셋
	void RESET()
	{
		for (int i = 0; i <= g_NowProfiling_Size; ++i)
		{
			profile[i].m_Name[0] = '\0';
			profile[i].m_TotalTime = 0;
			profile[i].m_MInTime[0] = 100000000;
			profile[i].m_MInTime[1] = 100000000;
			profile[i].m_MaxTime[0] = 0;
			profile[i].m_MaxTime[1] = 0;
			profile[i].m_StartTime = 0;
		}
		g_NowProfiling_Size = -1;
	}

	// 프로파일링 정보 전체 보기
	void PROFILING_SHOW()
	{
		for (int i = 0; i <= g_NowProfiling_Size; ++i)
		{
			printf("\n이름 : %s\n", profile[i].m_Name);
			printf("호출 횟수 :%d\n", profile[i].m_CallCount);
			printf("총 시간 : %0.6lf\n", profile[i].m_TotalTime);
			printf("평균 : %0.6lf\n", profile[i].m_TotalTime / profile[i].m_CallCount);
			printf("최소 시간1 / 2 : %0.6lf / %0.6lf\n", profile[i].m_MInTime[0], profile[i].m_MInTime[1]);
			printf("최대 시간1 / 2 : %0.6lf / %0.6lf\n", profile[i].m_MaxTime[0], profile[i].m_MaxTime[1]);
		}
	}

	// 프로파일링을 파일로 출력. Profiling와 friend
	void PROFILING_FILE_SAVE()
	{
		FILE* wStream;			// 파일 스트림
		size_t iFileCheck;	// 스트림 오픈 여부 체크
		iFileCheck = fopen_s(&wStream, "Profiling.txt", "wt");
		if (iFileCheck != 0)
			return;

		fprintf_s(wStream, "%15s  |%12s  |%11s   |%11s   |%11s |", "Name", "Average", "Min", "Max", "Call");
		fprintf_s(wStream, "\n---------------------------------------------------------------------------- ");

		for (int i = 0; i <= g_NowProfiling_Size; ++i)
		{
			// 최소의 [1]번 배열에 값이 들어간 적 없다면, [0]의 값을 [1]로 옮긴다.
			// 동시에 TotalTime에 시간을 추가하고 m_CallCount에 횟수도 추가한다.
			if (profile[i].m_MInTime[1] == 100000000)
			{
				profile[i].m_MInTime[1] = profile[i].m_MInTime[0];
				profile[i].m_TotalTime += profile[i].m_MInTime[1];
				profile[i].m_CallCount++;
			}

			if (profile[i].m_MaxTime[1] == 0)
			{
				profile[i].m_MaxTime[1] = profile[i].m_MaxTime[0];
				profile[i].m_TotalTime += profile[i].m_MaxTime[1];
				profile[i].m_CallCount++;
			}

			fprintf_s(wStream, "\n%16s |%11.3lf㎲ |%11.3lf㎲ |%11.3lf㎲ |%11d |", profile[i].m_Name, profile[i].m_TotalTime / profile[i].m_CallCount, profile[i].m_MInTime[1], profile[i].m_MaxTime[1], profile[i].m_CallCount);
		}

		fclose(wStream);

	}

}