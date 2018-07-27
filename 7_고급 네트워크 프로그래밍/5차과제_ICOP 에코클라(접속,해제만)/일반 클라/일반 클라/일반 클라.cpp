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
	int aa = WSAGetLastError();

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
	system("mode con: cols=50 lines=20");

	_tsetlocale(LC_ALL, L"korean");

	int Count = 0;
	_tprintf(L"접속,해제할 더미 수 : ");

	_tscanf_s(L"%d", &Count);


	timeBeginPeriod(1);


	

	srand((UINT)time(NULL));	

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	int retval;


	// 상대편 서버 셋팅
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVERPORT);
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr.s_addr);

	int SleepTime = 1000;

	ULONGLONG LogicCount = 0;

	SOCKET* sock = new SOCKET[Count];

	while (1)
	{	
		ZeroMemory(sock, sizeof(SOCKET) * Count);

		_tprintf(L"접속 시작-----\n");
		// 접속하기
		for (int i = 0; i < Count; ++i)
		{
			// 소켓 생성
			sock[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sock[i] == INVALID_SOCKET)
				err_quit(L"socket()");

			// SO_LINGER 옵션 사용
			LINGER LingerOptval;
			LingerOptval.l_onoff = 1;
			LingerOptval.l_linger = 0;
			retval = setsockopt(sock[i], SOL_SOCKET, SO_LINGER, (char*)&LingerOptval, sizeof(LingerOptval));
			if (retval == SOCKET_ERROR)
				err_quit(L"setsockopt()");

			// 커넥트
			retval = connect(sock[i], (SOCKADDR*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR)
				err_quit(L"Connect()");

			//printf("%d명 접속 성공\n", i);

		}

		_tprintf(L"%d명 접속!\n접속해제 시작-----\n", Count );

		//Sleep(SleepTime);

		// 접속 해제
		for (int i = 0; i < Count; ++i)
		{
			closesocket(sock[i]);

			/*char buf[100];
			shutdown(sock[i], SD_SEND);
			int val = recvn(sock[i], buf, 100, 0);
			if (val == 0)
				closesocket(sock[i]);*/
			//printf("%d명 접속해제\n", i);
		}

		LogicCount++;
		_tprintf(L"%d명 접속, %d명 접속해제 (%lld)\n\n",Count, Count, LogicCount);	

		Sleep(SleepTime);
	}

	delete sock;

	WSACleanup();
	timeEndPeriod(1);

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

