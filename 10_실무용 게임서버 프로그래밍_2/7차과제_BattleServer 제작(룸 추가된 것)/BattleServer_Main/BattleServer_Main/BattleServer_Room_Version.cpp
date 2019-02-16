#include "pch.h"
#include "BattleServer_Room_Version.h"
#include "Parser\Parser_Class.h"
#include "Protocol_Set\CommonProtocol_2.h"		// 차후 일반으로 변경 필요
#include "Protocol_Set/ContentsData.h"
#include "Log\Log.h"
#include "CPUUsage\CPUUsage.h"
#include "PDHClass\PDHCheck.h"

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

#include <process.h>
#include <strsafe.h>
#include <cmath>
#include <algorithm>
#include <list>

using namespace rapidjson;
using namespace std;

// -----------------------
// shDB와 통신하는 클래스
// -----------------------
namespace Library_Jingyu
{
	// 덤프용
	CCrashDump* g_BattleServer_Room_Dump = CCrashDump::GetInstance();

	// 로그용
	CSystemLog* g_BattleServer_RoomLog = CSystemLog::GetInstance();

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
		HTTP_Exchange m_HTTP_Post((TCHAR*)_T("10.10.10.1"), 80);

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
				TCHAR Body[1000] = { 0, };

				ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));

				// 1. Body 만들기
				swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld}", NowWork->AccountNo);

				// 2. http 통신 및 결과 얻기
				int TryCount = 5;
				while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("select_account.php"), Body, NowWork->m_tcResponse) == false)
				{
					TryCount--;

					if (TryCount == 0)
						gThis->m_Dump->Crash();

					Sleep(100);
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
				TCHAR Body[1000] = { 0, };

				ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));

				// 1. Body 만들기
				swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld}", NowWork->AccountNo);

				// 2. http 통신 및 결과 얻기
				int TryCount = 5;
				while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("select_contents.php"), Body, NowWork->m_tcResponse) == false)
				{
					TryCount--;

					if (TryCount == 0)
						gThis->m_Dump->Crash();

					Sleep(100);
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

		// DB_WORK를 관리하는 메모리풀
		CMemoryPoolTLS<DB_WORK>* pDBWorkPool = gThis->m_pDB_Work_Pool;


		// --------------
		// 변수 선언
		// --------------

		// 일 시키기용, 종료용 이벤트 받아두기
		// [종료 신호, 일하기 신호] 순서대로
		HANDLE hEvent[2] = { gThis->m_hDBWrite_Exit_Event , gThis->m_hDBWrite_Event };

		DB_WORK* pWork;

		HTTP_Exchange m_HTTP_Post((TCHAR*)_T("10.10.10.1"), 80);

		// 출력 체크용
		LONG* TempDBWriteTPS = &gThis->m_lDBWriteTPS;

		// Write카운트를 깍을 용도의 AccountNo 보관용
		INT64 TempAccountNo;

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

			// 1. 큐에서 일감 1개 빼오기	
			while (pWorkerQueue->Dequeue(pWork) != -1)
			{
				// 2. 일감 타입에 따라 로직 처리
				switch (pWork->m_wWorkType)
				{
					// 플레이 카운트 저장
				case eu_DB_AFTER_TYPE::eu_PLAYCOUNT_UPDATE:
				{
					DB_WORK_CONTENT_UPDATE* NowWork = (DB_WORK_CONTENT_UPDATE*)pWork;

					// DB에 쿼리 날림
					TCHAR Body[1000] = { 0, };

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"playcount\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();

						Sleep(100);
					}

					// 3. Json데이터 파싱하기 (UTF-16)
					GenericDocument<UTF16<>> Doc;
					Doc.Parse(NowWork->m_tcResponse);

					int iResult = Doc[_T("result")].GetInt();

					// 4. DB 요청 결과 확인
					// 결과가 1이 아니라면 Crash.
					// Write는 무조건 성공한다는 가정
					if (iResult != 1)
						g_BattleServer_Room_Dump->Crash();

					// 5. AccountNo 받아두기.
					// 아래에서 감소시킨다.
					TempAccountNo = NowWork->AccountNo;
				}
				break;

				// 킬 카운트 저장
				case eu_DB_AFTER_TYPE::eu_KILL_UPDATE:
				{
					DB_WORK_CONTENT_UPDATE* NowWork = (DB_WORK_CONTENT_UPDATE*)pWork;

					// DB에 쿼리 날림
					TCHAR Body[1000] = { 0, };

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"kill\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();

						Sleep(100);
					}

					// 3. Json데이터 파싱하기 (UTF-16)
					GenericDocument<UTF16<>> Doc;
					Doc.Parse(NowWork->m_tcResponse);

					int iResult = Doc[_T("result")].GetInt();

					// 4. DB 요청 결과 확인
					// 결과가 1이 아니라면 Crash.
					// Write는 무조건 성공한다는 가정
					if (iResult != 1)
						g_BattleServer_Room_Dump->Crash();

					// 5. AccountNo 받아두기.
					// 아래에서 감소시킨다.
					TempAccountNo = NowWork->AccountNo;

				}
				break;

				// 사망 카운트, 플레이 타임 저장
				case eu_DB_AFTER_TYPE::eu_DIE_UPDATE:
				{
					DB_WORK_CONTENT_UPDATE_2* NowWork = (DB_WORK_CONTENT_UPDATE_2*)pWork;

					// DB에 쿼리 날림
					TCHAR Body[1000] = { 0, };

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"die\" : \"%d\", \"playtime\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount1, NowWork->m_iCount2);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();

						Sleep(100);
					}

					// 3. Json데이터 파싱하기 (UTF-16)
					GenericDocument<UTF16<>> Doc;
					Doc.Parse(NowWork->m_tcResponse);

					int iResult = Doc[_T("result")].GetInt();

					// 4. DB 요청 결과 확인
					// 결과가 1이 아니라면 Crash.
					// Write는 무조건 성공한다는 가정
					if (iResult != 1)
						g_BattleServer_Room_Dump->Crash();

					// 5. AccountNo 받아두기.
					// 아래에서 감소시킨다.
					TempAccountNo = NowWork->AccountNo;

				}
				break;

				// 승리 카운트, 플레이타임 저장
				case eu_DB_AFTER_TYPE::eu_WIN_UPDATE:
				{
					DB_WORK_CONTENT_UPDATE_2* NowWork = (DB_WORK_CONTENT_UPDATE_2*)pWork;

					// DB에 쿼리 날림
					TCHAR Body[1000] = { 0, };

					ZeroMemory(NowWork->m_tcResponse, sizeof(NowWork->m_tcResponse));

					// 1. Body 만들기
					swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"win\" : \"%d\", \"playtime\" : \"%d\"}", NowWork->AccountNo, NowWork->m_iCount1, NowWork->m_iCount2);

					// 2. HTTP 통신
					int TryCount = 5;
					while (m_HTTP_Post.HTTP_ReqANDRes((TCHAR*)_T("update_contents.php"), Body, NowWork->m_tcResponse) == false)
					{
						TryCount--;

						if (TryCount == 0)
							gThis->m_Dump->Crash();

						Sleep(100);
					}

					// 3. Json데이터 파싱하기 (UTF-16)
					GenericDocument<UTF16<>> Doc;
					Doc.Parse(NowWork->m_tcResponse);

					int iResult = Doc[_T("result")].GetInt();

					// 4. DB 요청 결과 확인
					// 결과가 1이 아니라면 Crash.
					// Write는 무조건 성공한다는 가정
					if (iResult != 1)
						g_BattleServer_Room_Dump->Crash();

					// 5. AccountNo 받아두기.
					// 아래에서 감소시킨다.
					TempAccountNo = NowWork->AccountNo;
				}
				break;

				default:
					gThis->m_Dump->Crash();
				}						


				// 3. DBWrite TPS 증가
				InterlockedIncrement(TempDBWriteTPS);

				// 4. DBWrite 카운트1 감소
				//gThis->m_pBattleServer->MinDBWriteCountFunc(TempAccountNo);

				// 5. DB_WORK 반환
				pDBWorkPool->Free(pWork);

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

		InterlockedIncrement(&m_lDBWriteCountTPS);
	}

	// Battle서버 this를 받아둔다.
	void shDB_Communicate::ParentSet(CBattleServer_Room* Parent)
	{
		m_pBattleServer = Parent;
	}




	// ---------------
	// 생성자와 소멸자
	// ---------------

	// 생성자
	shDB_Communicate::shDB_Communicate()
	{
		m_lDBWriteTPS = 0;
		m_lDBWriteCountTPS = 0;

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

		// Write 스레드에게 일시키기용 큐 동적할당.
		m_pDB_Wirte_Start_Queue = new CNormalQueue<DB_WORK*>();

		// DB_Read용 입출력 완료포트 생성
		// 30개의 스레드 생성, 2개의 스레드 활성화
		int Create = 30;
		int Active = 2;

		m_hDB_Read = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, Active);
		if (m_hDB_Read == NULL)
			m_Dump->Crash();

		// DB_Read용 워커 스레드 생성
		m_hDB_Read_Thread = new HANDLE[Create];
		for (int i = 0; i < Create; ++i)
		{
			m_hDB_Read_Thread[i] = (HANDLE)_beginthreadex(0, 0, DB_ReadThread, this, 0, 0);
			if (m_hDB_Read_Thread[i] == NULL)
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
		for (int i = 0; i < 10; ++i)
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


		// 6. 각종 동적해제

		// 워커 스레드 핸들 동적해제
		delete[] m_hDB_Read_Thread;

		// DB_WORK 메모리풀 동적해제
		delete m_pDB_Work_Pool;

		// Read 완료 락프리 큐 동적해제
		delete m_pDB_ReadComplete_Queue;

		// Write 스레드에게 일시키기용 큐 동적해제
		delete m_pDB_Wirte_Start_Queue;
	}
}

// ------------------
// CGameSession의 함수
// (CBattleServer_Room의 이너클래스)
// ------------------
namespace Library_Jingyu
{
	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CBattleServer_Room::CGameSession::CGameSession()
		:CMMOServer::cSession()
	{
		// ClientKey 초기값 셋팅
		m_int64ClientKey = -1;

		// 자료구조에 들어감 플래그
		m_bStructFlag = false;

		// 로그인 패킷 완료 처리
		m_bLoginFlag = false;

		// 곧 종료될 유저 체크 플래그
		m_bLogoutFlag = false;

		// 입장중인 방 초기 번호
		m_iRoomNo = -1;

		// 로그인 HTTP 요청 횟수
		m_lLoginHTTPCount = 0;

		// 생존 상태
		m_bAliveFlag = false;	

		// 마지막 전적 저장 상태 확인
		m_bLastDBWriteFlag = false;

		// 최초는 로그아웃 상태
		m_euModeType = eu_PLATER_MODE::LOG_OUT;

		m_euDebugMode = eu_USER_TYPE_DEBUG::LOGOUT;
	}

	// 소멸자
	CBattleServer_Room::CGameSession::~CGameSession()
	{
		// 할거 없음
	}



	// -----------------------
	// 외부에서 호출 가능한 함수
	// (게터)
	// -----------------------

	// 유저 생존 여부
	//
	// Parameter : 없음
	// return : 생존 상태일 시 true, 사망 상태일 시 false
	bool CBattleServer_Room::CGameSession::GetAliveState()
	{
		return m_bAliveFlag;
	}


	// -----------------------
	// 외부에서 호출 가능한 함수
	// (세터)
	// -----------------------	

	// 캐릭터 생성 시 셋팅되는 값
	//
	// Parameter : 생성될 X, Y좌표, 캐릭터 생성된 시간(DWORD)
	// return : 없음 
	void CBattleServer_Room::CGameSession::StartSet(float PosX, float PosY, DWORD NowTime)
	{
		// X,Y 좌표
		m_fPosX = PosX;
		m_fPosY = PosY;

		// 첫 HP
		m_iHP = g_Data_HP;

		// 총알 수
		m_iBullet = g_Data_Cartridge_Bullet;

		// 탄창 수. 최초는 0
		m_iCartridge = 0;

		// 헬멧 수. 최초는 0
		m_iHelmetCount = 0;

		// 게임 시작 시간.
		// 밀리 세컨드로 보관.
		m_dwGameStartTime = NowTime;
	}

	// 생존 상태 변경
	//
	// Parameter : 변경할 생존 상태(true면 생존, false면 사망)
	// return : 없음
	void CBattleServer_Room::CGameSession::AliveSet(bool Flag)
	{
		m_bAliveFlag = Flag;
	}

	// 유저 데미지 처리
	// !! HP가 0이 되면 자동으로 사망상태가 된다 !!
	//
	// Parameter : 유저가 입은 데미지
	// return : 감소 후 남은 HP
	int CBattleServer_Room::CGameSession::Damage(int Damage)
	{
		// 유저의 HP감소
		int HP = m_iHP - Damage;
		if (HP < 0)
			HP = 0;

		m_iHP = HP;

		// 감소 후 데미지가 0이라면 유저 사망한 것
		if (HP == 0)
			m_bAliveFlag = false;		

		// 남은 HP 리턴
		return HP;
	}




	// -----------------------
	// 전적 셋팅
	// -----------------------	

