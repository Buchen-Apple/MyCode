#include "pch.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include "HTTP_Exchange.h"
#include "Log/Log.h"
#include "CrashDump/CrashDump.h"


namespace Library_Jingyu
{

#define _Mycountof(Array)	sizeof(Array) / sizeof(TCHAR)

	CCrashDump* g_HTTP_Dump = CCrashDump::GetInstance();
	CSystemLog* g_HTTP_Log = CSystemLog::GetInstance();


	// 생성자
	// 
	// Parameter : 호스트(IP or 도메인), HostPort, Host가 도메인인지 여부 (false면 IP. 기본 false)	
	HTTP_Exchange::HTTP_Exchange(TCHAR* Host, USHORT Server_Port, bool DomainCheck)
	{
		// ---------------------------
		// 포트 저장
		// ---------------------------
		m_usServer_Port = Server_Port;

		// ---------------------------
		// Host 저장
		// ---------------------------
		_tcscpy_s(m_tHost, _Mycountof(m_tHost), Host);


		// ---------------------------
		// IP 저장.
		// host가 도메인이라면, 도메인을 IP로 변경
		// ---------------------------

		if (DomainCheck == true)
			DomainToIP(m_tIP, Host);

		// 그게 아니라면, Host에 IP주소가 입력되어 있는 것이니, IP에 Host를 복사한다.
		else
			_tcscpy_s(m_tIP, _Mycountof(m_tIP), Host);


		// 윈속 초기화
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			fputs("윈속 초기화 중 실패\n", stdout);
			g_HTTP_Dump->Crash();
		}

	}

	// 소멸자
	HTTP_Exchange::~HTTP_Exchange()
	{
		WSACleanup();
	}


	// http로 Request, Response하는 함수
	//
	// Parameter : Path, 전달할 Body, (out)Response결과를 받을 TCHAR형 변수 (UTF-16)
	// return : 성공 시 true, 실패 시 false
	bool HTTP_Exchange::HTTP_ReqANDRes(TCHAR* Path, TCHAR* RquestBody, TCHAR* ReturnBuff)
	{
		// -----------------
		// HTTP 헤더 + body 만들기
		// -----------------
		/*
		POST http://127.0.0.1/auth_reg.php/ HTTP/1.1\r\n
		User-Agent: XXXXXXX\r\n
		Host: 127.0.0.1\r\n
		Connection: Close\r\n
		Content-Length: 84\r\n
		\r\n
		*/

		// Type. 헤더의 첫 줄. Path가 없으면, Path를 헤더에 넣지 않음.
		TCHAR Type[100];
		if (_tcslen(Path))
			swprintf_s(Type, _Mycountof(Type), L"POST http://%s/%s HTTP/1.1\r\n", m_tHost, Path);

		else
			swprintf_s(Type, _Mycountof(Type), L"POST http://%s/ HTTP/1.1\r\n", m_tHost);

		TCHAR User_Agent[50] = L"User-Agent: CPP!!\r\n";

		TCHAR HeaderHost[50];
		swprintf_s(HeaderHost, _Mycountof(HeaderHost), L"Host: %s\r\n", m_tHost);

		TCHAR Connection[50] = L"Connection: Close\r\n";

		TCHAR Content_Length[50];
		int Length = (int)_tcslen(RquestBody); // << 마지막 null 문자 제외 문자열 길이 들어옴.
		swprintf_s(Content_Length, _Mycountof(Content_Length), L"Content-Length: %d\r\n\r\n", Length);

		TCHAR HTTP_Data[1000];
		swprintf_s(HTTP_Data, _Mycountof(HTTP_Data), L"%s%s%s%s%s%s", Type, User_Agent, HeaderHost, Connection, Content_Length, RquestBody);
		



		// -----------------
		// 만든 데이터를 UTF-8로 변환
		// -----------------
		char utf8_HTTP_Data[2000] = { 0, };

		// 아래 코드는 "_tcslen(HTTP_Data)"와 값이 완전히 동일함.
		// int len = WideCharToMultiByte(CP_UTF8, 0, HTTP_Data, (int)_tcslen(HTTP_Data), NULL, 0, NULL, NULL);
		int len = (int)_tcslen(HTTP_Data);

		WideCharToMultiByte(CP_UTF8, 0, HTTP_Data, (int)_tcslen(HTTP_Data), utf8_HTTP_Data, len, NULL, NULL);
		


		// -------------
		// 웹 서버로 연결하기
		// -------------
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// 내부에서는 Connect를 1회만 시도해 본다.
		// 실패하면 밖으로 false던진다.
		// 밖에서 일정 횟수만큼 HTTP_ReqANDRes() 함수를 호출해보는 형태로 로직 진행해야 함.
		if (HTTP_Connect(sock) == false)
		{			
			return false;
		}



		// ---------------------------
		// Recv / Send 시간 설정 (소켓 옵션 설정)
		// ---------------------------
		int Optval = 5000;

		if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&Optval, sizeof(Optval)) == SOCKET_ERROR)
		{
			fputs("SO_SNDTIMEO 설정 중 실패\n", stdout);

			closesocket(sock);
			g_HTTP_Dump->Crash();
			return false;
		}

		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&Optval, sizeof(Optval)) == SOCKET_ERROR)
		{
			fputs("SO_RCVTIMEO 설정 중 실패\n", stdout);

			closesocket(sock);
			g_HTTP_Dump->Crash();
			return false;
		}




		// -----------------
		// 웹 서버로 데이터 Send
		// -----------------
		if (send(sock, utf8_HTTP_Data, (int)strlen(utf8_HTTP_Data), 0) == SOCKET_ERROR)
		{
			closesocket(sock);
			g_HTTP_Dump->Crash();
			return false;
		}



		// -----------------
		// send 성공했으면 바로 recv 호출.
		// UTF-8의 형태로 Json_Buff에 넣어줌
		// -----------------
		char Json_Buff[2000];
		if (HTTP_Recv(sock, Json_Buff, 2000) == false)
		{
			closesocket(sock);
			return false;
		}





		// -----------------
		// HTTP_Recv()에서 받은 UTF-8을 UTF-16으로 변환 후, ReturnBuff에 넣음.
		// -----------------
		// 아래 코드는 "strlen(Json_Buff)"와 값이 동일함.
		//len = MultiByteToWideChar(CP_UTF8, 0, Json_Buff, (int)strlen(Json_Buff), NULL, NULL);
		len = (int)strlen(Json_Buff);

		MultiByteToWideChar(CP_UTF8, 0, Json_Buff, (int)strlen(Json_Buff), ReturnBuff, len);





		// -----------------
		// 소켓 정리
		// -----------------
		
		closesocket(sock);
		return true;
	}


	// 도메인을 IP로 변경
	//
	// Parameter : (out)변환된 Ip를 저장할 변수, 도메인
	// return : 성공 시 true, 실패 시 false
	bool HTTP_Exchange::DomainToIP(TCHAR* IP, TCHAR* Host)
	{
		SOCKADDR_IN* SockAddr;

		ADDRINFOW* pAddrInfo;
		if (GetAddrInfo(Host, _T("0"), NULL, &pAddrInfo) != 0)
		{
			int errorcode = WSAGetLastError();
			_tprintf(_T("GetAddrInfo Fail.. : %d\n"), errorcode);
			return false;
		}

		SockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
		IN_ADDR Add = SockAddr->sin_addr;

		// 네트워크 정렬 -> 호스트 정렬로 변경
		ntohl(Add.s_addr);

		//IPv4 숫자 -> 스트링으로 변경
		InetNtop(AF_INET, &Add, IP, sizeof(IP));

		// 리소스 해제
		FreeAddrInfo(pAddrInfo);

		return true;

	}

	// 연결된 웹 서버에게 Recv 하는 함수
	//
	// Parameter : 웹서버와 연결된 Socket, (out)리턴받을 char형 변수(UTF-8), char형 변수의 size
	// return : 정상적으로 모두 받을 시 true
	bool HTTP_Exchange::HTTP_Recv(SOCKET sock, char* ReturnBuff, int BuffSize)
	{
		while (1)
		{
			char recvBuff[1024];
			// --------------
			// recv()
			// --------------
			LONG Check = recv(sock, recvBuff, 1024, 0);
			if (Check == SOCKET_ERROR)
			{
				return false;
			}

			// 정상종료 시 break;
			if (Check == 0)
				break;


			// --------------
			// 헤더의 HTTP code 골라내기
			// --------------
			char* startPtr = strstr(recvBuff, " ");
			++startPtr;
			char* endPtr = strstr(startPtr, " ");

			__int64 Size = endPtr - startPtr;

			char Code[10];
			strncpy_s(Code, 10, startPtr, Size);




			// --------------
			// HTTP code 검사하기. 200아니면 다 에러
			// --------------		
			if (strcmp(Code, "200") != 0)
			{
				printf("Recv Header Code Error...(%s)\n", Code);
				g_HTTP_Dump->Crash();
				return false;
			}




			// --------------
			// 아직 헤더를 다 안받았으면, 다시 recv하러 간다.
			// --------------	
			char* HeaderEndPtr = strstr(recvBuff, "\r\n\r\n");
			if (HeaderEndPtr == NULL)
				continue;




			// --------------
			// 헤더를 다 받았으면, Content-Length를 알아와서 그만큼 recv했는지 확인
			// "Content-Length: " 는 띄어쓰기까지 포함해 총 16글자. 이 스트링 다음에 길이가 스트링으로 들어있다.
			// --------------	
			startPtr = strstr(recvBuff, "Content-Length: ");
			startPtr += 16;
			endPtr = strstr(startPtr, "\r\n");

			Size = endPtr - startPtr;

			char LengthString[10];
			strncpy_s(LengthString, 10, startPtr, Size);

			int Content_Length = atoi(LengthString);

			if (Content_Length == 0)
				return false;

			// HeaderEndPtr에는 헤더의 끝(\r\n\r\n)부터 들어있다.
			// 즉, Content_Length + 4 만큼 length가 있어야 한다.
			int BodyLength = (int)strlen(HeaderEndPtr);

			// 아직 덜받았으면 다시 처음으로 돌아가서 Recv한다.
			if (BodyLength < Content_Length + 4)
				continue;



			// -------------
			// 다 받았으면, HeaderEndPtr를 4칸 앞으로(\r\n\r\n이동)이동시킨 후, 
			// 리턴버퍼에 copy한다.
			// -------------
			HeaderEndPtr += 4;
			strncpy_s(ReturnBuff, BuffSize, HeaderEndPtr, Content_Length);
			break;
		}

		return true;
	}


	// 웹 서버로 Connect하는 함수
	//
	// Parameter : 할당된 소켓
	// return : 성공 시 true, 실패 시 false
	bool HTTP_Exchange::HTTP_Connect(SOCKET sock)
	{
		// -----------------
		// 웹 서버와 connect (논블락)
		// -----------------
		// 논블락 소켓으로 변경
		ULONG on = 1;
		if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR)
		{
			printf("NoneBlock Change Fail...\n");
			g_HTTP_Dump->Crash();
			return false;
		}

		SOCKADDR_IN clientaddr;
		ZeroMemory(&clientaddr, sizeof(clientaddr));
		clientaddr.sin_family = AF_INET;
		clientaddr.sin_port = htons(m_usServer_Port);
		InetPton(AF_INET, m_tIP, &clientaddr.sin_addr.S_un.S_addr);

		// connect 시도
		connect(sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));

		DWORD Check = WSAGetLastError();

		// 이미 연결된 경우
		if (Check == WSAEISCONN)
		{
			return true;
		}

		// 우드블럭이 뜬 경우
		else if (Check == WSAEWOULDBLOCK)
		{
			// 쓰기셋, 예외셋, 셋팅
			FD_SET wset;
			FD_SET exset;
			wset.fd_count = 0;
			exset.fd_count = 0;

			wset.fd_array[wset.fd_count] = sock;
			wset.fd_count++;

			exset.fd_array[exset.fd_count] = sock;
			exset.fd_count++;

			// timeval 셋팅. 500m/s  대기
			TIMEVAL tval;
			tval.tv_sec = 0;
			tval.tv_usec = 500000;

			// Select()
			DWORD retval = select(0, 0, &wset, &exset, &tval);

			// 쓰기셋에 반응온 것이 아니면 무조건 false 리턴
			if (wset.fd_count <= 0)
			{		
				return false;
			}
		}
		
		/*
		// connect 시도
		int iReConnectCount = 0;
		while (1)
		{
			connect(sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));

			DWORD Check = WSAGetLastError();

			// 이미 연결된 경우
			if (Check == WSAEISCONN)
			{
				break;
			}

			// 우드블럭이 뜬 경우
			else if (Check == WSAEWOULDBLOCK)
			{
				// 쓰기셋, 예외셋, 셋팅
				FD_SET wset;
				FD_SET exset;
				wset.fd_count = 0;
				exset.fd_count = 0;

				wset.fd_array[wset.fd_count] = sock;
				wset.fd_count++;

				exset.fd_array[exset.fd_count] = sock;
				exset.fd_count++;

				// timeval 셋팅. 500m/s  대기
				TIMEVAL tval;
				tval.tv_sec = 0;
				tval.tv_usec = 500000;

				// Select()
				DWORD retval = select(0, 0, &wset, &exset, &tval);


				// 에러 발생여부 체크
				if (retval == SOCKET_ERROR)
				{
					printf("Select error..(%d)\n", WSAGetLastError());					

					closesocket(sock);
					g_HTTP_Dump->Crash();
					return false;

				}

				// 타임아웃 체크
				else if (retval == 0)
				{
					printf("Select Timeout..\n");

					// 5회 재시도한다.
					iReConnectCount++;
					if (iReConnectCount == 5)
					{
						closesocket(sock);
						g_HTTP_Dump->Crash();
					}

					continue;
				}

				// 반응이 있다면, 예외셋에 반응이 왔는지 체크
				else if (exset.fd_count > 0)
				{
					//예외셋 반응이면 실패한 것.
					printf("Select ---> exset problem..\n");

					closesocket(sock);
					g_HTTP_Dump->Crash();
					return false;
				}

				// 쓰기셋에 반응이 왔는지 체크
				// 여기까지 오면 사실 그냥 성공이지만 한번 더 체크해봄
				else if (wset.fd_count > 0)
				{
					break;
				}

				// 여기까지 오면 뭔지 모를 에러..
				else
				{
					printf("Select ---> UnknownError..(retval %d, LastError : %d)\n", retval, WSAGetLastError());

					closesocket(sock);
					g_HTTP_Dump->Crash();
					return false;
				}

			}			
		}
		*/

		// -------------
		// 다시 소켓을 블락으로 변경
		// -------------
		on = 0;
		if (ioctlsocket(sock, FIONBIO, &on) == SOCKET_ERROR)
		{
			closesocket(sock);
			g_HTTP_Dump->Crash();
			return false;
		}

		// -------------
		// Time_Wait을 남기지 않기 위해 Linger옵션 설정.
		// -------------
		LINGER optval;
		optval.l_onoff = 1;
		optval.l_linger = 0;
		if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval)) == SOCKET_ERROR)
		{
			closesocket(sock);
			g_HTTP_Dump->Crash();
			return false;
		}

		return true;
	}
}
