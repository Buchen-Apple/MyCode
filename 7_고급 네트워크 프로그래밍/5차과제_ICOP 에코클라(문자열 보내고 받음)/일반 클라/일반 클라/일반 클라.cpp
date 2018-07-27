// 일반 클라.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <clocale>
#include <time.h>
#include <conio.h>

#include "NetworkFunc.h"

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")


#define SERVERIP	L"127.0.0.1"
#define SERVERPORT	9000

// ture면 샌드 중, false면 샌드중 아님
extern bool SendFlag;

// 이번에 슬립해야하는지 체크.
// true면 슬립 해야함. false면 슬립 안해야함
extern bool SleepFlag;



void err_quit(const TCHAR* msg)
{
	LPVOID	lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(const TCHAR* msg)
{
	LPVOID	lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	_tprintf(L"[%s] %s\n", msg, (TCHAR*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0)
	{
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;

		left -= received;
		ptr += received;
	}

	return (len - left);
}

// 헤더 만들기
void Create_Header();

// 에코 패킷 만들기
void Create_Packet_Echo();


extern SOCKET sock;


int _tmain()
{
	system("mode con: cols=60 lines=20");

	srand((UINT)time(NULL));

	int retval;

	_tsetlocale(LC_ALL, L"korean");

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
		err_quit(L"socket()");

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr.s_addr);

	retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
		err_quit(L"Connect()");

	// 논블락 소켓으로 변경
	
	ULONG on = 1;
	retval = ioctlsocket(sock, FIONBIO, &on);
	if (retval == SOCKET_ERROR)
	{
		printf("NoneBlock Change Fail...\n");
		return 0;
	}
	


	while (1)
	{	
		// 패킷 만들어서 샌드
		if (SendFlag == false)
		{
			fputs("[보낸 데이터] : ", stdout);

			CProtocolBuff header, Payload;
			Network_Send_Echo(&header, &Payload);
			if (SendPacket(&header, &Payload) == false)
				break;

			 SendPost();
		}
		

		// 리시브
		int Size = RecvPost();	
		if (Size == 0)
			break;
	}

	closesocket(sock);

	WSACleanup();

	printf("Exit!!!!\n");
	_getch();
    return 0;
}

// 헤더 만들기
void Create_Header()
{

}

// 에코 패킷 만들기
void Create_Packet_Echo()
{

}