	// 유저의 승리 카운트 1 증가
	// !! 승리 카운트와 플레이 시간을 같이 갱신 !!
	// !! 내부에서는 승리카운트 증가, 플레이타임 갱신 후, DB에 저장까지 한다. !!
	// 
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::Record_Win_Add()
	{
		// 전적 카운트 중, 승리 카운트 증가
		++m_iRecord_Win;

		// 승리 카운트 DB에 저장.
		// 승리자는, 아직 플레이 타임 갱신 안함. 여기서 갱신
		m_iRecord_PlayTime = m_iRecord_PlayTime + ((timeGetTime() - m_dwGameStartTime) / 1000);

		DB_WORK_CONTENT_UPDATE_2* WriteWork = (DB_WORK_CONTENT_UPDATE_2*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();

		WriteWork->m_wWorkType = eu_DB_AFTER_TYPE::eu_WIN_UPDATE;
		WriteWork->m_iCount1 = m_iRecord_Win;
		WriteWork->m_iCount2 = m_iRecord_PlayTime;

		WriteWork->AccountNo = m_Int64AccountNo;

		// Write 하기 전에, DBWrite카운트 올려야함.
		//m_pParent->AddDBWriteCountFunc(m_Int64AccountNo);

		// DBWrite 시도
		m_pParent->m_shDB_Communicate.DBWriteFunc((DB_WORK*)WriteWork);
	}
	
	// 유저의 사망 카운트 1 증가
	// !! 사망 카운트와 플레이 시간을 같이 갱신 !!
	// !! 내부에서는 사망 카운트 증가, 플레이타임 갱신 후, DB에 저장까지 한다. !!
	// 
	// Parameter : 없음
	// return : 없음
	void  CBattleServer_Room::CGameSession::Recored_Die_Add()
	{
		// Die카운트 증가.
		++m_iRecord_Die;

		// 플레이 타임 갱신
		m_iRecord_PlayTime = m_iRecord_PlayTime + ((timeGetTime() - m_dwGameStartTime) / 1000);
		
		// DBWrite 구조체 셋팅 (Die카운트 + 플레이 타임)
		DB_WORK_CONTENT_UPDATE_2* DieWrite = (DB_WORK_CONTENT_UPDATE_2*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();

		DieWrite->m_wWorkType = eu_DB_AFTER_TYPE::eu_DIE_UPDATE;
		DieWrite->m_iCount1 = m_iRecord_Die;
		DieWrite->m_iCount2 = m_iRecord_PlayTime;

		DieWrite->AccountNo = m_Int64AccountNo;

		// 요청하기
		//m_pParent->AddDBWriteCountFunc(m_Int64AccountNo);
		m_pParent->m_shDB_Communicate.DBWriteFunc((DB_WORK*)DieWrite);
	}

	// 유저의 킬 카운트 1 증가
	// !! 내부에서 킬 카운트 증가 후, DB에 저장까지 한다 !!
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::Record_Kill_Add()
	{
		// 킬 카운트 증가
		++m_iRecord_Kill;

		// DBWrite 구조체 셋팅
		DB_WORK_CONTENT_UPDATE* KillWrite = (DB_WORK_CONTENT_UPDATE*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();

		KillWrite->m_wWorkType = eu_DB_AFTER_TYPE::eu_KILL_UPDATE;
		KillWrite->m_iCount = m_iRecord_Kill;

		KillWrite->AccountNo = m_Int64AccountNo;

		// 요청하기
		//m_pParent->AddDBWriteCountFunc(m_Int64AccountNo);
		m_pParent->m_shDB_Communicate.DBWriteFunc((DB_WORK*)KillWrite);
	}

	// 유저의 플레이 횟수 1 증가
	// !! 내부에서 플레이 횟수 증가 후, DB에 저장까지 한다 !!
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::Record_PlayCount_Add()
	{
		// 전적 중, 플레이 횟수 증가. 그리고 DB에 저장
		++m_iRecord_PlayCount;

		DB_WORK_CONTENT_UPDATE* CountWrite = (DB_WORK_CONTENT_UPDATE*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();

		CountWrite->m_wWorkType = eu_DB_AFTER_TYPE::eu_PLAYCOUNT_UPDATE;
		CountWrite->m_iCount = m_iRecord_PlayCount;

		CountWrite->AccountNo = m_Int64AccountNo;

		// Write 하기 전에, WriteCount 증가.
		//m_pParent->AddDBWriteCountFunc(m_Int64AccountNo);

		// DBWrite
		m_pParent->m_shDB_Communicate.DBWriteFunc((DB_WORK*)CountWrite);
	}




	// -----------------
	// 가상함수
	// -----------------

	// --------------- AUTH 모드용 함수

	// 유저가 Auth 모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_ClientJoin()
	{
		// 컨텐츠 레벨의 모드 변경
		if (m_euModeType != LOG_OUT)
			g_BattleServer_Room_Dump->Crash();

		m_euModeType = eu_PLATER_MODE::AUTH;

		m_euDebugMode = eu_USER_TYPE_DEBUG::CONNECT;
	}

	// 유저가 Auth 모드에서 나감
	//
	// Parameter : Game모드로 변경된것인지 알려주는 Flag. 디폴트 false(변경되지 않음)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_ClientLeave(bool bGame)
	{
		// 모드 변경일 경우
		if (bGame == true)
		{
			// 모드 변경
			if (m_euModeType != eu_PLATER_MODE::AUTH)
				g_BattleServer_Room_Dump->Crash();

			m_euModeType = eu_PLATER_MODE::GAME;

			m_euDebugMode = eu_USER_TYPE_DEBUG::AUTO_TO_GAME;
		}

		// 실제 게임 종료일 경우
		else
		{
			// 모드 변경
			if (m_euModeType != eu_PLATER_MODE::AUTH)
				g_BattleServer_Room_Dump->Crash();

			m_euModeType = eu_PLATER_MODE::LOG_OUT;
			m_euDebugMode = eu_USER_TYPE_DEBUG::LOGOUT;

			// ClientKey 초기값으로 셋팅.
			// !! 이걸 안하면, Release되는 중에 HTTP응답이 올 경우 !!
			// !! Auth_Update에서 이미 샌드큐가 모두 정리된 유저에게 또 SendPacket 가능성 !!
			// !! 이 경우, 전혀 엉뚱한 유저에게 Send가 될 수 있음 !!
			UINT64 TempClientKey = m_int64ClientKey;
			m_int64ClientKey = -1;

			// 해당 유저가 있는 룸 안에서 유저 제거
			// -1이 아니면 룸에 있는것.
			if (m_iRoomNo != -1)
			{
				AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 락 

				// 1. 룸 검색
				auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

				// 룸이 없으면 Crash
				if (FindRoom == m_pParent->m_Room_Umap.end())
					g_BattleServer_Room_Dump->Crash();

				stRoom* NowRoom = FindRoom->second;

				// 룸 상태가 Play면 Crash. 말도 안됨
				// Auth모드에서 종료되는 유저가 있는 방은, 무조건 대기/준비 방이어야 함				
				if (NowRoom->m_iRoomState == eu_ROOM_STATE::PLAY_ROOM)
					g_BattleServer_Room_Dump->Crash();	

				// 여기까지 오면 Wait 혹은 Ready상태의 방.
				// Auth 스레드만 접근 가능. 락 풀어도 안전
				ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 
				


				// 2. 현재 룸 안에 있는 유저 수 감소
				--NowRoom->m_iJoinUserCount;

				// 감소 후, 룸 안의 유저 수가 0보다 적으면 말도 안됨.
				if (NowRoom->m_iJoinUserCount < 0)				
					g_BattleServer_Room_Dump->Crash();


				// 3. 룸 안의 자료구조에서 유저 제거
				if (NowRoom->Erase(this) == false)
					g_BattleServer_Room_Dump->Crash();			


				// 4. 방 안의 모든 유저에게, 방에서 유저가 나갔다고 알려줌
				// BroadCast해도 된다. 어차피 방 안에 지금 나간 유저는 없음.
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_GAME_RES_REMOVE_USER;

				SendBuff->PutData((char*)&Type, 2);
				SendBuff->PutData((char*)&m_iRoomNo, 4);
				SendBuff->PutData((char*)&m_Int64AccountNo, 8);

				// 여기서, false가 리턴될 수 있음. (방 안의 자료구조에 유저가 0명)
				// 대기방인 상태에서 모든 유저가 나갈 수 있기 때문에 문제로 안본다.
				// 때문에 return 안받음
				NowRoom->SendPacket_BroadCast(SendBuff);


				// 5. 마스터 서버에게 해당 유저 나갔다고 패킷 보내줘야 함.
				// Ready상태인 유저는, 이미 마스터에서 사라졌기 때문에 보내면 안됨.
				if(NowRoom->m_iRoomState == eu_ROOM_STATE::WAIT_ROOM)
					m_pParent->m_Master_LanClient->Packet_RoomLeave_Req(m_iRoomNo, TempClientKey);

				// 6. 룸 키 -1로 초기화
				m_iRoomNo = -1;
			}			

			// 7. m_bStructFlag가 true라면, 자료구조에 들어간 유저
			// 자료구조에서 제거한다.
			if (m_bStructFlag == true)
			{
				if (m_pParent->EraseAccountNoFunc(m_Int64AccountNo) == false)
					g_BattleServer_Room_Dump->Crash();

				// DBWrite에서 제거 시도.
				// m_bStructFlag가 true라면, DBWrite 횟수 자료구조에 없을 수가 없음.
				//if (m_pParent->MinDBWriteCountFunc(m_Int64AccountNo) == false)
					//g_BattleServer_Room_Dump->Crash();

				m_bStructFlag = false;
			}
		}

	}

	// Auth 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_Packet(CProtocolBuff_Net* Packet)
	{
		// 패킷 처리
		try
		{
			// 1. 타입 빼기
			WORD Type;
			Packet->GetData((char*)&Type, 2);

			// 2. 타입에 따라 분기 처리
			switch (Type)
			{
				// 로그인 요청
			case en_PACKET_CS_GAME_REQ_LOGIN:
				Auth_LoginPacket(Packet);
				break;

				// 방 입장 요청
			case en_PACKET_CS_GAME_REQ_ENTER_ROOM:
				Auth_RoomEnterPacket(Packet);
				break;

				// 하트비트
			case en_PACKET_CS_GAME_REQ_HEARTBEAT:
				break;

				// 그 외에는 문제있음.
			default:
				throw CException(_T("OnAuth_Packet() --> Type Error!!"));
				break;
			}
		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// 접속 끊기 요청
			Disconnect();
		}
	}

	// Auth 모드의 유저가 하트비트로 끊김
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnAuth_HeartBeat(DWORD DelayTime)
	{
		// 에러 로그 찍기
		// 로그 찍기 (로그 레벨 : 에러)
		g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
			L"Auth HeartBeat!! AccoutnNo : %lld, Time : %d, State : %d", m_Int64AccountNo, DelayTime, m_euDebugMode);
	}




	// --------------- GAME 모드용 함수

	// 유저가 Game모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientJoin()
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		// 1. 유저가 있는 방 알아오기
		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 말도 안됨.
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 방 모드가 Play가 아니면 말도 안됨.
		if(NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		// 락 푼다
		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락		

		// 2. 게임 모드로 전환된 유저 카운트 증가.

		// 증가 전, 이미 JoinUser와 같다면 말도 안됨. 한 명이 방에 더 들어온것. 크래시
		if(NowRoom->m_iGameModeUser == NowRoom->m_iJoinUserCount)
			g_BattleServer_Room_Dump->Crash();

		++NowRoom->m_iGameModeUser;

		m_euDebugMode = eu_USER_TYPE_DEBUG::INGAME;

		// 3. 증가 후, JoinUser와 같아진다면 방 내의 모든 유저에게 [내 캐릭터 생성] 과 [다른 유저 캐릭터 생성]을 보낸다.
		// OnGame_ClientJoin 마다 호출하는게 아니라, 모두 접속했는지 확인 후 보낸다. 동시에 캐릭터 생성을 하기 위해서.
		if (NowRoom->m_iGameModeUser == NowRoom->m_iJoinUserCount)
			NowRoom->CreateCharacter();


		// 플레이 횟수 전적 증가
		Record_PlayCount_Add();	
	}

	// 유저가 Game모드에서 나감
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientLeave()
	{
		// 모드 변경
		if (m_euModeType != eu_PLATER_MODE::GAME)
			g_BattleServer_Room_Dump->Crash();

		m_euModeType = eu_PLATER_MODE::LOG_OUT;
		m_euDebugMode = eu_USER_TYPE_DEBUG::LOGOUT;

		// ClientKey 초기값으로 셋팅.
		// !! 이걸 안하면, Release되는 중에 HTTP응답이 올 경우 !!
		// !! Game_Update에서 이미 샌드큐가 모두 정리된 유저에게 또 SendPacket 가능성 !!
		// !! 이 경우, 전혀 엉뚱한 유저에게 Send가 될 수 있음 !!
		m_int64ClientKey = -1;


		// 해당 유저가 있는 룸 안에서 유저 제거
		// -1인 것은 말도 안됨. 무조건 방에 있어야 함
		if(m_iRoomNo == -1)
			g_BattleServer_Room_Dump->Crash();


		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 락 

		// 1. 룸 검색
		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 룸이 없으면 Crash
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 룸 상태가 Play가 아니면 Crash. 말도 안됨
		// Game모드에서 종료되는 유저가 있는 방은, 무조건 플레이 방이어야 함
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		// 여기까지 오면 방 모드가 PLAY_ROOM 확정.
		// 즉, Game에서만 접근하는 방 확정. 락 푼다.
		// 락 풀었는데 방이 삭제될 가능성은 전혀 없음. 
		// 방 삭제는 Game 스레드가 하고, 해당 함수도 Game 스레드에서 호출된다. 
		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room 자료구조 Shard 언락 


		// 2. 현재 룸 안에 있는 유저 수 감소
		--NowRoom->m_iJoinUserCount;		

		// 감소 후, 룸 안의 유저 수가 0보다 작으면 말도 안됨. Crash
		if (NowRoom->m_iJoinUserCount < 0)
			g_BattleServer_Room_Dump->Crash();


		// 3. 나가는 유저가 생존한 유저였을 경우, 생존한 유저 수, Game모드 유저 수 감소.
		// 생존한 유저 수로 승리여부를 판단해야 하기 때문에 
		// 유저가 게임에서 나가거나 HP가 0이되어 사망할 경우 감소시켜야 함
		if (m_bAliveFlag == true)
		{
			--NowRoom->m_iAliveUserCount;
			--NowRoom->m_iGameModeUser;
			
			// 감소 후, 0보다 적으면 말도 안됨.
			if (NowRoom->m_iAliveUserCount < 0 || NowRoom->m_iGameModeUser < 0)
				g_BattleServer_Room_Dump->Crash();

			m_bAliveFlag = false;
		}



		// 4. 룸 안의 자료구조에서 유저 제거
		if (NowRoom->Erase(this) == false)
			g_BattleServer_Room_Dump->Crash();



		// 5. 방 안의 모든 유저에게, 방에서 유저가 나갔다고 알려줌
		// BroadCast해도 된다. 어차피 방 안에 지금 나간 유저는 없음.
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_LEAVE_USER;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&m_iRoomNo, 4);
		SendBuff->PutData((char*)&m_Int64AccountNo, 8);

		// 여기서, false가 리턴될 수 있음. (방 안의 자료구조에 유저가 0명)
		// 대기방인 상태에서 모든 유저가 나갈 수 있기 때문에 문제로 안본다.
		// 때문에 return 안받음
		NowRoom->SendPacket_BroadCast(SendBuff);

		m_iRoomNo = -1;



		// 6. 플레이 타임 저장을 아직 안했다면, 게임이 정상종료되지 않았는데 나가는 유저.
		// 사망한 유저로 판단한다.
		if (m_bLastDBWriteFlag == false)
		{
			// 유저의 사망카운트 1 증가
			Recored_Die_Add();		
		}

		// 7. m_bStructFlag가 true라면, 자료구조에 들어간 유저
		// 자료구조에서 제거한다.
		if (m_bStructFlag == true)
		{
			if (m_pParent->EraseAccountNoFunc(m_Int64AccountNo) == false)
				g_BattleServer_Room_Dump->Crash();

			// DBWrite에서 제거 시도.
			// m_bStructFlag가 true라면, DBWrite 횟수 자료구조에 없을 수가 없음.
			//if (m_pParent->MinDBWriteCountFunc(m_Int64AccountNo) == false)
				//g_BattleServer_Room_Dump->Crash();

			m_bStructFlag = false;
		}
	}

	// Game 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_Packet(CProtocolBuff_Net* Packet)
	{
		// 타입에 따라 패킷 처리
		try
		{
			// 1. 타입 빼기
			WORD Type;
			Packet->GetData((char*)&Type, 2);

			// 2. 타입에 따라 분기문 처리
			switch (Type)
			{	
				// 캐릭터 이동
			case en_PACKET_CS_GAME_REQ_MOVE_PLAYER:
				Game_MovePacket(Packet);
				break;

				// Fire1 발사 (총 발사)
			case en_PACKET_CS_GAME_REQ_FIRE1:
				Game_Frie_1_Packet(Packet);
				break;

				// HitDamage
			case en_PACKET_CS_GAME_REQ_HIT_DAMAGE:
				Game_HitDamage_Packet(Packet);
				break;

				// Fire 2 공격 (발차기)
			case en_PACKET_CS_GAME_REQ_FIRE2:
				Game_Fire_2_Packet(Packet);
				break;

				// KickDamage
			case en_PACKET_CS_GAME_REQ_KICK_DAMAGE:
				Game_KickDamage_Packet(Packet);
				break;

				// 재장전
			case en_PACKET_CS_GAME_REQ_RELOAD:
				Game_Reload_Packet(Packet);
				break;

				// 메드킷 아이템 획득
			case en_PACKET_CS_GAME_REQ_MEDKIT_GET:
				Game_GetItem_Packet(Packet, MEDKIT);
				break;

				// 탄창 아이템 획득
			case en_PACKET_CS_GAME_REQ_CARTRIDGE_GET:
				Game_GetItem_Packet(Packet, CARTRIDGE);
				break;

				// 헬멧 아이템 획득
			case en_PACKET_CS_GAME_REQ_HELMET_GET:
				Game_GetItem_Packet(Packet, HELMET);
				break;				

				// 캐릭터 HitPoint 갱신
			case en_PACKET_CS_GAME_REQ_HIT_POINT:
				Game_HitPointPacket(Packet);
				break;

				// 하트비트
			case en_PACKET_CS_GAME_REQ_HEARTBEAT:
				break;

				// 이 외에는 문제 있음
			default:
				throw CException(_T("OnGame_Packet() --> Type Error!!"));
				break;
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			// 접속 끊기 요청
			Disconnect();
		}
	}

	// Game 모드의 유저가 하트비트로 끊김
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_HeartBeat(DWORD DelayTime)
	{
		// 에러 로그 찍기
		// 로그 찍기 (로그 레벨 : 에러)
		g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
			L"Game HeartBeat!! AccoutnNo : %lld, Time : %d, State : %d", m_Int64AccountNo, DelayTime, m_euDebugMode);
	}



	// --------------- Release 모드용 함수

	// Release된 유저.
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnGame_ClientRelease()
	{
		// 모드가 LOG_OUT이 아니면 크래스
		if (m_euModeType != eu_PLATER_MODE::LOG_OUT)
			g_BattleServer_Room_Dump->Crash();
		
		m_bLoginFlag = false;	
		m_bLogoutFlag = false;
		m_lLoginHTTPCount = 0;		
		m_bLastDBWriteFlag = false;		
	}

	// GQCS에서 121에러 발생 시 호출되는 함수
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::CGameSession::OnSemaphore()
	{
		// 에러 로그 찍기
		// 로그 찍기 (로그 레벨 : 에러)
		g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
			L"Semaphore!! AccoutnNo : %lld", m_Int64AccountNo);
	}



	// -----------------
	// Auth모드 패킷 처리 함수
	// -----------------

	// 로그인 요청 
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Auth_LoginPacket(CProtocolBuff_Net* Packet)
	{
		// 1. 현재 ClientKey가 초기값이 아니면 Crash
		if (m_int64ClientKey != -1)
			g_BattleServer_Room_Dump->Crash();

		// 2. AccountNo 마샬링
		INT64 AccountNo;
		Packet->GetData((char*)&AccountNo, 8);

		// 3. 아직 DB에 Write중인지
		/*
		if (m_pParent->InsertDBWriteCountFunc(AccountNo) == false)
		{
			InterlockedIncrement(&m_pParent->m_OverlapLoginCount_DB);

			// 중복 로그인 패킷 보내기
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 타입
			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			SendBuff->PutData((char*)&Type, 2);

			// AccountNo
			SendBuff->PutData((char*)&AccountNo, 8);

			// Result
			BYTE Result = 6;		// 중복 로그인
			SendBuff->PutData((char*)&Result, 1);

			// SendPacket. 보내고 끊기
			SendPacket(SendBuff, TRUE);

			// 중복 로그인 처리된 유저도 접속 종료 요청
			m_pParent->DisconnectAccountNoFunc(AccountNo);

			return;
		}
		*/
		
		// 4. 나머지 마샬링 후, 세션키, AccountNo, 클라이언트키를 멤버에 셋팅
		Packet->GetData(m_cSessionKey, 64);
		char ConnectToken[32];
		UINT Ver_Code;

		Packet->GetData(ConnectToken, 32);
		Packet->GetData((char*)&Ver_Code, 4);

		INT64 ClinetKey;
		Packet->GetData((char*)&ClinetKey, 8);

		m_Int64AccountNo = AccountNo;
		m_int64ClientKey = ClinetKey;


		// 5. 배틀서버 입장 토큰 비교

		// 락 걸고 확인.
		AcquireSRWLockShared(&m_pParent->m_ServerEnterToken_srwl);	// ----- shasred 락

		// "현재" 토큰과 먼저 비교
		if (memcmp(ConnectToken, m_pParent->m_cConnectToken_Now, 32) != 0)
		{
			// 다르다면 "이전" 토큰과 비교
			if (memcmp(ConnectToken, m_pParent->m_cConnectToken_Before, 32) != 0)
			{
				ReleaseSRWLockShared(&m_pParent->m_ServerEnterToken_srwl);	// ----- shasred 언락

				// 에러 로그 찍기
				// 로그 찍기 (로그 레벨 : 에러)
				g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
					L"Auth_LoginPacket_AUTH()--> BattleEnterToken Error!! AccoutnNo : %lld", m_Int64AccountNo);

				InterlockedIncrement(&m_pParent->m_lBattleEnterTokenError);

				// 그래도 다르다면 이상한 유저로 판단.
				// 배틀서버 입장 토큰이 다르다는 패킷 보낸다.
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
				BYTE Result = 3;	// 현재는 세션키 오류랑 같이 취급. 차후 수정될까?

				SendBuff->PutData((char*)&Type, 2);
				SendBuff->PutData((char*)&AccountNo, 8);
				SendBuff->PutData((char*)&Result, 1);

				// SendPacket
				SendPacket(SendBuff);

				return;
			}
		}
		
		ReleaseSRWLockShared(&m_pParent->m_ServerEnterToken_srwl);	// ----- shasred 언락

		// 6. 버전 비교 
		if (m_pParent->m_uiVer_Code != Ver_Code)
		{
			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_AUTH()--> VerCode Error!! AccoutnNo : %lld, Code : %d", m_Int64AccountNo, Ver_Code);

			// 버전이 다를경우 Result 5(버전 오류)를 보낸다.
			InterlockedIncrement(&m_pParent->m_lVerError);

			WORD Type = en_PACKET_CS_MATCH_RES_LOGIN;
			BYTE Result = 5;

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			// SendPacket
			SendPacket(SendBuff);
			return;
		}
		
		// 7. AccountNo 자료구조에 추가.	
		// 이미 있으면(false 리턴) 중복 로그인으로 처리
		// 현재 유저에게는 실패 패킷, 접속 중인 유저는 DIsconnect.
		if (m_pParent->InsertAccountNoFunc(AccountNo, this) == false)
		{	
			InterlockedIncrement(&m_pParent->m_OverlapLoginCount);

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_AUTH()--> Overlapped Login!! AccoutnNo : %lld", m_Int64AccountNo);

			// 중복 로그인 패킷 보내기
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 타입
			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			SendBuff->PutData((char*)&Type, 2);

			// AccountNo
			SendBuff->PutData((char*)&AccountNo, 8);

			// Result
			BYTE Result = 6;		// 중복 로그인
			SendBuff->PutData((char*)&Result, 1);				

			// SendPacket. 보내고 끊기
			SendPacket(SendBuff, TRUE);

			// 중복 로그인 처리된 유저도 접속 종료 요청
			m_pParent->DisconnectAccountNoFunc(AccountNo);

			return;
		}

		// 자료구조에 들어감 플래그 변경
		m_bStructFlag = true;

		m_euDebugMode = eu_USER_TYPE_DEBUG::LOGIN_SEND;

		// 8. 로그인 인증처리를 위한 HTTP 통신
		// 통신 후, 해당 패킷에 대한 후처리는 Auth_Update에게 넘긴다.
		DB_WORK_LOGIN* Send_A = (DB_WORK_LOGIN*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();

		Send_A->m_wWorkType = eu_DB_AFTER_TYPE::eu_LOGIN_AUTH;
		Send_A->m_i64UniqueKey = ClinetKey;
		Send_A->pPointer = this;

		Send_A->AccountNo = AccountNo;

		// Select_Account.php 요청
		m_pParent->m_shDB_Communicate.DBReadFunc((DB_WORK*)Send_A, en_PHP_TYPE::SELECT_ACCOUNT);


		// 9. 유저 전적 정보를 위한 HTTP 통신
		// 통신 후, 해당 패킷에 대한 후처리는 Auth_Update에게 넘긴다.
		DB_WORK_LOGIN_CONTENTS* Send_B = (DB_WORK_LOGIN_CONTENTS*)m_pParent->m_shDB_Communicate.m_pDB_Work_Pool->Alloc();
	
		Send_B->m_wWorkType = eu_DB_AFTER_TYPE::eu_LOGIN_INFO;
		Send_B->m_i64UniqueKey = ClinetKey;
		Send_B->pPointer = this;

		Send_B->AccountNo = AccountNo;

		// Select_Contents.php 요청
		m_pParent->m_shDB_Communicate.DBReadFunc((DB_WORK*)Send_B, en_PHP_TYPE::SELECT_CONTENTS);
	}
	
	// 방 입장 요청
	// 
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Auth_RoomEnterPacket(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if(m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 1. 마샬링
		INT64 AccountNo;
		int RoomNo;
		char EnterToken[32];

		Packet->GetData((char*)&AccountNo, 8);
		Packet->GetData((char*)&RoomNo, 4);
		Packet->GetData(EnterToken, 32);		

		// 2. 에러 체크
		if (AccountNo != m_Int64AccountNo)
			g_BattleServer_Room_Dump->Crash();
		
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);	// ----- Room umap Shared 락

		
		// 3. 입장하고자 하는 방 검색
		auto ret = m_pParent->m_Room_Umap.find(RoomNo);

		// 4. 방 존재여부 체크
		if (ret == m_pParent->m_Room_Umap.end())
		{
			ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);	// ----- Room umap Shared 언락

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Enter Room --> Not Find Room !! AccoutnNo : %lld, RoomNo : %d", m_Int64AccountNo, RoomNo);

			// 방이 없으면, 에러 리턴
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 4;
			BYTE MaxUser = 0;	// 방 없음 패킷에는 의미가 없을듯.. 그냥 아무거나 보냄.

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}

		stRoom* NowRoom = ret->second;

		// Wait상태의 방은, Auth스레드에서만 접근.
		// 때문에, 여기까지 왔으면 Auth만 접근 확정.
		// 락 풀어도 안전하다.
		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);	// ----- Room umap Shared 언락

