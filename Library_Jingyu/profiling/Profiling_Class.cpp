#include "pch.h"
#include "Profiling_Class.h"
#include "CrashDump\CrashDump.h"
#include <Windows.h>

namespace Library_Jingyu
{

#define PROFILING_SIZE		50			// 스레드 별로 몇 개의 태그가 가능한지.
#define MIN_SET				100000000	// 최초 생성 시, Min의 값. 체크용도.
#define MAX_PROFILE_COUNT	1000		// 최대 몇 개의 스레드가 프로파일링 가능한지

	// 스레드 별로 TLS에 저장되는 구조체.
	struct stThread_Profile
	{
		Profiling m_Profile[PROFILING_SIZE];
		int m_NowProfiling_Size = -1;
		LONG ThreadID;
	};

	// 각 스레드에 동적할당되는 stThread_Profile의 주소 저장공간. 총 1000개의 스레드 가능.
	stThread_Profile* g_pThread_Profile_Array[MAX_PROFILE_COUNT];
	LONG g_pThread_Profile_Array_Count = -1;
	
	CCrashDump* g_ProfileDump = CCrashDump::GetInstance();		// 덤프 객체
	LARGE_INTEGER g_Frequency;									// 주파수 저장 변수
	DWORD g_TLSIndex;											// 모든 스레드가 사용하는 TLS 인덱스

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
		QueryPerformanceFrequency(&g_Frequency);

