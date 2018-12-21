#ifndef __SHDB_COMMUNICATE_H__
#define __SHDB_COMMUNICATE_H__

#include "ProtocolBuff/ProtocolBuff(Net)_ObjectPool.h"
#include "ObjectPool/Object_Pool_LockFreeVersion.h"
#include "LockFree_Queue/LockFree_Queue.h"
#include "CrashDump/CrashDump.h"
#include "Http_Exchange/HTTP_Exchange.h"
#include "NormalTemplate_Queue\Normal_Queue_Template.h"

// --------------------------------------------
// shDB 내부에서 메모리풀로 관리되는 구조체들
// --------------------------------------------
namespace Library_Jingyu
{
	// Auth 스레드에서 로그인 응답을 받을 시, 세션키 체크를 위한 프로토콜
	struct DB_WORK_LOGIN
	{
		WORD m_wWorkType;
		WORD m_wAPIType;
		INT64 m_i64UniqueKey;

		// Player 포인터
		LPVOID pPointer;
		TCHAR m_tcResponse[420];

		INT64 AccountNo;
	};

	// Auth 스레드에서 로그인 요청을 받을 시, 유저 전적 정보를 저장하기 위한 프로토콜.
	struct DB_WORK_LOGIN_CONTENTS
	{
		WORD m_wWorkType;
		WORD m_wAPIType;
		INT64 m_i64UniqueKey;

		// Player 포인터
		LPVOID pPointer;	
		TCHAR m_tcResponse[200];

		INT64 AccountNo;
	};

	// ContentDB에 유저 정보를 Write할 떄 사용
	struct DB_WORK_CONTENT_UPDATE
	{
		WORD m_wWorkType;

		// Update의 res는 굉장히 짧음. 보통 result : 1임
		TCHAR m_tcResponse[30];

		int		m_iCount;	// 갱신할 카운트		

		INT64 AccountNo;
	};

	// shDB의 어떤 API를 호출할 것인지. API Type
	enum en_PHP_TYPE
	{
		// Seelct_account.php
		SELECT_ACCOUNT = 1,

		// Seelct_contents.php
		SELECT_CONTENTS,

		// Update_account.php
		UPDATE_ACCOUN,

		// Updated_contents.php
		UPDATE_CONTENTS,

		// DB_Read 스레드 종료 신호
		EXIT = 9999
	};

	// DB 요청에 대한 후처리를 위한, Type
	enum eu_DB_AFTER_TYPE
	{
		// 로그인 패킷에 대한 인증 처리
		eu_LOGIN_AUTH = 0,

		// 로그인 패킷에 대한 정보 가져오기
		eu_LOGIN_INFO,

		// PlayCount 저장
		eu_PLAYCOUNT_UPDATE,

		// PlayTime 저장
		eu_PLAYTIME_UPDATE,

		// Kill 저장
		eu_KILL_UPDATE,

		// Die 저장
		eu_DIE_UPDATE,

		// Win 저장
		eu_WIN_UPDATE
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

		// 덤프
		CCrashDump* m_Dump;

		// DB_Read용 입출력 완료포트 핸들
		HANDLE m_hDB_Read;

		// DB_Read용 워커 스레드 핸들
		HANDLE* m_hDB_Read_Thread;

		// ---------------		

		// DB Write 스레드에게 일시키기 용 큐(Normal 큐)
		CNormalQueue<DB_WORK*>* m_pDB_Wirte_Start_Queue;		

		// DB Write용 스레드용 일시키기 용이벤트.
		HANDLE m_hDBWrite_Event;

		// DB Write용 스레드 종료용 이벤트
		HANDLE m_hDBWrite_Exit_Event;

		// DB_Write용 스레드 핸들
		HANDLE m_hDB_Write_Thread;


	public:
		// ---------------
		// 외부에서 접근 가능한 멤버 변수
		// ---------------		

		// DB_WORK를 관리하는 메모리 풀
		CMemoryPoolTLS<DB_WORK>* m_pDB_Work_Pool;

		// Read가 완료된 DB_WORK* 를 저장해두는 락프리 큐
		CLF_Queue<DB_WORK*> *m_pDB_ReadComplete_Queue;

		// DB Write 스레드의, 완료된 일감 저장 큐(Normal 큐)
		CNormalQueue<DB_WORK*>* m_pDB_Wirte_End_Queue;


	private:
		// --------------
		// 스레드
		// --------------

		// DB_Read 스레드
		static UINT WINAPI DB_ReadThread(LPVOID lParam);

		// DB_Write 스레드
		static UINT WINAPI DB_WriteThread(LPVOID lParam);
	

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

		// DB에 Write 할 것이 있을 때 호출되는 함수.
		// 인자로 받은 구조체의 정보를 확인해 로직 처리
		//
		// Parameter : DB_WORK*
		// return : 없음
		void DBWriteFunc(DB_WORK* Protocol);


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
