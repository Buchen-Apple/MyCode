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
		HTTP_Exchange m_HTTP_Post((TCHAR*)_T("10.0.0.1"), 80);	

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
				DB_WORK_LOGIN* NowWork = (DB_WORK_LOGIN*)pWork;

				// DB에 쿼리 날려서 확인.
				TCHAR Body[1000];				

				ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));
				ZeroMemory(Body, sizeof(Body));

				// 1. Body 만들기
				swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld}", NowWork->AccountNo);

				// 2. http 통신 및 결과 얻기
				int TryCount = 5;
				while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Select_account.php"), Body, NowWork->m_tcResponse) == false)
				{
					TryCount--;

					if(TryCount == 0)
						gThis->m_Dump->Crash();
				}					

				// 3. Read 완료 큐에 넣기
				m_pComQueue->Enqueue(pWork);
			}
				break;


				// Seelct_contents.php
			case SELECT_CONTENTS:
			{
				DB_WORK_LOGIN_CONTENTS* NowWork = (DB_WORK_LOGIN_CONTENTS*)pWork;

				// DB에 쿼리 날려서 확인.
				TCHAR Body[1000];

				ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));
				ZeroMemory(Body, sizeof(Body));

				// 1. Body 만들기
				swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld}", NowWork->AccountNo);

				// 2. http 통신 및 결과 얻기
				int TryCount = 5;
				while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Select_contents.php"), Body, NowWork->m_tcResponse) == false)
				{
					TryCount--;

					if (TryCount == 0)
						gThis->m_Dump->Crash();
				}

				// 3. Read 완료 큐에 넣기
				m_pComQueue->Enqueue(pWork);

			}
				break;


				// 없는 타입이면 크래시
			default:
				gThis->m_Dump->Crash();
				break;
			}
		}

		return 0;
	}

	// DB_Write 스레드
	UINT WINAPI shDB_Communicate::DB_WriteThread(LPVOID lParam)
	{
		shDB_Communicate* gThis = (shDB_Communicate*)lParam;

		// --------------------
		// 로컬로 받아두기
		// --------------------

		// 나한테 날라온 일감 큐
		CNormalQueue<DB_WORK*> *pWorkerQueue = gThis->m_pDB_Wirte_Start_Queue;

		// 완료된 작업 저장 큐
		CNormalQueue<DB_WORK*> *pEndQueue = gThis->m_pDB_Wirte_End_Queue;


		// --------------
		// 변수 선언
		// --------------

		// 일 시키기용, 종료용 이벤트 받아두기
		// [종료 신호, 일하기 신호] 순서대로
		HANDLE hEvent[2] = { gThis->m_hDBWrite_Exit_Event , gThis->m_hDBWrite_Event };

		DB_WORK* pWork;

		HTTP_Exchange m_HTTP_Post((TCHAR*)_T("10.0.0.1"), 80);
		
		while (1)
		{
			// 이벤트 대기
			DWORD Check = WaitForMultipleObjects(2, hEvent, FALSE, INFINITE);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("DB_WriteThread Exit Error!!! (%d) \n", Error);
				
				gThis->m_Dump->Crash();
			}

			// 종료 신호가 왔다면업데이트 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;


			// ----------------- 일감 있으면 일한다.
			// 일감 수를 스냅샷으로 받아둔다.
			// 해당 일감만큼만 처리하고 자러간다.
			// 처리 못한 일감은, 다음에 깨서 처리한다.
			int Size = pWorkerQueue->GetNodeSize();

			while (Size > 0)
			{
				Size--;

				// 1. 큐에서 일감 1개 빼오기	
				if (pWorkerQueue->Dequeue(pWork) == -1)
					gThis->m_Dump->Crash();

				DB_WORK_CONTENT_UPDATE* NowWork = (DB_WORK_CONTENT_UPDATE*)pWork;

				// 2. 일감 타입에 따라 로직 처리
				switch (NowWork->m_wWorkType)
				{
					// 플레이 카운트 저장
				case eu_DB_AFTER_TYPE::eu_PLAYCOUNT_UPDATE:
				{
					// DB에 쿼리 날림
					TCHAR Body[1000];

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));
					ZeroMemory(Body, sizeof(Body));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"playcount\" : \"%d\"}",	NowWork->AccountNo, NowWork->m_iCount);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();
					}

					// 3. Write 완료 큐에 넣기
					pEndQueue->Enqueue(pWork);
				}
					break;

					// 플레이 시간 저장
				case eu_DB_AFTER_TYPE::eu_PLAYTIME_UPDATE:
				{
					// DB에 쿼리 날림
					TCHAR Body[1000];

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));
					ZeroMemory(Body, sizeof(Body));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"playtime\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();
					}

					// 3. Write 완료 큐에 넣기
					pEndQueue->Enqueue(pWork);
				}
					break;

					// 킬 카운트 저장
				case eu_DB_AFTER_TYPE::eu_KILL_UPDATE:
				{
					// DB에 쿼리 날림
					TCHAR Body[1000];

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));
					ZeroMemory(Body, sizeof(Body));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"kill\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();
					}

					// 3. Write 완료 큐에 넣기
					pEndQueue->Enqueue(pWork);

				}
					break;

					// 사망 카운트 저장
				case eu_DB_AFTER_TYPE::eu_DIE_UPDATE:
				{
					// DB에 쿼리 날림
					TCHAR Body[1000];

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));
					ZeroMemory(Body, sizeof(Body));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"die\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();
					}

					// 3. Write 완료 큐에 넣기
					pEndQueue->Enqueue(pWork);

				}
					break;

					// 승리 카운트 저장
				case eu_DB_AFTER_TYPE::eu_WIN_UPDATE:
				{
					// DB에 쿼리 날림
					TCHAR Body[1000];

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));
					ZeroMemory(Body, sizeof(Body));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"win\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("Contents/Update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();
					}

					// 3. Write 완료 큐에 넣기
					pEndQueue->Enqueue(pWork);
				}
					break;

				default:
					gThis->m_Dump->Crash();
				}			
				
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

	// DB에 Write 할 것이 있을 때 호출되는 함수.
	// 인자로 받은 구조체의 정보를 확인해 로직 처리
	//
	// Parameter : DB_WORK*
	// return : 없음
	void shDB_Communicate::DBWriteFunc(DB_WORK* Protocol)
	{
		// Wirte용 스레드에게 일감 던짐 (Normal Q 사용)
		m_pDB_Wirte_Start_Queue->Enqueue(Protocol);

		// Write 스레드 깨우기
		SetEvent(m_hDBWrite_Event);
	}


	// ---------------
	// 생성자와 소멸자
	// ---------------

	// 생성자
	shDB_Communicate::shDB_Communicate()
	{

		// DB Write용 스레드용 일시키기 용이벤트 만들기
		// 자동 리셋 이벤트.
		m_hDBWrite_Event = CreateEvent(NULL, FALSE, FALSE, NULL);

		// DB Write용 스레드 종료용 이벤트 만들기
		// 자동 리셋 이벤트.
		m_hDBWrite_Exit_Event = CreateEvent(NULL, FALSE, FALSE, NULL);

		// 덤프 받기
		m_Dump = CCrashDump::GetInstance();

		// DB_WORK 메모리풀 동적할당
		m_pDB_Work_Pool = new CMemoryPoolTLS<DB_WORK>(0, false);
		
		// Read 완료 락프리 큐 동적할당
		m_pDB_ReadComplete_Queue = new CLF_Queue<DB_WORK*>(0);
		
		// Write 완료 노멀 큐 동적할당.
		m_pDB_Wirte_End_Queue = new CNormalQueue<DB_WORK*>();

		// Write 스레드에게 일시키기용 큐 동적할당.
		m_pDB_Wirte_Start_Queue = new CNormalQueue<DB_WORK*>();

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

		// DB_Write용 스레드
		m_hDB_Write_Thread = (HANDLE)_beginthreadex(0, 0, DB_WriteThread, this, 0, 0);
		if (m_hDB_Write_Thread == NULL)
			m_Dump->Crash();
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

			// 반환
			m_pDB_Work_Pool->Free(Delete);
		}

		// 4. DB_WriteThread 종료
		SetEvent(m_hDBWrite_Exit_Event);

		WaitForSingleObject(m_hDB_Write_Thread, INFINITE);

		// 5. Write 일시키기 용 큐 비우기
		while (1)
		{
			// 꺼내기
			if (m_pDB_Wirte_Start_Queue->Dequeue(Delete) == -1)
				break;

			// 반환
			m_pDB_Work_Pool->Free(Delete);
		}

		// 6. Write 일 완료용 큐 비우기
		while (1)
		{
			// 꺼내기
			if (m_pDB_Wirte_End_Queue->Dequeue(Delete) == -1)
				break;

			// 반환
			m_pDB_Work_Pool->Free(Delete);
		}


		// 7. 각종 동적해제

		// 워커 스레드 핸들 동적해제
		delete[] m_hDB_Read_Thread;

		// DB_WORK 메모리풀 동적해제
		delete m_pDB_Work_Pool;

		// Read 완료 락프리 큐 동적해제
		delete m_pDB_ReadComplete_Queue;

		// Write 완료 노멀 큐 동적해제
		delete m_pDB_Wirte_End_Queue;

		// Write 스레드에게 일시키기용 큐 동적해제
		delete m_pDB_Wirte_Start_Queue;
	}
}