		// 5. 방이 대기방이 아닐 경우 에러 3 리턴
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::WAIT_ROOM)
		{
			// 에러 로그 찍기
		// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Enter Room --> Not Wait Room !! AccoutnNo : %lld, RoomNo : %d", m_Int64AccountNo, RoomNo);

			// 대기방이 아니면, 에러 리턴
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 3;
			BYTE MaxUser = 0;	// 대기방 아님 패킷에는 의미가 없을듯.. 그냥 아무거나 보냄.

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}			
		
		// 6. 방 인원수 체크
		if (NowRoom->m_iJoinUserCount == NowRoom->m_iMaxJoinCount)
		{
			BYTE MaxUser = NowRoom->m_iJoinUserCount;	

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Enter Room --> Full Room !! AccoutnNo : %lld, RoomNo : %d", m_Int64AccountNo, RoomNo);

			// 이미 최대 인원수라면, 에러 리턴			
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 5;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}

		// 7. 방 토큰 체크
		if (memcmp(EnterToken, NowRoom->m_cEnterToken, 32) != 0)
		{
			BYTE MaxUser = NowRoom->m_iJoinUserCount;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Enter Room --> Room Token Error !! AccoutnNo : %lld, RoomNo : %d", m_Int64AccountNo, RoomNo);

			InterlockedIncrement(&m_pParent->m_lRoomEnterTokenError);		

			// 토큰이 다르면, 에러 리턴
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
			BYTE Result = 2;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&MaxUser, 1);
			SendBuff->PutData((char*)&Result, 1);

			SendPacket(SendBuff);

			return;
		}


		// 8. 여기까지 오면 정상
		// 룸에 인원 추가
		++NowRoom->m_iJoinUserCount;
		NowRoom->Insert(this);

		// 플레이어에게 방 할당
		m_iRoomNo = NowRoom->m_iRoomNo;

		m_euDebugMode = eu_USER_TYPE_DEBUG::ROOMENTER;

		// 9. 정상적인 방 입장 응답 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_ENTER_ROOM;
		BYTE Result = 1;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&NowRoom->m_iMaxJoinCount, 1);
		SendBuff->PutData((char*)&Result, 1);

		SendPacket(SendBuff);


		// 10. 방에 있는 모든 유저에게 "유저 추가됨" 패킷 보내기
		NowRoom->Send_AddUser(this);	


		// 11. 룸 인원수가 풀 방이 되었을 경우
		if (NowRoom->m_iJoinUserCount == NowRoom->m_iMaxJoinCount)
		{
			// 1) 마스터에게 방 닫힘 패킷 보내기
			m_pParent->m_Master_LanClient->Packet_RoomClose_Req(RoomNo);

			// 2) 모든 유저에게 카운트다운 패킷 보내기
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_PLAY_READY;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&RoomNo, 4);
			SendBuff->PutData((char*)&m_pParent->m_stConst.m_bCountDownSec, 1);

			if (NowRoom->SendPacket_BroadCast(SendBuff) == false)
				g_BattleServer_Room_Dump->Crash();

			// 3) 방 카운트다운 변수 갱신
			// 이 시간 + 유저에게 보낸 카운트다운(10초)가 되면 방 모드를 Play로 변경시키고
			// 유저도 AUTH_TO_GAME으로 변경시킨다.
			NowRoom->m_dwCountDown = timeGetTime();

			InterlockedDecrement(&m_pParent->m_lNowWaitRoomCount);
			InterlockedIncrement(&m_pParent->m_lReadyRoomCount);

			// 4) 방의 상태를 Ready로 변경
			NowRoom->m_iRoomState = eu_ROOM_STATE::READY_ROOM;			
		}		
	}



	// -----------------
	// Game모드 패킷 처리 함수
	// -----------------

	// 유저가 이동할 시 보내는 패킷.
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_MovePacket(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저의 이동 요청이면 그냥 무시.
		if (m_bAliveFlag == false)
			return;

		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락


		// 2. 마샬링
		float MoveTargetX;
		float MoveTargetY;
		float MoveTargetZ;

		float HitPointX;
		float HitPointY;
		float HitPointZ;

		Packet->GetData((char*)&MoveTargetX, 4);
		Packet->GetData((char*)&MoveTargetY, 4);
		Packet->GetData((char*)&MoveTargetZ, 4);

		Packet->GetData((char*)&HitPointX, 4);
		Packet->GetData((char*)&HitPointY, 4);
		Packet->GetData((char*)&HitPointZ, 4);


		// 2. 정보 갱신
		// Hit 포인트는 저장할 필요 없음. 그냥 릴레이만 한다.
		// MoveTargetX,Z만 저장한다. Z가 실제 높이 개념.
		// 즉, 서버는 X,Y만 저장하면 된다.
		m_fPosX = MoveTargetX;
		m_fPosY = MoveTargetZ;



		// 3. 방 내의 모든 유저에게 받은 패킷 Send하기(본인 제외)
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();
		WORD Type = en_PACKET_CS_GAME_RES_MOVE_PLAYER;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&m_Int64AccountNo, 8);

		SendBuff->PutData((char*)&MoveTargetX, 4);
		SendBuff->PutData((char*)&MoveTargetY, 4);
		SendBuff->PutData((char*)&MoveTargetZ, 4);

		SendBuff->PutData((char*)&HitPointX, 4);
		SendBuff->PutData((char*)&HitPointY, 4);
		SendBuff->PutData((char*)&HitPointZ, 4);

		NowRoom->SendPacket_BroadCast(SendBuff, m_Int64AccountNo);
	}
	
	// HitPoint 갱신
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_HitPointPacket(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저의 히트 포인트 갱신은 무시
		if (m_bAliveFlag == false)
			return;


		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락



		// 2. 마샬링
		float HitPointX;
		float HitPointY;
		float HitPointZ;

		Packet->GetData((char*)&HitPointX, 4);
		Packet->GetData((char*)&HitPointY, 4);
		Packet->GetData((char*)&HitPointZ, 4);



		// 3. 해당 방 내의 유저에게 HitPoint 갱신 패킷 보내기.
		// 자기 자신 제외

		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_HIT_POINT;

		SendBuff->PutData((char*)&Type, 2);

		SendBuff->PutData((char*)&m_Int64AccountNo, 8);

		SendBuff->PutData((char*)&HitPointX, 4);
		SendBuff->PutData((char*)&HitPointY, 4);
		SendBuff->PutData((char*)&HitPointZ, 4);

		NowRoom->SendPacket_BroadCast(SendBuff, m_Int64AccountNo);		
	}

	// Fire 1 패킷 (총 발사)
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_Frie_1_Packet(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저의 공격 요청은 무시
		if (m_bAliveFlag == false)
			return;


		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락



		// 2. 마샬링
		float HitPointX;
		float HitPointY;
		float HitPointZ;

		Packet->GetData((char*)&HitPointX, 4);
		Packet->GetData((char*)&HitPointY, 4);
		Packet->GetData((char*)&HitPointZ, 4);




		// 3. 총알 수가 0개일 경우 패킷 무시함
		if (m_iBullet == 0)
			return;

		// 총알 수 감소
		--m_iBullet;		




		// 4. 방 안의 유저들에게(자기자신은 제외) Fire 1에 대한 응답패킷 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_FIRE1;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&m_Int64AccountNo, 8);

		SendBuff->PutData((char*)&HitPointX, 4);
		SendBuff->PutData((char*)&HitPointY, 4);
		SendBuff->PutData((char*)&HitPointZ, 4);

		NowRoom->SendPacket_BroadCast(SendBuff, m_Int64AccountNo);



		// 5. 패킷을 받은 시간 갱신
		// 패킷을 브로드캐스트 하는 시간은 100m/s에 안잡히게 하기 위해 여기서 시간 갱신
		m_dwFire1_StartTime = timeGetTime();
	}

	// HitDamage
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_HitDamage_Packet(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저가 보낸 데미지 패킷은 무시
		if (m_bAliveFlag == false)
			return;


		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락



		// 2. Fire_1 패킷을 받은 시간 체크
		DWORD FireTime = m_dwFire1_StartTime;

		// 기존 시간은 0으로 초기화.
		m_dwFire1_StartTime = 0;

		// 100m/s 이내로 왔는지 체크
		if ((timeGetTime() - FireTime) > 100)
		{
			// 100m/s가 지난 후에 왔다면 패킷 무시
			return;
		}



		// 3. 100 m/s 이내로 왔다면 마샬링
		INT64 TargetAccountNo; // 피해자 AccountNo

		Packet->GetData((char*)&TargetAccountNo, 8);


		// 4. 해당 방에 피해자 AccountNo가 있나 체크
		CGameSession* Target = NowRoom->Find(TargetAccountNo);

		if (Target == nullptr)
		{
			// 없다면 패킷 무시
			// 없을 가능성 있음. 100 m/s 이내에 이미 나갔을 가능성

			//로그 남긴다.
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_DEBUG,
				L"Game_HitDamage_Packet() --> Target Not Find (Attack : %lld, Target : %lld)", m_Int64AccountNo, TargetAccountNo);

			return;
		}

		// 5. 타겟이 사망 상태라면 그냥 리턴
		if (Target->GetAliveState() == false)
			return;

		
		// 6. 타겟과 거리 계산
		// 피타고라스 정리 (a2 + b2 = c2)
		// 두 점간의 거리 구함 (c)
		float a = (m_fPosX - Target->m_fPosX);
		float b = (m_fPosY - Target->m_fPosY);

		float c = sqrtf((a*a) + (b*b));

		// 계산 결과, 타겟이 범위 내에 없다면, 패킷 안보냄
		// if(c >= 17) 와 같음
		if (isgreaterequal(c, 17))
			return;


		// 7. hp 차감 처리
		// 여기까지 오면 데미지 대상이 있는것. 타겟에게 입힐 데미지 계산		
		int MinusDamage = m_pParent->GunDamage(c);

		// 데미지가 0은 나올 수 있다. HP는 정수계산이기 때문에, 0.67...이 나와도 0임.
		// 0일 경우는 패킷 무시.
		if (MinusDamage == 0)
			return;

		// 데미지가 0보다 적을 순 없음. 위에서 거리 내에 있다고 확인했기 때문에.
		else if (MinusDamage < 0)
			g_BattleServer_Room_Dump->Crash();		

		// 타겟에게 헬멧이 있는 경우, 헬멧만 1 차감
		int TargetHP, HelmetCount;
		BYTE HelmetHit;
		if (Target->m_iHelmetCount > 0)
		{
			Target->m_iHelmetCount--;

			// Send할 변수 셋팅
			HelmetCount = Target->m_iHelmetCount;
			TargetHP = Target->m_iHP;
			HelmetHit = 1;
		}

		// 헬멧이 없는 경우, 데미지 적용
		else
		{
			// 타겟에게 데미지 입힘
			TargetHP = Target->Damage(MinusDamage);					

			// Send할 변수 셋팅
			HelmetCount = 0;
			HelmetHit = 0;
		}
		


		// 8. 방 내의 모든 유저에게 응답패킷 보내기 (자기 자신 포함)
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_HIT_DAMAGE;

		SendBuff->PutData((char*)&Type, 2);

		SendBuff->PutData((char*)&m_Int64AccountNo, 8);
		SendBuff->PutData((char*)&TargetAccountNo, 8);
		SendBuff->PutData((char*)&TargetHP, 4);

		SendBuff->PutData((char*)&HelmetHit, 1);
		SendBuff->PutData((char*)&HelmetCount, 4);

		NowRoom->SendPacket_BroadCast(SendBuff);


		// 9. 타겟이 사망했다면 유저 사망 로직 진행
		if (Target->GetAliveState() == false)
		{
			NowRoom->Player_Die(this, Target);
		}
	}

	// Frie 2 패킷 (발 차기)
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_Fire_2_Packet(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저의 공격 요청은 무시
		if (m_bAliveFlag == false)
			return;


		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락



		// 2. 마샬링
		float HitPointX;
		float HitPointY;
		float HitPointZ;

		Packet->GetData((char*)&HitPointX, 4);
		Packet->GetData((char*)&HitPointY, 4);
		Packet->GetData((char*)&HitPointZ, 4);



		// 3. 방 안의 유저들에게(자기자신은 제외) Fire 2에 대한 응답패킷 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_FIRE2;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&m_Int64AccountNo, 8);

		SendBuff->PutData((char*)&HitPointX, 4);
		SendBuff->PutData((char*)&HitPointY, 4);
		SendBuff->PutData((char*)&HitPointZ, 4);

		NowRoom->SendPacket_BroadCast(SendBuff, m_Int64AccountNo);
	}

	// KickDamage
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_KickDamage_Packet(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저의 공격 요청은 무시
		if (m_bAliveFlag == false)
			return;


		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락


		// 2. 방이 있다면, 마샬링
		INT64 TargetAccountNo; // 피해자 AccountNo

		Packet->GetData((char*)&TargetAccountNo, 8);


		// 3. 해당 방에 피해자 AccountNo가 있나 체크
		CGameSession* Target = NowRoom->Find(TargetAccountNo);

		if (Target == nullptr)
		{
			// 없다면 패킷 무시
			// 없을 가능성 있음. 100 m/s 이내에 이미 나갔을 가능성

			//로그 남긴다.
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_DEBUG,
				L"Game_KickDamage_Packet() --> Target Not Find (Attack : %lld, Target : %lld)", m_Int64AccountNo, TargetAccountNo);

			return;
		}

		// 4. 타겟이 사망 상태라면 그냥 리턴
		if (Target->GetAliveState() == false)
			return;


		// 5. 공격자와 피격자의 거리 구하기.

		// 피타고라스 정리 (a2 + b2 = c2)
		// 두 점간의 거리 구함 (c)
		float a = (m_fPosX - Target->m_fPosX);
		float b = (m_fPosY - Target->m_fPosY);

		float c = sqrtf((a*a) + (b*b));

		// 두 점의 거리가 2보다 멀다면, 공격 무시
		// c > 2와 같음
		if (isgreater(c, 2))
			return; 


		// 6. 타겟에게 발차가 데미지 적용
		int TargetHP = Target->Damage(g_Data_KickDamage);


		// HP 감소 패킷 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_KICK_DAMAGE;

		SendBuff->PutData((char*)&Type, 2);

		SendBuff->PutData((char*)&m_Int64AccountNo, 8);
		SendBuff->PutData((char*)&TargetAccountNo, 8);
		SendBuff->PutData((char*)&TargetHP, 4);

		NowRoom->SendPacket_BroadCast(SendBuff);


		// 7. 피격자가 사망했다면 해당 유저 사망 패킷을 방 전체에 뿌린다.
		if (Target->GetAliveState() == false)
		{
			NowRoom->Player_Die(this, Target);
		}
	}

	// Reload Request
	//
	// Parameter : CProtocolBuff_Net*
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_Reload_Packet(CProtocolBuff_Net* Packet)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저의 재장전 요청은 무시
		if (m_bAliveFlag == false)
			return;

		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락


		// 2. 내 탄창 수 확인
		//	-------- 탄창이 없다면 패킷 무시.
		if (m_iCartridge == 0)
			return;

		//	-------- 탄창이 있다면 탄창 -1 후  총알을 g_Data_Cartridge_Bullet 로 셋팅
		else
		{
			m_iCartridge = m_iCartridge - 1;
			m_iBullet = g_Data_Cartridge_Bullet;
		}

		// 3. 방 안의 모든 유저(자기자신 포함)에게 패킷 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_RELOAD;

		SendBuff->PutData((char*)&Type, 2);

		SendBuff->PutData((char*)&m_Int64AccountNo, 8);
		SendBuff->PutData((char*)&m_iBullet, 4);
		SendBuff->PutData((char*)&m_iCartridge, 4);

		NowRoom->SendPacket_BroadCast(SendBuff);
	}

	// 아이템 획득 요청
	//
	// Parameter : CProtocolBuff_Net*, 아이템의 Type
	// return : 없음
	void CBattleServer_Room::CGameSession::Game_GetItem_Packet(CProtocolBuff_Net* Packet, int Type)
	{
		// 로그인 여부 체크
		if (m_bLoginFlag == false)
			g_BattleServer_Room_Dump->Crash();

		// 생존 여부 체크
		// 죽은 유저의 아이템 획득 요청은 무시
		if (m_bAliveFlag == false)
			return;

		// 1. 내가있는 방 알아오기
		AcquireSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 락

		auto FindRoom = m_pParent->m_Room_Umap.find(m_iRoomNo);

		// 없으면 크래시
		if (FindRoom == m_pParent->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		stRoom* NowRoom = FindRoom->second;

		// 플레이모드가 아니면 크래시
		if (NowRoom->m_iRoomState != eu_ROOM_STATE::PLAY_ROOM)
			g_BattleServer_Room_Dump->Crash();

		ReleaseSRWLockShared(&m_pParent->m_Room_Umap_srwl);		// ----- Room Umap Shared 언락


		// 2. 마샬링
		UINT ItemID;
		Packet->GetData((char*)&ItemID, 4);

		// 3. 아이템 검색
		stRoomItem* NowItem = NowRoom->Item_Find(ItemID);

		// 없는 아이템이면 무시한다.
		if (NowItem == nullptr)
			return;

		// 4. 아이템과 유저의 거리 체크.
		// +3.0f ~ -3.0f 오차까지 허용한다. (실제 좌표는, 일종의 이동 더미이기 때문에)		
		float PosX = NowItem->m_fPosX;
		float PosY = NowItem->m_fPosY;

		// islessequal(1번인자, 2번인자) 사용법
		// 1번인자 <= 2번인자 : true
		// 1번인자 > 2번인자 : false
		if (islessequal(fabs(PosX - m_fPosX), m_pParent->m_stConst.m_fGetItem_Correction) == false ||
			islessequal(fabs(PosY - m_fPosY), m_pParent->m_stConst.m_fGetItem_Correction) == false)
		{
			return;
		}

		// 5. 이번에 유저가 획득한 아이템이, 레드존 외 구역(4개 구역)에 생성된 아이템인지 체크
		// 이걸 안하면, 유저가 사망하면서 남긴 아이템과 좌표가 겹칠 경우 구분방법 없음
		if (NowItem->m_bItemArea == 2)
		{
			// 레드존 외 구역이라면, 아이템을 획득한 시간 갱신(즉, 아이템이 소멸한 시간)
			int i = 0;
			while (i < 4)
			{
				if (g_Data_ItemPoint_Playzone[i][0] == PosX &&
					g_Data_ItemPoint_Playzone[i][1] == PosY)
				{
					NowRoom->m_dwItemCreateTick[i] = timeGetTime();
					break;
				}

				++i;
			}
		}

		// 6. 거리도 맞으면 정상적으로 획득한 아이템.
		// 아이템 자료구조에서 아이템 삭제
		if (NowRoom->Item_Erase(NowItem) == false)
			g_BattleServer_Room_Dump->Crash();

		// 7. 아이템에 따라 효과 적용 및 패킷 보내기
		switch (Type)
		{
			// 탄창 
		case CARTRIDGE:
		{
			// 탄창 +1
			m_iCartridge++;

			// 결과 패킷 보내기 (자기자신 포함. 브로드캐스팅)
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_CARTRIDGE_GET;

			SendBuff->PutData((char*)&Type, 2);

			SendBuff->PutData((char*)&m_Int64AccountNo, 8);
			SendBuff->PutData((char*)&ItemID, 4);
			SendBuff->PutData((char*)&m_iCartridge, 4);

			if (NowRoom->SendPacket_BroadCast(SendBuff) == false)
				g_BattleServer_Room_Dump->Crash();
		}
			break;

			// 헬멧
		case HELMET:
		{
			// 보유 헬멧 수를 g_Data_HelmetDefensive만큼 증가
			m_iHelmetCount = m_iHelmetCount + g_Data_HelmetDefensive;

			// 결과 패킷 보내기 (자기자신 포함. 브로드캐스팅)
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_HELMET_GET;

			SendBuff->PutData((char*)&Type, 2);

			SendBuff->PutData((char*)&m_Int64AccountNo, 8);
			SendBuff->PutData((char*)&ItemID, 4);
			SendBuff->PutData((char*)&m_iHelmetCount, 4);

			if (NowRoom->SendPacket_BroadCast(SendBuff) == false)
				g_BattleServer_Room_Dump->Crash();
		}
			break;

			// 메드킷
		case MEDKIT:			
		{
			// 유저의 hp 회복
			// g_Data_HP /2 만큼 회복
			m_iHP = m_iHP + (g_Data_HP / 2);

			// 최대 hp 이상 회복 불가능.
			if (m_iHP > g_Data_HP)
				m_iHP = g_Data_HP;			

			// 결과 패킷 보내기 (자기자신 포함. 브로드캐스팅)
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_MEDKIT_GET;

			SendBuff->PutData((char*)&Type, 2);

			SendBuff->PutData((char*)&m_Int64AccountNo, 8);
			SendBuff->PutData((char*)&ItemID, 4);
			SendBuff->PutData((char*)&m_iHP, 4);

			if (NowRoom->SendPacket_BroadCast(SendBuff) == false)
				g_BattleServer_Room_Dump->Crash();
		}
			break;

		default:
			g_BattleServer_Room_Dump->Crash();
		}		
	}	
}

// ------------------
// stRoom의 함수
// (CBattleServer_Room의 이너클래스)
// ------------------
namespace Library_Jingyu
{

	// ------------
	// 멤버 함수
	// ------------

	// 생성자
	CBattleServer_Room::stRoom::stRoom()
	{
		// 미리 메모리 공간 잡아두기
		m_JoinUser_Vector.reserve(10);
		m_RoomItem_umap.reserve(30);		

		m_iJoinUserCount = 0;
	}
	
	// 소멸자
	CBattleServer_Room::stRoom::~stRoom()
	{
	}

	// ------------
	// 브로드 캐스트
	// ------------

	// 자료구조 내의 모든 유저에게 인자로 받은 패킷 보내기
	//
	// Parameter : CProtocolBuff_Net*
	// return : 자료구조 내에 유저가 0명일 경우 false
	//		  : 그 외에는 true
	bool CBattleServer_Room::stRoom::SendPacket_BroadCast(CProtocolBuff_Net* SendBuff)
	{
		// 1. 자료구조 내의 유저 수 받기
		size_t Size = m_JoinUser_Vector.size();

		// 2. 유저 수가 0명일 경우, return false
		if (Size == 0)
		{
			CProtocolBuff_Net::Free(SendBuff);
			return false;
		}

		// !! while문 돌기 전에 카운트를, 유저 수 만큼 증가 !!
		// 엔진쪽에서, 완료 통지가 오면 Free를 하기 때문에 Add해야 한다.
		SendBuff->Add((int)Size);

		// 3. 유저 수 만큼 돌면서 패킷 전송.
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->m_euDebugMode = eu_USER_TYPE_DEBUG::COUNTDOWN;
			m_JoinUser_Vector[Index]->SendPacket(SendBuff);
			++Index;
		}

		// 4. 패킷 Free
		// !! 엔진쪽 완통에서 Free 하지만, 레퍼런스 카운트를 인원 수 만큼 Add 했기 때문에 1개가 더 증가한 상태. !!	
		CProtocolBuff_Net::Free(SendBuff);

