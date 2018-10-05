#include "pch.h"
#include "CGameServer.h"
#include "Parser\Parser_Class.h"



// ------------------
// CGameSession의 함수
// (CGameServer의 이너클래스)
// ------------------
namespace Library_Jingyu
{
	// -----------------------
	// 생성자와 소멸자
	// -----------------------

	// 생성자
	CGameServer::CGameSession::CGameSession()
		:CMMOServer::cSession()
	{
		// 할거 없음
	}

	// 소멸자
	CGameServer::CGameSession::~CGameSession()
	{
		// 할거 없음
	}

	// -----------------
	// 가상함수
	// -----------------


	// --------------- AUTH 모드용 함수

	// 유저가 Auth 모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnAuth_ClientJoin()
	{

	}

	// 유저가 Auth 모드에서 나감
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnAuth_ClientLeave()
	{

	}

	// Auth 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CGameServer::CGameSession::OnAuth_Packet(CProtocolBuff_Net* Packet)
	{
		// 어떤 패킷을 받으면 GAME모드로 변경.
	}



	// --------------- GAME 모드용 함수

	// 유저가 Game모드로 변경됨
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnGame_ClientJoin()
	{

	}

	// 유저가 Game모드에서 나감
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnGame_ClientLeave()
	{

	}

	// Game 모드의 유저에게 packet이 옴
	//
	// Parameter : 패킷 (CProtocolBuff_Net*)
	// return : 없음
	void CGameServer::CGameSession::OnGame_Packet(CProtocolBuff_Net* Packet)
	{

	}



	// --------------- Release 모드용 함수
	
	// Release된 유저.
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::CGameSession::OnGame_ClientRelease()
	{

	}
}

// ---------------
// CGameServer
// CMMOServer를 상속받는 게임 서버
// ---------------
namespace Library_Jingyu
{
	// -----------------
	// 생성자와 소멸자
	// -----------------
	CGameServer::CGameServer()
		:CMMOServer()
	{
		m_GameServerDump = CCrashDump::GetInstance();

		// 파일 읽어오기
		// ------------------- Config정보 셋팅		
		if (SetFile(&m_stConfig) == false)
			m_GameServerDump->Crash();
	}

	CGameServer::~CGameServer()
	{
	}

	// GameServerStart
	// 내부적으로 CMMOServer의 Start, 세션 셋팅까지 한다.
	//
	// Parameter : 없음
	// return : 실패 시 false
	bool CGameServer::GameServerStart()
	{
		// 1. 세션 셋팅
		m_cGameSession = new CGameSession[m_stConfig.MaxJoinUser];
		SetSession(m_cGameSession, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2);
		
		// 2. 서버 시작
		if (Start(m_stConfig.BindIP, m_stConfig.Port, m_stConfig.CreateWorker, m_stConfig.ActiveWorker, m_stConfig.CreateAccept, 
			m_stConfig.Nodelay, m_stConfig.MaxJoinUser, m_stConfig.HeadCode, m_stConfig.XORCode1, m_stConfig.XORCode2) == false)
			return false;			

		return true;
	}

	// GameServerStop
	// 내부적으로 Stop 실행
	//
	// Parameter : 없음
	// return : 없음
	void CGameServer::GameServerStop()
	{
		// 1. 서버 종료
		if (GetServerState() == true)
			Stop();

		// 2. 세션 삭제
		delete[] m_cGameSession;
	}





	// -----------------------
	// 내부에서만 사용하는 함수
	// -----------------------

	// 파일에서 Config 정보 읽어오기
	// 
	// Parameter : config 구조체
	// return : 정상적으로 셋팅 시 true
	//		  : 그 외에는 false
	bool CGameServer::SetFile(stConfigFile* pConfig)
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
		// ChatServer config 읽어오기
		////////////////////////////////////////////////////////

		// 구역 지정 -------------------------
		if (Parser.AreaCheck(_T("MMOGAMESERVER")) == false)
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

		// xorcode1
		if (Parser.GetValue_Int(_T("XorCode1"), &pConfig->XORCode1) == false)
			return false;

		// xorcode2
		if (Parser.GetValue_Int(_T("XorCode2"), &pConfig->XORCode2) == false)
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
	// 가상함수
	// -----------------------

	// AuthThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnAuth_Update()
	{

	}

	// GameThread에서 1Loop마다 1회 호출.
	// 1루프마다 정기적으로 처리해야 하는 일을 한다.
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnGame_Update()
	{}

	// 새로운 유저 접속 시, Auth에서 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CGameServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CGameServer::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CGameServer::OnError(int error, const TCHAR* errorStr)
	{}



}