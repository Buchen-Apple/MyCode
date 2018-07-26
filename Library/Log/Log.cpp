#include "stdafx.h"
#include "Log.h"
#include <strsafe.h>
#include <direct.h>		//_mkdir or _wmkdir

namespace Library_Jingyu
{

	// --------------------------------------
	// 게임 로그 DB와 커넥트 할 때 필요한 정보들.
	// --------------------------------------
	#define GAMELOG_IP			"127.0.0.1"
	#define	GAMELOG_USER		"root"
	#define GAMELOG_PASSWORD	"034689"
	#define GAMELOG_DBNAME		"test_05"
	#define GAMELOG_DBPORT		3306

	#define QUERY_SIZE	1024
	#define SYSTEM_LOG_SIZE	1024


	// --------------------------------------
	// 게임 로그 관련 함수
	// --------------------------------------
	// 게임 로그와 커넥트하는 함수
	//
	// return값
	// 0 : 치명적이지는 않지만 에러 남음. 서버 끌정도는 아님
	// 1 : 정상적으로 로그 잘 남김
	// 2 : 이거 받으면 즉시 서버 종료!
	int Log_GameLogConnect(MYSQL *GameLog_conn, MYSQL *GameLog_connection)
	{
		// 초기화
		mysql_init(GameLog_conn);

		// 게임 로그와 DB 연결
		GameLog_connection = mysql_real_connect(GameLog_conn, GAMELOG_IP, GAMELOG_USER, GAMELOG_PASSWORD, GAMELOG_DBNAME, GAMELOG_DBPORT, NULL, 0);
		if (GameLog_connection == NULL)
		{
			printf("Mysql connection error : %s(%d)\n", mysql_error(GameLog_conn), mysql_errno(GameLog_conn));
			return 0;
		}

		//한글사용을위해추가.
		mysql_set_character_set(GameLog_connection, "utf8");

		return 1;
	}