		return true;
	}

	// 자료구조 내의 모든 유저에게 인자로 받은 패킷 보내기
	// 인자로 받은 AccountNo를 제외하고 보낸다
	//
	// Parameter : CProtocolBuff_Net*, AccountNo(패킷 안보낼 유저)
	// return : 자료구조 내에 유저가 0명일 경우 false
	//		  : 그 외에는 true
	bool CBattleServer_Room::stRoom::SendPacket_BroadCast(CProtocolBuff_Net* SendBuff, INT64 AccountNo)
	{
		// 1. 자료구조 내의 유저 수 받기
		size_t Size = m_JoinUser_Vector.size();

		// 2. 유저 수가 0명일 경우, return false
		if (Size == 0)
		{
			CProtocolBuff_Net::Free(SendBuff);
			return false;
		}

		// !! while문 돌기 전에 카운트를, 유저 수 - 1 만큼 증가 !!
		// 엔진쪽에서, 완료 통지가 오면 Free를 하기 때문에 Add해야 한다.
		// 자기 자신에게는 안보내기 때문에, 나를 제외한 유저 수만 ++ 한다
		SendBuff->Add((int)Size - 1);

		// 3. 유저 수 만큼 돌면서 패킷 전송.
		size_t Index = 0;

		while (Index < Size)
		{
			// 자기 자신이 아닐 경우에만 보낸다.
			if(m_JoinUser_Vector[Index]->m_Int64AccountNo != AccountNo)
				m_JoinUser_Vector[Index]->SendPacket(SendBuff);

			++Index;
		}

		// 4. 패킷 Free
		// !! 엔진쪽 완통에서 Free 하지만, 레퍼런스 카운트를 인원 수 만큼 Add 했기 때문에 1개가 더 증가한 상태. !!	
		// 만약, 유저 수가 1명이었을 경우. 레퍼런스 카운트를 증가시키지 않아 그대로 1이지만 
		// 어차피 Send도 안했기 때문에 여기서 감소시켜야 함.
		CProtocolBuff_Net::Free(SendBuff);

		return true;
	}



	// ------------
	// 일반 기능함수
	// ------------

	// 방 내의 모든 유저를 Auth_To_Game으로 변경
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::ModeChange()
	{
		// 1. 자료구조 내의 유저 수 받기
		size_t Size = m_JoinUser_Vector.size();

		// 2. 유저 수가 0명일 가능성 있음.
		// 카운트 다운 중에 모든 유저가 나갈 경우.
		// 문제로 안본다.
		if (Size == 0)
			return;

		// 3. 0명이 아니라면, 내부에 있는 모든 유저를 AUTH_TO_GAME으로 변경
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->SetMode_GAME();
			++Index;
		}
	}

	// 룸 안의 모든 유저를 생존상태로 변경
	//
	// Parameter : 없음
	// return : 자료구조 내에 유저가 0명일 경우 false
	//		  : 그 외에는 true
	bool CBattleServer_Room::stRoom::AliveFlag_True()
	{
		// 1. 자료구조 내의 유저 수 받기
		size_t Size = m_JoinUser_Vector.size();

		// 2. 유저 수가 0명이거나 0이하일 경우, return false
		if (Size <= 0)
			return false;

		// 3. 유저 수 만큼 돌면서 생존 플래그 변경
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->AliveSet(true);
			++Index;
		}

		return true;
	}

	// 방 안의 유저들에게 게임 종료 패킷 보내기
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::GameOver()
	{
		// 1. 방 안에 유저가 0명인 경우 Crash
		if (m_iJoinUserCount == 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 방 인원수 체크 변수와 vector 안의 사이즈가 달라도 Crash
		size_t Size = m_JoinUser_Vector.size();

		if(Size == 0 || Size != m_iJoinUserCount)
			g_BattleServer_Room_Dump->Crash();		
			   

		// 3. 승리자에게 보낼 승리패킷 만들기
		CProtocolBuff_Net* winPacket = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_WINNER;
		winPacket->PutData((char*)&Type, 2);


		// 4. 패배자에게 보낼 패배 패킷 만들기
		CProtocolBuff_Net* LosePacket = CProtocolBuff_Net::Alloc();

		Type = en_PACKET_CS_GAME_RES_GAMEOVER;
		LosePacket->PutData((char*)&Type, 2);


		// 5. 순회하면서 패킷 보내기
		size_t Index = 0;
		int WinUserCount = 0;

		while (Index < Size)
		{
			// 유저가 승리자일 경우 (생존자)
			if (m_JoinUser_Vector[Index]->m_bAliveFlag == true)
			{
				CGameSession* NowPlayer = m_JoinUser_Vector[Index];	

				// 한 게임에 승리자는 1명.
				// WinUserCount가 1인데 여기 들어왔다면, 승리자가 1명 이상이 되었다는 의미.
				// 말도 안됨.
				if(WinUserCount == 1)
					g_BattleServer_Room_Dump->Crash();			

				// 유저의 승리카운트 1 증가
				NowPlayer->Record_Win_Add();

				// 강제 종료시(OnGame_ClientLeave)에도 저장해야 하기 때문에, LastDBWriteFlag를 하나 두고 저장 했나 안했나 체크한다.
				NowPlayer->m_bLastDBWriteFlag = true;

				// 승리 패킷 보내기
				NowPlayer->SendPacket(winPacket);

				// 승리패킷 보낸 수 증가
				++WinUserCount;			
			}

			// 유저가 패배자일 경우 (사망자)
			else
			{
				// 래퍼런스 카운트 증가.
				LosePacket->Add();

				// 패배 패킷 보내기
				m_JoinUser_Vector[Index]->SendPacket(LosePacket);
			}

			++Index;
		}

		// 6. 패배 패킷의 래퍼런스 카운트 감소.
		CProtocolBuff_Net::Free(LosePacket);
	}

	// 방 안의 유저들에게 셧다운 날리기
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::Shutdown_All()
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 4. 순회하며 Shutdown
		size_t Index = 0;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->Disconnect();

			++Index;
		}	
	}
	
	// 방 안의 모든 유저들에게 나 생성패킷과 다른 유저 생성 패킷 보내기
	// 
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::CreateCharacter()
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 4. 캐릭터들의 초기 정보 셋팅
		// OnGame_ClientJoin에서 셋팅하는게 아니라 여기서 셋팅하는 이유는, 생성 위치를 어떤 범위를 지정 후
		// 그 범위의 1, 2, 3, 4 순서대로 지정해줘야 하기 때문에.
		// 만약 OnGame_ClientJoin에서 하게되면 어떤 범위에 몇 번까지 했는지 모름..
		size_t Index = 0;

		DWORD NowTime = timeGetTime();

		// 초기 정보를 셋팅할 좌표값 셋팅
		int SourceIndex = rand() % m_iJoinUserCount;

		while (Index < Size)
		{
			m_JoinUser_Vector[Index]->m_euDebugMode = eu_USER_TYPE_DEBUG::GAME_START;

			// 캐릭터 초기 정보 셋팅
			m_JoinUser_Vector[Index]->StartSet(g_Data_Position[SourceIndex][0], 
												g_Data_Position[SourceIndex][1], NowTime);

			// 이번에 셋팅한 좌표가, 인원수 비례 마지막 위치라면 다시 0으로 돌아간다.
			if (SourceIndex == m_iJoinUserCount)
				SourceIndex = 0;

			else
				SourceIndex++;		

			++Index;
		}

		// 5. 방의 m_dwReaZoneTime과  m_dwTick갱신.
		m_dwReaZoneTime = NowTime;
		m_dwTick = NowTime;

		// 6. 순회하며 패킷 보냄.		
		int NowUserIndex = 0;
		while (NowUserIndex < Size)
		{
			size_t Index = 0;

			while (Index < Size)
			{
				// 내 캐릭터일 경우, 내 캐릭터 생성 패킷 보냄
				if (NowUserIndex == Index)
				{	
					// 패킷 만들기
					CProtocolBuff_Net* MySendBuff = CProtocolBuff_Net::Alloc();

					WORD Type = en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER;

					MySendBuff->PutData((char*)&Type, 2);
					MySendBuff->PutData((char*)&m_JoinUser_Vector[NowUserIndex]->m_fPosX, 4);
					MySendBuff->PutData((char*)&m_JoinUser_Vector[NowUserIndex]->m_fPosY, 4);
					MySendBuff->PutData((char*)&m_JoinUser_Vector[NowUserIndex]->m_iHP, 4);
					MySendBuff->PutData((char*)&m_JoinUser_Vector[NowUserIndex]->m_iBullet, 4);

					// 패킷 보내기	
					m_JoinUser_Vector[NowUserIndex]->SendPacket(MySendBuff);
				}


				// 다른 사람 캐릭터일 경우, 다른 사람 캐릭터 생성 패킷 보냄.
				else
				{
					// 패킷 만들기
					CProtocolBuff_Net* OtherSendBuff = CProtocolBuff_Net::Alloc();

					WORD Type = en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER;

					OtherSendBuff->PutData((char*)&Type, 2);
					OtherSendBuff->PutData((char*)&m_JoinUser_Vector[Index]->m_Int64AccountNo, 8);
					OtherSendBuff->PutData((char*)m_JoinUser_Vector[Index]->m_tcNickName, 40);
					OtherSendBuff->PutData((char*)&m_JoinUser_Vector[Index]->m_fPosX, 4);
					OtherSendBuff->PutData((char*)&m_JoinUser_Vector[Index]->m_fPosY, 4);
					OtherSendBuff->PutData((char*)&m_JoinUser_Vector[Index]->m_iHP, 4);
					OtherSendBuff->PutData((char*)&m_JoinUser_Vector[Index]->m_iBullet, 4);

					// 패킷 보내기	
					m_JoinUser_Vector[NowUserIndex]->SendPacket(OtherSendBuff);
				}

				++Index;
			}

			++NowUserIndex;
		}		
	}
	
	// 승리자에게 전적 보내기
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::WInRecodeSend()
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 4. 승리자에게 전적 보냄
		size_t Index = 0;

		while (Index < Size)
		{
			// 해당 유저가 생존자인지 체크. 생존자만 승리자임
			CGameSession* NowPlayer = m_JoinUser_Vector[Index];

			if (NowPlayer->GetAliveState() == true)
			{
				// 1. 전적 패킷 만들어서 보내기.
				CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

				WORD Type = en_PACKET_CS_GAME_RES_RECORD;

				SendBuff->PutData((char*)&Type, 2);

				SendBuff->PutData((char*)&NowPlayer->m_iRecord_PlayCount, 4);
				SendBuff->PutData((char*)&NowPlayer->m_iRecord_PlayTime, 4);
				SendBuff->PutData((char*)&NowPlayer->m_iRecord_Kill, 4);
				SendBuff->PutData((char*)&NowPlayer->m_iRecord_Die, 4);
				SendBuff->PutData((char*)&NowPlayer->m_iRecord_Win, 4);

				NowPlayer->SendPacket(SendBuff);
				break;
			}

			Index++;
		}
	}

	// 해당 방에, 아이템 생성 (최초 게임 시작 시 생성)
	// 생성 후, 방 안의 유저에게 아이템 생성 패킷 보냄
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::StartCreateItem()
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 3. 레드존 구역에 21개의 아이템 생성
		// 생성과 동시에 방 안의 모든 유저에게 해당 아이템 생성 패킷 보내기.
		int i = 0;
		while (i < 16)
		{
			// 레드존에는 탄창, 헬멧 중 1개 생성.
			int ItemType = rand() % 2;			

			// 아이템 1개 생성
			CreateItem(g_Data_ItemPoint_Redzone[i][0], g_Data_ItemPoint_Redzone[i][1], ItemType, 1);
			
			++i;
		}

		// 4. 레드존 아닌 구역에 4개의 아이템 생성
		// 마찬가지.
		i = 0;
		while (i < 4)
		{
			// 레드존 외의 구역에는 메디킷, 탄창, 헬멧 중 1개 생성.
			int ItemType = rand() % 3;

			// 아이템 1개 생성
			CreateItem(g_Data_ItemPoint_Playzone[i][0], g_Data_ItemPoint_Playzone[i][1], ItemType, 2);

			++i;
		}
	}

	// 유저가 사망한 위치에 아이템 생성
	// 생성 후, 방 안의 유저에게 아이템 생성 패킷 보냄
	//
	// Parameter : CGameSession* (사망한 유저)
	// return : 없음
	void CBattleServer_Room::stRoom::PlayerDieCreateItem(CGameSession* DiePlayer)
	{
		// 1. 탄창이 있으면 탄창 생성
		if (DiePlayer->m_iCartridge > 0)
		{
			// 이번에 아이템이 생성될 좌표
			// 좌표 기준 (-1, 0, +1) 위치 중 1곳에 생성
			// rand() % 3을 하면 (0, 1, 2) 중 1개가 나옴.
			// 여기서 -1를 하면 (-1, 0, 1) 중 1개가 나오게 된다.
			int Add = (rand() % 3) - 1;

			// 탄창 1개 생성
			CreateItem(DiePlayer->m_fPosX + Add, DiePlayer->m_fPosY + Add, eu_ITEM_TYPE::CARTRIDGE, 0);
		}

		// 2. 탄창과 관계 없이 추가로, 메디킷 또는 헬멧 중 1개 생성 
		
		// 1이면 헬멧, 2면 메디킷
		int Type = (rand() % 2) + 1;

		// 이번에 아이템이 생성될 좌표
		int Add = (rand() % 3) - 1;
		float itemX = DiePlayer->m_fPosX + Add;
		float itemY = DiePlayer->m_fPosY + Add;

		// 헬멧 or 메디킷 생성
		CreateItem(DiePlayer->m_fPosX + Add, DiePlayer->m_fPosY + Add, Type, 0);
	}
	
	// 좌표로 받은 위치에 아이템 1개 생성
	//
	// Parameter : 생성될 XY좌표, 아이템 Type
	// return : 없음
	void CBattleServer_Room::stRoom::CreateItem(float PosX, float PosY, int Type, BYTE Area)
	{
		// 아이템ID ++
		++m_uiItemID;

		// 룸 안의, 아이템 자료구조에 추가
		stRoomItem* Item = m_pBattleServer->m_Item_Pool->Alloc();
		Item->m_uiID = m_uiItemID;
		Item->m_fPosX = PosX;
		Item->m_fPosY = PosY;
		Item->m_euType = (eu_ITEM_TYPE)Type;
		Item->m_bItemArea = Area;

		Item_Insert(m_uiItemID, Item);

		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		// 탄창일 경우
		if (Type == CARTRIDGE)
		{
			WORD Type = en_PACKET_CS_GAME_RES_CARTRIDGE_CREATE;
			SendBuff->PutData((char*)&Type, 2);
		}

		// 헬멧일 경우
		else if (Type == HELMET)
		{
			WORD Type = en_PACKET_CS_GAME_RES_HELMET_CREATE;
			SendBuff->PutData((char*)&Type, 2);			
		}

		// 메디킷일 경우
		else if(Type == MEDKIT)
		{
			WORD Type = en_PACKET_CS_GAME_RES_MEDKIT_CREATE;
			SendBuff->PutData((char*)&Type, 2);			
		}

		// 그 무엇도 아니면 크래시
		else
			g_BattleServer_Room_Dump->Crash();

		// 아이템 ID
		SendBuff->PutData((char*)&m_uiItemID, 4);

		// 아이템 좌표
		SendBuff->PutData((char*)&PosX, 4);
		SendBuff->PutData((char*)&PosY, 4);

		// SendPacket_브로드캐스팅
		if (SendPacket_BroadCast(SendBuff) == false)
			g_BattleServer_Room_Dump->Crash(); 		 
	}
	
	// 방 안의 모든 유저에게 "유저 추가됨" 패킷 보내기
	//
	// Parameter : 이번에 입장한 유저 CGameSession*
	// return : 없음
	void CBattleServer_Room::stRoom::Send_AddUser(CGameSession* NowPlayer)
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		WORD Type = en_PACKET_CS_GAME_RES_ADD_USER;

		// 4. 나에게(인자로 받은 유저. 이번에 접속한 유저), 유저 추가됨 패킷 보내기 (방 안의 모든 유저)
		// 자기 자신 포함. 모두 보냄.
		size_t Index = 0;
		while (Index < Size)
		{
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			CGameSession* NowSession = m_JoinUser_Vector[Index];

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&m_iRoomNo, 4);
			SendBuff->PutData((char*)&NowSession->m_Int64AccountNo, 8);
			SendBuff->PutData((char*)NowSession->m_tcNickName, 40);

			SendBuff->PutData((char*)&NowSession->m_iRecord_PlayCount, 4);
			SendBuff->PutData((char*)&NowSession->m_iRecord_PlayTime, 4);
			SendBuff->PutData((char*)&NowSession->m_iRecord_Kill, 4);
			SendBuff->PutData((char*)&NowSession->m_iRecord_Die, 4);
			SendBuff->PutData((char*)&NowSession->m_iRecord_Win, 4);

			NowPlayer->SendPacket(SendBuff);

			++Index;
		}

		// 5. 방에 있는 나 제외 유저에게, 나의 "유저 추가됨" 패킷 보내기
		// 다만, 방에 유저가 1명일 경우(즉, 이번에 접속한 나 혼자)는 그냥 나감.
		if (Size == 1)
			return;
		
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		// 보낼 패킷 만들어두기.
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&m_iRoomNo, 4);
		SendBuff->PutData((char*)&NowPlayer->m_Int64AccountNo, 8);
		SendBuff->PutData((char*)NowPlayer->m_tcNickName, 40);

		SendBuff->PutData((char*)&NowPlayer->m_iRecord_PlayCount, 4);
		SendBuff->PutData((char*)&NowPlayer->m_iRecord_PlayTime, 4);
		SendBuff->PutData((char*)&NowPlayer->m_iRecord_Kill, 4);
		SendBuff->PutData((char*)&NowPlayer->m_iRecord_Die, 4);
		SendBuff->PutData((char*)&NowPlayer->m_iRecord_Win, 4);

		// 레퍼런스 카운트 증가
		// 나 자신은 무조건 제외이니 1 감소.
		// Alloc하면서 1 올라갔으니 1 감소.
		// 즉, 3명일 경우는 Alloc으로 1 올라가고, Add로 (3-2 = 1) 올라간다. --> RefCount 2가된다. 딱 맞음.
		SendBuff->Add((int)Size - 2);
		
		Index = 0;
		INT64 TempAccountNo = NowPlayer->m_Int64AccountNo;
		while (Index < Size)
		{
			// 4번에서 자기 자신에게 보냈으니, 여기서는 제외시켜야 함.
			if (TempAccountNo != m_JoinUser_Vector[Index]->m_Int64AccountNo)
			{
				// 내가 아닌게 확정이니, 패킷 보내기
				m_JoinUser_Vector[Index]->SendPacket(SendBuff);
			}

			++Index;
		}
	}

	// 누군가의 공격으로, 방 안의 유저 사망 시 처리 함수
	//
	// Parameter : 공격자(CGameSession*), 사망자(CGameSession*) 
	// return : 없음
	void CBattleServer_Room::stRoom::Player_Die(CGameSession* AttackPlayer, CGameSession* DiePlayer)
	{
		// 1. 공격자, 피해자의 AccountNo 로컬로 받아두기
		INT64 AtkAccountNo = AttackPlayer->m_Int64AccountNo;
		INT64 DieAccountNo = DiePlayer->m_Int64AccountNo;


		// 2. 피해자를 사망상태로 변경
		DiePlayer->AliveSet(false);


		// 3. 룸의 생존 유저 수가 이미 0이었으면 문제 있음. 
		// 모든 유저가 죽었는데 또 죽었다고 한 것.
		if (m_iAliveUserCount == 0)
			g_BattleServer_Room_Dump->Crash();


		// 4. 룸의 생존 유저 수, GameMode 유저 수 감소 카운트 1 감소
		--m_iAliveUserCount;
		--m_iGameModeUser;



		// 5. 패킷 보내기
		CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

		WORD Type = en_PACKET_CS_GAME_RES_DIE;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&DieAccountNo, 8);

		SendPacket_BroadCast(SendBuff);


		// 6. 유저가 사망한 위치에 신규 아이템 생성
		PlayerDieCreateItem(DiePlayer);



		// 7. 공격자의 Kill 카운트 1 증가
		AttackPlayer->Record_Kill_Add();



		// 8. 유저의 사망카운트 1 증가
		DiePlayer->Recored_Die_Add();	
		
		// 강제 종료시(OnGame_ClientLeave)에도 저장해야 하기 때문에, LastDBWriteFlag를 하나 두고 저장 했나 안했나 체크한다.
		DiePlayer->m_bLastDBWriteFlag = true;



		// 9. 사망자에게 전적 내용 보내기
		CProtocolBuff_Net* DieSendBuff = CProtocolBuff_Net::Alloc();
		Type = en_PACKET_CS_GAME_RES_RECORD;

		DieSendBuff->PutData((char*)&Type, 2);
		DieSendBuff->PutData((char*)&DiePlayer->m_iRecord_PlayCount, 4);
		DieSendBuff->PutData((char*)&DiePlayer->m_iRecord_PlayTime, 4);
		DieSendBuff->PutData((char*)&DiePlayer->m_iRecord_Kill, 4);
		DieSendBuff->PutData((char*)&DiePlayer->m_iRecord_Die, 4);
		DieSendBuff->PutData((char*)&DiePlayer->m_iRecord_Win, 4);

		DiePlayer->SendPacket(DieSendBuff);
	}


	// ------------
	// 자료구조 함수
	// ------------

	// 자료구조에 Insert
	//
	// Parameter : 추가하고자 하는 CGameSession*
	// return : 없음
	void CBattleServer_Room::stRoom::Insert(CGameSession* InsertPlayer)
	{
		m_JoinUser_Vector.push_back(InsertPlayer);
	}

	// 자료구조에 유저가 있나 체크
	//
	// Parameter : AccountNo
	// return : 찾은 유저의 CGameSession*
	//		  : 유저가 없을 시 nullptr
	CBattleServer_Room::CGameSession* CBattleServer_Room::stRoom::Find(INT64 AccountNo)
	{
		size_t Size = m_JoinUser_Vector.size();

		// 1. 자료구조 안에 유저가 0이라면 Crash
		if (Size == 0)
			g_BattleServer_Room_Dump->Crash();


		// 2. 룸 안에 있는 유저 수와 다르다면 Crash
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();


		// 3. 순회하면서 있나 없나 검색
		size_t Index = 0;
		while (Index < Size)
		{
			if (m_JoinUser_Vector[Index]->m_Int64AccountNo == AccountNo)
				return m_JoinUser_Vector[Index];

			++Index;
		}

		// 4. 여기까지 오면 없는것. 
		return nullptr;
	}

	// 자료구조에서 Erase
	//
	// Parameter : 제거하고자 하는 CGameSession*
	// return : 성공 시 true
	//		  : 실패 시  false
	bool CBattleServer_Room::stRoom::Erase(CGameSession* InsertPlayer)
	{
		size_t Size = m_JoinUser_Vector.size();
		bool Flag = false;

		// 1. 자료구조 안에 유저가 0이라면 return false
		if (Size == 0)
			return false;

		// 2. 자료구조 안에 유저가 1명이거나, 찾고자 하는 유저가 마지막에 있다면 바로 제거
		if (Size == 1 || m_JoinUser_Vector[Size - 1] == InsertPlayer)
		{
			Flag = true;
			m_JoinUser_Vector.pop_back();
		}

		// 3. 아니라면 Swap 한다
		else
		{
			size_t Index = 0;
			while (Index < Size)
			{
				// 내가 찾고자 하는 유저를 찾았다면
				if (m_JoinUser_Vector[Index] == InsertPlayer)
				{
					Flag = true;

					CGameSession* Temp = m_JoinUser_Vector[Size - 1];
					m_JoinUser_Vector[Size - 1] = m_JoinUser_Vector[Index];
					m_JoinUser_Vector[Index] = Temp;

					m_JoinUser_Vector.pop_back();

					break;
				}

				++Index;
			}
		}

		// 4. 만약, 제거 못했다면 return false
		if (Flag == false)
			return false;

		return true;
	}
	
	// 아이템 자료구조에 Insert
	//
	// Parameter : ItemID, stRoomItem*
	// return : 없음
	void CBattleServer_Room::stRoom::Item_Insert(UINT ID, stRoomItem* InsertItem)
	{
		// Insert 시도
		auto ret = m_RoomItem_umap.insert(make_pair(ID, InsertItem));

		// 중복일 경우 Crash
		if(ret.second == false)
			g_BattleServer_Room_Dump->Crash();		
	}

	// 아이템 자료구조에 아이템이 있나 체크
	//
	// Parameter : itemID
	// return : 찾은 아이템의 stRoomItem*
	//		  : 아이템이 없을 시 nullptr
	CBattleServer_Room::stRoomItem* CBattleServer_Room::stRoom::Item_Find(UINT ID)
	{
		// 아이템 검색
		auto itor = m_RoomItem_umap.find(ID);

		if (itor == m_RoomItem_umap.end())
			return nullptr;

		return itor->second;
	}

	// 아이템 자료구조에서 Erase
	//
	// Parameter : 제거하고자 하는 stRoomItem*
	// return : 성공 시 true
	//		  : 실패 시  false
	bool CBattleServer_Room::stRoom::Item_Erase(stRoomItem* InsertPlayer)
	{
		// 검색
		auto itor = m_RoomItem_umap.find(InsertPlayer->m_uiID);

		// 없으면 return false
		if (itor == m_RoomItem_umap.end())
			return false;

		// 찾았으면 Erase
		m_RoomItem_umap.erase(itor);

		// stRoomItem* 반환
		m_pBattleServer->m_Item_Pool->Free(InsertPlayer);

		return true;
	}



	// ------------
	// 레드존 관련 함수
	// ------------

	// 레드존 경고
	// 이 함수는, 레드존 경고를 보내야 할 시점에 호출된다.
	// 이미 밖에서, 호출 시점 다 체크 후 이 함수 호출.
	//
	// Parameter : 경고 시간(BYTE). 
	// return : 없음
	void CBattleServer_Room::stRoom::RedZone_Warning(BYTE AlertTimeSec)
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 3. 마지막 레드존이라면, 마지막 레드존 처리 진행
		if (m_iRedZoneCount + 1 == m_pBattleServer->m_stConst.m_iRedZoneActiveLimit)
		{
			// 방 안의 모든 유저에게 마지막 레드존 경고 패킷 보냄.
			CProtocolBuff_Net* SendBUff = CProtocolBuff_Net::Alloc();
			WORD Type = en_PACKET_CS_GAME_RES_REDZONE_ALERT_FINAL;

			SendBUff->PutData((char*)&Type, 2);
			SendBUff->PutData((char*)&AlertTimeSec, 1);
			SendBUff->PutData((char*)&m_bLastRedZoneSafeType, 1);

			if (SendPacket_BroadCast(SendBUff) == false)
				g_BattleServer_Room_Dump->Crash();
		}

		// 4. 마지막 레드존이 아니라면, 일반 처리
		else
		{
			// 20초 뒤에 활성화 될 레드존의 타입을 알아낸다.
			int RedZoneType = m_arrayRedZone[m_iRedZoneCount];

			// 레드존에 따라 타입 설정
			WORD Type;
			switch (RedZoneType)
			{
				// 왼쪽
			case eu_REDZONE_TYPE::LEFT:
				Type = en_PACKET_CS_GAME_RES_REDZONE_ALERT_LEFT;
				break;

				// 오른쪽
			case eu_REDZONE_TYPE::RIGHT:
				Type = en_PACKET_CS_GAME_RES_REDZONE_ALERT_RIGHT;
				break;

				// 위
			case eu_REDZONE_TYPE::TOP:
				Type = en_PACKET_CS_GAME_RES_REDZONE_ALERT_TOP;
				break;

				// 아래
			case eu_REDZONE_TYPE::BOTTOM:
				Type = en_PACKET_CS_GAME_RES_REDZONE_ALERT_BOTTOM;
				break;

			default:
				g_BattleServer_Room_Dump->Crash();
				break;
			}

			// 방 안의 모든 유저에게 레드존 경고 패킷 보내기
			CProtocolBuff_Net* SendBUff = CProtocolBuff_Net::Alloc();

			SendBUff->PutData((char*)&Type, 2);
			SendBUff->PutData((char*)&AlertTimeSec, 1);

			if (SendPacket_BroadCast(SendBUff) == false)
				g_BattleServer_Room_Dump->Crash();
		}

	}

	// 레드존 활성화
	// 이 함수는, 레드존이 활성화 될 시점에만 호출된다
	// 즉, 활성화 할지 말지는 밖에서 정한 다음에 이 함수 호출
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::RedZone_Active()
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 3. 이번에 활성시킬 레드존을 알아낸다
		int RedZoneType = m_arrayRedZone[m_iRedZoneCount];
		m_iRedZoneCount++;

		// 4. 마지막 레드존일 경우
		if (m_iRedZoneCount == m_pBattleServer->m_stConst.m_iRedZoneActiveLimit)
		{			
			float (*LastSafe)[2] = m_pBattleServer->m_arrayLastRedZoneSafeRange[m_bLastRedZoneSafeType - 1];

			// 안전지대 갱신
			m_fSafePos[0][0] = LastSafe[0][0];
			m_fSafePos[0][1] = LastSafe[0][1];
			m_fSafePos[1][0] = LastSafe[1][0];
			m_fSafePos[1][1] = LastSafe[1][1];

			// 방 안의 모든 유저에게 마지막 레드존 활성화 패킷 보냄
			CProtocolBuff_Net* SendBUff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_REDZONE_ACTIVE_FINAL;

			SendBUff->PutData((char*)&Type, 2);
			SendBUff->PutData((char*)&m_bLastRedZoneSafeType, 1);

			if (SendPacket_BroadCast(SendBUff) == false)
				g_BattleServer_Room_Dump->Crash();
		}

		// 5. 마지막 레드존이 아닐 경우
		else
		{
			// 활성화 시킬 레드존에 따라 안전지대를 갱신한다.
			WORD Type;
			switch (RedZoneType)
			{
				// 왼쪽
			case eu_REDZONE_TYPE::LEFT:
				m_fSafePos[0][1] = m_pBattleServer->m_arrayRedZoneRange[RedZoneType][1][1];
				Type = en_PACKET_CS_GAME_RES_REDZONE_ACTIVE_LEFT;
				break;

				// 오른쪽
			case eu_REDZONE_TYPE::RIGHT:
				m_fSafePos[1][1] = m_pBattleServer->m_arrayRedZoneRange[RedZoneType][0][1];
				Type = en_PACKET_CS_GAME_RES_REDZONE_ACTIVE_RIGHT;
				break;

				// 위
			case eu_REDZONE_TYPE::TOP:
				m_fSafePos[0][0] = m_pBattleServer->m_arrayRedZoneRange[RedZoneType][1][0];
				Type = en_PACKET_CS_GAME_RES_REDZONE_ACTIVE_TOP;
				break;

				// 아래
			case eu_REDZONE_TYPE::BOTTOM:
				m_fSafePos[1][0] = m_pBattleServer->m_arrayRedZoneRange[RedZoneType][0][0];
				Type = en_PACKET_CS_GAME_RES_REDZONE_ACTIVE_BOTTOM;
				break;

			default:
				g_BattleServer_Room_Dump->Crash();
				break;
			}

			// 방 안의 모든 유저에게 레드존 활성화 패킷 보내기
			CProtocolBuff_Net* SendBUff = CProtocolBuff_Net::Alloc();

			SendBUff->PutData((char*)&Type, 2);

			if (SendPacket_BroadCast(SendBUff) == false)
				g_BattleServer_Room_Dump->Crash();
		}
	}

	// 레드존 데미지 체크
	// 이 함수는, 유저에게 데미지를 줘야 할 시점에 호출
	// 즉, 줄지 말지는 밖에서 장한 다음에 이 함수 호출
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::stRoom::RedZone_Damage()
	{
		// 1. 방 안에 유저가 0명이면 말이 안됨. 밖에서 이미 0이 아니라는것을 알고 왔기 때문에.
		if (m_iJoinUserCount <= 0)
			g_BattleServer_Room_Dump->Crash();

		// 2. 백터 안의 유저와 변수가 다르면 크래시
		size_t Size = m_JoinUser_Vector.size();
		if (m_iJoinUserCount != Size)
			g_BattleServer_Room_Dump->Crash();

		// 3. 룸 안의 모든 유저를 확인.
		// 좌표가 안전지대가 맞는지 체크.
		// 안전지대가 아니라면 데미지를 입힐 유저.
		size_t Index = 0;
		while (Index < Size)
		{
			// 생존한 유저만, 좌표 체크
			if (m_JoinUser_Vector[Index]->GetAliveState() == true)
			{
				CGameSession* NowPlayer = m_JoinUser_Vector[Index];
				
				// 플레이어 X가
				// - safe[0]의 X보다 작거나
				// - safe[1]의 X보다 크거나
				//
				// 플레이어 Y가
				// - safe[0]의 Y보다 작거나
				// - safe[1]의 Y보다 크거나
				if (isless(NowPlayer->m_fPosX, m_fSafePos[0][0]) ||
					isgreater(NowPlayer->m_fPosX, m_fSafePos[1][0]) ||
					isless(NowPlayer->m_fPosY, m_fSafePos[0][1]) ||
					isgreater(NowPlayer->m_fPosY, m_fSafePos[1][1]) )
				{
					// 레드존 데미지 적용
					int AfterHP = NowPlayer->Damage(1);					

					// 데미지 패킷 보냄.
					CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();
					WORD Type = en_PACKET_CS_GAME_RES_REDZONE_DAMAGE;

					SendBuff->PutData((char*)&Type, 2);
					SendBuff->PutData((char*)&NowPlayer->m_Int64AccountNo, 8);
					SendBuff->PutData((char*)&AfterHP, 4);

					// 모든 유저에게 보냄(자기자신 포함)
					if (SendPacket_BroadCast(SendBuff) == false)
						g_BattleServer_Room_Dump->Crash();

					// 유저가 사망했다면
					if (NowPlayer->GetAliveState() == false)
					{
						INT64 AccountNo = NowPlayer->m_Int64AccountNo;

						// 룸의 생존 유저 수가 이미 0이었으면 문제 있음. 
						// 모든 유저가 죽었는데 또 죽었다고 한 것.
						if (m_iAliveUserCount == 0)
							g_BattleServer_Room_Dump->Crash();

						// 룸의 생존 유저 수, GameMode 유저 카운트 1 감소
						--m_iAliveUserCount;
						--m_iGameModeUser;

						// 패킷 보내기
						CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

						WORD Type = en_PACKET_CS_GAME_RES_DIE;

						SendBuff->PutData((char*)&Type, 2);
						SendBuff->PutData((char*)&AccountNo, 8);

						if (SendPacket_BroadCast(SendBuff) == false)
							g_BattleServer_Room_Dump->Crash();

						// 유저가 사망한 위치에 신규 아이템 생성
						PlayerDieCreateItem(NowPlayer);

						// Die카운트 1 증가
						NowPlayer->Recored_Die_Add();

						// 강제 종료시(OnGame_ClientLeave)에도 저장해야 하기 때문에, LastDBWriteFlag를 하나 두고 저장 했나 안했나 체크한다.
						NowPlayer->m_bLastDBWriteFlag = true;

						// 사망자에게 전적 내용 보내기
						CProtocolBuff_Net* DieSendBuff = CProtocolBuff_Net::Alloc();
						Type = en_PACKET_CS_GAME_RES_RECORD;

						DieSendBuff->PutData((char*)&Type, 2);
						DieSendBuff->PutData((char*)&NowPlayer->m_iRecord_PlayCount, 4);
						DieSendBuff->PutData((char*)&NowPlayer->m_iRecord_PlayTime, 4);
						DieSendBuff->PutData((char*)&NowPlayer->m_iRecord_Kill, 4);
						DieSendBuff->PutData((char*)&NowPlayer->m_iRecord_Die, 4);
						DieSendBuff->PutData((char*)&NowPlayer->m_iRecord_Win, 4);

						NowPlayer->SendPacket(DieSendBuff);
					}
				}
			}

			++Index;
		}

	}

}

