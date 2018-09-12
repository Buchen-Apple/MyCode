#ifndef __LOGIN_SERVER_H__
#define __LOGIN_SERVER_H__

#include "NetworkLib\NetworkLib_NetServer.h"
#include "NetworkLib\NetworkLib_LanServer.h"


// ----------------------------------
// LoginServer(NetServer)
// ----------------------------------
namespace Library_Jingyu
{
	class CLogin_NetServer :public CNetServer
	{
		friend class CLogin_LanServer;

		// Chat, Game의 LanClient와 통신할 LanServer
		CLogin_LanServer* m_cLanS;

	private:
		// 유저에게 로그인 완료 데이터 Send
		// --- CLogin_LanServer의 "OnRecv"에서 호출 --- 
		//
		// Parameter : AccountNo
		// return : 없음
		bool UserLoginPacketSend(INT64 AccountNo);

	public:
		// 생성자
		CLogin_NetServer();

		// 소멸자
		virtual ~CLogin_NetServer();


		// Accept 직후, 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

		// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
		//
		// parameter : 접속한 유저에게 할당된 세션키
		// return : 없음
		virtual void OnClientJoin(ULONGLONG SessionID);

		// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 유저 세션키
		// return : 없음
		virtual void OnClientLeave(ULONGLONG SessionID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);
	};
}




// ----------------------------------
// LoginServer(LanServer)
// ----------------------------------
namespace Library_Jingyu
{	
	class CLogin_LanServer :public CLanServer
	{
		friend class CLogin_NetServer;

		// 내 부모(CLogin_NetServer)의 포인터
		CLogin_NetServer* m_cParentS;

	private:
		// 부모 셋팅
		//
		// Parameter : CLogin_NetServer*
		// return : 없음
		void ParentSet(CLogin_NetServer* parent);

		// GameServer, ChatServer의 LanClient에 새로운 유저 접속 Send 
		// --- CLogin_NetServer의 "OnRecv"에서 호출 ---
		// 
		// Parameter : AccountNo, Token(64Byte)
		// return : 없음
		bool UserJoinSend(INT64 AccountNo, char* Token);

	public:
		// 생성자
		CLogin_LanServer();

		// 소멸자
		virtual ~CLogin_LanServer();


		// Accept 직후, 호출된다.
		//
		// parameter : 접속한 유저의 IP, Port
		// return false : 클라이언트 접속 거부
		// return true : 접속 허용
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port);

		// 연결 후 호출되는 함수 (AcceptThread에서 Accept 후 호출)
		//
		// parameter : 접속한 유저에게 할당된 세션키
		// return : 없음
		virtual void OnClientJoin(ULONGLONG ClinetID);

		// 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 유저 세션키
		// return : 없음
		virtual void OnClientLeave(ULONGLONG ClinetID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG ClinetID, CProtocolBuff_Lan* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG ClinetID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);

		
	};
}


#endif // !__LOGIN_SERVER_H__