	// 게임 Log 저장 함수
	// DB에 저장한다.
	// server / type / code / accountno / param1 ~ 4 / paramString 입력받기 가능
	//
	// return값
	// 0 : 치명적이지는 않지만 에러 남음. 서버 끌정도는 아님
	// 1 : 정상적으로 로그 잘 남김
	// 2 : 이거 받으면 즉시 서버 종료!
	int Log_GameLogSave(MYSQL *conn, MYSQL *connection, int iServer, int iType, int iCode, int iAccountNo,
		const char* paramString, int iParam1, int iParam2, int iParam3, int iParam4)
	{
		// 1. 테이블 이름 생성 
		// 테이블은 월 단위로 생성된다.
		// 쿼리문 안에 무조건 테이블 이름이 들어가야하기 때문에 매번 생성한다.
		char cTableName[200];

		SYSTEMTIME	stNowTime;
		GetLocalTime(&stNowTime);
		StringCbPrintfA(cTableName, 200, "gamelog_%d%02d", stNowTime.wYear, stNowTime.wMonth);

		// 2. 쿼리문 만들기
		char cQuery[QUERY_SIZE];
		StringCbPrintfA(cQuery, QUERY_SIZE, "INSERT INTO `%s`(`server`, `type`, `code`, `accountno`, `param1`, `param2`, `param3`, `param4`, `paramstring`) "
			"VALUES (%d, %d, %d, %d, %d, %d, %d, %d,'%s')",
			cTableName, iServer, iType, iCode, iAccountNo, iParam1, iParam2, iParam3, iParam4, paramString);


		// 3. 서버에 쿼리문 날리기
		int Error = mysql_query(conn, cQuery);

		// 4. 쿼리 실패 시 에러 처리
		if (Error != 0)
		{
			// case 1. 연결이 끊긴경우 ----> 5회동안 재연결 시도
			if (Error == CR_SOCKET_CREATE_ERROR ||
				Error == CR_CONNECTION_ERROR ||
				Error == CR_CONN_HOST_ERROR ||
				Error == CR_SERVER_HANDSHAKE_ERR ||
				Error == CR_SERVER_GONE_ERROR ||
				Error == CR_INVALID_CONN_HANDLE ||
				Error == CR_SERVER_LOST)
			{

				int Count = 0;
				while (Count < 5)
				{
					// DB 재연결
					connection = mysql_real_connect(conn, GAMELOG_IP, GAMELOG_USER, GAMELOG_PASSWORD, GAMELOG_DBNAME, GAMELOG_DBPORT, NULL, 0);
					if (connection != NULL)
						break;

					Count++;
				}

				// 5회 연결 시도했는데도 연결 실패면, 에러 찍고 서버 종료
				if (connection == NULL)
				{
					// 여기 시스템 로그 남겨야함. 출력도 하고!
					printf("Log_GameLogSave()1. Mysql connection error : %s(%d)\n", mysql_error(conn), mysql_errno(conn));
					return 2;
				}

				// 연결이 성공했으면 쿼리 다시 날려본다.
				else
				{
					// 또 에러가 났다면,
					if (mysql_query(connection, cQuery) != 0)
					{
						// 테이블이 없어서 난 에러일 경우, 테이블 생성후 다시 쿼리 날려본다.
						// 그래도 오류가 났으면, 시스템 로그 남기고 서버 종료
						if (mysql_errno(conn) == 1146)
						{
							if (CreateTableAndQuery(conn, connection, cTableName) != 0)
							{
								// 여기 시스템 로그 남겨야함. 출력도 하고!
								printf("Log_GameLogSave()2. Mysql query error : %s(%d)\n", mysql_error(conn), mysql_errno(conn));
								return 2;
							}
						}

						else
						{
							// 그게 아니라면 시스템 로그 남기고 서버 종료.
							// 여기 시스템 로그 남겨야함. 출력도 하고!
							printf("Log_GameLogSave()3. Mysql query error : %s(%d)\n", mysql_error(conn), mysql_errno(conn));
							return 2;
						}

					}
				}
			}

			// case 2. 월 등이 지나 테이블이 없다 ----> 테이블 생성 쿼리 날린 후, 쿼리문 다시 날림
			else if (mysql_errno(conn) == 1146)
			{
				if (CreateTableAndQuery(conn, connection, cTableName) != 0)
				{
					// 테이블 생성 쿼리 날리다가 에러났으면, 여기 시스템 로그 남겨야함. 출력도 하고!
					printf("Log_GameLogSave()4. Mysql query error : %s(%d)\n", mysql_error(conn), mysql_errno(conn));
					return 2;
				}

				// 테이블 생성 쿼리가 잘 갔으면, 원래 쿼리를 다시 날려봄.
				// 여기서도 에러가 났으면 그냥 서버 종료시킴...
				if (mysql_query(conn, cQuery) != 0)
				{
					// 여기 시스템 로그 남겨야함.출력도 하고!
					printf("Log_GameLogSave()5. Mysql query error : %s(%d)\n", mysql_error(conn), mysql_errno(conn));
					return 2;
				}

			}

			// case 3. 정말 쿼리문 에러라면  ----> 시스템 로그 남김
			else
			{
				// 여기 시스템 로그 남겨야함.출력도 하고!
				printf("Log_GameLogSave()6. Mysql query error : %s(%d)\n", mysql_error(conn), mysql_errno(conn));
				return 2;
			}

		}

		return 1;
	}


	// 한번 쿼리문을 날렸는데, 테이블이 없다면, 테이블 만든 후 쿼리를 다시 날리는 함수.
	//
	// return값
	// mysql_query의 리턴값을 그대로 리턴한다.
	int CreateTableAndQuery(MYSQL *conn, MYSQL *connection, char* cTableName)
	{
		// 테이블 생성 쿼리 제작
		char TableCreate[QUERY_SIZE];
		StringCbPrintfA(TableCreate, 200, "CREATE TABLE %s LIKE gamelog_template", cTableName);

		// 테이블 생성 쿼리 날리기
		int Error = mysql_query(connection, TableCreate);

		return Error;
	}











	// --------------------------------------
	// 시스템 로그 클래스
	// 시스템 로그는 "파일"에 저장한다.
	// --------------------------------------
	// 생성자
	// 싱글톤을 위해 Private으로 만듬.
	CSystemLog::CSystemLog()
	{
		// SRWLOCK 초기화
		InitializeSRWLock(&m_lSrwlock);

		// 로그 카운트 0으로 초기화
		m_ullLogCount = 0;
	}

