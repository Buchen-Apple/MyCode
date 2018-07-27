#pragma once
#ifndef __PROFILING_CLASS_H__
#define __PROFILING_CLASS_H__

#include <iostream>
#include <Windows.h>
#define PROFILING_SIZE 50

class Profiling
{
private:
	char m_Name[30];
	int m_CallCount;
	long long m_StartTime;

	LARGE_INTEGER m_TotalTime;
	long long m_Average;
	long long m_MaxTime[2];
	long long m_MInTime[2];

public:
	Profiling();
	friend void BEGIN(const char*);
	friend void END(const char*);
	friend void RESET();
	friend void PROFILING_SHOW();
	friend void PROFILING_FILE_SAVE();
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
