#include "pch.h"
#include "Chat_LanClient.h"

namespace Library_Jingyu
{

	// -----------------------
	// 생성자와 소멸자
	// -----------------------
	Chat_LanClient::Chat_LanClient()
		:CLanClient()
	{
		// 토큰을 관리하는 umap의 용량을 할당해둔다.
		m_umapTokenCheck.reserve(10000);

		// 토근 구조체 관리 TLS 동적할당
		m_MTokenTLS = new CMemoryPoolTLS< stToken >(500, false);

		// 락 초기화
		InitializeSRWLock(&srwl);
	}

	Chat_LanClient::~Chat_LanClient()
	{
		// 토근 구조체 관리 TLS 동적해재
		delete m_MTokenTLS;
	}




	// -----------------------
	// 패킷 처리 함수
	// -----------------------

	// 새로운 유저가 로그인 서버로 들어올 시, 로그인 서버로부터 토큰키를 받는다.
	// 이 토큰키를 저장한 후 응답을 보내는 함수
	void Chat_LanClient::NewUserJoin(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 1. 토큰 TLS에서 Alloc
		// !! 토큰 TLS  Free는, ChatServer에서, 실제 유저가 나갔을 때(OnClientLeave)에서 한다 !!
		stToken* NewToken = m_MTokenTLS->Alloc();

		// 2. AccountNo 빼온다.
		INT64 AccountNo;
		Payload->GetData((char*)&AccountNo, 8);

		// 3. 토큰키 빼온다.	
		// NewToken에 직접 넣는다.
		Payload->GetData((char*)NewToken->m_cToken, 64);

		// 4. 파라미터 빼온다.
		INT64 Parameter;
		Payload->GetData((char*)&Parameter, 8);

		// 5. 자료구조에 토큰 저장
		InsertTokenFunc(AccountNo, NewToken);
			   

		// 응답패킷 제작 후 보내기 -------------------
		
		// 1. 직렬화버퍼 Alloc
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		// 2. 타입 셋팅
		WORD Type = en_PACKET_SS_RES_NEW_CLIENT_LOGIN;
				
		// 3. 전송 데이터를 SendBUff에 넣는다
		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&AccountNo, 8);
		SendBuff->PutData((char*)&Parameter, 8);

