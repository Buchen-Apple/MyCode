
// *********************************************
// 게임 로그와 시스템 로그를 사용할 수 있는 헤더파일
// 게임 로그 : DB에 저장
// 시스템 로그 : 파일 경로 지정 후 파일에 저장
// *********************************************

#ifndef __LOG_H__
#define __LOG_H__

#include "Mysql\include\mysql.h"
#include "Mysql\include\errmsg.h"

#pragma comment(lib, "Mysql/lib/vs14/mysqlclient.lib")



namespace Library_Jingyu
{
	#define _Mycountof(_array)  sizeof(_array) / sizeof(TCHAR)
	

	// 싱글톤 얻은 후, 디렉토리 생성 define
	#define SYSLOG_SET_DIRECTORY(_dir)								\
	CSystemLog* cLog = CSystemLog::GetInstance();					\
	cLog->SetDirectory(L"Test")										\

	// LogLevel 설정 define
	#define SYSLOG_SET_LEVEL(_level)								\
	cLog->SetLogLeve(_level)										\

	// 시스템 로그 남기기 (일반)
	#define LOG(_type, _level, _format, ...)						\
	cLog->LogSave(_type, _level, _format, ## __VA_ARGS__)			\

	// 시스템 로그 남기기 (16진수)
	// 무조건 BYTE형태로 Buff 받아야함.
	#define LOG_HEX(_type, _level, _desc, _buff)					\
	cLog->LogHexSave(_type, _level, _desc, _buff, sizeof(_buff))	\


	// --------------------------------------
	// 게임 로그 관련 함수
	// --------------------------------------

	// 게임 로그와 커넥트하는 함수
	//
	// return값
	// 0 : 치명적이지는 않지만 에러 남음. 서버 끌정도는 아님
	// 1 : 정상적으로 로그 잘 남김
	// 2 : 이거 받으면 즉시 서버 종료!
	int Log_GameLogConnect(MYSQL *GameLog_conn, MYSQL *GameLog_connection);

	// 게임 Log 저장 함수
	// DB에 저장한다.
	// server / type / code / accountno / param1 ~ 4 / paramString 입력받기 가능
	//
	// return값
	// 0 : 치명적이지는 않지만 에러 남음. 서버 끌정도는 아님ㄴ
	// 1 : 정상적으로 로그 잘 남김
	// 2 : 이거 받으면 즉시 서버 종료!
	int Log_GameLogSave(MYSQL *conn, MYSQL *connection, int iServer, int iType, int iCode, int iAccountNo,
		const char* paramString = "0", int iParam1 = 0, int iParam2 = 0, int iParam3 = 0, int iParam4 = 0);

	// 한번 쿼리문을 날렸는데, 테이블이 없다면, 테이블 만든 후 쿼리를 다시 날리는 함수.
	//
	// return값
	// mysql_query의 리턴값을 그대로 리턴한다.
	int CreateTableAndQuery(MYSQL *conn, MYSQL *connection, char* cTableName);








	// --------------------------------------
	// 시스템 로그 클래스
	// 시스템 로그는 "파일"에 저장한다.
	// --------------------------------------
	class CSystemLog
	{
	public:
		// 로그 레벨
		// ex)LEVEL_DEBUG으로 셋팅된다면, LEVEL_DEBUG보다 큰 레벨의 로그가 남음
		enum en_LogLevel
		{
			LEVEL_DEBUG = 0, LEVEL_WARNING, LEVEL_ERROR, LEVEL_SYSTEM
		};

	private:
		// 생성자
		// 싱글톤을 위해 Private으로 만듬.
		CSystemLog();

		// 로그 레벨 변수, 디렉토리 변수
		en_LogLevel m_iLogLevel;
		TCHAR m_tcDirectory[1024];

		// Log()함수에서 fopen 후 파일 저장할때 사용하는 락.
		SRWLOCK m_lSrwlock;

		// 로그 저장할 때 마다 안전하게 1씩 증가하는 카운트.
		// 로그에 함께 출력된다.
		ULONGLONG m_ullLogCount;

	private:
		// 실제로 파일에 텍스틑를 저장하는 함수
		bool FileSave(TCHAR* FileName, TCHAR* SaveText);

		// 16진수를 10진수로 변환
		void HexToDecimal(char* Desc, BYTE* Source, int Bufflen);
		
	public:
		// 싱글톤 얻기
		static CSystemLog* GetInstance();

		// 디렉토리 생성 후, 디렉토리 설정하는 함수
		bool SetDirectory(const TCHAR* directory);

		// 로그 레벨을 셋팅하는 함수
		// 여기서 셋팅된 레벨보다 값이 큰 로그만 남김
		// ex) m_iLogLevel이 LEVEL_DEBUG으로 셋팅된다면, LEVEL_DEBUG보다 큰 로그가 남음
		void SetLogLeve(en_LogLevel level);

		// Log 저장 함수. 가변인자 사용
		//
		// return 값
		// true : 로그 정상적으로 저장됨 or 로그 레벨이 낮은 로그를 저장시도함. 문제는 아니니까 걍 화면에 출력만 하고 true 리턴
		// false : 로그 저장중 에러가 생김 등 로그 저장이 안된 상황에 false 반환.
		bool LogSave(const TCHAR* type, en_LogLevel level, const TCHAR* stringFormat, ...);

		// Log를 Hex로 저장하는 함수.
		// 패킷 오류났을 시 패킷을 남기는 용도이며, 무조건 BYTE형태로 Buff받는다는 가정!!
		//
		// return 값
		// true : 로그 정상적으로 저장됨 or 로그 레벨이 낮은 로그를 저장시도함. 문제는 아니니까 걍 화면에 출력만 하고 true 리턴
		// false : 로그 저장중 에러가 생김 등 로그 저장이 안된 상황에 false 반환.
		bool LogHexSave(const TCHAR* type, en_LogLevel level, const TCHAR* PacketDesc, BYTE* Buff, int Bufflen);

	};
}


#endif // !__LOG_H__
