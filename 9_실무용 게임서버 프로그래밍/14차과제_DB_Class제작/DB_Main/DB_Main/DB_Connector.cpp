#include "pch.h"
#include "DB_Connector.h"
#include "CrashDump\CrashDump.h"
#include <strsafe.h>


namespace Library_Jingyu
{

#define 
	CCrashDump* g_DBDump = CCrashDump::GetInstance();

	//////////////////////////////////////////////////////////////////////
	// 생성자
	// 
	// Parameter : 연결할 DB IP, 사용자 이름, 비밀번호, DB 이름, 포트
	//////////////////////////////////////////////////////////////////////
	CDBConnector::CDBConnector(WCHAR *DBIP, WCHAR *User, WCHAR *Password, WCHAR *DBName, int DBPort)
	{
		// -----------------------------------
		// 인자값들을 멤버변수에 저장
		// 저장하다 실패하면 크래시.
		// 여기서 실패하는건 그냥 코드실수이니 말도안됨.
		// -----------------------------------

		// 1. IP
		if (StringCchCopy(m_wcDBIP, 16, DBIP) != S_OK)
			g_DBDump->Crash();

		// 2. 사용자 이름
		if (StringCchCopy(m_wcDBUser, 64, User) != S_OK)
			g_DBDump->Crash();

		// 3. 비밀번호
		if (StringCchCopy(m_wcDBPassword, 64, Password) != S_OK)
			g_DBDump->Crash();

		// 4. DB 이름
		if (StringCchCopy(m_wcDBName, 64, DBName) != S_OK)
			g_DBDump->Crash();

		// 5. 포트
		m_iDBPort = DBPort;

		// -----------------------------------
		// DB 연결 객체 포인터를 nullptr로 초기화.
		// 이 포인터를 이용해 DB 연결 여부 판단.
		// -----------------------------------
		m_pMySQL == nullptr;
		
	}

	// 소멸자
	CDBConnector::~CDBConnector()
	{
		Disconnect();
	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB 연결
	// 생성자에서 전달받은 DB정보를 이용해, DB와 연결
	// 
	// Parameter : 없음
	// return : 연결 성공 시 true, 
	//		  : 연결 실패 시 false
	//////////////////////////////////////////////////////////////////////
	bool	CDBConnector::Connect()
	{
		// 1. 초기화
		mysql_init(&m_MySQL);

		// 2. 연결을 위한 정보들, char 형태로 변환
		char IP[32] = { 0, };
		char User[32] = { 0, };
		char Password[32] = { 0, };
		char Name[32] = { 0, };

		int len = (int)_tcslen(m_wcDBIP);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBIP, (int)_tcslen(m_wcDBIP), IP, len, NULL, NULL);

		len = (int)_tcslen(m_wcDBUser);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBUser, (int)_tcslen(m_wcDBUser), User, len, NULL, NULL);

		len = (int)_tcslen(m_wcDBPassword);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBPassword, (int)_tcslen(m_wcDBPassword), Password, len, NULL, NULL);

		len = (int)_tcslen(m_wcDBName);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBName, (int)_tcslen(m_wcDBName), Name, len, NULL, NULL);

		// 3. DB에 연결 시도
		// 약 5회 연결 시도.
		// 5회동안 연결 못하면 Crash
		int iConnectCount = 0;
		while (1)
		{
			m_pMySQL = mysql_real_connect(&m_MySQL, IP, User, Password, Name, m_iDBPort, NULL, 0);
			if (m_pMySQL != NULL)
				break;

			if (iConnectCount >= 5)
				g_DBDump->Crash();

			iConnectCount++;
		}

		// 디폴트 셋팅을 utf8로 셋팅
		mysql_set_character_set(m_pMySQL, "utf8");
		
	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB 끊기
	//////////////////////////////////////////////////////////////////////
	bool	CDBConnector::Disconnect()
	{
		// 디비에 연결되어 있을 경우, 연결 해제
		if(m_pMySQL != nullptr)
			mysql_close(m_pMySQL);
	}


	//////////////////////////////////////////////////////////////////////
	// 쿼리 날리고 결과셋 임시 보관
	//
	//////////////////////////////////////////////////////////////////////
	bool	CDBConnector::Query(WCHAR *szStringFormat, ...)
	{

	}

	// DBWriter 스레드의 Save 쿼리 전용
	// 결과셋을 저장하지 않음.
	bool	CDBConnector::Query_Save(WCHAR *szStringFormat, ...)	
	{

	}
															

	//////////////////////////////////////////////////////////////////////
	// 쿼리를 날린 뒤에 결과 뽑아오기.
	//
	// 결과가 없다면 NULL 리턴.
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	CDBConnector::FetchRow()
	{

	}

	//////////////////////////////////////////////////////////////////////
	// 한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::FreeResult()
	{

	}


	//////////////////////////////////////////////////////////////////////
	// mysql 의 LastError 를 맴버변수로 저장한다.
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::SaveLastError()
	{

	}

}