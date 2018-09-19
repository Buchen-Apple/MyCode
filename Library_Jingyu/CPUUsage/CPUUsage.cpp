#include "pch.h"
#include "CPUUsage.h"

// ---------------------
// '프로세서'의 사용율을 구하는 클래스
// ---------------------
namespace Library_Jingyu
{
	// 생성자
	CCpuUsage_Processor::CCpuUsage_Processor()
	{
		// 멤버변수 초기화
		m_fProcessorTotal = 0;
		m_fProcessorUser = 0;
		m_fProcessorKernel = 0;

		m_ftProcessor_LastKernel.QuadPart = 0;
		m_ftProcessor_LastUser.QuadPart = 0;
		m_ftProcessor_LastIdle.QuadPart = 0;

		// 최초, CPU 사용율 갱신
		UpdateCpuTime();
	}

	// 프로세서 사용율 갱신
	//
	// 해당 함수가 호출되면, 현 PC 프로세서의 Idle, User, Kernel 정보가 갱신된다.
	void CCpuUsage_Processor::UpdateCpuTime()
	{
		// 필요한 구조체는 FILETIME 이지만, ULARGE_INTEGER와 구조가 같으므로 이를 사용.
		// FILETIME이 필요한 이유는 100 나노세컨드 단위의 시간이 필요하기 때문에.
		// 오히려 정밀도는 떨어진다.
		ULARGE_INTEGER Idle;
		ULARGE_INTEGER Kernel;
		ULARGE_INTEGER User;

		// 1. 시스템 사용 시간을 구한다.
		// 아이들 / 커널 / 유저 타임
		if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
		{
			return;
		}

		// 2. 커널, 유저, 아이들 시간 구한다.
		// 커널 타임에는 아이들 타임이 포함되어 있다.
		ULONGLONG KernelDiff = Kernel.QuadPart - m_ftProcessor_LastKernel.QuadPart;
		ULONGLONG UserDiff = User.QuadPart - m_ftProcessor_LastUser.QuadPart;
		ULONGLONG IdleDiff = Idle.QuadPart - m_ftProcessor_LastIdle.QuadPart;

		// 3. 토탈 구한다.
		ULONGLONG Total = KernelDiff + UserDiff;

		// 만약, Total이 0이라면, 프로세서가 전혀 사용되지 않은 것이니 모든 값 0으로 셋팅
		if (Total == 0)
		{
			m_fProcessorTotal = 0;
			m_fProcessorUser = 0;
			m_fProcessorKernel = 0;
		}

		// Total이 0이, 아니라면 값 계산
		else
		{
			// 커널 타임에 아이들 타임이 있으므로, 뺀다.
			m_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
			m_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
			m_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);

		}

		// 4. 대기 시간, 커널 시간, 유저 시간 갱신
		m_ftProcessor_LastKernel = Kernel;
		m_ftProcessor_LastUser = User;
		m_ftProcessor_LastIdle = Idle;

	}
}


// ---------------------
// '지정된 프로세스'의 사용율을 구하는 클래스
// ---------------------
namespace Library_Jingyu
{
	// 생성자
	//
	// Parameter : 사용율을 체크할 프로세스 핸들. (디폴트 : 자기자신)
	CCpuUsage_Process::CCpuUsage_Process(HANDLE hProcess)
	{
		// 입력받은 프로세스 핸들이 없다면, 자기 자신의 사용율을 체크.
		if (hProcess == INVALID_HANDLE_VALUE)
			m_hProcess = hProcess;

		// 프로세서 수 확인(코어 수 확인)
		// 프로세스 (exe) 실행율 계산 시, CPU 수로 나눠 실제 사용율을 구한다.
		SYSTEM_INFO SystemInfo;

		GetSystemInfo(&SystemInfo);
		m_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

		// 멤버변수 초기화
		m_fProcessTotal = 0;
		m_fProcessUser = 0;
		m_fProcessKernel = 0;

		m_ftProcess_LastKernel.QuadPart = 0;
		m_ftProcess_LastUser.QuadPart = 0;
		m_ftProcess_LastTime.QuadPart = 0;

		// 최초, CPU 사용율 갱신
		UpdateCpuTime();
			   
	}

	// 프로세스 사용율 갱신
	//
	// 해당 함수가 호출되면, 지정된 프로세스의 Total, User, Kernel 정보가 갱신된다.
	void CCpuUsage_Process::UpdateCpuTime()
	{
		// 1. 경과된 시간을 구한다 (100 나노세컨드 단위)
		// 현재 시간을 받아둘 변수		
		ULARGE_INTEGER NowTime;

		// 현재시간 구한다.
		GetSystemTimeAsFileTime((LPFILETIME)&NowTime);


		// 2. 해당 프로세스가 사용한 시간 구함		
		// 두 번째, 세 번째 인자는 실행, 종료 시간. 필요없으니 형식만 맞추고 안쓴다.
		// 이 때 인자로 던질 변수
		ULARGE_INTEGER None;

		// 커널 시간, 유저 시간을 받아둘 변수
		ULARGE_INTEGER Kernel;
		ULARGE_INTEGER User;

		// 시간 구하기
		GetProcessTimes(m_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);


		// 3. 이전에 저장된 프로세스 시간과의 차를 구해서 실제로 얼마의 시간이 지났는지 계산
		// 이 결과를 실제 지나온 시간으로 나누면 사용율이 나온다.
		ULONGLONG TimeDiff = NowTime.QuadPart - m_ftProcess_LastTime.QuadPart;
		ULONGLONG UserDiff = User.QuadPart - m_ftProcess_LastUser.QuadPart;
		ULONGLONG KernelDiff = Kernel.QuadPart - m_ftProcess_LastKernel.QuadPart;

		ULONGLONG Total = KernelDiff + UserDiff;

		m_fProcessTotal = (float)(Total / (double)m_iNumberOfProcessors / (double)TimeDiff * 100.0f);
		m_fProcessKernel = (float)(KernelDiff / (double)m_iNumberOfProcessors / (double)TimeDiff * 100.0f);
		m_fProcessUser = (float)(UserDiff / (double)m_iNumberOfProcessors / (double)TimeDiff * 100.0f);


		// 4. 경과 시간, 커널시간, 유저시간 갱신
		m_ftProcess_LastTime = NowTime;
		m_ftProcess_LastKernel = Kernel;
		m_ftProcess_LastUser = User;
	}
}