	// 싱글톤 얻기
	CSystemLog* CSystemLog::GetInstance()
	{
		static CSystemLog cCSystemLog;
		return &cCSystemLog;
	}

	// 디렉토리 생성 후, 디렉토리 설정하는 함수
	//
	// return 값
	// false : 디렉토리 생성에 실패할 시, 화면에 에러 출력 후 false 리턴
	// true : 디렉토리 생성 성공
	bool CSystemLog::SetDirectory(const TCHAR* directory)
	{
		// 1. 디렉토리 생성하기
		if (_tmkdir(directory) == -1)
		{
			// 이미 디렉토리에 폴더가 있어서 실패한거면, 에러 아님
			// 그게 아니라면 에러를 화면에 출력.
			if (errno != EEXIST)
			{
				perror("폴더 생성 실패 - 폴더가 이미 있거나 부정확함\n");
				printf("errorno : %d", errno);
				return false;
			}
		}

		// 2. 설정된 디렉토리 보관하기.
		// 앞으로 파일 저장은 다 경로에 저장된다.
		_tcscpy_s(m_tcDirectory, _Mycountof(m_tcDirectory), directory);

		return true;
	}

	// 로그 레벨을 셋팅하는 함수
	// 여기서 셋팅된 레벨보다 값이 큰 로그만 남김
	// ex) m_iLogLevel이 LEVEL_DEBUG으로 셋팅된다면, LEVEL_DEBUG보다 큰 로그가 남음
	void CSystemLog::SetLogLeve(en_LogLevel level)
	{
		m_iLogLevel = level;
	}

	// 실제로 파일에 텍스틑를 저장하는 함수
	bool CSystemLog::FileSave(TCHAR* FileName, TCHAR* SaveText)
	{
		// 5. 파일 저장.
		// 락 걸고 한다. -------------
		AcquireSRWLockExclusive(&m_lSrwlock);

		FILE *fp;
		if (_tfopen_s(&fp, FileName, _T("at")) != 0)
		{
			// 파일 오픈 실패 시 메시지 출력.
			printf("CSystemLog. _tfopen_s() error. (%d)", errno);

			ReleaseSRWLockExclusive(&m_lSrwlock);
			return false;
		}

		if (_fputts(SaveText, fp) == EOF)
		{
			// 파일 쓰기 실패 시 메시지 출력
			printf("CSystemLog. _fputts() error. (%d)", errno);

			ReleaseSRWLockExclusive(&m_lSrwlock);
			return false;
		}

		fclose(fp);

		ReleaseSRWLockExclusive(&m_lSrwlock);
		// 락 푼다. ----------------

		return true;
	}