		// 전역에 인덱스 하나 얻어두기. 모든 스레드의 TLS는 이 인덱스 사용.
		g_TLSIndex = TlsAlloc();
		if (g_TLSIndex == TLS_OUT_OF_INDEXES)
		{
			DWORD Error = GetLastError();
			g_ProfileDump->Crash();
		}
	}

	// 프로파일링 시작
	void BEGIN(const char* str)
	{
		// 스레드의 TLS 가져오기
		stThread_Profile* MyProfile = (stThread_Profile*)TlsGetValue(g_TLSIndex);

		// 만약, nullptr이라면 새로 할당한다.
		if (MyProfile == nullptr)
		{
			MyProfile = new stThread_Profile;

			// 할당 후, 내 TLS에 셋팅한다.
			if (TlsSetValue(g_TLSIndex, MyProfile) == 0)
			{
				DWORD Error = GetLastError();
				g_ProfileDump->Crash();
			}

			LONG Temp = InterlockedIncrement(&g_pThread_Profile_Array_Count);
			MyProfile->ThreadID = GetThreadId(GetCurrentThread());
			g_pThread_Profile_Array[Temp] = MyProfile;
		}

		
		// 배열 내에, 해당 스트링이 존재하는지 체크
		int i = 0;
		bool bFlag = false;

		for (i = 0; i <= MyProfile->m_NowProfiling_Size; ++i)
		{
			if (strcmp(MyProfile->m_Profile[i].m_Name, str) == 0)
			{
				bFlag = true;
				break;
			}
		}

		// 현재 할당된 프로파일링만큼 검색했지만 이름이 없다면, 추가로 현재까지 할당된 사이즈가 PROFILING_SIZE보다 작다면
		// 새로운 프로파일링 생성
		if (bFlag == false && MyProfile->m_NowProfiling_Size < PROFILING_SIZE)
		{
			int NowSize = ++MyProfile->m_NowProfiling_Size;
			Profiling* safe_Profile = &MyProfile->m_Profile[NowSize];
			
			strcpy_s(safe_Profile->m_Name, sizeof(safe_Profile->m_Name), str);			// 이름 저장
			safe_Profile->m_CallCount = 1;												// BGEIN 호출 횟수 저장
			QueryPerformanceCounter(&safe_Profile->m_StartCount);
			safe_Profile->m_StartTime = (double)safe_Profile->m_StartCount.QuadPart;	// 시작 시간 저장.		
		}

		// 검색된 이름이 있다면, 해당 프로파일링에 시작 시간 저장
		else if (bFlag == true)
		{
			Profiling* safe_Profile = &MyProfile->m_Profile[i];

			safe_Profile->m_CallCount++;												// BEGIN 호출 횟수 저장
			QueryPerformanceCounter(&safe_Profile->m_StartCount);
			safe_Profile->m_StartTime = (double)safe_Profile->m_StartCount.QuadPart;	// 시작 시간 저장		
		}
	}

	// 프로파일링 종료
	void END(const char* str)
	{
		// 카운트 값 구해두기.
		LARGE_INTEGER TempEndCount;
		QueryPerformanceCounter(&TempEndCount);

		// 스레드의 TLS 가져오기
		stThread_Profile* MyProfile = (stThread_Profile*)TlsGetValue(g_TLSIndex);

		// 만약, nullptr이라면 에러.
		if (MyProfile == nullptr)
		{
			DWORD Error = GetLastError();
			g_ProfileDump->Crash();
		}		

		int i;
		bool bFlag = true;

		for (i = 0; strcmp(MyProfile->m_Profile[i].m_Name, str) != 0; ++i)
		{
			if (i > MyProfile->m_NowProfiling_Size)
			{
				bFlag = false;
				break;
			}
		}

		// 검색된 이름이 있고, 그 프로파일링의 시작 시간이 0이 아니라면, 정보 저장
		if (bFlag == true && MyProfile->m_Profile[i].m_StartTime != 0)
		{
			Profiling* safe_Profile = &MyProfile->m_Profile[i];

			safe_Profile->m_EndCount = TempEndCount;

			bool bCallmulCheck = false;
			bool bTimeMulCheck = false;

			// BEGIN부터 END까지의 시간을 마이크로 세컨드 단위로 저장.
			double dSecInterval = ((double)(safe_Profile->m_EndCount.QuadPart - safe_Profile->m_StartTime) / g_Frequency.QuadPart) * 1000000;

			// 총 시간 저장
			safe_Profile->m_TotalTime += dSecInterval;

			// dSecInterval가 최소에 들어가야하는지 최대에 들어가야하는지 찾는다.
			// 버려지는 배열(최소) 체크.
			// 버려지는 배열에 들어가는 경우. 
			if (safe_Profile->m_MInTime[0] > dSecInterval)
			{
				if (safe_Profile->m_MInTime[0] == MIN_SET)	// 만약, 최초 호출이라면
				{
					safe_Profile->m_CallCount--;			// 콜 수를 1 감소. 교체가 아니라 아예 새로 넣는것이기 때문에 1 감소해야 함.	
					bCallmulCheck = true;				// 아래 최대의 버려지는 배열에도 현재 dSecInterval값이 들어갈 수 있기 때문에 콜을 뺏다는 것을 체크. 중복 빼기 방지.
				}
				else		                            // 최초 호출이 아니라면
				{
					safe_Profile->m_TotalTime -= dSecInterval;				// 최초 호출이 아니면, 총 시간에서 현재 시간(dSecInterval)을 다시 뺀다. 최초 호출 시에는 시간을 뺼 필요도 없음. 애초에 추가된 적이 없기 때문에.
					safe_Profile->m_TotalTime += safe_Profile->m_MInTime[0];	// 기존의 [0] 위치(버려지는 배열)의 시간을 총 시간에 더한다. 이걸 이제 최소로 쓸 것이기 때문에.
					safe_Profile->m_MInTime[1] = safe_Profile->m_MInTime[0];	// 기존 [0]의 시간을 [1]로 옮긴다.
					bTimeMulCheck = true;								// 아래 최대의 버려지는 배열에도 현재 dSecInterval값이 들어갈 수 있기 때문에 시간을 뺏다는 것을 체크. 중복 빼기 방지
				}

				safe_Profile->m_MInTime[0] = dSecInterval;		// 최초 호출 여부랑 상관없이, 현재 시간(dSecInterval)을 [0]에 넣는다. 이건 안쓰는 값이 됨.	

			}

			// 버려지지 않는 배열에 들어가는 경우, 그 시간을 1번 배열에 저장한다.
			else if (safe_Profile->m_MInTime[1] >= dSecInterval)
				safe_Profile->m_MInTime[1] = dSecInterval;

			// 이제 최대 체크
			// 버려지는 배열에 들어가는 경우, 시간을 빼고 호출횟수 감소. 그리고 버려진 시간을 0번 배열에 저장.
			if (safe_Profile->m_MaxTime[0] < dSecInterval)
			{
				if (safe_Profile->m_MaxTime[0] == 0 && bCallmulCheck == false)
					safe_Profile->m_CallCount--;		

				else
				{
					if (bTimeMulCheck == false)
						safe_Profile->m_TotalTime -= dSecInterval;

					safe_Profile->m_TotalTime += safe_Profile->m_MaxTime[0];
					safe_Profile->m_MaxTime[1] = safe_Profile->m_MaxTime[0];
				}

				safe_Profile->m_MaxTime[0] = dSecInterval;
			}

			// 버려지지 않는 배열에 들어가는 경우, 그 시간을 1번 배열에 저장한다.
			else if (safe_Profile->m_MaxTime[1] <= dSecInterval)
				safe_Profile->m_MaxTime[1] = dSecInterval;

			safe_Profile->m_StartTime = 0;	// StartTime을 0으로 만들어서, 해당 BEGIN이 끝났다는것 알려줌.
		}
		// 검색된 이름이 없다.
		else if (bFlag == false)
		{
			printf("END(). [%s] Not Find Name\n", str);
			g_ProfileDump->Crash();
		}
		// 검색된 이름은 있는데, BEGIN중이 아니라면
		else if (MyProfile->m_Profile[i].m_StartTime == 0)
		{
			printf("END(). [%s] Not Begin Name\n", str);
			g_ProfileDump->Crash();

		}

	}

	// 프로파일링 전체 리셋
	void RESET()
	{
		// 스레드의 TLS 가져오기
		stThread_Profile* MyProfile = (stThread_Profile*)TlsGetValue(g_TLSIndex);

		// 만약, nullptr이라면 에러.
		if (MyProfile == nullptr)
		{
			DWORD Error = GetLastError();
			g_ProfileDump->Crash();
		}

		for (int i = 0; i <= MyProfile->m_NowProfiling_Size; ++i)
		{
			MyProfile->m_Profile[i].m_Name[0] = '\0';
			MyProfile->m_Profile[i].m_TotalTime = 0;
			MyProfile->m_Profile[i].m_MInTime[0] = 100000000;
			MyProfile->m_Profile[i].m_MInTime[1] = 100000000;
			MyProfile->m_Profile[i].m_MaxTime[0] = 0;
			MyProfile->m_Profile[i].m_MaxTime[1] = 0;
			MyProfile->m_Profile[i].m_StartTime = 0;
		}
		MyProfile->m_NowProfiling_Size = -1;
	}

	// 프로파일링 정보 전체 보기
	void PROFILING_SHOW()
	{
		// 스레드의 TLS 가져오기
		stThread_Profile* MyProfile = (stThread_Profile*)TlsGetValue(g_TLSIndex);

		// 만약, nullptr이라면 에러.
		if (MyProfile == nullptr)
		{
			DWORD Error = GetLastError();
			g_ProfileDump->Crash();
		}

		for (int i = 0; i <= MyProfile->m_NowProfiling_Size; ++i)
		{
			printf("\n이름 : %s\n", MyProfile->m_Profile[i].m_Name);
			printf("호출 횟수 :%d\n", MyProfile->m_Profile[i].m_CallCount);
			printf("총 시간 : %0.6lf\n", MyProfile->m_Profile[i].m_TotalTime);
			printf("평균 : %0.6lf\n", MyProfile->m_Profile[i].m_TotalTime / MyProfile->m_Profile[i].m_CallCount);
			printf("최소 시간1 / 2 : %0.6lf / %0.6lf\n", MyProfile->m_Profile[i].m_MInTime[0], MyProfile->m_Profile[i].m_MInTime[1]);
			printf("최대 시간1 / 2 : %0.6lf / %0.6lf\n", MyProfile->m_Profile[i].m_MaxTime[0], MyProfile->m_Profile[i].m_MaxTime[1]);
		}
	}

	// 프로파일링을 파일로 출력. Profiling와 friend
	void PROFILING_FILE_SAVE()
	{
		// 파일 스트림
		FILE* wStream;		

		// 스트림 오픈 여부 체크
		if (fopen_s(&wStream, "Profiling.txt", "wt") != 0)
			return;

		for (int i = 0; i <= g_pThread_Profile_Array_Count; ++i)
		{
			Profiling* NowSaveProfile = g_pThread_Profile_Array[i]->m_Profile;
			LONG TempSize = g_pThread_Profile_Array[i]->m_NowProfiling_Size;
			LONG ThreadID = g_pThread_Profile_Array[i]->ThreadID;

			// 잘 가져왔으면 파일 저장 시작한다.
			fprintf_s(wStream, "\n============================================================================\n ");
			fprintf_s(wStream, "%13s	|%30s  |%12s  |%13s   |%13s   |%13s |", "ThreadID", "Name", "Average", "Min", "Max", "Call");
			fprintf_s(wStream, "\n------------------------------------------------------------------------------------------------------------------ ");

			for (int i = 0; i <= TempSize; ++i)
			{
				// 최소의 [1]번 배열에 값이 들어간 적 없다면, [0]의 값을 [1]로 옮긴다.
				// 동시에 TotalTime에 시간을 추가하고 m_CallCount에 횟수도 추가한다.
				if (NowSaveProfile[i].m_MInTime[1] == 100000000)
				{
					NowSaveProfile[i].m_MInTime[1] = NowSaveProfile[i].m_MInTime[0];
					NowSaveProfile[i].m_TotalTime += NowSaveProfile[i].m_MInTime[1];
					NowSaveProfile[i].m_CallCount++;
				}

				// 최대도 마찬가지.
				if (NowSaveProfile[i].m_MaxTime[1] == 0)
				{
					NowSaveProfile[i].m_MaxTime[1] = NowSaveProfile[i].m_MaxTime[0];
					NowSaveProfile[i].m_TotalTime += NowSaveProfile[i].m_MaxTime[1];
					NowSaveProfile[i].m_CallCount++;
				}

				fprintf_s(wStream, "\n%13d	|%31s |%11.3lf㎲ |%11.3lf㎲ |%11.3lf㎲ |%11d |",
					ThreadID, NowSaveProfile[i].m_Name, NowSaveProfile[i].m_TotalTime / NowSaveProfile[i].m_CallCount,
					NowSaveProfile[i].m_MInTime[1], NowSaveProfile[i].m_MaxTime[1], NowSaveProfile[i].m_CallCount);
			}

			// 1개 스레드 저장 완료되었으니 완료표식 저장
			fprintf_s(wStream, "\n============================================================================\n\n ");

			// 저장한 스레드 동적해제
			delete g_pThread_Profile_Array[i];
		}

		// 인덱스 반납
		if (TlsFree(g_TLSIndex) == 0)
		{
			DWORD Error = GetLastError();
			g_ProfileDump->Crash();
		}

		fclose(wStream);
	}

}