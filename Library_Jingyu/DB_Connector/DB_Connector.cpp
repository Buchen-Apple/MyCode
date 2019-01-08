#include "pch.h"

#include <strsafe.h>

#include "DB_Connector.h"
#include "CrashDump\CrashDump.h"
#include "Log\Log.h"

#pragma comment(lib, "Mysql/lib/vs14/mysqlclient")


// 일반 버전
namespace Library_Jingyu
{
#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	CCrashDump* g_DBDump = CCrashDump::GetInstance();
	CSystemLog* g_DBLog = CSystemLog::GetInstance();


	//////////////////////////////////////////////////////////////////////
	// 생성자
	// 
	// Parameter : 연결할 DB IP, 사용자 이름, 비밀번호, DB 이름, 포트
	//////////////////////////////////////////////////////////////////////
	CDBConnector::CDBConnector(WCHAR *DBIP, WCHAR *User, WCHAR *Password, WCHAR *DBName, int DBPort)
	{
		// -----------------------------------
		// 인자값들 멤버변수에 저장
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
		m_pMySQL = nullptr;

		// 커넥트
		Connect();		
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
		int len = WideCharToMultiByte(CP_UTF8, 0, m_wcDBIP, lstrlenW(m_wcDBIP), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBIP, lstrlenW(m_wcDBIP), m_cDBIP, len, NULL, NULL);

		len = WideCharToMultiByte(CP_UTF8, 0, m_wcDBUser, lstrlenW(m_wcDBUser), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBUser, lstrlenW(m_wcDBUser), m_cDBUser, len, NULL, NULL);

		len = WideCharToMultiByte(CP_UTF8, 0, m_wcDBPassword, lstrlenW(m_wcDBPassword), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBPassword, lstrlenW(m_wcDBPassword), m_cDBPassword, len, NULL, NULL);

		len = WideCharToMultiByte(CP_UTF8, 0, m_wcDBName, lstrlenW(m_wcDBName), NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_UTF8, 0, m_wcDBName, lstrlenW(m_wcDBName), m_cDBName, len, NULL, NULL);

		// 3. DB에 연결 시도
		// 약 5회 연결 시도.
		// 5회동안 연결 못하면 Crash
		int iConnectCount = 0;
		while (1)
		{
			m_pMySQL = mysql_real_connect(&m_MySQL, m_cDBIP, m_cDBUser, m_cDBPassword, m_cDBName, m_iDBPort, NULL, 0);
			if (m_pMySQL != NULL)
				break;		

			// 멤버변수에 에러 셋팅
			SaveLastError();
			
			// 카운트 증가
			iConnectCount++;

			// 로그에 저장
			g_DBLog->LogSave(false, L"DB_Connector", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Connect() --> Connect Fail... (Error : %d)(Count : %d)", m_iLastError, iConnectCount);

			if (iConnectCount >= 5)
				g_DBDump->Crash();	

			// 바로 시도하면 실패할 가능성이 있기 때문에 Sleep(200)을 한다.
			Sleep(200);
		}

		// 디폴트 셋팅을 utf8로 셋팅
		mysql_set_character_set(m_pMySQL, "utf8");

		return true;		
	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB 끊기
	//////////////////////////////////////////////////////////////////////
	bool	CDBConnector::Disconnect()
	{
		// 디비에 연결되어 있을 경우, 연결 해제
		if (m_pMySQL != nullptr)
			mysql_close(m_pMySQL);

		return true;
	}


	//////////////////////////////////////////////////////////////////////
	// 쿼리 날리고 결과셋 임시 보관
	//
	// Parameter : char형 쿼리 메시지, vlist 가변인자
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::Query(char *szStringFormat, va_list* vlist)
	{
		// 1. 가변인자의 값을 스트링 1개로 뽑아내기.		
		HRESULT retval = StringCbVPrintfA(m_cQueryUTF8, eQUERY_MAX_LEN, szStringFormat, *vlist);

		// 쿼리 글자 수 이상이면 로그찍고 끝.
		if (retval != S_OK)
		{
			// 실패 시 로그 남김
			g_DBLog->LogSave(false, L"DB_Connector", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Query --> vlist Change Error...");

			g_DBDump->Crash();
		}		

		// 2. 쿼리 날리기
		int Error, Count;
		while (1)
		{
			Error = mysql_query(m_pMySQL, m_cQueryUTF8);

			// 성공 시 break;
			if (Error == 0)
				break;

			// 연결이 끊긴거면 5회동안 재연결 시도
			if (Error == CR_SOCKET_CREATE_ERROR ||
				Error == CR_CONNECTION_ERROR ||
				Error == CR_CONN_HOST_ERROR ||
				Error == CR_SERVER_HANDSHAKE_ERR ||
				Error == CR_SERVER_GONE_ERROR ||
				Error == CR_INVALID_CONN_HANDLE ||
				Error == CR_SERVER_LOST)
			{
				Count = 0;
				while (1)
				{
					// DB 연결
					m_pMySQL = mysql_real_connect(&m_MySQL, m_cDBIP, m_cDBUser, m_cDBPassword, m_cDBName, m_iDBPort, NULL, 0);
					if (m_pMySQL != NULL)
						break;

					// 멤버변수에 에러 셋팅
					SaveLastError();

					// 카운트 1 증가
					Count++;

					// 실패 시 로그 남김
					g_DBLog->LogSave(false, L"DB_Connector", CSystemLog::en_LogLevel::LEVEL_ERROR,
						L"Query() --> Connect Fail... (Error : %d)(Count : %d)", Error, Count);

					// 5번째 돌렸으면 서버 끈다. 더 이상 돌려도 어차피 저장 안됨.
					if (Count > 5)
						g_DBDump->Crash();

					// 바로 시도하면 실패할 가능성이 있기 때문에 Sleep(200)을 한다.
					Sleep(200);
				}
			}
			
			// 연결 끊긴게 아니면 에러 보관
			else
			{
				SaveLastError();
				return;
			}

		}		

		// 4. 결과 받아두기
		// 밖에서 FetchRow() 함수를 호출해서 사용.
		m_pSqlResult = mysql_store_result(m_pMySQL);
	}

	//////////////////////////////////////////////////////////////////////
	// DBWriter 스레드의 Save 쿼리 전용
	// 결과셋을 저장하지 않음.
	// 
	// Parameter : char형 쿼리 메시지, vlist 가변인자
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::Query_Save(char *szStringFormat, va_list* vlist)
	{
		// 1. 가변인자의 값을 스트링 1개로 뽑아내기.		
		HRESULT retval = StringCbVPrintfA(m_cQueryUTF8, eQUERY_MAX_LEN, szStringFormat, *vlist);

		// 쿼리 글자 수 이상이면 로그찍고 끝.
		if (retval != S_OK)
		{
			// 실패 시 로그 남김
			g_DBLog->LogSave(false, L"DB_Connector", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Query_Save --> vlist Change Error...");

			g_DBDump->Crash();
		}	
		
		// 2. 쿼리 날리기
		int Error, Count;
		while (1)
		{			
			Error = mysql_query(m_pMySQL, m_cQueryUTF8);

			// 성공 시 break;
			if (Error == 0)
				break;

			// 연결이 끊긴거면 5회동안 재연결 시도
			else if (Error == CR_SOCKET_CREATE_ERROR ||
				Error == CR_CONNECTION_ERROR ||
				Error == CR_CONN_HOST_ERROR ||
				Error == CR_SERVER_HANDSHAKE_ERR ||
				Error == CR_SERVER_GONE_ERROR ||
				Error == CR_INVALID_CONN_HANDLE ||
				Error == CR_SERVER_LOST)
			{
				Count = 0;
				while (1)
				{
					// DB 연결
					m_pMySQL = mysql_real_connect(&m_MySQL, m_cDBIP, m_cDBUser, m_cDBPassword, m_cDBName, m_iDBPort, NULL, 0);
					if (m_pMySQL != NULL)
						break;

					// 멤버변수에 에러 셋팅
					SaveLastError();

					// 카운트 1 증가
					Count++;

					// 실패 시 로그 남김
					g_DBLog->LogSave(false, L"DB_Connector", CSystemLog::en_LogLevel::LEVEL_ERROR,
						L"Query() --> Connect Fail... (Count : %d)", Count);

					// 5번 시도했으면 서버 끈다. 
					// 더 이상 돌려도 어차피 저장 안됨.
					if (Count > 5)
						g_DBDump->Crash();

					// 바로 시도하면 실패할 가능성이 있기 때문에 Sleep(200)을 한다.
					Sleep(200);
				}
			}

			// 연결 끊긴게 아니면 에러 보관
			else
			{
				SaveLastError();
				return;
			}
		}
		

		// 4. 결과 받은 후, 바로 result 한다.		
		m_pSqlResult = mysql_store_result(m_pMySQL);
		mysql_free_result(m_pSqlResult);
		m_iLastError = 0;
	}
		

	//////////////////////////////////////////////////////////////////////
	// DBWriter 스레드의 Save 쿼리 전용
	// 결과셋을 저장하지 않음.
	// !! 월 별로 테이블 등을 관리하고 싶을때 사용하는 함수.
	// !! 테이블이 없으면, 입력받은 테이블 이름으로 테이블을 생성한다.
	// 
	// Parameter : char형 템플릿 테이블 이름, char형 테이블 이름, char형 쿼리 메시지, vlist 가변인자
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::Query_Save(char* TemplateName, char* TableName, char *szStringFormat, va_list* vlist)
	{
		// 1. 가변인자의 값을 스트링 1개로 뽑아내기.		
		HRESULT retval = StringCbVPrintfA(m_cQueryUTF8, eQUERY_MAX_LEN, szStringFormat, *vlist);

		// 쿼리 글자 수 이상이면 로그찍고 끝.
		if (retval != S_OK)
		{
			// 실패 시 로그 남김
			g_DBLog->LogSave(false, L"DB_Connector", CSystemLog::en_LogLevel::LEVEL_ERROR,
				L"Query_Save(CreatTable) --> vlist Change Error...");

			g_DBDump->Crash();
		}

		// 2. 쿼리 날리기
		int Error, Count;
		while (1)
		{
			Error = mysql_query(m_pMySQL, m_cQueryUTF8);

			// 성공 시 break;
			if (Error == 0)
				break;

			// 연결이 끊긴거면 5회동안 재연결 시도
			else if (Error == CR_SOCKET_CREATE_ERROR ||
				Error == CR_CONNECTION_ERROR ||
				Error == CR_CONN_HOST_ERROR ||
				Error == CR_SERVER_HANDSHAKE_ERR ||
				Error == CR_SERVER_GONE_ERROR ||
				Error == CR_INVALID_CONN_HANDLE ||
				Error == CR_SERVER_LOST)
			{
				Count = 0;
				while (1)
				{
					// DB 연결
					m_pMySQL = mysql_real_connect(&m_MySQL, m_cDBIP, m_cDBUser, m_cDBPassword, m_cDBName, m_iDBPort, NULL, 0);
					if (m_pMySQL != NULL)
						break;

					// 멤버변수에 에러 셋팅
					SaveLastError();

					// 카운트 1 증가
					Count++;

					// 실패 시 로그 남김
					g_DBLog->LogSave(false, L"DB_Connector", CSystemLog::en_LogLevel::LEVEL_ERROR,
						L"Query() --> Connect Fail... (Count : %d)", Count);

					// 5번째 돌렸으면 서버 끈다. 더 이상 돌려도 어차피 저장 안됨.
					if (Count > 5)
						g_DBDump->Crash();

					// 바로 시도하면 실패할 가능성이 있기 때문에 Sleep(0)을 한다.
					Sleep(200);
				}
			}

			// 만약, 테이블이 없다면, 새로 만든다.
			else if (mysql_errno(&m_MySQL) == 1146)
			{
				// 테이블 생성 쿼리 만들기
				char query[1024] = { 0, };
				StringCbPrintfA(query, 1024, "CREATE TABLE `%s` LIKE %s", TableName, TemplateName);

				// 테이블 생성 쿼리 날림
				mysql_query(m_pMySQL, query);
			}

			// 연결 끊긴게 아니면 에러 보관
			else
			{
				SaveLastError();
				return;
			}
		}


		// 4. 결과 받은 후, 바로 result 한다.		
		m_pSqlResult = mysql_store_result(m_pMySQL);
		mysql_free_result(m_pSqlResult);
		m_iLastError = 0;
	}

	//////////////////////////////////////////////////////////////////////
	// 쿼리를 날린 뒤에 결과 뽑아오기.
	//
	// 결과가 없다면 NULL 리턴.
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	CDBConnector::FetchRow()
	{
		// 줄 단위로 넘긴다.
		return mysql_fetch_row(m_pSqlResult);
	}

	//////////////////////////////////////////////////////////////////////
	// 한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::FreeResult()
	{
		mysql_free_result(m_pSqlResult);
		m_iLastError = 0;
	}


	//////////////////////////////////////////////////////////////////////
	// mysql 의 LastError 를 맴버변수로 저장한다.
	//////////////////////////////////////////////////////////////////////
	void	CDBConnector::SaveLastError()
	{
		// 에러 넘버 보관
		m_iLastError = mysql_errno(&m_MySQL);

		// 에러 메시지 보관
		const char* cTempError = mysql_error(m_pMySQL);

		TCHAR tcError[1024];
		int len = MultiByteToWideChar(CP_UTF8, 0, cTempError, (int)strlen(cTempError), NULL, NULL);
		MultiByteToWideChar(CP_UTF8, 0, cTempError, (int)strlen(cTempError), tcError, len);

		StringCchPrintf(m_wcLastErrorMsg, _MyCountof(m_wcLastErrorMsg), 
			L"Mysql error : %s\n", tcError);

		// 마지막으로 날렸던 쿼리 보관
		len = MultiByteToWideChar(CP_UTF8, 0, m_cQueryUTF8, (int)strlen(m_cQueryUTF8), NULL, NULL);
		MultiByteToWideChar(CP_UTF8, 0, m_cQueryUTF8, (int)strlen(m_cQueryUTF8), m_wcQuery, len);
	}	
}

// TLS 버전
namespace Library_Jingyu
{
	// 생성자
	CBConnectorTLS::CBConnectorTLS(WCHAR *DBIP, WCHAR *User, WCHAR *Password, WCHAR *DBName, int DBPort)
	{
		// ------------------- 로그 저장할 파일 셋팅
		//g_DBLog->SetDirectory(L"DBConnector");
		//g_DBLog->SetLogLeve((CSystemLog::en_LogLevel)CSystemLog::en_LogLevel::LEVEL_DEBUG);

		// 1. DBConnector 보관용 스택
		m_stackConnector = new CLF_Stack< CDBConnector*>(false);

		// 2. TLSIndex를 얻어온다.
		m_dwTLSIndex = TlsAlloc();
		if (m_dwTLSIndex == TLS_OUT_OF_INDEXES)
			g_DBDump->Crash();

		// -----------------------------------
		// 인자값들 멤버변수에 저장
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
	}


	// 소멸자
	// 스택에 보관하고 있는 DBConnector delete.
	// 자동으로 DBConnector의 소멸자 호출
	CBConnectorTLS::~CBConnectorTLS()
	{
		LONG Count = m_stackConnector->GetInNode();

		// 스택에 들어있는 DBConnector만큼 돌면서 delete
		while (Count > 0)
		{
			CDBConnector* retval = m_stackConnector->Pop();
			delete retval;
			Count--;
		}

	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB 연결
	//
	// return : 성공 시 true, 실패 시 false.
	//////////////////////////////////////////////////////////////////////
	bool		CBConnectorTLS::Connect(void)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, DBConnector셋팅
		if (NowDB == nullptr)
		{
			NowDB = new CDBConnector(m_wcDBIP, m_wcDBUser, m_wcDBPassword, m_wcDBName, m_iDBPort);

			// 새로운 디비 보관
			m_stackConnector->Push(NowDB);

			if (TlsSetValue(m_dwTLSIndex, NowDB) == 0)
			{
				DWORD Error = GetLastError();
				g_DBDump->Crash();
			}

		}

		// 2. 꺼내온 DBConnector의 connect 함수 호출 후, 결과 값 리턴
		return NowDB->Connect();
	}

	//////////////////////////////////////////////////////////////////////
	// MySQL DB 끊기
	//
	// return : 정상적으로 끊길 시 true
	//////////////////////////////////////////////////////////////////////
	bool		CBConnectorTLS::Disconnect(void)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, Crash
		if (NowDB == nullptr)
			g_DBDump->Crash();

		// 2. 꺼내온 DBConnector의 Disconnect 함수 호출 후, 결과 값 리턴
		return NowDB->Disconnect();
	}


	//////////////////////////////////////////////////////////////////////
	// 쿼리 날리고 결과셋 임시 보관
	//
	// Parameter : char형 쿼리 메시지
	//////////////////////////////////////////////////////////////////////
	void		CBConnectorTLS::Query(char *szStringFormat, ...)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, DBConnector셋팅
		if (NowDB == nullptr)
		{
			NowDB = new CDBConnector(m_wcDBIP, m_wcDBUser, m_wcDBPassword, m_wcDBName, m_iDBPort);

			// 새로운 디비 보관
			m_stackConnector->Push(NowDB);

			if (TlsSetValue(m_dwTLSIndex, NowDB) == 0)
			{
				DWORD Error = GetLastError();
				g_DBDump->Crash();
			}
		}

		va_list vlist;
		va_start(vlist, szStringFormat);

		// 2. 꺼내온 DBConnector의 Query 함수 호출
		NowDB->Query(szStringFormat, &vlist);

		// 3. 다 쓴 가변인자 리소스 해제
		va_end(vlist);
	}

	//////////////////////////////////////////////////////////////////////
	// DBWriter 스레드의 Save 쿼리 전용
	// 결과셋을 저장하지 않음.
	// 
	// Parameter : char형 쿼리 메시지
	//////////////////////////////////////////////////////////////////////
	void		CBConnectorTLS::Query_Save(char *szStringFormat, ...)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, DBConnector셋팅
		if (NowDB == nullptr)
		{
			NowDB = new CDBConnector(m_wcDBIP, m_wcDBUser, m_wcDBPassword, m_wcDBName, m_iDBPort);

			// 새로운 디비 보관
			m_stackConnector->Push(NowDB);

			if (TlsSetValue(m_dwTLSIndex, NowDB) == 0)
			{
				DWORD Error = GetLastError();
				g_DBDump->Crash();
			}
		}

		va_list vlist;
		va_start(vlist, szStringFormat);

		// 2. 꺼내온 DBConnector의 Query_Save 함수 호출 후, 결과 값 리턴
		NowDB->Query_Save(szStringFormat, &vlist);
		
		// 3. 다 쓴 가변인자 리소스 해제
		va_end(vlist);
	}


	//////////////////////////////////////////////////////////////////////
	// DBWriter 스레드의 Save 쿼리 전용
	// 결과셋을 저장하지 않음.
	// !! 월 별로 테이블 등을 관리하고 싶을때 사용하는 함수.
	// !! 테이블이 없으면, 입력받은 테이블 이름으로 테이블을 생성한다.
	// 
	// Parameter : char형 템플릿 테이블 이름, char형 테이블 이름, char형 쿼리 메시지
	//////////////////////////////////////////////////////////////////////
	void	CBConnectorTLS::Query_Save(char* TemplateName, char* TableName, char *szStringFormat, ...)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, DBConnector셋팅
		if (NowDB == nullptr)
		{
			NowDB = new CDBConnector(m_wcDBIP, m_wcDBUser, m_wcDBPassword, m_wcDBName, m_iDBPort);

			// 새로운 디비 보관
			m_stackConnector->Push(NowDB);

			if (TlsSetValue(m_dwTLSIndex, NowDB) == 0)
			{
				DWORD Error = GetLastError();
				g_DBDump->Crash();
			}
		}

		va_list vlist;
		va_start(vlist, szStringFormat);

		// 2. 꺼내온 DBConnector의 Query_Save 함수 호출 후, 결과 값 리턴
		NowDB->Query_Save(TemplateName, TableName, szStringFormat, &vlist);

		// 3. 다 쓴 가변인자 리소스 해제
		va_end(vlist);
	}