	// Log 저장 함수. 가변인자 사용
	//
	// return 값
	// true : 로그 정상적으로 저장됨 or 로그 레벨이 낮은 로그를 저장시도함. 문제는 아니니까 걍 화면에 출력만 하고 true 리턴
	// false : 로그 저장중 에러가 생김 등 로그 저장이 안된 상황에 false 반환.
	bool CSystemLog::LogSave(const TCHAR* type, en_LogLevel level, const TCHAR* stringFormat, ...)
	{
		// 1. 입력받은 로그 레벨 스트링 생성
		TCHAR tcLogLevel[10];

		switch (level)
		{
		case CSystemLog::LEVEL_DEBUG:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("DEBUG"));
			break;
		case CSystemLog::LEVEL_WARNING:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("WARNING"));
			break;
		case CSystemLog::LEVEL_ERROR:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("ERROR"));
			break;
		case CSystemLog::LEVEL_SYSTEM:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("SYSTEM"));
			break;
		default:
			printf("'level' Parameter Error!!!\n");
			return false;
		}

		// 2. 시간 구해오기. 이 함수에서는 모두 이 시간으로 작성한다.
		SYSTEMTIME	stNowTime;
		GetLocalTime(&stNowTime);

		// 3. 파일 이름 만들기
		// [년월_타입.txt]
		TCHAR tcFileName[100];
		HRESULT retval = StringCbPrintf(tcFileName, 100, _T("%s\\%d%02d_%s.txt"), m_tcDirectory, stNowTime.wYear, stNowTime.wMonth, type);

		// StringCbPrintf 에러처리. S_OK가 아니면 무조건 에러
		// 에러나면 에러났다고 로그 저장.
		if (retval != S_OK)
		{
			// 파일 이름을 못만들면 로그 저장 못함. 그냥 출력하고 끝.
			printf("LOG_ERROR_FileName. LogSave Error!!\n");

			return false;

		}

		// 4. 가변인자의 값을 스트링 1개로 뽑아내기.
		TCHAR tcContent[SYSTEM_LOG_SIZE];

		va_list vlist;
		va_start(vlist, stringFormat);
		retval = StringCbVPrintf(tcContent, SYSTEM_LOG_SIZE, stringFormat, vlist);
		va_end(vlist);

		// 인터락으로 로그 카운트 1 증가
		ULONGLONG Count = InterlockedIncrement(&m_ullLogCount);

		// StringCbVPrintf() 에러 처리. S_OK가 아니면 무조건 에러
		// 에러나면 에러났다고 로그 저장.
		if (retval != S_OK)
		{
			TCHAR tcLogSaveingError[SYSTEM_LOG_SIZE];

			// 아직 로그가 완성되지 않았기 때문에.. 복사 목표였던 tcContent를 출력해본다.
			// tcContent가 너무 길 수도 있으니 잘라낸다.
			// 약 700글자 정도
			_tcsncpy_s(tcContent, SYSTEM_LOG_SIZE, tcContent, 700);

			StringCbPrintf(tcLogSaveingError, SYSTEM_LOG_SIZE, _T("[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09u] %s-->%s\n"),
				_T("LOG_ERROR_Content"), stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, Count, type, tcContent);

			// 파일에 저장
			FileSave(tcFileName, tcLogSaveingError);

			printf("LOG_ERROR_Content. LogSave Error!!\n");

			return false;

		}


		// 4. 입력받은 타입 + 가변인자 값을 이용해 실제 파일에 저장할 로그 스트링, 화면에 출력할 스트링을 만든다.
		TCHAR tcSaveBuff[SYSTEM_LOG_SIZE];
		TCHAR tcConsoleBuff[SYSTEM_LOG_SIZE];

		// 저장용 로그 스트링 만들기 ---------------------
		retval = StringCbPrintf(tcSaveBuff, SYSTEM_LOG_SIZE, _T("[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09u] %s\n"),
			type, stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, Count, tcContent);

		// StringCbPrintf() 에러 처리. S_OK가 아니면 무조건 에러
		// 에러나면 에러났다고 로그 저장.
		if (retval != S_OK)
		{
			TCHAR tcLogSaveingError[SYSTEM_LOG_SIZE];

			// tcContent가 너무 길 수도 있으니 잘라낸다.
			// 약 700글자 정도
			_tcsncpy_s(tcContent, SYSTEM_LOG_SIZE, tcContent, 700);

			StringCbPrintf(tcLogSaveingError, SYSTEM_LOG_SIZE, _T("[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09u] %s-->%s\n"),
				_T("LOG_ERROR_SaveBuff"), stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, Count, type, tcContent);

			// 파일에 저장
			FileSave(tcFileName, tcLogSaveingError);

			printf("LOG_ERROR_SaveBuff. LogSave Error!!\n");

			return false;
		}

		// 화면 출력용 스트링 만들기 ---------------------
		retval = StringCbPrintf(tcConsoleBuff, SYSTEM_LOG_SIZE, _T("[%s] [%02d:%02d:%02d / %s ] %s\n"),
			type, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, tcContent);

		// StringCbPrintf() 에러 처리. S_OK가 아니면 무조건 에러
		// 에러나면 에러났다고 로그 저장.
		if (retval != S_OK)
		{
			TCHAR tcLogSaveingError[SYSTEM_LOG_SIZE];

			// tcContent가 너무 길 수도 있으니 잘라낸다.
			// 약 700글자 정도
			_tcsncpy_s(tcContent, SYSTEM_LOG_SIZE, tcContent, 700);

			StringCbPrintf(tcLogSaveingError, SYSTEM_LOG_SIZE, _T("[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09u] %s-->%s\n"),
				_T("LOG_ERROR_ConsoleBuff"), stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, Count, type, tcContent);

			// 파일에 저장
			FileSave(tcFileName, tcLogSaveingError);

			printf("LOG_ERROR_ConsoleBuff. LogSave Error!!\n");

			return false;
		}

		// 5. 화면에 출력한다.
		_tprintf(_T("%s"), tcConsoleBuff);


		// 6. 파일에 저장
		// 이번에 입력받은 로그 레벨이, 설정한 로그 레벨보다 크거나 같다면, 파일에 저장
		if (level >= m_iLogLevel)
			FileSave(tcFileName, tcSaveBuff);

		return true;

	}

	// 16진수를 10진수로 변환
	// Source에 있는 데이터 중, Bufflen만큼을 16진수로 변환 후 Desc에 저장한다.
	void CSystemLog::HexToDecimal(char* Desc, BYTE* Source, int Bufflen)
	{
		BYTE* pin = Source;
		const char* hex = "0123456789ABCDEF";

		char* pout = Desc;

		for (int i = 0; i < Bufflen - 1; ++i)
		{
			*pout++ = hex[(*pin >> 4) & 0xF];
			*pout++ = hex[(*pin++) & 0xF];
			*pout++ = ':';
		}
		*pout++ = hex[(*pin >> 4) & 0xF];
		*pout++ = hex[(*pin++) & 0xF];
		*pout = 0;
	}

	// Log를 Hex로 저장하는 함수.
	// 패킷 오류났을 시 패킷을 남기는 용도이며, 무조건 BYTE형태로 Buff받는다는 가정!!
	//
	// return 값
	// true : 로그 정상적으로 저장됨 or 로그 레벨이 낮은 로그를 저장시도함. 문제는 아니니까 걍 화면에 출력만 하고 true 리턴
	// false : 로그 저장중 에러가 생김 등 로그 저장이 안된 상황에 false 반환.
	bool CSystemLog::LogHexSave(const TCHAR* type, en_LogLevel level, const TCHAR* PacketDesc, BYTE* Buff, int Bufflen)
	{
		// 1. 입력받은 로그 레벨 스트링 생성
		TCHAR tcLogLevel[10];

		switch (level)
		{
		case CSystemLog::LEVEL_DEBUG:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("DEBUG"));
			break;
		case CSystemLog::LEVEL_WARNING:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("WARNING"));
			break;
		case CSystemLog::LEVEL_ERROR:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("ERROR"));
			break;
		case CSystemLog::LEVEL_SYSTEM:
			_tcscpy_s(tcLogLevel, _Mycountof(tcLogLevel), _T("SYSTEM"));
			break;
		default:
			printf("'level' Parameter Error!!!\n");
			return false;
		}

		// 2. 시간 구해오기. 이 함수에서는 모두 이 시간으로 작성한다.
		SYSTEMTIME	stNowTime;
		GetLocalTime(&stNowTime);

		// 3. 파일 이름 만들기
		// [년월_타입.txt]
		TCHAR tcFileName[100];
		HRESULT retval = StringCbPrintf(tcFileName, 100, _T("%s\\%d%02d_%s.txt"), m_tcDirectory, stNowTime.wYear, stNowTime.wMonth, type);

		// StringCbPrintf 에러처리. S_OK가 아니면 무조건 에러
		// 에러나면 에러났다고 로그 저장.
		if (retval != S_OK)
		{
			// 파일 이름을 못만들면 로그 저장 못함. 그냥 출력하고 끝.
			printf("LOG_ERROR_FileName. LogSave Error!!\n");

			return false;

		}


		// 4. 입력받은 Buff를 16진수로 변환하기.
		// 16진수는 1바이트를 숫자 2개로 표시하기 때문에, 입력받은 len의 2배만큼 동적할당.
		// 바이트 단위로 : 를 찍어주기 위해 Bufflen만큼 추가.
		char* bContent = new char[Bufflen * 2 + Bufflen];
		HexToDecimal(bContent, Buff, Bufflen);

		// 변환한 16진수를 TCHAR 형태로 변경
		//len = MultiByteToWideChar(CP_UTF8, 0, Json_Buff, (int)strlen(Json_Buff), NULL, NULL);
		int len = (int)strlen(bContent);

		TCHAR* tcHex = new TCHAR[Bufflen * 2 + Bufflen];
		MultiByteToWideChar(CP_UTF8, 0, bContent, (int)strlen(bContent), tcHex, len);
		tcHex[(Bufflen * 2 + Bufflen) - 1] = '\0';


		// 5. 변환한 16진수와 PacketDesc인자를 합쳐서 1개의 로그 스트링으로 만든다.
		TCHAR tcContent[SYSTEM_LOG_SIZE];
		retval = StringCbPrintf(tcContent, SYSTEM_LOG_SIZE, _T("%s : %s"), PacketDesc, tcHex);

		delete[] bContent;
		delete[] tcHex;



		// 6. 실제 파일에 저장할 로그 스트링, 화면에 출력할 스트링을 만든다.
		TCHAR tcSaveBuff[SYSTEM_LOG_SIZE];
		TCHAR tcConsoleBuff[SYSTEM_LOG_SIZE];

		// 인터락으로 로그 카운트 1 증가
		ULONGLONG Count = InterlockedIncrement(&m_ullLogCount);

		// 저장용 로그 스트링 만들기 ---------------------
		retval = StringCbPrintf(tcSaveBuff, SYSTEM_LOG_SIZE, _T("[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09u] %s\n"),
			type, stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, Count, tcContent);

		// StringCbPrintf() 에러 처리. S_OK가 아니면 무조건 에러
		// 에러나면 에러났다고 로그 저장.
		if (retval != S_OK)
		{
			TCHAR tcLogSaveingError[SYSTEM_LOG_SIZE];

			// tcSaveBuff가 너무 길 수도 있으니 잘라낸다.
			// 약 700글자 정도
			_tcsncpy_s(tcSaveBuff, SYSTEM_LOG_SIZE, tcSaveBuff, 700);

			StringCbPrintf(tcLogSaveingError, SYSTEM_LOG_SIZE, _T("[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09u] %s-->%s\n"),
				_T("LOG_ERROR_SaveBuff"), stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, Count, type, tcSaveBuff);

			// 파일에 저장
			FileSave(tcFileName, tcLogSaveingError);

			printf("LOG_ERROR_SaveBuff. LogSave Error!!\n");

			return false;
		}

		// 화면 출력용 스트링 만들기 ---------------------
		retval = StringCbPrintf(tcConsoleBuff, SYSTEM_LOG_SIZE, _T("[%s] [%02d:%02d:%02d / %s ] %s\n"),
			type, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, tcContent);

		// StringCbPrintf() 에러 처리. S_OK가 아니면 무조건 에러
		// 에러나면 에러났다고 로그 저장.
		if (retval != S_OK)
		{
			TCHAR tcLogSaveingError[SYSTEM_LOG_SIZE];

			// tcConsoleBuff가 너무 길 수도 있으니 잘라낸다.
			// 약 700글자 정도
			_tcsncpy_s(tcConsoleBuff, SYSTEM_LOG_SIZE, tcConsoleBuff, 700);

			StringCbPrintf(tcLogSaveingError, SYSTEM_LOG_SIZE, _T("[%s] [%d-%02d-%02d %02d:%02d:%02d / %s / %09u] %s-->%s\n"),
				_T("LOG_ERROR_ConsoleBuff"), stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond, tcLogLevel, Count, type, tcConsoleBuff);

			// 파일에 저장
			FileSave(tcFileName, tcLogSaveingError);

			printf("LOG_ERROR_ConsoleBuff. LogSave Error!!\n");

			return false;
		}



		// 7. 화면에 출력한다.
		_tprintf(_T("%s"), tcConsoleBuff);



		// 8. 파일에 저장
		// 이번에 입력받은 로그 레벨이, 설정한 로그 레벨보다 크거나 같다면, 파일에 저장
		if (level >= m_iLogLevel)
			FileSave(tcFileName, tcSaveBuff);

		return true;
	}
}