// ----------------------------------------
// 
// MMOServer를 이용한 배틀 서버. 룸 버전
//
// ----------------------------------------
namespace Library_Jingyu
{
	// Net 직렬화 버퍼 1개의 크기 (Byte)
	LONG g_lNET_BUFF_SIZE = 512;
		   	  


	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// Start
	// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
	//
	// Parameter : 없음
	// return : 실패 시 false
	bool CBattleServer_Room::ServerStart()
	{
		// 카운트 값 초기화
		m_lNowWaitRoomCount = 0;
		m_lNowTotalRoomCount = 0;
		m_lGlobal_RoomNo = 0;

		m_lBattleEnterTokenError = 0;
		m_lRoomEnterTokenError = 0;
		m_lQuery_Result_Not_Find = 0;
		m_lTempError = 0;
		m_lTokenError = 0;
		m_lVerError = 0;
		m_OverlapLoginCount = 0;
		m_OverlapLoginCount_DB = 0;
		m_lReadyRoomCount = 0;
		m_lPlayRoomCount = 0;
		m_lAuthFPS = 0;
		m_lGameFPS = 0;

		m_shDB_Communicate.ParentSet(this);

		// 1. 세션 셋팅
		m_cGameSession = new CGameSession[m_stConfig.MaxJoinUser];

		int i = 0;
		while (i < m_stConfig.MaxJoinUser)
		{
			// GameServer의 포인터 셋팅
			m_cGameSession[i].m_pParent = this;

			// 엔진에 세션 셋팅
			SetSession(&m_cGameSession[i], m_stConfig.MaxJoinUser);
			++i;
		}

		// 2. 모니터링 서버와 연결되는, 랜 클라이언트 가동
		if (m_Monitor_LanClient->ClientStart(m_stConfig.MonitorServerIP, m_stConfig.MonitorServerPort, m_stConfig.MonitorClientCreateWorker,
			m_stConfig.MonitorClientActiveWorker, m_stConfig.MonitorClientNodelay) == false)
			return false;

		// 3. 채팅 랜클라와 연결되는 랜서버 가동
		if (m_Chat_LanServer->ServerStart(m_stConfig.ChatLanServerIP, m_stConfig.ChatPort, m_stConfig.ChatCreateWorker, m_stConfig.ChatActiveWorker,
			m_stConfig.ChatCreateAccept, m_stConfig.ChatNodelay, m_stConfig.ChatMaxJoinUser) == false)
			return false;

		// 4. 마스터 서버와 연결되는, 랜 클라이언트 가동
		if (m_Master_LanClient->ClientStart(m_stConfig.MasterServerIP, m_stConfig.MasterServerPort, m_stConfig.MasterClientCreateWorker,
			m_stConfig.MasterClientActiveWorker, m_stConfig.MasterClientNodelay) == false)
			return false;

		// 4. Battle 서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept,
			m_stConfig.Nodelay, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode) == false)
			return false;

		// 서버 오픈 로그 찍기		
		g_BattleServer_RoomLog->LogSave(true, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;

	}

	// Stop
	// 내부적으로 Stop 실행
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::ServerStop()
	{
		// 1. 모니터링 클라 종료
		if (m_Monitor_LanClient->GetClinetState() == true)
			m_Monitor_LanClient->ClientStop();

		// 2. 마스터 Lan 클라 종료
		if (m_Master_LanClient->GetClinetState() == true)
			m_Master_LanClient->ClientStop();

		// 3. 서버 종료
		if (GetServerState() == true)
			Stop();

		// 3. 세션 삭제
		delete[] m_cGameSession;
	}

	// 출력용 함수
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::ShowPrintf()
	{
		// 해당 프로세스의 사용량 체크할 클래스
		static CCpuUsage_Process ProcessUsage;

		// CPU 사용율 체크 클래스 (하드웨어)
		static CCpuUsage_Processor CProcessorCPU;

		// 화면 출력할 것 셋팅
		/*
		Total SessionNum : 					- MMOServer 의 세션수
		AuthMode SessionNum :				- Auth 모드의 유저 수
		GameMode SessionNum (Auth + Game) :	- Game 모드의 유저 수 (Auth + Game모드 유저 수)

		PacketPool_Net : 		- 외부에서 사용 중인 Net 직렬화 버퍼의 수
		Accept Socket Queue :	- Accept Socket Queue 안의 일감 수
		HeartBeat Flag	:		- 하트비트 플래그. 1이면 하트비트 중

		Accept Total :		- Accept 전체 카운트 (accept 리턴시 +1)
		Accept TPS :		- 초당 Accept 처리 횟수
		Auth FPS :			- 초당 Auth 스레드 처리 횟수
		Game FPS :			- 초당 Game 스레드 처리 횟수

		Send TPS:			- 초당 Send완료 횟수. (완료통지에서 증가)
		Recv TPS:			- 초당 Recv완료 횟수. (패킷 1개가 완성되었을 때 증가. RecvProc에서 패킷에 넣기 전에 1씩 증가)

		NetBuff_ChunkAlloc_Count : - Net 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		Player_ChunkAlloc_Count :	- 플레이어 직렬화 버퍼 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)
		ASQPool_ChunkAlloc_Count : - Accept Socket Queue에 들어가는 일감 총 Alloc한 청크 수 (밖에서 사용중인 청크 수)

		------------------ Room -------------------
		WaitRoom :			- 대기방 수
		ReadyRoom :			- 준비방 수
		PlayRoom :			- 플레이방 수
		TotalRoom :			- 총 방 수
		TotalRoom_Pool :	- 룸 umap에 있는 카운트.	

		Room_ChunkAlloc_Count : - 할당받은 룸 청크 수(밖에서 사용중인 수)

		------------------ DBWrite -------------------
		Node Alloc Count :	(Add : )	- DBWrite 스레드에게 일시키용 큐 내부 사이즈. - Add는 초당 큐에 들어온 데이터의 수.
		DBWrite TPS :	- DBWrite의 TPS

		------------------ Error -------------------
		Battle_EnterTokenError:		- 배틀서버 입장 토큰 에러
		Room_EnterTokenError:		- 방 입장 토큰 에러
		Login_Query_Not_Find :		- auth의 로그인 패킷에서, DB 쿼리시 -10이 결과로 옴.
		Login_Query_Temp :			- auth의 로그인 패킷에서, DB 쿼리 시 -10이 아닌 에러가 옴.
		Login_UserTokenError :		- Auth의 로그인 패킷에서, 유저 토큰이 다름
		Login_VerError :			- auth의 로그인 패킷에서, 유저가 들고온 버전이 다름
		Login_Duplicate :			- 중복 로그인
		Login_DBWrite :				- DBWrite중인데 또 들어올 경우
		SemCount :					- 세마포어(121)에러 시 1 증가

		---------- Battle LanServer(Chat) ---------
		SessionNum :				- 배틀 랜서버 (채팅서버)에 접속한 세션 수
		PacketPool_Lan :			- 사용중인 직렬화버퍼 랜 버전. 토탈.

		Lan_BuffChunkAlloc_Count :	- 사용중인 직렬화 버퍼의 청크 수(밖에서 사용중인 수)

		-------------- LanClient -------------------
		Monitor Connect :			- 모니터 서버와 연결되는 랜 클라. 접속 여부
		Master Connect :			- 마스터 서버와 연결되는 랜 클라. 접속여부

		----------------------------------------------------
		CPU usage [T:%.1f%% U:%.1f%% K:%.1f%%] [BattleServer:%.1f%% U:%.1f%% K:%.1f%%] - 프로세서, 프로세스 사용량.

		*/

		// 출력 전에, 프로세스/프로세서 사용량 갱신
		ProcessUsage.UpdateCpuTime();
		CProcessorCPU.UpdateCpuTime();

		LONG AuthUser = GetAuthModeUserCount();
		LONG GameUser = GetGameModeUserCount();
		LONG TempDBWriteTPS = InterlockedExchange(&m_shDB_Communicate.m_lDBWriteTPS, 0);
		LONG TempDBWriteCountTPS = InterlockedExchange(&m_shDB_Communicate.m_lDBWriteCountTPS, 0);
		m_lAuthFPS = GetAuthFPS();
		m_lGameFPS = GetGameFPS();

		printf("================== Battle Server ==================\n"
			"Total SessionNum : %lld\n"
			"AuthMode SessionNum : %d\n"
			"GameMode SessionNum : %d (Auth + Game : %d)\n\n"

			"PacketPool_Net : %d\n"
			"Accept Socket Queue : %d\n"
			"HeartBeat Flag : %d\n\n"

			"Accept Total : %lld\n"
			"Accept TPS : %d\n"
			"Send TPS : %d\n"
			"Recv TPS : %d\n\n"

			"Auth FPS : %d\n"
			"Game FPS : %d\n\n"

			"NetBuff_ChunkAlloc_Count : %d (Out : %d)\n"
			"ASQPool_ChunkAlloc_Count : %d (Out : %d)\n\n"

			"------------------ Room -------------------\n"
			"WaitRoom : %d\n"
			"ReadyRoom : %d\n"
			"PlayRoom : %d\n"
			"TotalRoom : %d\n"
			"TotalRoom_Pool : %lld\n\n"

			"Room_ChunkAlloc_Count : %d (Out : %d)\n\n"

			"------------------DBWrite------------------\n"
			"Node Alloc Count : %d (Add : %d)\n"
			"DBWrite TPS : %d\n\n"

			"------------------ Error -------------------\n"
			"Battle_EnterTokenError : %d\n"
			"Room_EnterTokenError : %d\n"
			"Login_Query_Not_Find : %d\n"
			"Login_Query_Temp : %d\n"
			"Login_UserTokenError : %d\n"
			"Login_VerError : %d\n"
			"Login_Overlap_DB : %d\n"
			"Login_Overlap : %d\n"
			"SemCount : %d\n"
			"HeartBeat_Count : %d\n\n"

			"---------- Battle LanServer(Chat) ---------\n"
			"SessionNum : %lld\n"
			"PacketPool_Lan : %d\n\n"

			"Lan_BuffChunkAlloc_Count : %d (Out : %d)\n\n"

			"-------------- LanClient -----------\n"
			"Monitor Connect : %d\n"
			"Master Connect : %d\n\n"

			"========================================================\n\n"
			"CPU usage [T:%.1f%% U:%.1f%% K:%.1f%%] [BattleServer:%.1f%% U:%.1f%% K:%.1f%%]\n",

			// ----------- 게임 서버용
			GetClientCount(),
			AuthUser,
			GameUser, AuthUser + GameUser,

			CProtocolBuff_Net::GetNodeCount(),
			GetASQ_Count(),
			GetHeartBeatFlag(),

			GetAccpetTotal(),
			GetAccpetTPS(),
			GetSendTPS(),
			GetRecvTPS(),

			m_lAuthFPS,
			m_lGameFPS,

			CProtocolBuff_Net::GetChunkCount(), CProtocolBuff_Net::GetOutChunkCount(),
			GetChunkCount(), GetOutChunkCount(),

			m_lNowWaitRoomCount,
			m_lReadyRoomCount,
			m_lPlayRoomCount,
			m_lNowTotalRoomCount,		
			m_Room_Umap.size(),
			m_Room_Pool->GetAllocChunkCount(), m_Room_Pool->GetOutChunkCount(),

			m_shDB_Communicate.m_pDB_Wirte_Start_Queue->GetNodeSize(), TempDBWriteCountTPS,
			TempDBWriteTPS,

			// ----------- 에러
			m_lBattleEnterTokenError,
			m_lRoomEnterTokenError,
			m_lQuery_Result_Not_Find,
			m_lTempError,
			m_lTokenError,
			m_lVerError,
			m_OverlapLoginCount_DB,
			m_OverlapLoginCount,
			GetSemCount(),
			GetHeartBeatCount(),

			// ----------- 배틀 랜서버
			m_Chat_LanServer->GetClientCount(),
			CProtocolBuff_Lan::GetNodeCount(),
			CProtocolBuff_Lan::GetChunkCount(), CProtocolBuff_Lan::GetOutChunkCount(),


			// ----------- 랜클라 (마스터, 모니터)
			m_Monitor_LanClient->GetClinetState(),
			m_Master_LanClient->GetClinetState(),

			// ----------- 프로세서 사용량
			CProcessorCPU.ProcessorTotal(), CProcessorCPU.ProcessorUser(), CProcessorCPU.ProcessorKernel(),

			// ----------- 프로세스 사용량 
			ProcessUsage.ProcessTotal(), ProcessUsage.ProcessUser(), ProcessUsage.ProcessKernel()
		);
	}

	   


	// -----------------------
	// 내부에서만 사용하는 함수
	// -----------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CBattleServer_Room::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MMOGameServer_Config.ini"));
		}
		catch (int expn)
		{
			if (expn == 1)
			{
				printf("File Open Fail...\n");
				return false;
			}
			else if (expn == 2)
			{
				printf("FileR Read Fail...\n");
				return false;
			}
		}



		////////////////////////////////////////////////////////
		// BATTLESERVER config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("BATTLESERVER")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("BindIP"), pConfig->BindIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("Port"), &pConfig->Port) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("CreateWorker"), &pConfig->CreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ActiveWorker"), &pConfig->ActiveWorker) == false)
			return false;

		// 생성 엑셉트
		if (Parser.GetValue_Int(_T("CreateAccept"), &pConfig->CreateAccept) == false)
			return false;

		// 헤더 코드
		if (Parser.GetValue_Int(_T("HeadCode"), &pConfig->HeadCode) == false)
			return false;

		// xorcode
		if (Parser.GetValue_Int(_T("XorCode"), &pConfig->XORCode) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("Nodelay"), &pConfig->Nodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("MaxJoinUser"), &pConfig->MaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("LogLevel"), &pConfig->LogLevel) == false)
			return false;




		////////////////////////////////////////////////////////
		// 기본 config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CONFIG")) == false)
			return false;

		// VerCode
		if (Parser.GetValue_Int(_T("VerCode"), &m_uiVer_Code) == false)
			return false;

		// BattleNetServer의 IP
		if (Parser.GetValue_String(_T("BattleNetServerIP"), m_Master_LanClient->m_tcBattleNetServerIP) == false)
			return false;

		// MasterNetServer의 Port 셋팅.
		m_Master_LanClient->m_iBattleNetServerPort = pConfig->Port;		

		// 마스터 서버에 입장 시 들고갈 토큰
		TCHAR m_tcMasterToken[33];
		if (Parser.GetValue_String(_T("MasterEnterToken"), m_tcMasterToken) == false)
			return false;

		// 들고갈 때는 char형으로 들고가기 때문에 변환해서 가지고 있어야한다.
		int len = WideCharToMultiByte(CP_UTF8, 0, m_tcMasterToken, lstrlenW(m_tcMasterToken), 0, 0, 0, 0);
		WideCharToMultiByte(CP_UTF8, 0, m_tcMasterToken, lstrlenW(m_tcMasterToken), m_Master_LanClient->m_cMasterToken, len, 0, 0);



		
		////////////////////////////////////////////////////////
		// 채팅 LanServer Config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("CHATLANSERVER")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("ChatBindIP"), pConfig->ChatLanServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("ChatPort"), &pConfig->ChatPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("ChatCreateWorker"), &pConfig->ChatCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("ChatActiveWorker"), &pConfig->ChatActiveWorker) == false)
			return false;

		// 생성 엑셉트
		if (Parser.GetValue_Int(_T("ChatCreateAccept"), &pConfig->ChatCreateAccept) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("ChatNodelay"), &pConfig->ChatNodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("ChatMaxJoinUser"), &pConfig->ChatMaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("ChatLogLevel"), &pConfig->ChatLogLevel) == false)
			return false;






		////////////////////////////////////////////////////////
		// 마스터 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MASTERLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("MasterServerIP"), pConfig->MasterServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("MasterServerPort"), &pConfig->MasterServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("MasterClientCreateWorker"), &pConfig->MasterClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("MasterClientActiveWorker"), &pConfig->MasterClientActiveWorker) == false)
			return false;

		// Nodelay
		if (Parser.GetValue_Int(_T("MasterClientNodelay"), &pConfig->MasterClientNodelay) == false)
			return false;




		////////////////////////////////////////////////////////
		// 모니터링 LanClient config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MONITORLANCLIENT")) == false)
			return false;

		// IP
		if (Parser.GetValue_String(_T("MonitorServerIP"), pConfig->MonitorServerIP) == false)
			return false;

		// Port
		if (Parser.GetValue_Int(_T("MonitorServerPort"), &pConfig->MonitorServerPort) == false)
			return false;

		// 생성 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientCreateWorker"), &pConfig->MonitorClientCreateWorker) == false)
			return false;

		// 활성화 워커 수
		if (Parser.GetValue_Int(_T("MonitorClientActiveWorker"), &pConfig->MonitorClientActiveWorker) == false)
			return false;


		// Nodelay
		if (Parser.GetValue_Int(_T("MonitorClientNodelay"), &pConfig->MonitorClientNodelay) == false)
			return false;


		return true;
	}

	// 총알 데미지 계산 시 사용되는 함수 (발차기 계산시에는 사용되지 않음.)
	// 실제 감소시켜야 하는 HP를 계산한다.
	//
	// Parameter : 공격자와 피해자의 거리
	// return : 감소시켜야하는 HP
	int CBattleServer_Room::GunDamage(float Range)
	{
		// 공격 비율에 의해 입힐 데미지 계산

		// 거리가 2보다 짧다면 100%데미지
		// Range <= 2 와 같음
		if (islessequal(Range, 2))
			return g_Data_HitDamage;

		// 그게 아니라면 비율에 따라 계산
		// 0~2까지는 동일한 거리로 취급한다.
		// 즉, 0~17까지 거리를 체크하긴 하지만, 사실 2~17이며
		// 이는 즉, 실제 계산때는 0~15까지 있는것으로 취급해야 한다.
		return (int)(g_Data_HitDamage - (m_fFire1_Damage * (Range - 2)));
	}
		
	// 방 생성 함수
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::RoomCreate()
	{
		// 1. 현재 총 방 수 증가
		InterlockedIncrement(&m_lNowTotalRoomCount);

		// 2. 대기방 수 증가.
		InterlockedIncrement(&m_lNowWaitRoomCount);

		// 3. 방 Alloc
		stRoom* NowRoom = m_Room_Pool->Alloc();

		// 만약, 방 안에 유저 수가 0명이 아니면 Crash
		if (NowRoom->m_iJoinUserCount != 0 || NowRoom->m_JoinUser_Vector.size() != 0)
			g_BattleServer_Room_Dump->Crash();


		// 4. 기본 셋팅
		LONG RoomNo = InterlockedIncrement(&m_lGlobal_RoomNo);
		NowRoom->m_iBattleServerNo = m_iServerNo;
		NowRoom->m_iRoomNo = RoomNo;
		NowRoom->m_iRoomState = eu_ROOM_STATE::WAIT_ROOM;
		NowRoom->m_iGameModeUser = 0;
		NowRoom->m_bGameEndFlag = false;
		NowRoom->m_dwGameEndMSec = 0;
		NowRoom->m_bShutdownFlag = false;
		NowRoom->m_uiItemID = 0;
		NowRoom->m_pBattleServer = this;
		NowRoom->m_bRedZoneWarningFlag = false;

		// 10초마다 아이템 생성시킬 변수 모두 0으로 초기화
		int i = 0;
		while (i < 4)
		{
			NowRoom->m_dwItemCreateTick[i] = 0;
			++i;
		}

		// 5. 레드존 셋팅
		// 레드존 생성 순서 셋팅
		int RedIndex = rand() % 24;

		i = 0;
		while (i < 4)
		{
			NowRoom->m_arrayRedZone[i] = m_arrayRedZoneCreate[RedIndex][i];
			++i;
		}

		// 그 외 레드존 관련 셋팅
		NowRoom->m_dwReaZoneTime = 0;
		NowRoom->m_dwTick = 0;
		NowRoom->m_iRedZoneCount = 0;

		// 라스트 레드존 타입.
		// 1 ~ 4까지의 값.
		NowRoom->m_bLastRedZoneSafeType = (rand() % 4) + 1;


		// 6. 최초 안전지대 좌표 셋팅
		NowRoom->m_fSafePos[0][0] = 0;
		NowRoom->m_fSafePos[0][1] = 0;

		NowRoom->m_fSafePos[1][0] = 153;
		NowRoom->m_fSafePos[1][1] = 170;


		// 7. 방 안의 아이템 클리어시키기
		auto itor_begin = NowRoom->m_RoomItem_umap.begin();
		auto itor_end = NowRoom->m_RoomItem_umap.end();

		while (itor_begin != itor_end)
		{
			// 아이템 구조체 반환
			m_Item_Pool->Free(itor_begin->second);

			// Erase
			itor_begin = NowRoom->m_RoomItem_umap.erase(itor_begin);
		}


		// 8. 방 입장 토큰 셋팅				
		WORD Index = rand() % 64;	// 0 ~ 63 중 인덱스 골라내기				
		memcpy_s(NowRoom->m_cEnterToken, 32, m_cRoomEnterToken[Index], 32);


		// 9. 방 관리 자료구조에 추가
		if (InsertRoomFunc(RoomNo, NowRoom) == false)
			g_BattleServer_Room_Dump->Crash();


		// 10. 채팅 서버에게 [신규 대기방 생성 알림] 패킷 보냄
		// 랜 클라를 통해 보낸다.
		// 채팅 응답이 오면 마스터에게도 보낸다.
		m_Chat_LanServer->Packet_NewRoomCreate_Req(NowRoom);
	}
	   
	// HTTP 통신 후 후처리
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::AuthLoop_HTTP()
	{
		// 1. Q 사이즈 확인
		int iQSize = m_shDB_Communicate.m_pDB_ReadComplete_Queue->GetInNode();

		// 한 프레임에 최대 m_iHTTP_MAX개의 후처리.
		if (iQSize > m_stConst.m_iHTTP_MAX)
			iQSize = m_stConst.m_iHTTP_MAX;

		// 2. 있으면 로직 진행
		DB_WORK* DBData;
		while (iQSize > 0)
		{
			// 일감 디큐		
			if (m_shDB_Communicate.m_pDB_ReadComplete_Queue->Dequeue(DBData) == -1)
				g_BattleServer_Room_Dump->Crash();

			try
			{
				// 일감 타입에 따라 일감 처리
				switch (DBData->m_wWorkType)
				{
					// 로그인 패킷에 대한 인증처리
				case eu_DB_AFTER_TYPE::eu_LOGIN_AUTH:
					Auth_LoginPacket_AUTH((DB_WORK_LOGIN*)DBData);
					break;

					// 로그인 패킷에 대한 정보 얻어오기 처리
				case eu_DB_AFTER_TYPE::eu_LOGIN_INFO:
					Auth_LoginPacket_Info((DB_WORK_LOGIN_CONTENTS*)DBData);
					break;

					// 없는 일감 타입이면 에러.
				default:
					TCHAR str[200];
					StringCchPrintf(str, 200, _T("OnAuth_Update(). HTTP Type Error. Type : %d"), DBData->m_wWorkType);

					throw CException(str);
				}

			}
			catch (CException& exc)
			{
				// 로그 찍기 (로그 레벨 : 에러)
				g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
					(TCHAR*)exc.GetExceptionText());

				// 덤프
				g_BattleServer_Room_Dump->Crash();
			}

			// DB_WORK 반환
			m_shDB_Communicate.m_pDB_Work_Pool->Free(DBData);

			--iQSize;
		}
	}
	
	// 마스터 서버 혹은 채팅서버가 죽었을 경우
	// Auth 스레드에서 호출된다.
	//
	// Parameter : 없음
	// return : 없음 
	void CBattleServer_Room::AuthLoop_ServerDie()
	{
		// Wait모드의 방만 접근하기 때문에, Shared 락 가능
		AcquireSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 락

			// 방이 하나 이상 있다면
		if (m_Room_Umap.size() > 0)
		{
			auto itor_Now = m_Room_Umap.begin();
			auto itor_End = m_Room_Umap.end();

			// 방 순회
			while (itor_Now != itor_End)
			{
				// wait모드의 방인 경우
				if (itor_Now->second->m_iRoomState == eu_ROOM_STATE::WAIT_ROOM)
				{
					stRoom* NowRoom = itor_Now->second;

					// 대기방 수 감소, 레디 방 수 증가
					InterlockedDecrement(&m_lNowWaitRoomCount);
					InterlockedIncrement(&m_lReadyRoomCount);

					// 마스터에게 방 닫힘 패킷 보내기
					m_Master_LanClient->Packet_RoomClose_Req(NowRoom->m_iRoomNo);

					// 방 모드를 Ready로 변경
					NowRoom->m_iRoomState = eu_ROOM_STATE::READY_ROOM;

					// 유저가 있는 방의 경우 방 안의 모든 유저 Shutdown
					if (NowRoom->m_iJoinUserCount > 0)
						NowRoom->Shutdown_All();
				}

				++itor_Now;
			}

		}

		ReleaseSRWLockShared(&m_Room_Umap_srwl);		// ----- 룸 Shared 언락
	}
	
	// 방 상태 처리
	// 방을 game모드로 변경하기 등등..
	// Auth 스레드에서 루프마다 호출된다.
	//
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::AuthLoop_RoomLogic()
	{
		// 한 프레임에 최대 m_iLoopRoomModeChange개의 방 모드 변경
		int ModeChangeCount = m_stConst.m_iLoopRoomModeChange;
		DWORD CmpTime = timeGetTime();

		AcquireSRWLockShared(&m_Room_Umap_srwl);	// ----- Room umap Shared 락

		auto itor_Now = m_Room_Umap.begin();
		auto itor_End = m_Room_Umap.end();

		// 방 자료구조를 처음부터 끝까지 순회
		while (itor_Now != itor_End)
		{
			// 만약, 이번 프레임에, 지정한 만큼의 방 모드 변경이 발생했다면 그만한다.
			if (ModeChangeCount == 0)
				break;

			stRoom* NowRoom = itor_Now->second;

			// 해당 방이 준비방일 경우
			if (NowRoom->m_iRoomState == eu_ROOM_STATE::READY_ROOM)
			{
				// 유저 수가 0명이면 바로 삭제
				// Auth모드에서 이 로직을 타는 경우는, 
				// 1. 중간에 채팅 or 마스터 서버가 죽어서 방을 파괴해야 할 경우.
				// 2. Ready상태의 모든 유저가 나간 경우
				// Play방으로 넘겨서 자연스럽게 종료되도록 함
				if (NowRoom->m_iJoinUserCount == 0)
				{
					InterlockedDecrement(&m_lReadyRoomCount);
					InterlockedIncrement(&m_lPlayRoomCount);

					// 방 상태를 Play로 변경
					// Game스레드는 방 체크 로직중 가장 먼저 인원수가 0인지 체크한다.
					// 때문에, 넘어가면 자동으로 파괴될 것.
					NowRoom->m_iRoomState = eu_ROOM_STATE::PLAY_ROOM;

					--ModeChangeCount;
				}

				// 게임 종료된 방이 아닐 경우 로직.
				else
				{
					// 카운트 다운이 완료되었는지 체크
					if ((NowRoom->m_dwCountDown + m_stConst.m_iCountDownMSec) <= CmpTime)
					{
						// 1. 생존한 유저 수 갱신
						if (NowRoom->m_iJoinUserCount != NowRoom->m_JoinUser_Vector.size())
							g_BattleServer_Room_Dump->Crash();

						NowRoom->m_iAliveUserCount = NowRoom->m_iJoinUserCount;

						// 2. 유저가 0명이 아닐때만 로직 진행
						if (NowRoom->m_iJoinUserCount > 0)
						{
							// 룸 내 모든 유저의 생존 플래그 변경
							// false가 리턴될 수 있음. 자료구조 내에 유저가 0명일 수 있기 때문에.
							// 때문에 리턴값 안받는다.
							NowRoom->AliveFlag_True();

							// 방 안의 모든 유저에게, 배틀서버 대기방 플레이 시작 패킷 보내기
							CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

							WORD Type = en_PACKET_CS_GAME_RES_PLAY_START;

							SendBuff->PutData((char*)&Type, 2);
							SendBuff->PutData((char*)&NowRoom->m_iRoomNo, 4);

							// 여기서는 false가 리턴될 수 있음(자료구조 내에 유저가 0명일 수 있음)
							// 카운트다운이 끝나기 전에 모든 유저가 나갈 가능성.
							// 그래서 리턴값 안받는다.
							NowRoom->SendPacket_BroadCast(SendBuff);

							// 아이템 생성 후 stRoom의 아이템 자료구조에 추가.
							// 그리고 모든 유저에게 아이템 생성 패킷 보냄
							NowRoom->StartCreateItem();
						}

						InterlockedDecrement(&m_lReadyRoomCount);
						InterlockedIncrement(&m_lPlayRoomCount);

						// 3. 방 상태를 Play로 변경
						NowRoom->m_iRoomState = eu_ROOM_STATE::PLAY_ROOM;

						// 4. 모든 유저를 AUTH_TO_GAME으로 변경
						NowRoom->ModeChange();

						// 5. 이번 프레임에서 방 모드 변경 남은 카운트 감소
						--ModeChangeCount;
					}
				}
			}

			++itor_Now;
		}

		ReleaseSRWLockShared(&m_Room_Umap_srwl);	// ----- Room umap Shared 언락
	}

	// 게임모드의 중복 로그인 유저 삭제
	// Game 스레드에서 루프마다 호출된다.
	// 
	// Parameter : 없음
	// return : 없음
	void CBattleServer_Room::GameLoop_OverlapLogin()
	{
		AcquireSRWLockExclusive(&m_Overlap_list_srwl);		// forward_list 자료구조 Exclusive 락 ------- 

		// 처음부터 끝까지 순회
		auto itor_listBegin = m_Overlap_list.begin();
		auto itor_listEnd = m_Overlap_list.end();

		while (itor_listBegin != itor_listEnd)
		{
			CGameSession* NowPlayer = itor_listBegin->second;

			// 1. 모드 확인
			// Auth일 경우, 아예 이 자료구조에 안들어오며, Game이었다가 LOG_OUT이 되는 것은 OnGame_ClientLeave에서 처리하기 때문에, 
			// 여기서 Game이라는 것은 정말 Game모드 유저라는게 확정
			if (NowPlayer->m_euModeType == eu_PLATER_MODE::GAME)
			{
				// ClientKey 비교
				// 여기서 ClientKey가 다르다는 것은, 모드는 같지만 다른 유저가 됐다는 것.
				// 이 list에 들어올 당시의 유저는 이미 나간것.		
				if (NowPlayer->m_int64ClientKey == itor_listBegin->first)
				{
					// shutdown 날림
					NowPlayer->Disconnect();
				}
			}

			// 2. itor_listBegin을 Next로 이동
			// 그리고 m_Overlap_list의 가장 앞 데이터(금방 확인한 데이터)를 pop 한다
			// 다른 유저였더라도 list에서는 제거되어야 함.
			itor_listBegin = next(itor_listBegin);
			m_Overlap_list.pop_front();
		}

		ReleaseSRWLockExclusive(&m_Overlap_list_srwl);		// forward_list 자료구조 Exclusive 언락 ------- 

	}

	// 게임 모드의 방 체크
	// Game 스레드에서 루프마다 호출된다.
	//
	// Parameter : (out)int형 배열(삭제할 룸 번호 보관용), (out)삭제할 룸의 수
	void CBattleServer_Room::GameLoop_RoomLogic(int DeleteRoomNo[], int* Index)
	{
		AcquireSRWLockShared(&m_Room_Umap_srwl);	// ----- Room Umap Shared 락

		auto itor_Now = m_Room_Umap.begin();
		auto itor_End = m_Room_Umap.end();

		// Step 1. 삭제될 방 체크
		// Step 2. 게임 종료된 방 처리 	
		// Setp 3. 게임 종료 체크
		// Step 4. 생존자가 0명인 경우에도 게임 종료.
		// Step 5. 정상적으로 플레이 중인 방의 경우 처리.

		// 모든 방을 순회하며, PLAY 모드인 방에 한해 작업 진행
		while (itor_Now != itor_End)
		{
			// 방 모드 체크. 
			if (itor_Now->second->m_iRoomState == eu_ROOM_STATE::PLAY_ROOM)
			{
				// 여기까지 오면 게임 스레드만 접근하는 방.
				stRoom* NowRoom = itor_Now->second;

				// Step 1. 삭제될 방 체크
				// 접속한 인원 수가 0명인 방. (생존 수 아님)
				// 게임 종료 후, 모든 유저를 쫒아내는 작업까지 완료된 방.
				if (NowRoom->m_iJoinUserCount == 0)
				{
					if ((*Index) < 100)
					{
						DeleteRoomNo[(*Index)] = NowRoom->m_iRoomNo;
						++(*Index);
					}
				}

				// Step 2. 게임 종료된 방 처리 
				else if (NowRoom->m_bGameEndFlag == true)
				{
					// 셧다운을 하지 않았을 경우에만 로직 진행.
					// 이미 이전 프레임에 셧다운을 날렸는데, 아직 모든 유저가 종료되지 않아
					// 또 이 로직을 탈 가능성이 있기 때문에.
					if (NowRoom->m_bShutdownFlag == false)
					{
						// 삭제 시간이 되었는지 체크
						if ((timeGetTime() - NowRoom->m_dwGameEndMSec) >= m_stConst.m_iRoomCloseDelay)
						{
							// 셧다운 날렸다는 플래그 변경.
							NowRoom->m_bShutdownFlag = true;

							// 아직 방에 있는 유저에게 Shutdown
							NowRoom->Shutdown_All();
						}
					}
				}

				// Setp 3. 게임 종료 체크
				// 생존자가 1명이면서 게임모드의 유저 수와 생존자 수가 동일할 경우, 게임 종료.			
				else if (NowRoom->m_iAliveUserCount == 1)
				{
					// !! Auth 모드에서 1명을 제외한 모든 유저가 나간 후에 !!
					// !! 남은 1명이 Game모드로 변경되기 전에 OnGame_Update에서 해당 로직이 먼저 체크할 경우 !!
					// !! 플레이 카운트가 저장되지 않을 수도 있다. !!
					// !! 때문에, Game모드 유저 수와 AliveUser수가 동일한지 꼭 확인한다 !!
					if (NowRoom->m_iAliveUserCount == NowRoom->m_iGameModeUser)
					{
						// 게임 종료 패킷 보내기
						// - 생존자 1명에겐 승리 패킷
						// - 나머지 접속자들에겐 패배 패킷
						NowRoom->GameOver();

						// 방 내 승리자에게 전적 보내기
						NowRoom->WInRecodeSend();

						// 게임 방 종료 플래그 변경
						NowRoom->m_bGameEndFlag = true;

						// 현 시점의 시간 저장
						// 방 파괴 체크 용도
						NowRoom->m_dwGameEndMSec = timeGetTime();
					}
				}

				// Step 4. 생존자가 0명인 경우에도 게임 종료.
				// 방 내의 생존자들이 같은 프레임에 hp가 0이되어 죽을 경우.
				else if (NowRoom->m_iAliveUserCount == 0)
				{
					if (NowRoom->m_iGameModeUser != 0)
						g_BattleServer_Room_Dump->Crash();

					// 게임 종료 패킷 보내기
					// - 모든 유저에게 패배 패킷 보냄. 승리자 없음
					CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

					WORD Type = en_PACKET_CS_GAME_RES_GAMEOVER;

					SendBuff->PutData((char*)&Type, 2);

					NowRoom->SendPacket_BroadCast(SendBuff);

					// 방 내 승리자 없음
					// NowRoom->WInRecodeSend();

					// 게임 방 종료 플래그 변경
					NowRoom->m_bGameEndFlag = true;

					// 현 시점의 시간 저장
					// 방 파괴 체크 용도
					NowRoom->m_dwGameEndMSec = timeGetTime();
				}

				// Step 5. 정상적으로 플레이 중인 방의 경우
				else
				{
					// 현재 시간 구해둠
					DWORD NowTime = timeGetTime();

					// -- 레드존 관련 체크
					// 현재 게임이 시작된 방인지 체크
					// 룸의 상태만 PLAY로 넘어오고, 아직 모든 유저가 Game모드로 넘어오지 않았을 경우에도
					// 이 로직을 탈 수 있으니, 정말로 모든 유저가 Game모드로 넘어와서 게임이 시작했는지 체크
					if (NowRoom->m_dwReaZoneTime > 0)
					{
						// -- 레드존이 활성화 되어도 되는지 체크.
						// 이미, 모든 레드존이 활성화 되었다면, 더 이상 활성화되면 안됨.
						if (NowRoom->m_iRedZoneCount < m_stConst.m_iRedZoneActiveLimit)
						{
							// -- 레드존 경고 체크						
							if (NowRoom->m_bRedZoneWarningFlag == false)
							{
								// 레드존 경고를 안보낸 상태라면, 시간 체크 후 경고패킷 보냄
								if ((NowTime - NowRoom->m_dwReaZoneTime) >= m_stConst.m_dwRedZoneWarningTime)
								{
									// 레드존 경고 함수 호출
									NowRoom->RedZone_Warning((BYTE)(m_stConst.m_dwRedZoneWarningTime / 1000));

									// 레드존 경고 보냄 플래그를 true로 만든다.
									NowRoom->m_bRedZoneWarningFlag = true;
								}
							}

							// -- 레드존 알람을 보낸 상태라면 레드존 활성화 체크
							else
							{
								// -- 레드존 활성화 체크
								if ((NowTime - NowRoom->m_dwReaZoneTime) >= m_stConst.m_dwReaZoneActiveTime)
								{
									// 레드존 활성화 함수 호출
									NowRoom->RedZone_Active();

									// 레드존 활성화 시간 갱신. 다시 40초를 기다려야함.
									NowRoom->m_dwReaZoneTime = NowTime;

									// 레드존 경고 보냄 플래그 원복
									NowRoom->m_bRedZoneWarningFlag = false;

								}
							}
						}

						// -- 레드존 데미지 체크						
						// 활성화된 레드존이 하나라도 있는 경우에만 해당 로직 진행
						if (NowRoom->m_iRedZoneCount > 0)
						{
							// 1초가 되었나 체크. 데미지는 1초단위로 체크하기 때문에
							if ((NowTime - NowRoom->m_dwTick) >= 1000)
							{
								// 레드존 틱 데미지 체크 함수 호출
								NowRoom->RedZone_Damage();

								// tick 갱신
								NowRoom->m_dwTick = NowTime;
							}
						}
					}

					// -- 룸 내의 정기 아이템 생성 체크
					// 레드존 외 구역에 생성된 아이템은(총 4개 구역) 누군가가 획득 후 10초 후에 재생성되어야 한다.
					// 이걸 안하면, 더미 테스트 시 게임이 끝나지 않는다.
					int i = 0;
					while (i < 4)
					{
						// 해당 위치에 아이템이 없을 경우 아래 로직
						if (NowRoom->m_dwItemCreateTick[i] > 0)
						{
							// 현재 시간이, 해당 위치의 아이템이 소멸된 시간으로 부터 10초가 되었는지
							if ((NowTime - NowRoom->m_dwItemCreateTick[i]) >= 10000)
							{
								// 되었다면, 아이템 생성되었으니 시간을 0으로 초기화
								NowRoom->m_dwItemCreateTick[i] = 0;

								// 10초가 되었다면, 해당 위치에 탄창 아이템 생성
								NowRoom->CreateItem(g_Data_ItemPoint_Playzone[i][0],
									g_Data_ItemPoint_Playzone[i][1], eu_ITEM_TYPE::CARTRIDGE, 2);
							}
						}

						++i;
					}
				}
			}

			++itor_Now;
		}

		ReleaseSRWLockShared(&m_Room_Umap_srwl);	// ----- Room Umap Shared 언락
	}

	// 게임 모드의 방 삭제
	// Game 스레드에서 루프마다 호출된다.
	//
	// Parameter : (out)int형 배열(삭제할 룸 번호 보관용), (out)삭제할 룸의 수
	// return : 없음
	void CBattleServer_Room::GameLoop_RoomDelete(int DeleteRoomNo[], int* Index)
	{
		int TempIndex = *Index;

		// Step 1. 방 삭제
		// Step 2. 채팅서버로 방 삭제 패킷 보내기

		// Index가 0보다 크다면, 삭제할 방이 있는 것. 		
		if (TempIndex > 0)
		{
			// Step 1. 방 삭제
			int Index_B = 0;

			AcquireSRWLockExclusive(&m_Room_Umap_srwl);	// ----- Room Umap Exclusive 락		

			while (Index_B < TempIndex)
			{
				// 1. 검색
				auto FindRoom = m_Room_Umap.find(DeleteRoomNo[Index_B]);

				// 여기서 없으면 안됨. 방 삭제는 이 로직에서 밖에 안하기 때문에
				if (FindRoom == m_Room_Umap.end())
					g_BattleServer_Room_Dump->Crash();

				// 2. stRoom* 해제
				m_Room_Pool->Free(FindRoom->second);

				// 3. Erase
				m_Room_Umap.erase(FindRoom);

				InterlockedDecrement(&m_lPlayRoomCount);
				InterlockedDecrement(&m_lNowTotalRoomCount);

				++Index_B;
			}

			ReleaseSRWLockExclusive(&m_Room_Umap_srwl);	// ----- Room Umap Exclusive 언락


			// Step 2. 채팅서버로 방 삭제 패킷 보내기
			// 채팅서버와 연결되어 있을 경우
			if (m_Chat_LanServer->m_bLoginCheck == true)
			{
				Index_B = 0;

				// 채팅서버 세션 ID
				ULONGLONG ChatSessionID = m_Chat_LanServer->m_ullSessionID;

				while (Index_B < TempIndex)
				{
					UINT ReqSequence = InterlockedIncrement(&m_Chat_LanServer->m_uiReqSequence);

					// 패킷 만들기
					CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

					WORD Type = en_PACKET_CHAT_BAT_REQ_DESTROY_ROOM;

					SendBuff->PutData((char*)&Type, 2);
					SendBuff->PutData((char*)&m_iServerNo, 4);
					SendBuff->PutData((char*)&DeleteRoomNo[Index_B], 4);
					SendBuff->PutData((char*)&ReqSequence, 4);

					// 패킷 보내기
					m_Chat_LanServer->SendPacket(ChatSessionID, SendBuff);

					++Index_B;
				}
			}

		}

	}


	// -----------------------
	// 패킷 후처리 함수
	// -----------------------

	// Login 패킷에 대한 인증 처리 (토큰 체크 등..)
	//
	// Parameter : DB_WORK_LOGIN*
	// return : 없음
	void CBattleServer_Room::Auth_LoginPacket_AUTH(DB_WORK_LOGIN* DBData)
	{
		// 1. ClientKey 체크
		CGameSession* NowPlayer = (CGameSession*)DBData->pPointer;

		// 다르면 이미 종료한 유저로 판단. 가능성 있는 상황
		if (NowPlayer->m_int64ClientKey != DBData->m_i64UniqueKey)
			return;


		// 2. Json데이터 파싱하기 (UTF-16)
		GenericDocument<UTF16<>> Doc;
		Doc.Parse(DBData->m_tcResponse);

		int iResult = Doc[_T("result")].GetInt();


		// 3. DB 요청 결과 확인
		// 결과가 1이 아니라면, 
		if (iResult != 1)
		{
			// 이미 종료 패킷이 간 유저인지 체크
			if (NowPlayer->m_bLogoutFlag == true)
				return;

			// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
			NowPlayer->m_bLogoutFlag = true;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_AUTH()--> DB Result Error!! AccoutnNo : %lld, Error : %d", NowPlayer->m_Int64AccountNo, iResult);

			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 2;			

			// 결과가 -10일 경우 (회원가입 자체가 안되어 있음)
			if (iResult == -10)
				InterlockedIncrement(&m_lQuery_Result_Not_Find);

			// 그 외 기타 에러일 경우
			else
				InterlockedIncrement(&m_lTempError);

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			// 보내고 끊기
			NowPlayer->SendPacket(SendBuff, TRUE);
			return;
		}

		// 4. 토큰키 비교
		const TCHAR* tDBToekn = Doc[_T("sessionkey")].GetString();

		char DBToken[64];
		int len = (int)_tcslen(tDBToekn);
		WideCharToMultiByte(CP_UTF8, 0, tDBToekn, (int)_tcslen(tDBToekn), DBToken, len, NULL, NULL);

		if (memcmp(DBToken, NowPlayer->m_cSessionKey, 64) != 0)
		{
			// 이미 종료 패킷이 간 유저인지 체크
			if (NowPlayer->m_bLogoutFlag == true)
				return;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_AUTH()--> SessionKey Error!! AccoutnNo : %lld", NowPlayer->m_Int64AccountNo);

			// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
			NowPlayer->m_bLogoutFlag = true;

			// 토큰이 다를경우 Result 3(세션키 오류)를 보낸다.
			InterlockedIncrement(&m_lTokenError);

			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 3;

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			// 보내고 끊기
			NowPlayer->SendPacket(SendBuff, TRUE);
			return;
		}
		

		// 5. 닉네임 복사
		const TCHAR* TempNick = Doc[_T("nick")].GetString();
		ZeroMemory(&NowPlayer->m_tcNickName, sizeof(NowPlayer->m_tcNickName));
		StringCchCopy(NowPlayer->m_tcNickName, _Mycountof(NowPlayer->m_tcNickName), TempNick);


		// 6. Contents 정보도 셋팅됐다면 정상 패킷 보냄
		NowPlayer->m_lLoginHTTPCount++;

		if (NowPlayer->m_lLoginHTTPCount == 2)
		{
			// 로그인 패킷 처리 Flag 변경
			NowPlayer->m_bLoginFlag = true;

			NowPlayer->m_euDebugMode = eu_USER_TYPE_DEBUG::LOGIN_OK;

			// 정상 응답 패킷 보냄
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 1;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
		}
	}

	// Login 패킷에 대한 Contents 정보 가져오기
	//
	// Parameter : DB_WORK_LOGIN*
	// return : 없음
	void CBattleServer_Room::Auth_LoginPacket_Info(DB_WORK_LOGIN_CONTENTS* DBData)
	{
		// 1. ClientKey 체크
		CGameSession* NowPlayer = (CGameSession*)DBData->pPointer;

		// 다르면 이미 종료한 유저로 판단. 가능성 있는 상황
		if (NowPlayer->m_int64ClientKey != DBData->m_i64UniqueKey)
			return;

		// 2. Json데이터 파싱하기 (UTF-16)
		GenericDocument<UTF16<>> Doc;
		Doc.Parse(DBData->m_tcResponse);

		int iResult = Doc[_T("result")].GetInt();


		// 3. DB 요청 결과 확인
		// 결과가 1이 아니라면, 
		if (iResult != 1)
		{
			// 이미 종료 패킷이 간 유저인지 체크
			if (NowPlayer->m_bLogoutFlag == true)
				return;

			// 에러 로그 찍기
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Auth_LoginPacket_Info()--> DB Result Error!! AccoutnNo : %lld, Error : %d", NowPlayer->m_Int64AccountNo, iResult);

			// 다른 HTTP 요청이 종료 패킷을 또 보내지 못하도록 플래그 변경.
			NowPlayer->m_bLogoutFlag = true;

			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 2;

			// 결과가 -10일 경우 (회원가입 자체가 안되어 있음)
			if (iResult == -10)
				InterlockedIncrement(&m_lQuery_Result_Not_Find);

			// 그 외 기타 에러일 경우
			else
				InterlockedIncrement(&m_lTempError);

			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			// 보내고 끊기
			NowPlayer->SendPacket(SendBuff, TRUE);
			return;
		}

		// 4. 여기까지 오면 정상으로 패킷 찾아온 것.

		// 정보 복사
		NowPlayer->m_iRecord_PlayCount = Doc[_T("playcount")].GetInt();	// 플레이 횟수
		NowPlayer->m_iRecord_PlayTime = Doc[_T("playtime")].GetInt();	// 플레이 시간 초단위
		NowPlayer->m_iRecord_Kill = Doc[_T("kill")].GetInt();			// 죽인 횟수
		NowPlayer->m_iRecord_Die = Doc[_T("die")].GetInt();				// 죽은 횟수
		NowPlayer->m_iRecord_Win = Doc[_T("win")].GetInt();				// 최종승리 횟수

		// 5. 인증용 로그인 HTTP 요청이 완료되었다면 정상 패킷 보냄
		NowPlayer->m_lLoginHTTPCount++;

		if (NowPlayer->m_lLoginHTTPCount == 2)
		{
			// 로그인 패킷 처리 Flag 변경
			NowPlayer->m_bLoginFlag = true;

			NowPlayer->m_euDebugMode = eu_USER_TYPE_DEBUG::LOGIN_OK;

			// 정상 응답 패킷 보냄
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			WORD Type = en_PACKET_CS_GAME_RES_LOGIN;
			BYTE Result = 1;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&DBData->AccountNo, 8);
			SendBuff->PutData((char*)&Result, 1);

			NowPlayer->SendPacket(SendBuff);
		}

	}
	   





	// -----------------------
	// AccountNo 자료구조 관리 함수
	// -----------------------

	// AccountNo 자료구조에 유저를 추가하는 함수
	//
	// Parameter : AccountNo, CGameSession*
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CBattleServer_Room::InsertAccountNoFunc(INT64 AccountNo, CGameSession* InsertPlayer)
	{

		AcquireSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 락

		// 1. Insert
		auto ret = m_AccountNo_Umap.insert(make_pair(AccountNo, InsertPlayer));

		ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 언락

		// 2. 중복키일 시 false 리턴
		if (ret.second == false)
			return false;

		return true;
	}

	// AccountNo 자료구조에서 유저 검색 후, 해당 유저에게 Disconenct 하는 함수
	//
	// Parameter : AccountNo
	// return : 없음
	void CBattleServer_Room::DisconnectAccountNoFunc(INT64 AccountNo)
	{
		AcquireSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 락

		// 1. 검색
		auto ret = m_AccountNo_Umap.find(AccountNo);

		// 2. 없는 유저일 시 그냥 리턴.
		// 여기 락 걸고 들어오기 전에, 이미 유저가 종료했을 수도 있기 때문에
		// 정상적인 상황으로 판단.
		if (ret == m_AccountNo_Umap.end())
		{
			ReleaseSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락
			return;
		}

		// 3. Auth라면 즉시 Disconnect
		if (ret->second->m_euModeType == eu_PLATER_MODE::AUTH)
			ret->second->Disconnect();

		// Auth가 아니라면, Game모드이거나 LogOut이니, Game으로 이관시킨다.
		else
			InsertOverlapFunc(ret->second->m_int64ClientKey, ret->second);

		ReleaseSRWLockShared(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락
	}


	// AccountNo 자료구조에서 유저를 제거하는 함수
	//
	// Parameter : AccountNo
	// return : 성공 시 true
	//		  : 실패 시 false
	bool CBattleServer_Room::EraseAccountNoFunc(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 락

		// 1. 검색
		auto ret = m_AccountNo_Umap.find(AccountNo);

		// 2. 없는 유저일 시 false 리턴
		if (ret == m_AccountNo_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Exclusive 언락
			return false;
		}

		// 3. 있는 유저라면 Erase
		m_AccountNo_Umap.erase(ret);

		ReleaseSRWLockExclusive(&m_AccountNo_Umap_srwl);		// AccountNo uamp Shared 언락

		return true;
	}







	// -----------------------
	// DBWrite 카운트 자료구조 관리 함수
	// -----------------------

	// DBWrite 카운트 관리 자료구조에 유저를 추가하는 함수
	//
	// Parameter : AccountNo
	// return : 성공 시 true
	//		  : 실패(키 중복) 시 false
	bool CBattleServer_Room::InsertDBWriteCountFunc(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 락

		// 1. 추가
		auto ret = m_DBWrite_Umap.insert(make_pair(AccountNo, 1));

		// 추가 실패 시 (중복 키) return false
		if (ret.second == false)
		{
			ReleaseSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 언락
			return false;
		}

		ReleaseSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 언락
		return true;
	}

	// DBWrite 카운트 관리 자료구조에서 유저를 검색 후, 
	// 카운트(Value)를 1 증가하는 함수
	//
	// Parameter : AccountNo
	// return : 없음
	void CBattleServer_Room::AddDBWriteCountFunc(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 락

		// 1. 검색
		auto ret = m_DBWrite_Umap.find(AccountNo);

		// 없으면 크래시
		if (ret == m_DBWrite_Umap.end())
			g_BattleServer_Room_Dump->Crash();

	
		// 2. Value의 값 1 증가
		++ret->second;

		ReleaseSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 언락
	}

	// DBWrite 카운트 관리 자료구조에서 유저를 검색 후, 
	// 카운트(Value)를 1 감소시키는 함수
	// 감소 후 0이되면 Erase한다.
	//
	// Parameter : AccountNo
	// return : 성공 시 true
	//		  : 검색 실패 시 false
	bool CBattleServer_Room::MinDBWriteCountFunc(INT64 AccountNo)
	{
		AcquireSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 락

		// 1. 검색
		auto ret = m_DBWrite_Umap.find(AccountNo);

		// 없으면 return false
		if (ret == m_DBWrite_Umap.end())
		{
			ReleaseSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 언락
			return false;
		}

		// 이미 Value가 0이면 크래시
		if(ret->second == 0)
			g_BattleServer_Room_Dump->Crash();


		// 2. Value의 값 1 감소
		--ret->second;


		// 3. 감소 후 0이면 Erase
		if (ret->second == 0)
			m_DBWrite_Umap.erase(ret);

		ReleaseSRWLockExclusive(&m_DBWrite_Umap_srwl);		// ----- DBWrite Umap Exclusive 언락

		return true;
	}








	// ---------------------------------
	// Auth모드의 방 관리 자료구조 변수
	// ---------------------------------

	// 방을 Room 자료구조에 Insert하는 함수
	//
	// Parameter : RoomNo, stRoom*
	// return : 성공 시 true
	//		  : 실패(중복 키) 시 false	
	bool CBattleServer_Room::InsertRoomFunc(int RoomNo, stRoom* InsertRoom)
	{
		AcquireSRWLockExclusive(&m_Room_Umap_srwl);		// ----- Room 자료구조 Exclusive 락 

		// 1. insert
		auto ret = m_Room_Umap.insert(make_pair(RoomNo, InsertRoom));

		ReleaseSRWLockExclusive(&m_Room_Umap_srwl);		// ----- Room 자료구조 Exclusive 언락 

		// 2. 중복키라면 false 리턴
		if (ret.second == false)
			return false;

		return true;
	}




	// -----------------------
	// 중복로그인 자료구조(list) 관리 함수
	// -----------------------

	// 중복로그인 자료구조(list)에 Insert
	//
	// Parameter : ClientKey, CGameSession*
	// return : 없음
	void CBattleServer_Room::InsertOverlapFunc(INT64 ClientKey, CGameSession* InsertPlayer)
	{		
		AcquireSRWLockExclusive(&m_Overlap_list_srwl);		// Exclusive 락	-----------------

		// Push
		m_Overlap_list.push_front(make_pair(ClientKey, InsertPlayer));

		ReleaseSRWLockExclusive(&m_Overlap_list_srwl);		// Exclusive 락 해제-------------
	}





	// -----------------------
	// 가상함수
	// -----------------------

	// AuthThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnAuth_Update()
	{
		// ------------------- HTTP 통신 후, 후처리
		AuthLoop_HTTP();


		// ------------------- 방 상태 처리
		AuthLoop_RoomLogic();		


		// ------------------- 방생성
		// !! 대기방 생성 조건 !!
		// - [현재 게임 내에 만들어진 총 방 수 < 최대 존재할 수 있는 방 수] 만족
		// - [현재 대기방 수 < 최대 존재할 수 있는 대기방 수] 만족

		// 마스터 서버와 로그인, 채팅 랜 클라가 로그인 상태일 때만 보낸다.
		// 일단, 마스터에게 연결되어 있어야 배틀서버 No를 받는다.
		// 그리고 채팅서버에게도 전달이 된 후에야 정상적으로 방이 생성되었다고 볼 수 있다.
		// 즉, 방 생성 가능한 시점이 되어야만 생성.
		if (m_Master_LanClient->m_bLoginCheck == true &&
			m_Chat_LanServer->m_bLoginCheck == true)
		{
			// 한 프레임에 최대 m_iLoopCreateRoomCount개의 방 생성
			int LoopCount = m_stConst.m_iLoopCreateRoomCount;

			while (LoopCount > 0)
			{
				// 1. 현재 만들어진 총 방수 체크
				if (m_lNowTotalRoomCount < m_stConst.m_lMaxTotalRoomCount)
				{
					// 2. 현재 대기방 수 체크
					if (m_lNowWaitRoomCount < m_stConst.m_lMaxWaitRoomCount)
					{
						// 여기까지 오면 방생성 조건 만족	
						// 방 생성 함수 호출
						RoomCreate();	
						--LoopCount;
					}

					// 현재 대기방 수가 이미 max라면 방 안만들고 나간다.
					else
						break;
				}

				// 현재 만들어진 총 방수가 이미 max라면 방 안만들고 나간다.
				else
					break;
			}
		}
		

		// -------------------- 마스터 서버 혹은 채팅 서버가 죽었을 경우

		// Wait상태의 방을 파괴한다.
		//
		// !! 마스터 서버에게 방 파괴 패킷을 보내는 시점 : 아래 로직 !!
		// !! 채팅 서버에게 방 파괴 패킷을 보내는 시점 : Play에서 방이 파괴될 때. !!
		//
		// Play는 게임스레드에서 접근하며, 해당 방의 인원이 0명이 되면 방을 삭제한다.
		else if (m_Master_LanClient->m_bLoginCheck == false ||
			m_Chat_LanServer->m_bLoginCheck == false)
		{
			AuthLoop_ServerDie();
		}	


		// ------------------- 토큰 재발급
		// 채팅 랜 서버를 통해 채팅 서버로 토큰 전달.
		// 이게 완료되면, 알아서 마스터 서버에게도 토큰이 전달된다.
		m_Chat_LanServer->Packet_TokenChange_Req();
	}

	// GameThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnGame_Update()
	{
		// ------------------- 게임 모드의 중복 로그인 유저 삭제
		GameLoop_OverlapLogin();
		

		// ------------------- 방 로직

		// 이번 프레임에 삭제될 방 받아두기.
		// 한 프레임에 최대 100개의 방 삭제 가능
		int DeleteRoomNo[100];
		int Index = 0;

		GameLoop_RoomLogic(DeleteRoomNo, &Index);


		// ------------------- 방 삭제
		GameLoop_RoomDelete(DeleteRoomNo, &Index);
	}

	// 새로운 유저 접속 시, Auth에서 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CBattleServer_Room::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattleServer_Room::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CBattleServer_Room::OnError(int error, const TCHAR* errorStr)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s (ErrorCode : %d)",
			errorStr, error);

		// 이후, 엔진에서 접속 끊김.
	}

	   	  

	// -----------------
	// 생성자와 소멸자
	// -----------------
	CBattleServer_Room::CBattleServer_Room()
		:CMMOServer()
	{	
		srand((UINT)time(NULL));	

		// 방 입장 토큰 만들어두기
		for (int i = 0; i < 64; ++i)
		{
			for (int h = 0; h < 32; ++h)
			{
				m_cRoomEnterToken[i][h] = (rand() % 128) + 1;
			}
		}	

		// 배틀서버 최초 입장 토큰 만들어 두기
		for (int i = 0; i < 32; ++i)
		{
			m_cConnectToken_Now[i] = (rand() % 128) + 1;
		}

		// 모니터링 서버와 통신하기 위한 LanClient 동적할당
		m_Monitor_LanClient = new CGame_MinitorClient;
		m_Monitor_LanClient->ParentSet(this);

		// 마스터 서버와 통신하기 위한 LanClient 동적할당
		m_Master_LanClient = new CBattle_Master_LanClient;
		m_Master_LanClient->ParentSet(this);

		// 채팅서버와 연결되는 LanServer 동적할당
		m_Chat_LanServer = new CBattle_Chat_LanServer;
		m_Chat_LanServer->SetMasterClient(m_Master_LanClient, this);


		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			g_BattleServer_Room_Dump->Crash();

		// ------------------- 로그 저장할 파일 셋팅
		g_BattleServer_RoomLog->SetDirectory(L"CBattleServer_Room");
		g_BattleServer_RoomLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);
				


		// TLS 동적할당
		m_Room_Pool = new CMemoryPoolTLS<stRoom>(0, false);	
		m_Item_Pool = new CMemoryPoolTLS<stRoomItem>(0, false);

		// reserve 셋팅
		m_AccountNo_Umap.reserve(m_stConfig.MaxJoinUser);
		m_Room_Umap.reserve(m_stConst.m_lMaxTotalRoomCount);
		m_DBWrite_Umap.reserve(m_stConfig.MaxJoinUser);

		//SRW락 초기화
		InitializeSRWLock(&m_AccountNo_Umap_srwl);
		InitializeSRWLock(&m_Room_Umap_srwl);
		InitializeSRWLock(&m_DBWrite_Umap_srwl);
		InitializeSRWLock(&m_Overlap_list_srwl);
		InitializeSRWLock(&m_ServerEnterToken_srwl);

		// 게임 내에서 사용되는, 거리 1당 데미지.
		m_fFire1_Damage = g_Data_HitDamage / (float)15;	// 총알 데미지


		// next_permutation을 이용해 레드존 생성 순서를 미리 만들어둠.
		// 방생성 시, 이것 중 하나를 랜덤으로 뽑아서 방에게 할당해준다.
		vector<int> v(4);

		for (int i = 0; i < 4; ++i)
			v[i] = i;

		int h = 0;
		do 
		{
			for (int i = 0; i < 4; i++)
				m_arrayRedZoneCreate[h][i] = v[i];

			h++;

		} while (next_permutation(v.begin(), v.end()));

		// 레드존 타입에 따른 활성 위치 미리 정해줌.
		m_arrayRedZoneRange[0][0][0] = 0;	// Left x1
		m_arrayRedZoneRange[0][0][1] = 0;	// Left y1
		m_arrayRedZoneRange[0][1][0] = 153;	// Left x2
		m_arrayRedZoneRange[0][1][1] = 50;	// Left y2

		m_arrayRedZoneRange[1][0][0] = 0;	// Right x1
		m_arrayRedZoneRange[1][0][1] = 115;	// Right y1
		m_arrayRedZoneRange[1][1][0] = 153;	// Right x2
		m_arrayRedZoneRange[1][1][1] = 170;	// Right y2

		m_arrayRedZoneRange[2][0][0] = 0;	// Top x1
		m_arrayRedZoneRange[2][0][1] = 0;	// Top y1
		m_arrayRedZoneRange[2][1][0] = 44;	// Top x2
		m_arrayRedZoneRange[2][1][1] = 170;	// Top y2

		m_arrayRedZoneRange[3][0][0] = 102;	// bottom x1
		m_arrayRedZoneRange[3][0][1] = 0;	// bottom y1
		m_arrayRedZoneRange[3][1][0] = 153;	// bottom x2
		m_arrayRedZoneRange[3][1][1] = 170;	// bottom y2

		// 마지막 레드존 활성 시 안전지대
		m_arrayLastRedZoneSafeRange[0][0][0] = 47;	//Level1 안전구역 x1
		m_arrayLastRedZoneSafeRange[0][0][1] = 51;	//Level1 안전구역 y1
		m_arrayLastRedZoneSafeRange[0][1][0] = 75;	//Level1 안전구역 x2
		m_arrayLastRedZoneSafeRange[0][1][1] = 84;	//Level1 안전구역 y2

		m_arrayLastRedZoneSafeRange[1][0][0] = 47;	//Level2 안전구역 x1
		m_arrayLastRedZoneSafeRange[1][0][1] = 82;	//Level2 안전구역 y1
		m_arrayLastRedZoneSafeRange[1][1][0] = 75;	//Level2 안전구역 x2
		m_arrayLastRedZoneSafeRange[1][1][1] = 112;	//Level2 안전구역 y2

		m_arrayLastRedZoneSafeRange[2][0][0] = 76;	//Level3 안전구역 x1
		m_arrayLastRedZoneSafeRange[2][0][1] = 51;	//Level3 안전구역 y1
		m_arrayLastRedZoneSafeRange[2][1][0] = 101;	//Level3 안전구역 x2
		m_arrayLastRedZoneSafeRange[2][1][1] = 85;	//Level3 안전구역 y2

		m_arrayLastRedZoneSafeRange[3][0][0] = 74;	//Level4 안전구역 x1
		m_arrayLastRedZoneSafeRange[3][0][1] = 84;	//Level4 안전구역 y1
		m_arrayLastRedZoneSafeRange[3][1][0] = 100;	//Level4 안전구역 x2
		m_arrayLastRedZoneSafeRange[3][1][1] = 114;	//Level4 안전구역 y2
	}

	CBattleServer_Room::~CBattleServer_Room()
	{
		// 모니터링 서버와 통신하는 LanClient 동적해제
		delete m_Monitor_LanClient;

		// 마스터 서버와 통신하는 LanClient 동적 해제
		delete m_Master_LanClient;

		// TLS 동적 해제
		delete m_Room_Pool;
		delete m_Item_Pool;
	}

}

