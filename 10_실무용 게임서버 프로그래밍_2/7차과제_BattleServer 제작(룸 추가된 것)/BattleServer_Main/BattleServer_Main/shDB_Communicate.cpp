#include "pch.h"
#include "shDB_Communicate.h"

#include <process.h>


// -----------------------
// shDB와 통신하는 클래스
// -----------------------
namespace Library_Jingyu
{

	// --------------
	// 스레드
	// --------------

	// DB_Read 스레드
	UINT WINAPI shDB_Communicate::DB_ReadThread(LPVOID lParam)
	{
		shDB_Communicate* gThis = (shDB_Communicate*)lParam;
		
		// --------------------
		// 로컬로 받아두기
		// --------------------

		// IOCP 핸들
		HANDLE hIOCP = gThis->m_hDB_Read;

		// Read가 완료된 DB_WORK* 를 저장해두는 락프리 큐 받아두기
		CLF_Queue<DB_WORK*> *m_pComQueue = gThis->m_pDB_ReadComplete_Queue;


		// --------------------
		// 변수 선언
		// --------------------
		HTTP_Exchange m_HTTP_Post((TCHAR*)_T("127.0.0.1"), 80);	

		DWORD APIType;
		DB_WORK* pWork;
		OVERLAPPED* overlapped;
		
		while (1)
		{
			// 변수들 초기화
			APIType = 0;
			pWork = nullptr;
			overlapped = nullptr;

			// 신호 기다리기
			GetQueuedCompletionStatus(hIOCP, &APIType, (PULONG_PTR)&pWork, &overlapped, INFINITE);

			// 종료 신호일 경우 처리
			if (APIType == en_PHP_TYPE::EXIT)
				break;


			// 1. APIType 확인
			switch (APIType)
			{

				// Seelct_account.php
			case SELECT_ACCOUNT:
			{
				// DB에 쿼리 날려서 확인.
				TCHAR Body[1000];

				ZeroMemory(pWork->m_tcRequest, sizeof(pWork->m_tcRequest));
				ZeroMemory(Body, sizeof(Body));

				// 1. Body 만들기
				swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld}", pWork->AccountNo);

				// 2. http 통신 및 결과 얻기
				if (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Select_account.php"), Body, pWork->m_tcRequest) == false)
					gThis->m_Dump->Crash();

				// 3. Read 완료 큐에 넣기
				m_pComQueue->Enqueue(pWork);
			}
				break;


			default:
				break;
			}
		}

		return 0;
	}


	// ---------------
	// 기능 함수. 외부에서 호출 가능
	// ---------------

	// DB에 Read할 것이 있을 때 호출되는 함수
	// 인자로 받은 구조체의 정보를 확인해 로직 처리
	// 
	// Parameter : DB_WORK*, APIType
	// return : 없음
	void shDB_Communicate::DBReadFunc(DB_WORK* Protocol, WORD APIType)
	{
		// Read용 스레드에게 일감 던짐 (PQCS)
		// 이게 끝
		PostQueuedCompletionStatus(m_hDB_Read, APIType, (ULONG_PTR)Protocol, 0);
	}	


	// ---------------
	// 생성자와 소멸자
	// ---------------

	// 생성자
	shDB_Communicate::shDB_Communicate()
	{
		// 덤프 받기
		m_Dump = CCrashDump::GetInstance();

		// DB_WORK 메모리풀 동적할당
		m_pDB_Work_Pool = new CMemoryPoolTLS<DB_WORK>(0, false);
		
		// Read 완료 락프리 큐 동적할당
		m_pDB_ReadComplete_Queue = new CLF_Queue<DB_WORK*>(0);

		// DB_Read용 입출력 완료포트 생성
		// 4개의 스레드 생성, 2개의 스레드 활성화
		int Create = 4;
		int Active = 2;		

		m_hDB_Read = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, Active);
		if (m_hDB_Read == NULL)
			m_Dump->Crash();

		// DB_Read용 워커 스레드 생성
		m_hDB_Read_Thread = new HANDLE[Create];
		for (int i = 0; i < Create; ++i)
		{
			m_hDB_Read_Thread[i] = (HANDLE)_beginthreadex(0, 0, DB_ReadThread, this, 0, 0);
			if(m_hDB_Read_Thread[i] == NULL)
				m_Dump->Crash();
		}

	}

	// 소멸자
	shDB_Communicate::~shDB_Communicate()
	{
		// 1. DB_ReadThread 종료
		for(int i=0; i<4; ++i)
			PostQueuedCompletionStatus(m_hDB_Read, en_PHP_TYPE::EXIT, 0, 0);

		WaitForMultipleObjects(4, m_hDB_Read_Thread, TRUE, INFINITE);

		// 2. 입출력 완료포트 닫기
		CloseHandle(m_hDB_Read);

		// 3. Read 완료 락프리 큐 비우기
		DB_WORK* Delete;
		while (1)
		{
			// 꺼내기
			if (m_pDB_ReadComplete_Queue->Dequeue(Delete) == -1)
				break;

			// 직렬화 버퍼 Free
			CProtocolBuff_Net::Free(Delete->m_pBuff);

			// 반환
			m_pDB_Work_Pool->Free(Delete);
		}


		// 4. 각종 동적해제

		// 워커 스레드 핸들 동적해제
		delete[] m_hDB_Read_Thread;

		// DB_WORK 메모리풀 동적해제
		delete m_pDB_Work_Pool;

		// Read 완료 락프리 큐 동적해제
		delete m_pDB_ReadComplete_Queue;
	}
}