#ifndef __DB_CONNECTOR_H__
#define __DB_CONNECTOR_H__

#include <Windows.h>
#include "Mysql\include\mysql.h"
#include "Mysql\include\errmsg.h"

#pragma comment(lib, "Mysql/lib/vs14/mysqlclient")

namespace Library_Jingyu
{
	class CDBConnector
	{
	public:
		
		enum en_DB_CONNECTOR
		{
			// 쿼리 1개의 길이
			eQUERY_MAX_LEN = 2048
		};

		//////////////////////////////////////////////////////////////////////
		// 생성자
		// 
		// Parameter : 연결할 DB IP, 사용자 이름, 비밀번호, 닉네임, 포트
		//////////////////////////////////////////////////////////////////////
		CDBConnector(WCHAR *DBIP, WCHAR *User, WCHAR *Password, WCHAR *DBName, int DBPort);

		//////////////////////////////////////////////////////////////////////
		// 소멸자
		//////////////////////////////////////////////////////////////////////
		virtual		~CDBConnector();

		//////////////////////////////////////////////////////////////////////
		// MySQL DB 연결
		//
		// return : 성공 시 true, 실패 시 false.
		// 실패 시, GetLastError와, GetLastErrorMsg를 이용해 에러 확인
		//////////////////////////////////////////////////////////////////////
		bool		Connect(void);

		//////////////////////////////////////////////////////////////////////
		// MySQL DB 끊기
		//
		// return : 정상적으로 끊길 시 true
		//////////////////////////////////////////////////////////////////////
		bool		Disconnect(void);


		//////////////////////////////////////////////////////////////////////
		// 쿼리 날리고 결과셋 임시 보관
		//
		// Parameter : WCHAR형 쿼리 메시지
		//////////////////////////////////////////////////////////////////////
		bool		Query(WCHAR *szStringFormat, ...);
		bool		Query_Save(WCHAR *szStringFormat, ...);	// DBWriter 스레드의 Save 쿼리 전용
																// 결과셋을 저장하지 않음.

		//////////////////////////////////////////////////////////////////////
		// 쿼리를 날린 뒤에 결과 뽑아오기.
		// 결과가 없다면 NULL 리턴.
		// 
		// return : result의 Row. Row가 없을 시 null 리턴
		//////////////////////////////////////////////////////////////////////
		MYSQL_ROW	FetchRow(void);

		//////////////////////////////////////////////////////////////////////
		// 한 쿼리에 대한 결과 모두 사용 후 정리.
		//////////////////////////////////////////////////////////////////////
		void		FreeResult(void);


		//////////////////////////////////////////////////////////////////////
		// Error 얻기.
		//////////////////////////////////////////////////////////////////////
		int			GetLastError(void) { return m_iLastError; };
		WCHAR		*GetLastErrorMsg(void) { return m_wcLastErrorMsg; }


	private:

		//////////////////////////////////////////////////////////////////////
		// mysql 의 LastError 를 맴버변수로 저장한다.
		//////////////////////////////////////////////////////////////////////
		void		SaveLastError(void);

	private:



		//-------------------------------------------------------------
		// MySQL 연결객체 본체
		//-------------------------------------------------------------
		MYSQL		m_MySQL;

		//-------------------------------------------------------------
		// MySQL 연결객체 포인터. 위 변수의 포인터임. 
		// 이 포인터의 null 여부로 연결상태 확인.
		//-------------------------------------------------------------
		MYSQL		*m_pMySQL;

		//-------------------------------------------------------------
		// 쿼리를 날린 뒤 Result 저장소.
		//
		//-------------------------------------------------------------
		MYSQL_RES	*m_pSqlResult;

		//-------------------------------------------------------------
		// DB 연결을 위한 정보 보관
		//
		//-------------------------------------------------------------
		char	m_cDBIP[32];
		char	m_cDBUser[128];
		char	m_cDBPassword[128];
		char	m_cDBName[128];

		WCHAR	m_wcDBIP[16];
		WCHAR	m_wcDBUser[64];
		WCHAR	m_wcDBPassword[64];
		WCHAR	m_wcDBName[64];

		int		m_iDBPort;

		WCHAR		m_wcQuery[eQUERY_MAX_LEN];
		char		m_cQueryUTF8[eQUERY_MAX_LEN];

		int			m_iLastError;
		WCHAR		m_wcLastErrorMsg[128];

	};

	class CBConnectorTLS
	{
		// TLS 인덱스
		int m_iTLSIndex;
	};

}

#endif // !__DB_CONNECTOR_H__
