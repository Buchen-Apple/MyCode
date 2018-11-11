#ifndef __SHDB_COMMUNICATE_H__
#define __SHDB_COMMUNICATE_H__

#include "ProtocolBuff/ProtocolBuff(Net)_ObjectPool.h"
#include "ObjectPool/Object_Pool_LockFreeVersion.h"
#include "LockFree_Queue/LockFree_Queue.h"
#include "CrashDump/CrashDump.h"
#include "Http_Exchange/HTTP_Exchange.h"

// --------------------------------------------
// shDB 내부에서 메모리풀로 관리되는 구조체들
// --------------------------------------------
namespace Library_Jingyu
{
	// Auth 스레드에서 로그인 응답을 받을 시, 세션키 체크를 위한 프로토콜
	struct DB_WORK_LOGIN
	{
		WORD m_wWorkType;
		INT64 m_i64UniqueKey;
		LPVOID pPointer;
		CProtocolBuff_Net* m_pBuff;
		TCHAR m_tcRequest[200];

		// --- 아래부터가 컨텐츠 별 고유 정보.
		INT64 AccountNo;
	};

	// shDB의 어떤 API를 호출할 것인지.
	enum en_PHP_TYPE
	{
		// Seelct_account.php
		SELECT_ACCOUNT = 1,

		// DB_Read 스레드 종료 신호
		EXIT = 9999
	};

	
}


// -----------------------
// shDB와 통신하는 클래스
// -----------------------
namespace Library_Jingyu
{
	// 메모리풀에서 관리할 구조체.
	// 가장 크기가 큰 것 1개를 정의한다.
#define DB_WORK	DB_WORK_LOGIN

	class shDB_Communicate
	{
		// ---------------
		// 멤버 변수
		// ---------------			

		// DB_Read용 입출력 완료포트 핸들
		HANDLE m_hDB_Read;

		// DB_Read용 워커 스레드 핸들
		HANDLE* m_hDB_Read_Thread;

		// 덤프
		CCrashDump* m_Dump;	

	public:
		// ---------------
		// 외부에서 접근 가능한 멤버 변수
		// ---------------		

		// DB_WORK를 관리하는 메모리 풀
		CMemoryPoolTLS<DB_WORK>* m_pDB_Work_Pool;

		// Read가 완료된 DB_WORK* 를 저장해두는 락프리 큐
		CLF_Queue<DB_WORK*> *m_pDB_ReadComplete_Queue;


	private:
		// --------------
		// 스레드
		// --------------

		// DB_Read 스레드
		static UINT WINAPI DB_ReadThread(LPVOID lParam);


	private:
		// ---------------
		// API 타입에 따른 쿼리문
		// ---------------

		// Select_Account.php에 날리는 쿼리문
		TCHAR m_tcSELECT_ACCOUNT[30] = L"{\"accountno\" : %lld}";



	public:
		// ---------------
		// 기능 함수. 외부에서 호출 가능
		// ---------------

		// DB에 Read할 것이 있을 때 호출되는 함수
		// 인자로 받은 구조체의 정보를 확인해 로직 처리
		// 
		// Parameter : DB_WORK*, APIType
		// return : 없음
		void DBReadFunc(DB_WORK* Protocol, WORD APIType);


	public:
		// ---------------
		// 생성자와 소멸자
		// ---------------

		// 생성자
		shDB_Communicate();

		// 소멸자
		virtual ~shDB_Communicate();
	};
}


#endif // !__SHDB_COMMUNICATE_H__