		// 4. Send 한다.
		SendPacket(SessionID, SendBuff);
	}




	// -----------------------
	// 기능 함수
	// -----------------------

	// 토큰 관리 자료구조에, 새로 접속한 토큰 추가
	// 현재 umap으로 관리중
	// 
	// Parameter : AccountNo, stToken*
	// return : 추가 성공 시, true
	//		  : AccountNo가 중복될 시 false
	bool Chat_LanClient::InsertTokenFunc(INT64 AccountNo, stToken* isnertToken)
	{
		// umap에 추가
		AcquireSRWLockExclusive(&srwl);		// ---------------- Lock
		auto ret = m_umapTokenCheck.insert(make_pair(AccountNo, isnertToken));
		ReleaseSRWLockExclusive(&srwl);		// ---------------- Unock

		// 중복된 키일 시 false 리턴.
		if (ret.second == false)
			return false;

		return true;
	}


	// 토큰 관리 자료구조에서, 토큰 검색
	// 현재 umap으로 관리중
	// 
	// Parameter : AccountNo
	// return : 검색 성공 시, stToken*
	//		  : 검색 실패 시 nullptr
	Chat_LanClient::stToken* Chat_LanClient::FindTokenFunc(INT64 AccountNo)
	{
		AcquireSRWLockShared(&srwl);	// ---------------- Shared Lock
		auto FindToken = m_umapTokenCheck.find(AccountNo);
		ReleaseSRWLockShared(&srwl);	// ---------------- Shared UnLock

		if (FindToken == m_umapTokenCheck.end())
			return nullptr;

		return FindToken->second;
	}


	// 토큰 관리 자료구조에서, 토큰 제거
	// 현재 umap으로 관리중
	// 
	// Parameter : AccountNo
	// return : 성공 시, 제거된 토큰 stToken*
	//		  : 검색 실패 시 nullptr
	Chat_LanClient::stToken* Chat_LanClient::EraseTokenFunc(INT64 AccountNo)
	{
		// 1) map에서 유저 검색

		// erase까지가 한 작업이기 때문에, Exclusive 락 사용.
		AcquireSRWLockExclusive(&srwl);		// ---------------- Lock

		auto FindToken = m_umapTokenCheck.find(AccountNo);
		if (FindToken == m_umapTokenCheck.end())
		{
			ReleaseSRWLockExclusive(&srwl);	// ---------------- Unlock
			return nullptr;
		}

		stToken* ret = FindToken->second;

		// 2) 맵에서 제거
		m_umapTokenCheck.erase(FindToken);

		ReleaseSRWLockExclusive(&srwl);	// ---------------- Unlock

		return ret;
	}





	// -----------------------
	// 순수 가상함수
	// -----------------------

	// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
	//
	// parameter : 세션키
	// return : 없음
	void Chat_LanClient::OnConnect(ULONGLONG SessionID)
	{
		// 로그인 서버로, 로그인 패킷 보냄.

		// 1. 타입 셋팅
		WORD Type = en_PACKET_SS_LOGINSERVER_LOGIN;

		// 2. 로그인에게 알려줄, 접속하는 서버의 타입
		BYTE ServerType = en_PACKET_SS_LOGINSERVER_LOGIN::dfSERVER_TYPE_CHAT;

		// 3. 서버 이름 셋팅. 아무거나
		WCHAR ServerName[32] = L"Chat_LanClient";

		// 4. 직렬화 버퍼를 Alloc받아서 셋팅
		CProtocolBuff_Lan* SendBuff = CProtocolBuff_Lan::Alloc();

		SendBuff->PutData((char*)&Type, 2);
		SendBuff->PutData((char*)&ServerType, 1);
		SendBuff->PutData((char*)&ServerName, 64);

		// 5. Send한다.
		SendPacket(SessionID, SendBuff);
	}

	// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
	//
	// parameter : 세션키
	// return : 없음
	void Chat_LanClient::OnDisconnect(ULONGLONG SessionID)
	{
		// 지금 하는거 없음. 
		// 연결이 끊기면, LanClient에서 이미 연결 다시 시도함.
	}

	// 패킷 수신 완료 후 호출되는 함수.
	//
	// parameter : 유저 세션키, 받은 패킷
	// return : 없음
	void Chat_LanClient::OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload)
	{
		// 토큰키를 관리하는 자료구조에 토큰 삽입

		// 1. Type 얻어옴
		WORD Type;
		Payload->GetData((char*)&Type, 2);

		// 2. 타입에 따라 로직 처리
		switch (Type)
		{
			// 새로운 유저가 접속했음 (로그인 서버로부터 받음)
		case en_PACKET_SS_REQ_NEW_CLIENT_LOGIN:
			NewUserJoin(SessionID, Payload);
			break;


		default:
			break;
		}
	}

	// 패킷 송신 완료 후 호출되는 함수
	//
	// parameter : 유저 세션키, Send 한 사이즈
	// return : 없음
	void Chat_LanClient::OnSend(ULONGLONG SessionID, DWORD SendSize)
	{
		// 현재 할거 없음
	}

	// 워커 스레드가 깨어날 시 호출되는 함수.
	// GQCS 바로 하단에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void Chat_LanClient::OnWorkerThreadBegin()
	{
		// 현재 할거 없음
	}

	// 워커 스레드가 잠들기 전 호출되는 함수
	// GQCS 바로 위에서 호출
	// 
	// parameter : 없음
	// return : 없음
	void Chat_LanClient::OnWorkerThreadEnd()
	{
		// 현재 할거 없음
	}

	// 에러 발생 시 호출되는 함수.
	//
	// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
	//			 : 에러 코드에 대한 스트링
	// return : 없음
	void Chat_LanClient::OnError(int error, const TCHAR* errorStr)
	{
		// 현재 할거 없음
	}


}