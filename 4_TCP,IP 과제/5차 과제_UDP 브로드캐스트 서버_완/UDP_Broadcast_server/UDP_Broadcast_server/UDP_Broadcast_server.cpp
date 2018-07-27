#pragma comment(lib, "ws2_32")
#include <stdio.h>
#include <WS2tcpip.h>
#include <locale.h>
#include <tchar.h>

int _tmain()
{
	setlocale(LC_ALL, "korean");
	TCHAR tRoomName[30];
	DWORD dPortNumber;
	int retval;
	UINT size = sizeof(tRoomName);

	// 포트와 방 이름 입력받기
	_tprintf(_T("방 이름 입력 : "));
	_tscanf_s(_T("%s"), tRoomName, size);

	_tprintf(_T("포트 : "));
	_tscanf_s(_T("%d"), &dPortNumber);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
	{
		_tprintf(_T("윈속 초기화 실패\n"));
		return 1;
	}

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET)
	{
		_tprintf(_T("socket() 실패\n"));
		return 1;
	}

	// 바인딩
	SOCKADDR_IN recvaddr;
	ZeroMemory(&recvaddr, sizeof(recvaddr));
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	recvaddr.sin_port = htons(dPortNumber);

	retval = bind(sock, (SOCKADDR*)&recvaddr, sizeof(recvaddr));
	if (retval == SOCKET_ERROR)
	{
		_tprintf(_T("bind() 실패\n"));
		return 1;
	}
	
	BYTE bRecvBuff[10];
	BYTE bCheckBuff[10] = {0xff, 0xee, 0xdd, 0xaa, 0x00, 0x99, 0x77, 0x55, 0x33, 0x11};
	SOCKADDR_IN peeraddr;
	while (1)
	{
		// 데이터 리시브 대기
		int addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, (char*)bRecvBuff, sizeof(bRecvBuff), 0, (SOCKADDR*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR)
		{
			_tprintf(_T("recvfrom() 실패 : %d\n"), WSAGetLastError());
			return 1;
		}

		// 리시브 후, 헤더가 맞다면 보낸 상대에게 방 이름 샌드
		if (memcmp(bRecvBuff, bCheckBuff, sizeof(bCheckBuff)) == 0)
		{
			retval = sendto(sock, (char*)tRoomName, _tcslen(tRoomName) *2, 0, (SOCKADDR*)&peeraddr, sizeof(peeraddr));
			if (retval == SOCKET_ERROR)
			{
				_tprintf(_T("sendto() 실패\n"));
				return 1;
			}
			TCHAR tIPBuff[30];
			InetNtop(AF_INET, &peeraddr.sin_addr, tIPBuff, sizeof(tIPBuff));

			_tprintf(_T("[%s:%d] sendto 성공!\n"), tIPBuff, ntohs(peeraddr.sin_port));
		}	

	}

	closesocket(sock);
	WSACleanup();
	return 0;
}