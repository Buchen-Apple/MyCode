#include "stdafx.h"
#include "Profiling.h"

Profiling profile[PROFILING_SIZE];			// 프로파일링 객체
LARGE_INTEGER Frequency;					// 주파수 저장 변수
LARGE_INTEGER count1;						// 시간 저장 변수 1
LARGE_INTEGER count2;						// 시간 저장 변수 2

int g_NowProfiling_Size = -1;					// 프로파일링 객체 갯수 체크
#define MIN_SET 100000000

Profiling::Profiling()
	:m_CallCount(0), m_StartTime(0), m_Average(0)
{
	m_TotalTime.QuadPart = 0;
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
		profile[g_NowProfiling_Size].m_CallCount = 1;														// 해당 함수(BEGIN 말고, 현재 BEGIN을 호출한 함수)호출 횟수 저장
		profile[g_NowProfiling_Size].m_StartTime = count1.QuadPart;											// 시작 시간 저장.

	}

	// 검색된 이름이 있다면, 해당 프로파일링에 시작 시간 저장
	else if (bFlag == true)
	{
		profile[i].m_CallCount += 1;									// 해당 함수(BEGIN 말고, 현재 BEGIN을 호출한 함수)호출 횟수 저장
		profile[i].m_StartTime = count1.QuadPart;						// 시작 시간 저장.
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
		bool bMinCheck = false;

		// BEGIN부터 END까지의 시간을 QueryPerformanceCounter값 그대로 저장. 마이크로 세컨드로 바꾸는건, 출력할 때 한다.
		profile[i].m_TotalTime.QuadPart += count2.QuadPart - profile[i].m_StartTime;
		long long loSecInterval = count2.QuadPart - profile[i].m_StartTime;

		// 최소에 들어가야하는지 최대에 들어가야하는지 찾는다.
		// 최소에 들어가는 경우
		if (profile[i].m_MInTime[0] > loSecInterval || profile[i].m_MInTime[1] > loSecInterval)
		{
			// 최소 중에서도 0번에 들어가야 하는 경우
			if (profile[i].m_MInTime[0] > loSecInterval)
			{
				profile[i].m_TotalTime.QuadPart -= loSecInterval;			// 기존 시간은 빼고

				if (profile[i].m_MInTime[0] == MIN_SET)						// [0]에 최초로 넣는거라면
					profile[i].m_CallCount--;								// 콜수를 감소시킨다. 추가한게 하나도 없으니까!

				else															// 최초로 [0]에 들어가는게 아니라면			
					profile[i].m_TotalTime.QuadPart += profile[i].m_MInTime[0];	// [0]의 시간을 넣는다.

				profile[i].m_MInTime[1] = profile[i].m_MInTime[0];				// 현재 [0]을 [1]로 대입
				profile[i].m_MInTime[0] = loSecInterval;						// 현재 시간(loSecInterval)을 [0]에다가 넣는다.
			}

			// 최소 중에서도 1번에 들어가야 하는 경우
			else if (profile[i].m_MInTime[1] > loSecInterval)
				profile[i].m_MInTime[1] = loSecInterval;						// 현재 시간(loSecInterval)을 [1]에다가 넣는다
		}

		// 최대에 들어가는 경우
		else if (profile[i].m_MaxTime[0] < loSecInterval || profile[i].m_MaxTime[1] < loSecInterval)
		{
			// 최대 중에서도 0번에 들어가야 하는 경우
			if (profile[i].m_MaxTime[0] < loSecInterval)
			{
				profile[i].m_TotalTime.QuadPart -= loSecInterval;			// 기존 시간은 빼고

				if (profile[i].m_MaxTime[0] == 0)							// [0]에 최초로 넣는거라면
					profile[i].m_CallCount--;								// 콜수를 감소시킨다. 추가한게 하나도 없으니까!

				else                                                            // 최초로 [0]에 들어가는게 아니라면	
					profile[i].m_TotalTime.QuadPart += profile[i].m_MaxTime[0];	// [0] 시간을 넣는다.

				profile[i].m_MaxTime[1] = profile[i].m_MaxTime[0];				// 현재 [0]을 [1]로 대입
				profile[i].m_MaxTime[0] = loSecInterval;						// 현재 시간(loSecInterval)을 [0]에다가 넣는다.
			}
			// 최대 중에서도 1번에 들어가야 하는 경우
			else if (profile[i].m_MaxTime[1] < loSecInterval)
				profile[i].m_MaxTime[1] = loSecInterval;						// 현재 시간(loSecInterval)을 [1]에다가 넣는다.
		}

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
		profile[i].m_TotalTime.QuadPart = 0;
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
		printf("총 시간 : %0.6lf\n", (double)profile[i].m_TotalTime.QuadPart / (double)Frequency.QuadPart * 1000000);
		printf("평균 : %0.6lf\n", (double)profile[i].m_TotalTime.QuadPart / (double)Frequency.QuadPart * 1000000 / profile[i].m_CallCount);
		printf("최소 시간1 / 2 : %0.6lf / %0.6lf\n", ((double)profile[i].m_MInTime[0] / (double)Frequency.QuadPart) * 1000000, ((double)profile[i].m_MInTime[1] / (double)Frequency.QuadPart) * 1000000);
		printf("최대 시간1 / 2 : %0.6lf / %0.6lf\n", ((double)profile[i].m_MaxTime[0] / (double)Frequency.QuadPart) * 1000000, (double)(profile[i].m_MaxTime[1] / (double)Frequency.QuadPart) * 1000000);
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
		if (profile[i].m_MInTime[1] == MIN_SET)
		{
			profile[i].m_MInTime[1] = profile[i].m_MInTime[0];
			profile[i].m_TotalTime.QuadPart += profile[i].m_MInTime[1];
			profile[i].m_CallCount++;
		}

		// 마찬가지로 최대의 [1]번 배열에 값이 들어간 적 없다면, [0]의 값을 [1]로 옮긴다.
		if (profile[i].m_MaxTime[1] == 0)
		{
			profile[i].m_MaxTime[1] = profile[i].m_MaxTime[0];
			profile[i].m_TotalTime.QuadPart += profile[i].m_MaxTime[1];
			profile[i].m_CallCount++;
		}
		// 마이크로 세컨드 단위로 변경 후 출력
		fprintf_s(wStream, "\n%16s |%11.3lf㎲ |%11.3lf㎲ |%11.3lf㎲ |%11d |", profile[i].m_Name, ((double)profile[i].m_TotalTime.QuadPart / (double)Frequency.QuadPart) * 1000000 / profile[i].m_CallCount, ((double)profile[i].m_MInTime[1] / (double)Frequency.QuadPart) * 1000000, ((double)profile[i].m_MaxTime[1] / (double)Frequency.QuadPart) * 1000000, profile[i].m_CallCount);
	}

	fclose(wStream);

}

