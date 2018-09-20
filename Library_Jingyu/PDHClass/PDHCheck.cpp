#include "pch.h"
#include "PDHCheck.h"
#include "CrashDump\CrashDump.h"

#include <strsafe.h>


namespace Library_Jingyu
{
	CCrashDump* gPdhDump = CCrashDump::GetInstance();

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

		// -------------- 필요한 쿼리들 모두 등록

		// 사용가능 메모리 쿼리 등록
		if (PdhAddCounter(m_pdh_Query, L"\\Memory\\Available MBytes", NULL, &m_pdh_AVA_MEM_Counter) != ERROR_SUCCESS)
		{
			gPdhDump->Crash();
		}

		// 논페이지 메모리 사용량 쿼리 등록
		if(PdhAddCounter(m_pdh_Query, L"\\Memory\\Pool Nonpaged Bytes", NULL, &m_pdh_NonPagedPool_Counter) != ERROR_SUCCESS)
		{
			gPdhDump->Crash();
		}

		// 네트워크, Recv용 쿼리 등록
		if (PdhAddCounter(m_pdh_Query, L"\\Network Interface(*)\\Bytes Received/sec", NULL, &m_pdh_Recv_Counter) != ERROR_SUCCESS)
		{
			gPdhDump->Crash();
		}

		// 네트워크, Send용 쿼리 등록
		if (PdhAddCounter(m_pdh_Query, L"\\Network Interface(*)\\Bytes Sent/sec", NULL, &m_pdh_Send_Counter) != ERROR_SUCCESS)
		{
			gPdhDump->Crash();
		}

		// 유저 커밋 사용량 쿼리 등록
		// 멤버변수에 프로세스 이름 셋팅
		SetProcessName();

		// 쿼리 등록
		TCHAR tQuery[256] = { 0, };
		StringCbPrintf(tQuery, sizeof(TCHAR) * 256, L"\\Process(%s)\\Private Bytes", m_ProcessName);

		if(PdhAddCounter(m_pdh_Query, tQuery, NULL, &m_pdh_UserCommit) != ERROR_SUCCESS)
		{
			gPdhDump->Crash();
		}


		// -------------- 샌드,리시브 배열 미리 만들어두기   	      

		// 데이터 가져오기
		PdhCollectQueryData(m_pdh_Query);

		// 1. 리시브
		// 무조건 PDH_MORE_DATA가 나와야 한다. 버퍼 사이즈가 부족하기 때문에.
		if (PdhGetFormattedCounterArray(m_pdh_Recv_Counter, PDH_FMT_DOUBLE, &m_dwBufferSize_Recv, &m_dwItemCount_Recv, m_pitems_Recv) == PDH_MORE_DATA)
		{
			// 버퍼 동적할당
			m_pitems_Recv = (PDH_FMT_COUNTERVALUE_ITEM *)malloc(m_dwBufferSize_Recv);
		}
		else
			gPdhDump->Crash();

