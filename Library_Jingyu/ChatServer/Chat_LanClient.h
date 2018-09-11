#ifndef __CHAT_LANCLIENT_H__
#define __CHAT_LANCLIENT_H__

#include "NetworkLib\NetworkLib_LanClinet.h"
#include "Protocol_Set\CommonProtocol.h"
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include <unordered_map>

namespace Library_Jingyu
{
	// CLanClient를 상속받는 Chat_LanClient
	// !! 채팅 서버가 로그인 서버에 접속하기 위한 용도 !!

	class Chat_LanClient :public CLanClient
	{		
		friend class CChatServer;

		// -----------------------
		// 내부 구조체
		// -----------------------
		// 토큰 구조체
		struct stToken
		{
			char	m_cToken[64];
		};

	private:
		// -----------------------
		// 멤버 변수
		// -----------------------

		// 토큰 구조체를 관리하는 TLS
		CMemoryPoolTLS< stToken >* m_MTokenTLS;

		// 토큰 자료구조 Lock
		SRWLOCK srwl;

		// 토큰키를 관리하는 자료구조
		// Key : AccountNO
		// Value : 토큰 구조체
		unordered_map<INT64, stToken*> m_umapTokenCheck;

	private:
		// -----------------------
		// 패킷 처리 함수
		// -----------------------

		// 새로운 유저가 로그인 서버로 들어올 시, 로그인 서버로부터 토큰키를 받는다.
		// 이 토큰키를 저장한 후 응답을 보내는 함수
		//
		// Parameter : 세션키, CProtocolBuff_Lan*
		// return : 없음
		void NewUserJoin(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);


		// -----------------------
		// 기능 함수
		// -----------------------

		// 토큰 관리 자료구조에, 새로 접속한 토큰 추가
		// 현재 umap으로 관리중
		// 
		// Parameter : AccountNo, stToken*
		// return : 추가 성공 시, true
		//		  : AccountNo가 중복될 시 false
		bool InsertTokenFunc(INT64 AccountNo, stToken* isnertToken);


		// 토큰 관리 자료구조에서, 토큰 검색
		// 현재 umap으로 관리중
		// 
		// Parameter : AccountNo
		// return : 검색 성공 시, stToken*
		//		  : 검색 실패 시 nullptr
		stToken* FindTokenFunc(INT64 AccountNo);


		// 토큰 관리 자료구조에서, 토큰 제거
		// 현재 umap으로 관리중
		// 
		// Parameter : AccountNo
		// return : 성공 시, 제거된 토큰 stToken*
		//		  : 검색 실패 시 nullptr
		stToken* EraseTokenFunc(INT64 AccountNo);


	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		Chat_LanClient();
		virtual ~Chat_LanClient();

		// -----------------------
		// 외부에서 호출하는 함수
		// -----------------------
		// Start 함수




	public:
		// -----------------------
		// 순수 가상함수
		// -----------------------

		// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnConnect(ULONGLONG SessionID);

		// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnDisconnect(ULONGLONG SessionID);

		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, CProtocolBuff_Lan*
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Lan* Payload);

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






#endif // !__CHAT_LANCLIENT_H__
