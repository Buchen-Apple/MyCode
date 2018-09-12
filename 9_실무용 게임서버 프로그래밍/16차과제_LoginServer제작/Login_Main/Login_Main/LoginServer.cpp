#include "pch.h"
#include "LoginServer.h"

// ----------------------------------
// LoginServer(LanServer)
// ----------------------------------
namespace Library_Jingyu
{
	// 부모 셋팅
	//
	// Parameter : CLogin_NetServer*
	// return : 없음
	void CLogin_LanServer::ParentSet(CLogin_NetServer* parent)
	{
		m_cParentS = parent;
	}


	// GameServer, ChatServer의 LanClient에 새로운 유저 접속 Send 
	// --- CLogin_NetServer의 "OnClientJoin"에서 호출 ---
	// 
	// Parameter : AccountNo, Token(64Byte)
	// return : 없음
	bool CLogin_LanServer::UserJoinSend(INT64 AccountNo, char* Token)
	{

	}


	// 생성자
	CLogin_LanServer::CLogin_LanServer()
	{
		// 하는거 없음
	}

	// 소멸자
	CLogin_LanServer::~CLogin_LanServer()
	{
		// 하는거 없음
	}




	// -----------------------------------
	// 가상함수
	// -----------------------------------

	// Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CLogin_LanServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CLogin_LanServer::OnClientJoin(ULONGLONG ClinetID)
	{

	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CLogin_LanServer::OnClientLeave(ULONGLONG ClinetID)
	{

	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CLogin_LanServer::OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload)
	{
		// 1. 받은 패킷 마샬링
		INT64 AccountNo;
		char Token[64];
		Payload->GetData((char*)&AccountNo, 8);
		Payload->GetData(Token, 64);

		// 2. AccountNo로 자료구조에서 검색



		// 3. 파라미터 검증



		// 4. NetServer의 UserLoginPacketSend함수 호출
		m_cParentS->UserLoginPacketSend(AccountNo);
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CLogin_LanServer::OnSend(ULONGLONG ClinetID, DWORD SendSize)
	{

	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_LanServer::OnWorkerThreadBegin()
	{

	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_LanServer::OnWorkerThreadEnd()
	{

	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CLogin_LanServer::OnError(int error, const TCHAR* errorStr)
	{

	}




}



// ----------------------------------
// LoginServer(NetServer)
// ----------------------------------
namespace Library_Jingyu
{
	// 생성자
	CLogin_NetServer::CLogin_NetServer()
	{
		m_cLanS = new CLogin_LanServer;

		// LanServer에 this 셋팅
		m_cLanS->ParentSet(this);
	}

	// 소멸자
	CLogin_NetServer::~CLogin_NetServer()
	{

	}



	// 유저에게 로그인 완료 데이터 Send
	// --- CLogin_LanServer의 "OnRecv"에서 호출 --- 
	//
	// Parameter : AccountNo
	// return : 없음
	bool CLogin_NetServer::UserLoginPacketSend(INT64 AccountNo)
	{
		
		return true;
	}


	// Accept 직후, 호출된다.
	//
	// parameter : 접속한 유저의 IP, Port
	// return false : 클라이언트 접속 거부
	// return true : 접속 허용
	bool CLogin_NetServer::OnConnectionRequest(TCHAR* IP, USHORT port)
	{
		return true;
	}

	// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
	//
	// parameter : 접속한 유저에게 할당된 세션키
	// return : 없음
	void CLogin_NetServer::OnClientJoin(ULONGLONG SessionID)
	{
		
	}

	// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 유저 세션키
	// return : 없음
	void CLogin_NetServer::OnClientLeave(ULONGLONG SessionID)
	{}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void CLogin_NetServer::OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload)
	{
		// 1. 마샬링


		// 랜서버를 통해, 로그인서버와 채팅서버에 새로운 유저 접속패킷 보냄
		m_cLanS->UserJoinSend();


	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void CLogin_NetServer::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_NetServer::OnWorkerThreadBegin()
	{}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void CLogin_NetServer::OnWorkerThreadEnd()
	{}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void CLogin_NetServer::OnError(int error, const TCHAR* errorStr)
	{}





}