		// 2. 샌드
		// 무조건 PDH_MORE_DATA가 나와야 한다. 버퍼 사이즈가 부족하기 때문에.
		if (PdhGetFormattedCounterArray(m_pdh_Send_Counter, PDH_FMT_DOUBLE, &m_dwBufferSize_Send, &m_dwItemCount_Send, m_pitems_Send) == PDH_MORE_DATA)
		{
			// 버퍼 동적할당
			m_pitems_Send = (PDH_FMT_COUNTERVALUE_ITEM *)malloc(m_dwBufferSize_Send);
		}
		else
			gPdhDump->Crash();
		

	}



	// -----------------
	// 기능 함수
	// -----------------	

	// 현재 프로세스 이름 셋팅
	// 멤버변수에 저장한다.
	//
	// Parameter : 없음
	// return : 없음
	void CPDH::SetProcessName()
	{
		TCHAR Name[1000];

		// 파일의 경로가 나온다.
		GetModuleFileName(NULL, Name, 1000);

		// Path의 길이를 알아온다.
		int Size = (int)_tcslen(Name);
		--Size;

		// 가장 끝을 가리킨다. 끝부터 <<로 한칸씩 가면서 \를 찾는다.
		TCHAR* Start = &Name[Size];
		while (1)
		{
			// \를 찾았으면, >>로 한칸 움직여, 파일 이름의 처음을 가리키게 한다.
			if (*Start == L'\\')
			{
				Size++;
				Start = &Name[Size];
				break;
			}

			--Size;
			Start = &Name[Size];
		}

		// Start는 파일 이름을 가리키는중. 하지만, exe까지 붙어있기 때문에 3을 뺀다.
		// 정확히는 ".exe"로 점까지 있지만, StringCbCopy는 Null문자 고려해, 하나 뺀 후 복사하기 때문에
		// 무시해도 된다.
		//
		// ex) PDH_Main.exe는 길이를 구하면 12가 나온다. 
		// ex) 3을빼면 9가 나온다.
		// ex) 널문자 고려하면 총 8개의 글자가 복사된다 (PDH_Main 까지만 복사됨)
		StringCbCopy(m_ProcessName, sizeof(TCHAR) * (_tcslen(Start) - 3), Start);
	}





	// 정보 갱신하기
	// 카운터의 정보 모두 갱신 후, 각 변수에 넣어둔다.
	// 
	// Parameter : 없음
	// return : 없음
	void CPDH::SetInfo()
	{
		// 샌드, 리시브 값 0으로 갱신.
		m_pdh_Recv_Value = m_pdh_Send_Value = 0;


		// ---------- 데이터 가져오기
		PdhCollectQueryData(m_pdh_Query);

		// ---------- 데이터 뽑기은 후 Value 갱신
		PDH_FMT_COUNTERVALUE CounterValue;

		// 1. 사용가능 메모리
		if (PdhGetFormattedCounterValue(m_pdh_AVA_MEM_Counter, PDH_FMT_DOUBLE, NULL, &CounterValue) == ERROR_SUCCESS)
			m_pdh_AVA_Mem_Value = CounterValue.doubleValue;

		// 2. 네트워크 리시브
		if (PdhGetFormattedCounterArray(m_pdh_Recv_Counter, PDH_FMT_DOUBLE, &m_dwBufferSize_Recv, &m_dwItemCount_Recv, m_pitems_Recv) == ERROR_SUCCESS)
		{
			// Loop through the array and print the instance name and counter value.
			for (DWORD i = 0; i < m_dwItemCount_Recv; i++)
			{
				m_pdh_Recv_Value += m_pitems_Recv[i].FmtValue.doubleValue;
			}
		}		

		// 3. 네트워크 샌드
		if (PdhGetFormattedCounterArray(m_pdh_Send_Counter, PDH_FMT_DOUBLE, &m_dwBufferSize_Send, &m_dwItemCount_Send, m_pitems_Send) == ERROR_SUCCESS)
		{
			// Loop through the array and print the instance name and counter value.
			for (DWORD i = 0; i < m_dwItemCount_Send; i++)
			{
				m_pdh_Send_Value += m_pitems_Send[i].FmtValue.doubleValue;
			}
		}

		// 4. 논페이지 메모리
		if (PdhGetFormattedCounterValue(m_pdh_NonPagedPool_Counter, PDH_FMT_DOUBLE, NULL, &CounterValue) == ERROR_SUCCESS)
			m_pdh_NonPagedPool_Value = CounterValue.doubleValue;

		// 5. 메모리 유저 커밋 사용량
		if (PdhGetFormattedCounterValue(m_pdh_UserCommit, PDH_FMT_DOUBLE, NULL, &CounterValue) == ERROR_SUCCESS)
			m_pdh_UserCommit_Value = CounterValue.doubleValue;
	}

	// 사용가능 메모리 얻기 (MByte단위)
	//
	// parameter : 없음
	// return : 하드웨어의 사용가능 메모리
	double CPDH::Get_AVA_Mem()
	{
		return m_pdh_AVA_Mem_Value;
	}

	// 논 페이지드 메모리 사용량 얻기 (Byte단위)
	//
	// parameter : 없음
	// return : 하드웨어의 논 페이지드 메모리 사용량
	double CPDH::Get_NonPaged_Mem()
	{
		return m_pdh_NonPagedPool_Value;
	}

	// 네트워크 이더넷 Recv 얻기 (Byte)
	//
	// parameter : 없음
	// return : 네트워크 이더넷 Recv
	double CPDH::Get_Net_Recv()
	{
		return m_pdh_Recv_Value;
	}


	// 네트워크 이더넷 Send 얻기 (Byte)
	//
	// parameter : 없음
	// return : 네트워크 이더넷 Send
	double CPDH::Get_Net_Send()
	{
		return m_pdh_Send_Value;
	}

	// 유저 커밋 크기 (Byte)
	//
	// parameter : 없음
	// return : 유저 커밋 크기
	double  CPDH::Get_UserCommit()
	{
		return m_pdh_UserCommit_Value;
	}

}