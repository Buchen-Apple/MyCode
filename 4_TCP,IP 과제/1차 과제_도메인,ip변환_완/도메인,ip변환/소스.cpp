#pragma comment(lib, "ws2_32")
#include <locale.h>
#include <stdio.h>
#include <tchar.h>
#include <WinSock2.h>
#include <Ws2tcpip.h>

using namespace std;

int main()
{
	setlocale(LC_ALL, "korean");
	TCHAR tDomainName[20];	
	TCHAR Buff[33];
	IN_ADDR Addr[10];
	IN6_ADDR Addr_6[10];
	WSADATA wsa;
	SOCKADDR_IN* SockAddr;
	SOCKADDR_IN6* SockAddr6;

	// 윈속 초기화 에러처리
	if (WSAStartup(0x0202, &wsa) != 0)
	{
		int errorcode = WSAGetLastError();
		_tprintf(_T("WSAStartup 에러 : %d\n"), errorcode);
		return 1;
	}

	while (1)
	{
		// 저장소 초기화
		ADDRINFOW* pAddrInfo;	

		_tprintf(_T("도메인 이름 : "));
		_tscanf_s(_T("%s"), tDomainName, (unsigned)_countof(tDomainName));

		// 도메인 관련 에러 처리
		if (GetAddrInfo(tDomainName, _T("0"), NULL, &pAddrInfo) != 0)
		{
			int errorcode = WSAGetLastError();
			_tprintf(_T("GetAddrInfo 에러 : %d\n"), errorcode);
			break;
		}

		else
		{
			int i = 0;

			// 무한 반복 입력받는다.
			while (1)
			{
				// IPv4 데이터 저장
				if (pAddrInfo->ai_family == 2)
				{
					SockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
					Addr[i] = SockAddr->sin_addr;
				}

				// IPv6 데이터 저장
				else
				{
					SockAddr6 = (SOCKADDR_IN6*)pAddrInfo->ai_addr;
					Addr_6[i] = SockAddr6->sin6_addr;
				}
						
				// 네트워크 정렬 -> 호스트 정렬로 변경
				ntohl(Addr[i].s_addr);

				//IPv4 숫자 -> 스트링으로 변경
				if(pAddrInfo->ai_family == 2)
					InetNtop(AF_INET, &Addr[i], Buff, sizeof(Buff));

				
				//IPv6 숫자 -> 스트링으로 변경
				else
					InetNtop(AF_INET6, &Addr_6[i], Buff, sizeof(Buff));

				// 출력
				_tprintf(_T("IP : %s\n"), Buff);

				// 다음 IP 체크.
				if (pAddrInfo->ai_next != NULL)
				{
					pAddrInfo = pAddrInfo->ai_next;
					i++;
				}
				else
					break;

			}
			_tprintf(_T("\n"));
		}

		FreeAddrInfo(pAddrInfo);
	}

	WSACleanup();
	return 0;
}