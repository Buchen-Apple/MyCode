#ifndef __HTTP_EXCHANGE_H__
#define __HTTP_EXCHANGE_H__


#include <windows.h>

namespace Library_Jingyu
{
	// 생성자에서 host / port를 입력받음.
	// 이후부터는 Path만 바꿔가면서 호출.
	class HTTP_Exchange
	{
	private:

		USHORT m_usServer_Port;
		TCHAR m_tHost[100];
		TCHAR m_tIP[50];


	public:
		// 생성자
		// 최초 생성 시, 서버 포트를 전달받음.
		// [호스트(IP나 도메인 둘 중 아무거나), 서버 포트, Host가 도메인인지 여부] 총 3개를 입력받음
		// true면 도메인. false면 IP]
		HTTP_Exchange(TCHAR* Host, USHORT Server_Port, bool DomainCheck = false);

		// Request / Response까지 해주는 함수.
		// [웹서버 URL, 전달할 Body, (out)UTF-16로 리턴받을 TCHAR형 변수, URL이 도메인인지 여부. true면 도메인. false면 IP]
		// 총 4개를 입력받음
		bool HTTP_ReqANDRes(TCHAR* Path, TCHAR* RquestBody, TCHAR* ReturnBuff);


	private:
		// HTTP_ReqANDRes() 함수 안에서 호출
		// Host에는 도메인이 들어있음.
		// Host도메인을 IP로 변경헤서 "IP"에 저장.
		bool DomainToIP(TCHAR* IP, TCHAR* Host);

		// HTTP_ReqANDRes()함수 안에서 호출.
		// 연결된 웹서버가 보내는 데이터를 모두 받음
		// UTF-8의 형태로 Return Buff에 넣어줌
		// 
		// 전달받는 인자값
		// [웹서버와 연결된 Socket, (out)UTF-8로 리턴받을 char형 변수]
		// 
		// 리턴값
		// ture : 정상적으로 모두 받음 / false : 무언가 문제가 생겨서 다 못받음.
		bool HTTP_Recv(SOCKET sock, char* ReturnBuff, int BuffSize);

	};
}


#endif // !__HTTP_EXCHANGE_H__
