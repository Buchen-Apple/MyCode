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

		// 2. 연결을 위한 정보들, char 형태로 변환해 보관
		int len = (int)_tcslen(m_wcDBIP);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBIP, (int)_tcslen(m_wcDBIP), m_cDBIP, len, NULL, NULL);

		len = (int)_tcslen(m_wcDBUser);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBUser, (int)_tcslen(m_wcDBUser), m_cDBUser, len, NULL, NULL);

		len = (int)_tcslen(m_wcDBPassword);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBPassword, (int)_tcslen(m_wcDBPassword), m_cDBPassword, len, NULL, NULL);

		len = (int)_tcslen(m_wcDBName);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBName, (int)_tcslen(m_wcDBName), m_cDBName, len, NULL, NULL);

		// 3. DB에 연결 시도
		// 약 5회 연결 시도.
		// 5회동안 연결 못하면 Crash
		int iConnectCount = 0;
		while (1)
		{
			m_pMySQL = mysql_real_connect(&m_MySQL, m_cDBIP, m_cDBUser, m_cDBPassword, m_cDBName, m_iDBPort, NULL, 0);
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
		// 1. 받은 쿼리를 utf-8로 변환. 동시에 utf-8 저장용 멤버변수에 보관
		int len = (int)_tcslen(szStringFormat);
		WideCharToMultiByte(CP_UTF8, 0, szStringFormat, (int)_tcslen(szStringFormat), m_cQueryUTF8, len, NULL, NULL);

		// 2. 유니코드 저장용 멤버변수에 보관. 차후 로그를 찍거나 하는 등을 할때는 유니코드가 필요
		if (StringCbCopy(m_wcQuery, eQUERY_MAX_LEN, szStringFormat) != S_OK)
		{
			// 카피 실패 시 이유 보관 후, false 리턴
			return false;
		}

		// 3. 쿼리 날리기
		int Error = mysql_query(m_pMySQL, m_cQueryUTF8);

		// 연결이 끊긴거면 5회동안 재연결 시도
		if (Error == CR_SOCKET_CREATE_ERROR ||
			Error == CR_CONNECTION_ERROR ||
			Error == CR_CONN_HOST_ERROR ||
			Error == CR_SERVER_HANDSHAKE_ERR ||
			Error == CR_SERVER_GONE_ERROR ||
			Error == CR_INVALID_CONN_HANDLE ||
			Error == CR_SERVER_LOST)
		{
		}

		int Count = 0;
		while (Count < 5)
		{
			// DB 연결
			m_pMySQL = mysql_real_connect(&m_MySQL, m_cDBIP, m_cDBUser, m_cDBPassword, m_cDBName, m_iDBPort, NULL, 0);
			if (m_pMySQL != NULL)
				break;

			Count++;
			Sleep(0);
		}

		// 5회 연결 시도했는데도 연결 실패면, 에러 찍고 리턴 false.
		if (m_pMySQL == NULL)
		{
			printf("DBQuery()1. Mysql connection error : %s(%d)\n", mysql_error(&m_MySQL), mysql_errno(&m_MySQL));
			return false;
		}


		// 4. 결과 받아두기
		// 밖에서 Query_Save() 함수를 호출해서 사용.
		m_pSqlResult = mysql_store_result(m_pMySQL);

		return true;
	}

	// DBWriter 스레드의 Save 쿼리 전용
	// 결과셋을 저장하지 않음.
	// 보내기만하고 결과 받을 필요 없을때 사용
	bool	CDBConnector::Query_Save(WCHAR *szStringFormat, ...)	
	{
		// 1. 받은 쿼리를 utf-8로 변환. 동시에 utf-8 저장용 멤버변수에 보관
		int len = (int)_tcslen(szStringFormat);
		WideCharToMultiByte(CP_UTF8, 0, szStringFormat, (int)_tcslen(szStringFormat), m_cQueryUTF8, len, NULL, NULL);

		// 2. 유니코드 저장용 멤버변수에 보관. 차후 로그를 찍거나 하는 등을 할때는 유니코드가 필요
		if (StringCbCopy(m_wcQuery, eQUERY_MAX_LEN, szStringFormat) != S_OK)
		{
			// 카피 실패 시 이유 보관 후, false 리턴
			return false;
		}

		// 3. 쿼리 날리기
		int Error = mysql_query(m_pMySQL, m_cQueryUTF8);

		// 연결이 끊긴거면 5회동안 재연결 시도
		if (Error == CR_SOCKET_CREATE_ERROR ||
			Error == CR_CONNECTION_ERROR ||
			Error == CR_CONN_HOST_ERROR ||
			Error == CR_SERVER_HANDSHAKE_ERR ||
			Error == CR_SERVER_GONE_ERROR ||
			Error == CR_INVALID_CONN_HANDLE ||
			Error == CR_SERVER_LOST)
		{
		}

		int Count = 0;
		while (Count < 5)
		{
			// DB 연결
			m_pMySQL = mysql_real_connect(&m_MySQL, m_cDBIP, m_cDBUser, m_cDBPassword, m_cDBName, m_iDBPort, NULL, 0);
			if (m_pMySQL != NULL)
				break;

			Count++;
			Sleep(0);
		}

		// 5회 연결 시도했는데도 연결 실패면, 에러 찍고 리턴 false.
		if (m_pMySQL == NULL)
		{
			printf("DBQuery()1. Mysql connection error : %s(%d)\n", mysql_error(&m_MySQL), mysql_errno(&m_MySQL));
			return false;
		}


		// 4. 결과 받은 후, 바로 result 한다.		
		m_pSqlResult = mysql_store_result(m_pMySQL);
		mysql_free_result(m_pSqlResult);

		return true;

	}
															

	//////////////////////////////////////////////////////////////////////
	// 쿼리를 날린 뒤에 결과 뽑아오기.
	//
	// 결과가 없다면 NULL 리턴.
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	CDBConnector::FetchRow()
	{
		// 1 줄 단위로 넘긴다.
		mysql_fetch_row(m_pSqlResult);
	}

	//////////////////////////////////////////////////////////////////////
	// 한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::FreeResult()
	{
		mysql_free_result(m_pSqlResult);
	}


	//////////////////////////////////////////////////////////////////////
	// mysql 의 LastError 를 맴버변수로 저장한다.
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::SaveLastError()
	{

	}

}