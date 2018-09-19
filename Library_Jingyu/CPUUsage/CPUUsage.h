#ifndef __CPU_USAGE_H__
#define __CPU_USAGE_H__

#include <Windows.h>

// ---------------------
// '프로세서'의 사용율을 구하는 클래스
// ---------------------
namespace Library_Jingyu
{
	class CCpuUsage_Processor
	{
		// ---------------------
		// 멤버 변수		
		// ---------------------
		float m_fProcessorTotal;
		float m_fProcessorUser;
		float m_fProcessorKernel;		

		ULARGE_INTEGER m_ftProcessor_LastKernel;
		ULARGE_INTEGER m_ftProcessor_LastUser;
		ULARGE_INTEGER m_ftProcessor_LastIdle;

	public:
		// ---------------------
		// 생성자
		// ---------------------
		CCpuUsage_Processor();

		// ---------------------
		// 기능 함수
		// ---------------------

		// 프로세서 사용율 갱신
		//
		// 해당 함수가 호출되면, 현 PC 프로세서의 Idle, User, Kernel 정보가 갱신된다.
		void UpdateCpuTime();


		// ---------------------
		// 게터 함수
		// ---------------------
		// 프로세서 total 얻기
		//
		// Parameter : 없음
		// return : 프로세서의 토탈값 (float)
		float  ProcessorTotal()
		{
			return m_fProcessorTotal;
		}

		// 프로세서 중, User 모드 사용율 얻기
		//
		// Parameter : 없음
		// return : 프로세서 중, User모드 사용율 (float)
		float  ProcessorUser()
		{
			return m_fProcessorUser;
		}

		// 프로세서 중, Kernel 모드 사용율 얻기
		//
		// Parameter : 없음
		// return : 프로세서 중, Kernel모드 사용율 (float)
		float  ProcessorKernel()
		{
			return m_fProcessorKernel;
		}
	};
}

// ---------------------
// '지정된 프로세스'의 사용율을 구하는 클래스
// ---------------------
namespace Library_Jingyu
{
	class CCpuUsage_Process
	{
		// ---------------------
		// 멤버 변수		
		// ---------------------
		HANDLE m_hProcess;

		int m_iNumberOfProcessors;	// 해당 PC의 스레드 가동 수. (하이퍼 스레드 포함)

		float m_fProcessTotal;
		float m_fProcessUser;
		float m_fProcessKernel;

		ULARGE_INTEGER m_ftProcess_LastKernel;
		ULARGE_INTEGER m_ftProcess_LastUser;
		ULARGE_INTEGER m_ftProcess_LastTime;

	public:
		// 생성자
		//
		// Parameter : 사용율을 체크할 프로세스 핸들. (디폴트 : 자기자신)
		CCpuUsage_Process(HANDLE hProcess = INVALID_HANDLE_VALUE);



		// ---------------------
		// 기능 함수
		// ---------------------

		// 프로세스 사용율 갱신
		//
		// 해당 함수가 호출되면, 지정된 프로세스의 Total, User, Kernel 정보가 갱신된다.
		void UpdateCpuTime();




		// ---------------------
		// 게터 함수
		// ---------------------

		// 지정된 프로세스의 total 얻기
		//
		// Parameter : 없음
		// return : 지정된 프로세스의 토탈값 (float)
		float  ProcessTotal()
		{
			return m_fProcessTotal;
		}

		// 지정된 프로세스의, User 모드 사용율 얻기
		//
		// Parameter : 없음
		// return : 지정된 프로세스의, User모드 사용율 (float)
		float  ProcessUser()
		{
			return m_fProcessUser;
		}

		// 지정된 프로세스의, Kernel 모드 사용율 얻기
		//
		// Parameter : 없음
		// return : 지정된 프로세스의, Kernel모드 사용율 (float)
		float  ProcessKernel()
		{
			return m_fProcessKernel;
		}
	};
}

#endif // !__CPU_USAGE_H__