// ----------------------------------------
// 
// 마스터 서버와 연결되는 LanClient
//
// ----------------------------------------
namespace Library_Jingyu
{

	// -----------------------
	// 패킷 처리 함수
	// -----------------------

	// 마스터에게 보낸 로그인 요청 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_Login_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. Connect 상태인지 확인
		if (SessionID != m_ullSessionID)
			g_BattleServer_Room_Dump->Crash();

		if(m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		if (m_bLoginCheck == true)
			g_BattleServer_Room_Dump->Crash();

		// 2. 마샬링
		int ServerNo;
		Payload->GetData((char*)&ServerNo, 4);


		// 3. 서버 번호 셋팅
		m_BattleServer_this->m_iServerNo = ServerNo;	


		// 4. 최초 로그인이기 때문에 배틀서버 입장 토큰 보내기
		UINT ReqSequence = uiReqSequence;

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_BAT_MAS_REQ_CONNECT_TOKEN;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)m_BattleServer_this->m_cConnectToken_Now, 32);
		SendBuff->PutData((char*)&uiReqSequence, 4);

		SendPacket(SessionID, SendBuff);

		
		// 5. 기존에 방이 있었다면, 마스터 서버로 방 생성 패킷 보냄. (지금은, 연결 끊기면 모두 파괴중.)
		// Shared로 접근해도 안전한 이유!
		// 1. OnAuth_Update의 방 상태 체크 부분은, Ready상태의 방만 접근하기 때문에 동일한 방 접근 가능성 X
		// 2. 마찬가지로 OnGame_Update는 Play상태의 방만 접근.
		// 3. 클라이언트가 방 입장 패킷을 배틀로 보내기 위해서는, 해당 방이 마스터에 있어야 함. 
		// 근데, 이 패킷이 마스터에게 방을 보내는 로직이기 때문에, 여기서 패킷을 보내기 전에 방은 없음.
		/*
		AcquireSRWLockShared(&m_BattleServer_this->m_Room_Umap_srwl);	// ----- 룸 Shared 락

		// 방이 존재한다면
		if (m_BattleServer_this->m_Room_Umap.size() > 0)
		{
			auto itor_Now = m_BattleServer_this->m_Room_Umap.begin();
			auto itor_End = m_BattleServer_this->m_Room_Umap.end();

			WORD Type = en_PACKET_BAT_MAS_REQ_CREATED_ROOM;

			// 순회
			while (itor_Now != itor_End)
			{
				// 대기 상태의 방이라면, 마스터에게 방 생성 패킷 보냄
				if (itor_Now->second->m_iRoomState == CBattleServer_Room::eu_ROOM_STATE::WAIT_ROOM)
				{
					CBattleServer_Room::stRoom* NowRoom = itor_Now->second;

					UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

					// 패킷 만들기
					CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

					SendBuff->PutData((char*)&Type, 2);
					SendBuff->PutData((char*)&NowRoom->m_iBattleServerNo, 4);
					SendBuff->PutData((char*)&NowRoom->m_iRoomNo, 4);
					SendBuff->PutData((char*)&NowRoom->m_iMaxJoinCount, 4);
					SendBuff->PutData((char*)NowRoom->m_cEnterToken, 32);
					SendBuff->PutData((char*)&ReqSequence, 4);

					// 패킷 보내기
					SendPacket(SessionID, SendBuff);
				}

				++itor_Now;
			}
		}

		ReleaseSRWLockShared(&m_BattleServer_this->m_Room_Umap_srwl);	// ----- 룸 Shared 언락
		*/


		// 6. 로그인 상태로 변경
		m_bLoginCheck = true;
	}

	// 신규 대기방 생성 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_NewRoomCreate_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}

	// 토큰 재발행 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_TokenChange_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}

	// 방 닫힘 응답
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomClose_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}

	// 방 퇴장 응답
	//
	// Parameter : RoomNo, AccountNo
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomLeave_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 현재 하는거 없음..
	}




	// -----------------------
	// 채팅 Lan 서버가 호출하는 함수
	// -----------------------

	// 마스터에게, 신규 대기방 생성 패킷 보내기
	//
	// Parameter : RoomNo
	// return : 없음
	void CBattle_Master_LanClient::Packet_NewRoomCreate_Req(int RoomNo)
	{
		// 마스터와 연결이 끊긴 상태라면 패킷 안보낸다.
		if (m_bLoginCheck == false)
			return;

		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();


		// 1. 룸 검색
		AcquireSRWLockShared(&m_BattleServer_this->m_Room_Umap_srwl);	// ----- 룸 Shared 락

		auto FindRoom = m_BattleServer_this->m_Room_Umap.find(RoomNo);

		if (FindRoom == m_BattleServer_this->m_Room_Umap.end())
			g_BattleServer_Room_Dump->Crash();

		CBattleServer_Room::stRoom* NewRoom = FindRoom->second;


		// 2. 패킷 만들기
		WORD Type = en_PACKET_BAT_MAS_REQ_CREATED_ROOM;
		UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&NewRoom->m_iBattleServerNo, 4);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&NewRoom->m_iMaxJoinCount, 4);
		SendBuff->PutData(NewRoom->m_cEnterToken, 32);

		SendBuff->PutData((char*)&ReqSequence, 4);

		ReleaseSRWLockShared(&m_BattleServer_this->m_Room_Umap_srwl);	// ----- 룸 Shared 언락


		// 3. 마스터에게 Send하기
		SendPacket(m_ullSessionID, SendBuff);
	}

	// 토큰 재발급 함수
	//
	// Parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::Packet_TokenChange_Req()
	{
		// 마스터와 연결이 끊긴 상태라면 패킷 안보낸다.
		if (m_bLoginCheck == false)
			return;

		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();


		// 1. 토큰 재발급 패킷 만들기
		WORD Type = en_PACKET_BAT_MAS_REQ_CONNECT_TOKEN;
		UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData(m_BattleServer_this->m_cConnectToken_Now, 32);
		SendBuff->PutData((char*)&ReqSequence, 4);


		// 2. 마스터에게 패킷 보내기
		SendPacket(m_ullSessionID, SendBuff);
	}

	



	// -----------------------
	// Battle Net 서버가 호출하는 함수
	// -----------------------		
	
	// 마스터에게, 방 닫힘 패킷 보내기
	//
	// Parameter : RoomNo
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomClose_Req(int RoomNo)
	{
		// 마스터와 연결이 끊긴 상태라면 패킷 안보낸다.
		if (m_bLoginCheck == false)
			return;

		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		// 1. 패킷 만들기
		WORD Type = en_PACKET_BAT_MAS_REQ_CLOSED_ROOM;
		UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&ReqSequence, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}

	// 마스터에게, 방 퇴장 패킷 보내기
	//
	// Parameter : RoomNo, ClientKey
	// return : 없음
	void CBattle_Master_LanClient::Packet_RoomLeave_Req(int RoomNo, INT64 ClientKey)
	{
		// 마스터와 연결이 끊긴 상태라면 패킷 안보낸다.
		if (m_bLoginCheck == false)
			return;

		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		// 1. 패킷 만들기
		WORD Type = en_PACKET_BAT_MAS_REQ_LEFT_USER;
		UINT ReqSequence = InterlockedIncrement(&uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&RoomNo, 4);
		SendBuff->PutData((char*)&ClientKey, 8);
		SendBuff->PutData((char*)&ReqSequence, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}





	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 시작 함수
	// 내부적으로, 상속받은 CLanClient의 Start호출.
	//
	// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
	// return : 성공 시 true , 실패 시 falsel 
	bool CBattle_Master_LanClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 마스터 서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
			return false;

		return true;
	}

	// 종료 함수
	// 내부적으로, 상속받은 CLanClient의 Stop호출.
	// 추가로, 리소스 해제 등
	//
	// Parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::ClientStop()
	{
		// 마스터 서버와 연결 종료
		Stop();
	}

	// 배틀서버의 this를 입력받는 함수
	// 
	// Parameter : 배틀 서버의 this
	// return : 없음
	void CBattle_Master_LanClient::ParentSet(CBattleServer_Room* ChatThis)
	{
		m_BattleServer_this = ChatThis;
	}




	// -----------------------
	// 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CBattle_Master_LanClient::OnConnect(ULONGLONG SessionID)
	{
		if(m_bLoginCheck == true)
			g_BattleServer_Room_Dump->Crash();

		if (m_ullSessionID != 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		m_ullSessionID = SessionID;
		
		while (1)
		{
			// 채팅서버랑 연결되었나 체크.
			// 연결 된 상태에서, 마스터 서버로 로그인 패킷 보냄.
			if (m_BattleServer_this->m_Chat_LanServer->m_bLoginCheck == true)
			{
				// 마스터 서버(Lan)로 로그인 패킷 보냄
				CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

				WORD Type = en_PACKET_BAT_MAS_REQ_SERVER_ON;

				SendBuff->PutData((char*)&Type, 2);

				SendBuff->PutData((char*)m_tcBattleNetServerIP, 32);
				SendBuff->PutData((char*)&m_iBattleNetServerPort, 2);
				SendBuff->PutData(m_BattleServer_this->m_cConnectToken_Now, 32);
				SendBuff->PutData(m_cMasterToken, 32);

				SendBuff->PutData((char*)m_tcChatNetServerIP, 32);
				SendBuff->PutData((char*)&m_iChatNetServerPort, 2);


				SendPacket(SessionID, SendBuff);

				break;
			}

			Sleep(1);
		}
		
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CBattle_Master_LanClient::OnDisconnect(ULONGLONG SessionID)
	{
		// SessionID를 초기값으로 변경
		m_ullSessionID = 0xffffffffffffffff;

		// 비 로그인 상태로 변경
		m_bLoginCheck = false;
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Master_LanClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// Type 분리
		WORD Type;
		Payload->GetData((char*)&Type, 2);


		// 타입에 따라 분기 처리
		try
		{
			switch (Type)
			{
				// 로그인 요청에 대한 응답
			case en_PACKET_BAT_MAS_RES_SERVER_ON:
				Packet_Login_Res(SessionID, Payload);
				break;
				
				// 신규 대기방 생성 응답
			case en_PACKET_BAT_MAS_RES_CREATED_ROOM:
				Packet_NewRoomCreate_Res(SessionID, Payload);
				break;

				// 토큰 재발행 응답
			case en_PACKET_BAT_MAS_RES_CONNECT_TOKEN:
				Packet_TokenChange_Res(SessionID, Payload);
				break;

				// 방 닫힘 응답
			case en_PACKET_BAT_MAS_RES_CLOSED_ROOM:
				Packet_RoomClose_Res(SessionID, Payload);
				break;

				// 방 퇴장 응답
			case en_PACKET_BAT_MAS_RES_LEFT_USER:
				Packet_RoomLeave_Res(SessionID, Payload);
				break;

				// 없는 타입이면 크래시
			default:
				TCHAR str[100];
				StringCchPrintf(str, 100, _T("CBattle_Master_LanClient. OnRecv(). TypeError. SessionID : %lld, Type : %d"), SessionID, Type);

				throw CException(str);
				break;
			}

		}
		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			g_BattleServer_Room_Dump->Crash();

			// 접속 끊기 요청
			//Disconnect(SessionID);
		}
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CBattle_Master_LanClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattle_Master_LanClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CBattle_Master_LanClient::OnError(int error, const TCHAR* errorStr)
	{}






	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CBattle_Master_LanClient::CBattle_Master_LanClient()
	{
		// SessionID 초기화
		m_ullSessionID = 0xffffffffffffffff;

		// Req시퀀스 초기화
		uiReqSequence = 0;

		// 로그인 상태 false
		m_bLoginCheck = false;
	}

	// 소멸자
	CBattle_Master_LanClient::~CBattle_Master_LanClient()
	{
		// 아직 연결이 되어있으면, 연결 해제
		if (GetClinetState() == true)
			ClientStop();
	}
}

// ---------------
// CGame_MinitorClient
// CLanClienet를 상속받는 모니터링 클라
// ---------------
namespace Library_Jingyu
{
	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	CGame_MinitorClient::CGame_MinitorClient()
		:CLanClient()
	{
		// 모니터링 서버 정보전송 스레드를 종료시킬 이벤트 생성
		//
		// 자동 리셋 Event 
		// 최초 생성 시 non-signalled 상태
		// 이름 없는 Event	
		m_hMonitorThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

		m_ullSessionID = 0xffffffffffffffff;
	}

	CGame_MinitorClient::~CGame_MinitorClient()
	{
		// 아직 연결이 되어있으면, 연결 해제
		if (GetClinetState() == true)
			ClientStop();

		// 이벤트 삭제
		CloseHandle(m_hMonitorThreadExitEvent);
	}



	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 시작 함수
	// 내부적으로, 상속받은 CLanClient의 Start호출.
	//
	// Parameter : 연결할 서버의 IP, 포트, 워커스레드 수, 활성화시킬 워커스레드 수, TCP_NODELAY 사용 여부(true면 사용)
	// return : 성공 시 true , 실패 시 falsel 
	bool CGame_MinitorClient::ClientStart(TCHAR* ConnectIP, int Port, int CreateWorker, int ActiveWorker, int Nodelay)
	{
		// 모니터링 서버에 연결
		if (Start(ConnectIP, Port, CreateWorker, ActiveWorker, Nodelay) == false)
			return false;

		// 모니터링 서버로 정보 전송할 스레드 생성
		m_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);

		return true;
	}

	// 종료 함수
	// 내부적으로, 상속받은 CLanClient의 Stop호출.
	// 추가로, 리소스 해제 등
	//
	// Parameter : 없음
	// return : 없음
	void CGame_MinitorClient::ClientStop()
	{
		// 1. 모니터링 서버 정보전송 스레드 종료
		SetEvent(m_hMonitorThreadExitEvent);

		// 종료 대기
		if (WaitForSingleObject(m_hMonitorThread, INFINITE) == WAIT_FAILED)
		{
			DWORD Error = GetLastError();
			printf("MonitorThread Exit Error!!! (%d) \n", Error);
		}

		// 2. 스레드 핸들 반환
		CloseHandle(m_hMonitorThread);

		// 3. 모니터링 서버와 연결 종료
		Stop();
	}

	// 배틀서버의 this를 입력받는 함수
	// 
	// Parameter : 배틀 서버의 this
	// return : 없음
	void CGame_MinitorClient::ParentSet(CBattleServer_Room* ChatThis)
	{
		m_BattleServer_this = ChatThis;
	}




	// -----------------------
	// 내부에서만 사용하는 기능 함수
	// -----------------------

	// 일정 시간마다 모니터링 서버로 정보를 전송하는 스레드
	UINT	WINAPI CGame_MinitorClient::MonitorThread(LPVOID lParam)
	{
		// this 받아두기
		CGame_MinitorClient* g_This = (CGame_MinitorClient*)lParam;

		// 종료 신호 이벤트 받아두기
		HANDLE* hEvent = &g_This->m_hMonitorThreadExitEvent;

		// CPU 사용율 체크 클래스 (배틀서버 소프트웨어)
		CCpuUsage_Process CProcessCPU;

		// CPU 사용율 체크 클래스 (하드웨어)
		CCpuUsage_Processor CProcessorCPU;

		// PDH용 클래스
		CPDH	CPdh;

		while (1)
		{
			// 1초에 한번 깨어나서 정보를 보낸다.
			DWORD Check = WaitForSingleObject(*hEvent, 1000);

			// 이상한 신호라면
			if (Check == WAIT_FAILED)
			{
				DWORD Error = GetLastError();
				printf("MoniterThread Exit Error!!! (%d) \n", Error);
				break;
			}

			// 만약, 종료 신호가 왔다면 스레드 종료.
			else if (Check == WAIT_OBJECT_0)
				break;


			// 그게 아니라면, 일을 한다.
			// 모니터링 서버와 연결중일 때만!
			if (g_This->GetClinetState() == false)
				continue;


			// 프로세서, 프로세스 CPU 사용율, PDH 정보 갱신
			CProcessorCPU.UpdateCpuTime();
			CProcessCPU.UpdateCpuTime();
			CPdh.SetInfo();

			// ----------------------------------
			// 하드웨어 정보 보내기 (프로세서)
			// ----------------------------------
			int TimeStamp = (int)(time(NULL));

			// 1. 하드웨어 CPU 사용률 전체
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL, (int)CProcessorCPU.ProcessorTotal(), TimeStamp);

			// 2. 하드웨어 사용가능 메모리 (MByte)
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY, (int)CPdh.Get_AVA_Mem(), TimeStamp);

			// 3. 하드웨어 이더넷 수신 바이트 (KByte)
			int iData = (int)(CPdh.Get_Net_Recv() / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV, iData, TimeStamp);

			// 4. 하드웨어 이더넷 송신 바이트 (KByte)
			iData = (int)(CPdh.Get_Net_Send() / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND, iData, TimeStamp);

			// 5. 하드웨어 논페이지 메모리 사용량 (MByte)
			iData = (int)(CPdh.Get_NonPaged_Mem() / 1024 / 1024);
			g_This->InfoSend(dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY, iData, TimeStamp);



			// ----------------------------------
			// 배틀서버 정보 보내기
			// ----------------------------------

			// 배틀서버가 On일 경우, 게임서버 정보 보낸다.
			if (g_This->m_BattleServer_this->GetServerState() == true)
			{
				// 1. 배틀서버 ON		
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, TRUE, TimeStamp);

				// 2. 배틀서버 CPU 사용률 (커널 + 유저)
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_CPU, (int)CProcessCPU.ProcessTotal(), TimeStamp);

				// 3. 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
				int Data = (int)(CPdh.Get_UserCommit() / 1024 / 1024);
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, Data, TimeStamp);

				// 4. 배틀서버 패킷풀 사용량
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, CProtocolBuff_Net::GetNodeCount() + CProtocolBuff_Lan::GetNodeCount(), TimeStamp);

				// 5. Auth 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, g_This->m_BattleServer_this->m_lAuthFPS, TimeStamp);

				// 6. Game 스레드 초당 루프 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, g_This->m_BattleServer_this->m_lGameFPS, TimeStamp);

				// 7. 배틀서버 접속 세션전체
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, (int)g_This->m_BattleServer_this->GetClientCount(), TimeStamp);

				// 8. Auth 스레드 모드 인원
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH, g_This->m_BattleServer_this->GetAuthModeUserCount(), TimeStamp);

				// 9. Game 스레드 모드 인원
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME, g_This->m_BattleServer_this->GetGameModeUserCount(), TimeStamp);

				// 10. 대기방 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT, g_This->m_BattleServer_this->m_lNowWaitRoomCount, TimeStamp);

				// 11. 플레이 방 수
				g_This->InfoSend(dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY, g_This->m_BattleServer_this->m_lPlayRoomCount, TimeStamp);
			}

		}

		return 0;
	}

	// 모니터링 서버로 데이터 전송
	//
	// Parameter : DataType(BYTE), DataValue(int), TimeStamp(int)
	// return : 없음
	void CGame_MinitorClient::InfoSend(BYTE DataType, int DataValue, int TimeStamp)
	{
		WORD Type = en_PACKET_SS_MONITOR_DATA_UPDATE;

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&DataType, 1);
		SendBuff->PutData((char*)&DataValue, 4);
		SendBuff->PutData((char*)&TimeStamp, 4);

		SendPacket(m_ullSessionID, SendBuff);
	}





	// -----------------------
	// 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CGame_MinitorClient::OnConnect(ULONGLONG SessionID)
	{
		if (m_ullSessionID != 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		m_ullSessionID = SessionID;

		// 모니터링 서버(Lan)로 로그인 패킷 보냄
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_SS_MONITOR_LOGIN;
		int ServerNo = dfSERVER_NO;

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerNo, 4);

		SendPacket(SessionID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void CGame_MinitorClient::OnDisconnect(ULONGLONG SessionID)
	{
		m_ullSessionID = 0xffffffffffffffff;
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, CProtocolBuff_Lan*
	// return : 없음
	void CGame_MinitorClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CGame_MinitorClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGame_MinitorClient::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGame_MinitorClient::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CGame_MinitorClient::OnError(int error, const TCHAR* errorStr)
	{}
}

// ---------------------------
//
// 채팅서버의 랜클라와 연결되는 Lan 서버
//
// ---------------------------
namespace Library_Jingyu
{
	// -----------------------
	// Battle Net 서버가 호출하는 함수
	// -----------------------

	// 채팅 랜클라에게, 신규 대기방 생성 패킷 보내기
	//
	// Parameter : stRoom*
	// return : 없음
	void CBattle_Chat_LanServer::Packet_NewRoomCreate_Req(CBattleServer_Room::stRoom* NewRoom)
	{
		// 접속한 채팅 랜클라가 없다면 패킷 안보낸다.
		if (m_bLoginCheck == false)
			return;

		// 접속한 채팅 클라가 없다면, 안보낸다.
		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();


		// 1. 패킷 만들기
		WORD Type = en_PACKET_CHAT_BAT_REQ_CREATED_ROOM;
		UINT ReqSequence = InterlockedIncrement(&m_uiReqSequence);

		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&NewRoom->m_iBattleServerNo, 4);
		SendBuff->PutData((char*)&NewRoom->m_iRoomNo, 4);
		SendBuff->PutData((char*)&NewRoom->m_iMaxJoinCount, 4);
		SendBuff->PutData(NewRoom->m_cEnterToken, 32);

		SendBuff->PutData((char*)&ReqSequence, 4);

		// 2. 채팅 클라에게 Send하기
		SendPacket(m_ullSessionID, SendBuff);
	}

	// 채팅 랜클라에게, 토큰 발급
	//
	// Parameter : 없음
	// return : 없음
	void CBattle_Chat_LanServer::Packet_TokenChange_Req()
	{
		// 접속한 채팅 랜클라가 없다면 패킷 안보낸다.
		if (m_bLoginCheck == false)
			return;

		// 접속한 채팅 클라가 없다면, 안보낸다.
		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();


		// 토큰 재발급 시점이 되었는지 확인하기.
		DWORD NowTime = timeGetTime();

		// !! 절대 안나오겠지만, 혹시 음수가 나오는 상황이 발생할 수도 있으니 signed형으로 받는다. !!
		// !! 그리고 그걸 비교한다 !!
		int CheckTime = NowTime - m_dwTokenSendTime;
		
		//if ((m_dwTokenSendTime + m_BattleServer->m_stConst.m_iTokenChangeSlice) <= NowTime)
		if (CheckTime > m_BattleServer->m_stConst.m_iTokenChangeSlice)
		{
			// 1. 토큰 발급 시간 갱신
			m_dwTokenSendTime = NowTime;

			// 락 걸기. 락 안걸면 복사하는 중 누가 체크했을 때 위험.
			AcquireSRWLockExclusive(&m_BattleServer->m_ServerEnterToken_srwl);	// ----- Exclusive 락

			// 2. "현재" 토큰을 "이전" 토큰에 복사
			memcpy_s(m_BattleServer->m_cConnectToken_Before, 32, m_BattleServer->m_cConnectToken_Now, 32);

			// 3. "현재" 토큰 다시 만들기
			int i = 0;
			while (i < 32)
			{
				m_BattleServer->m_cConnectToken_Now[i] = (rand() % 128) + 1;

				++i;
			}

			ReleaseSRWLockExclusive(&m_BattleServer->m_ServerEnterToken_srwl);	// ----- Exclusive 언락

			// 4. 토큰 재발급 패킷 만들기
			WORD Type = en_PACKET_CHAT_BAT_REQ_CONNECT_TOKEN;
			UINT ReqSequence = InterlockedIncrement(&m_uiReqSequence);

			CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData(m_BattleServer->m_cConnectToken_Now, 32);
			SendBuff->PutData((char*)&ReqSequence, 4);


			// 5. 채팅에게 토큰 패킷 보내기
			SendPacket(m_ullSessionID, SendBuff);
		}
	}




	// ------------------------
	// 외부에서 호출 가능한 함수
	// ------------------------

	// 서버 시작
	//
	// Parameter : IP, Port, 생성 워커, 활성화 워커, 생성 엑셉트 스레드, 노딜레이, 최대 접속자 수
	// return : 실패 시 false
	bool CBattle_Chat_LanServer::ServerStart(TCHAR* IP, int Port, int CreateWorker, int ActiveWorker, int CreateAccept, int Nodelay, int MaxUser)
	{
		if (Start(IP, Port, CreateWorker, ActiveWorker, CreateAccept, Nodelay, MaxUser) == false)
			return false;

		return true;
	}

	// 서버 종료
	//
	// Parameter : 없음
	// return : 없음
	void CBattle_Chat_LanServer::ServerStop()
	{
		Stop();
	}




	// -----------------------
	// 패킷 처리 함수
	// -----------------------
	
	// 신규 대기방 생성 패킷 응답.
	// 이 안에서 마스터에게도 보내준다.
	//
	// Parameter : CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Chat_LanServer::Packet_NewRoomCreate_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Packet)
	{
		if (SessionID != m_ullSessionID)
			g_BattleServer_Room_Dump->Crash();

		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		if (m_bLoginCheck == false)
			g_BattleServer_Room_Dump->Crash();


		// 1. 마샬링
		int RoomNo;
		Packet->GetData((char*)&RoomNo, 4);

		// 2. 마스터에게 신규 방 생성 패킷 보내기
		m_pMasterClient->Packet_NewRoomCreate_Req(RoomNo);
	}
	
	// 방 삭제에 대한 응답
	//
	// Parameter : SessinID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Chat_LanServer::Packet_RoomClose_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Packet)
	{

	}

	// 토큰 재발행에 대한 응답
	// 이 안에서 마스터에게도 보내준다.
	//
	// Parameter : SessionID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Chat_LanServer::Packet_TokenChange_Res(ULONGLONG SessionID, CProtocolBuff_Lan* Packet)
	{
		if (SessionID != m_ullSessionID)
			g_BattleServer_Room_Dump->Crash();

		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		if (m_bLoginCheck == false)
			g_BattleServer_Room_Dump->Crash();


		// 1. 마스터 서버에게 토큰 보내기
		m_pMasterClient->Packet_TokenChange_Req();
	}

	// 로그인 요청
	//
	// Parameter : SessinID, CProtocolBuff_Lan*
	// return : 없음
	void CBattle_Chat_LanServer::Packet_Login(ULONGLONG SessionID, CProtocolBuff_Lan* Packet)
	{
		if (SessionID != m_ullSessionID)
			g_BattleServer_Room_Dump->Crash();

		if (m_ullSessionID == 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		if (m_bLoginCheck == true)
			g_BattleServer_Room_Dump->Crash();

		// 채팅서버의 IP와 Port 가져오기
		Packet->GetData((char*)m_pMasterClient->m_tcChatNetServerIP, 32);
		Packet->GetData((char*)&m_pMasterClient->m_iChatNetServerPort, 2);		


		// 응답 패킷 보내기
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		WORD Type = en_PACKET_CHAT_BAT_RES_SERVER_ON;

		SendBuff->PutData((char*)&Type, 2);

		SendPacket(SessionID, SendBuff);



		// 채팅서버로 토큰 변경 패킷 보내기
		UINT ReqSequence = InterlockedIncrement(&m_uiReqSequence);
		CProtocolBuff_Lan* TokenBuff = CProtocolBuff_Lan::Alloc();

		Type = en_PACKET_CHAT_BAT_REQ_CONNECT_TOKEN;

		TokenBuff->PutData((char*)&Type, 2);
		TokenBuff->PutData((char*)m_BattleServer->m_cConnectToken_Now, 32);
		TokenBuff->PutData((char*)&ReqSequence, 4);

		SendPacket(SessionID, TokenBuff);


		// 토큰 발급시간 갱신
		m_dwTokenSendTime = timeGetTime();

		// 로그인 상태로 변경
		m_bLoginCheck = true;
	}






	// --------------------------
	// 내부에서만 사용하는 함수
	// --------------------------

	// 마스터 랜 클라, 배틀 Net 서버 셋팅
	//
	// Parameter : CBattle_Master_LanClient*
	// return : 없음
	void CBattle_Chat_LanServer::SetMasterClient(CBattle_Master_LanClient* SetPoint, CBattleServer_Room* SetPoint2)
	{
		m_pMasterClient = SetPoint;
		m_BattleServer = SetPoint2;
	}



	   	 



	// -----------------------
	// 가상함수
	// -----------------------

	// Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CBattle_Chat_LanServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		// 이미 접속한 유저가 있을 시 return false;
		// 채팅 랜 서버에는 한 클라만 접속 가능
		if (m_ullSessionID != 0xffffffffffffffff)
			return false;

		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CBattle_Chat_LanServer::OnClientJoin(ULONGLONG SessionID)
	{
		if (m_ullSessionID != 0xffffffffffffffff)
			g_BattleServer_Room_Dump->Crash();

		if(m_bLoginCheck == true)
			g_BattleServer_Room_Dump->Crash();

		m_ullSessionID = SessionID;
	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CBattle_Chat_LanServer::OnClientLeave(ULONGLONG SessionID)
	{
		m_ullSessionID = 0xffffffffffffffff;
		m_bLoginCheck = false;
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CBattle_Chat_LanServer::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 타입 확인
		WORD Type;

		Payload->GetData((char*)&Type, 2);

		try
		{
			// 타입에 따라 분기 처리
			switch (Type)
			{
				// 신규 대기방 생성에 대한 응답
			case en_PACKET_CHAT_BAT_RES_CREATED_ROOM:
				Packet_NewRoomCreate_Res(SessionID, Payload);
				break;			

				// 방 삭제에 대한 응답
			case en_PACKET_CHAT_BAT_RES_DESTROY_ROOM:
				break;

				// 연결 토큰 재발행에 대한 응답
			case en_PACKET_CHAT_BAT_RES_CONNECT_TOKEN:
				Packet_TokenChange_Res(SessionID, Payload);
				break;

				// 로그인 
			case en_PACKET_CHAT_BAT_REQ_SERVER_ON:
				Packet_Login(SessionID, Payload);
				break;

				// 없는 타입이면 크래시
			default:
				TCHAR str[100];
				StringCchPrintf(str, 100, _T("CBattle_Chat_LanServer. OnRecv(). TypeError. SessionID : %lld, Type : %d"), SessionID, Type);

				throw CException(str);
				break;
			}
		}

		catch (CException& exc)
		{
			// 로그 찍기 (로그 레벨 : 에러)
			g_BattleServer_RoomLog->LogSave(false, L"CBattleServer_Room", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
				(TCHAR*)exc.GetExceptionText());

			g_BattleServer_Room_Dump->Crash();

			// 접속 끊기 요청
			//Disconnect(SessionID);
		}
	}
		

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CBattle_Chat_LanServer::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{

	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattle_Chat_LanServer::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CBattle_Chat_LanServer::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CBattle_Chat_LanServer::OnError(int error, const TCHAR* errorStr)
	{

	}




	// ---------------
	// 생성자와 소멸자
	// ----------------

	// 생성자
	CBattle_Chat_LanServer::CBattle_Chat_LanServer()
	{
		m_ullSessionID = 0xffffffffffffffff;
		m_bLoginCheck = false;
	}

	// 소멸자
	CBattle_Chat_LanServer::~CBattle_Chat_LanServer()
	{

	}

}