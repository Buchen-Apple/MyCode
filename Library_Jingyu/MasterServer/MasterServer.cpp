#include "pch.h"
#include "MasterServer.h"


// -----------------------
//
// 마스터 Lan 서버
// 
// -----------------------
namespace Library_Jingyu
{

	// -------------------------------------
	// 내부에서만 사용하는 함수
	// -------------------------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CMasterServer_Lan::SetFile(stConfigFile* pConfig)
	{
		Parser Parser;

		// 파일 로드
		try
		{
			Parser.LoadFile(_T("MasterServer_Config.ini"));
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
		// Matchmaking Net서버의 config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MASTERSERVER")) == false)
			return false;

		// BindIP
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

		// Nodelay
		if (Parser.GetValue_Int(_T("Nodelay"), &pConfig->Nodelay) == false)
			return false;

		// 최대 접속 가능 유저 수
		if (Parser.GetValue_Int(_T("MaxJoinUser"), &pConfig->MaxJoinUser) == false)
			return false;

		// 로그 레벨
		if (Parser.GetValue_Int(_T("LogLevel"), &pConfig->LogLevel) == false)
			return false;


		return true;
	}

	






	// -----------------------
	// 외부에서 사용 가능한 함수
	// -----------------------

	// 서버 시작
	//
	// Parameter : 없음
	// return : 실패 시 false.
	bool CMasterServer_Lan::ServerStart()
	{
		// LanServer.Start 함수 호출
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker,
			m_stConfig.CreateAccept, m_stConfig.Nodelay, m_stConfig.MaxJoinUser) == false)
		{
			return false;
		}

		return true;		
	}

	// 서버 종료
	//
	// Parameter : 없음
	// return : 없음
	void CMasterServer_Lan::ServerStop()
	{
		// LanServer.Stop()함수 호출
		Stop();

		// 정리할 것 있으면 정리한다.
	}





	// -----------------------
	// 가상함수
	// -----------------------

	bool CMasterServer_Lan::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	void CMasterServer_Lan::OnClientJoin(ULONGLONG SessionID)
	{}

	void CMasterServer_Lan::OnClientLeave(ULONGLONG SessionID)
	{}

	void CMasterServer_Lan::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{}

	void CMasterServer_Lan::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	void CMasterServer_Lan::OnWorkerThreadBegin()
	{}

	void CMasterServer_Lan::OnWorkerThreadEnd()
	{}

	void CMasterServer_Lan::OnError(int error, const TCHAR* errorStr)
	{}




	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CMasterServer_Lan::CMasterServer_Lan()
		:CLanServer()
	{
		// 싱글톤 받기
		m_CDump = CCrashDump::GetInstance();
		m_CLog = CSystemLog::GetInstance();

		//  Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			m_CDump->Crash();

		// 로그 저장할 파일 셋팅
		m_CLog->SetDirectory(L"MasterServer");
		m_CLog->SetLogLeve((CSystemLog::en_LogLevel)m_stConfig.LogLevel);

		// 락 초기화
		InitializeSRWLock(&m_srwl_ClientKey_Umap);
		InitializeSRWLock(&m_srwl_AccountNo_Umap);
		InitializeSRWLock(&m_srwl_MatchServer_Umap);
		InitializeSRWLock(&m_srwl_BattleServer_Umap);

		// TLS 동적할당
		m_TLSPool_MatchServer = new CMemoryPoolTLS<CMatchingServer>(0, false);
		m_TLSPool_BattleServer = new CMemoryPoolTLS<CBattleServer>(0, false);
		m_TLSPool_Player = new CMemoryPoolTLS<CPlayer>(0, false);
		m_TLSPool_Room = new CMemoryPoolTLS<CRoom>(0, false);
	}

	// 소멸자
	CMasterServer_Lan::~CMasterServer_Lan()
	{
		// TLS 동적 해제
		delete m_TLSPool_MatchServer;
		delete m_TLSPool_BattleServer;
		delete m_TLSPool_Player;
		delete m_TLSPool_Room;
	}

}