	//////////////////////////////////////////////////////////////////////
	// 쿼리를 날린 뒤에 결과 뽑아오기.
	// 결과가 없다면 NULL 리턴.
	// 
	// return : result의 Row. Row가 없을 시 null 리턴
	//////////////////////////////////////////////////////////////////////
	MYSQL_ROW	CBConnectorTLS::FetchRow(void)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, Crash
		if (NowDB == nullptr)
			g_DBDump->Crash();

		// 2. 꺼내온 DBConnector의 FetchRow 함수 호출 후, 결과 값 리턴
		return NowDB->FetchRow();
	}

	//////////////////////////////////////////////////////////////////////
	// 한 쿼리에 대한 결과 모두 사용 후 정리.
	//////////////////////////////////////////////////////////////////////
	void		CBConnectorTLS::FreeResult(void)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, Crash
		if (NowDB == nullptr)
			g_DBDump->Crash();

		// 2. 꺼내온 DBConnector의 FreeResult 함수 호출.
		NowDB->FreeResult();
	}


	//////////////////////////////////////////////////////////////////////
	// Error 얻기.
	//////////////////////////////////////////////////////////////////////
	int			CBConnectorTLS::GetLastError(void)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, Crash
		if (NowDB == nullptr)
			g_DBDump->Crash();

		// 2. 꺼내온 DBConnector의 GetLastError 함수 호출 후 결과값 리턴
		return NowDB->GetLastError();
	}

	WCHAR*		CBConnectorTLS::GetLastErrorMsg(void)
	{
		// 1. TLS에서 DBConnector를 꺼내온다.
		CDBConnector* NowDB = (CDBConnector*)TlsGetValue(m_dwTLSIndex);

		// 만약, 아직 DBConnector를 할당하지 않은 상태라면, Crash
		if (NowDB == nullptr)
			g_DBDump->Crash();

		// 2. 꺼내온 DBConnector의 GetLastErrorMsg 함수 호출 후 결과값 리턴
		return NowDB->GetLastErrorMsg();
	}

}