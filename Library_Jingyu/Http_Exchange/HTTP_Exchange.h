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
		// 
		// Parameter : 호스트(IP or 도메인), HostPort, Host가 도메인인지 여부 (false면 IP. 기본 false)	
		HTTP_Exchange(TCHAR* Host, USHORT Server_Port, bool DomainCheck = false);

		// 소멸자
		~HTTP_Exchange();

		// http로 Request, Response하는 함수
		//
		// Parameter : Path, 전달할 Body, (out)Response결과를 받을 TCHAR형 변수 (UTF-16)
		// return : 성공 시 true, 실패 시 false
		bool HTTP_ReqANDRes(TCHAR* Path, TCHAR* RquestBody, TCHAR* ReturnBuff);


	private:
		// 도메인을 IP로 변경
		//
		// Parameter : (out)변환된 Ip를 저장할 변수, 도메인
		// return : 성공 시 true, 실패 시 false
		bool DomainToIP(TCHAR* IP, TCHAR* Host);

		// 연결된 웹 서버에게 Recv 하는 함수
		//
		// Parameter : 웹서버와 연결된 Socket, (out)리턴받을 char형 변수(UTF-8), char형 변수의 size
		// return : 정상적으로 모두 받을 시 true
		bool HTTP_Recv(SOCKET sock, char* ReturnBuff, int BuffSize);

		// 웹 서버로 Connect하는 함수
		//
		// Parameter : 
		// return : 성공 시 true, 실패 시 false
		bool HTTP_Connect(SOCKET sock);

	};
}


#endif // !__HTTP_EXCHANGE_H__
