#include "pch.h"

#include <Ws2tcpip.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

#include "Network_Func.h"
#include "Protocol_Set/CommonProtocol_2.h"

using namespace rapidjson;

// 토큰
#define TOKEN_T	L"sakljflksajfklsafnkscksjlfasf12345adflsadlfjkefafkj%kldjsklf121"
#define TOKEN_C	"sakljflksajfklsafnkscksjlfasf12345adflsadlfjkefafkj%kldjsklf121"

// VerCode
#define VERCODE 123456

// Disconenct 확률
#define DISCONNECT_RATE 20


namespace Library_Jingyu
{
	// 직렬화 버퍼 1개의 크기
	// 각 서버에 전역 변수로 존재해야 함.
	LONG g_lNET_BUFF_SIZE = 512;
}

// ------------------------------------
// 자료구조 관리용 함수
// ------------------------------------

// 더미 관리 자료구조에 더미 추가
// 현재 umap으로 관리중
// 
// Parameter : AccountNo, stDummy*
// return : 추가 성공 시, true
//		  : AccountNo가 중복될 시(이미 접속중인 유저) false
bool cMatchTestDummy::InsertDummyFunc(UINT64 AccountNo, stDummy* insertDummy)
{
	// 1. umap에 추가		
	auto ret = m_uamp_Dummy.insert(make_pair(AccountNo, insertDummy));

	// 2. 중복된 키일 시 false 리턴.
	if (ret.second == false)
		return false;

	// 3. 아니면 true 리턴
	return true;
}

// 더미 관리 자료구조에서, 유저 검색
// 현재 map으로 관리중
// 
// Parameter : AccountNo
// return : 검색 성공 시, stDummy*
//		  : 검색 실패 시 nullptr
cMatchTestDummy::stDummy* cMatchTestDummy::FindDummyFunc(UINT64 AccountNo)
{
	// 1. umap에서 검색
	auto FindPlayer = m_uamp_Dummy.find(AccountNo);

	// 2. 검색 실패 시 nullptr 리턴
	if (FindPlayer == m_uamp_Dummy.end())
		return nullptr;

	// 3. 검색 성공 시, 찾은 stPlayer* 리턴
	return FindPlayer->second;
}



// ------------------------------------
// 내부에서만 사용하는 함수
// ------------------------------------

// 매치메이킹 서버에 Connect
//
// Parameter : 없음
// return : 없음
void cMatchTestDummy::MatchConnect()
{
	// 로컬로 받아두기
	int Count = m_iDummyCount;

	// 더미 수 만큼 돌면서, Connect 시도
	int Temp = 0;
	while (1)
	{
		// 1. 더미 검색
		stDummy* NowDummy = FindDummyFunc(Temp + 1);
		if (NowDummy == nullptr)
			m_Dump->Crash();

		// 2. Connect 되지 않은 더미만 Connect 시도
		if (NowDummy->m_bMatchConnect == false)
		{
			// 소켓 생성
			NowDummy->m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (NowDummy->m_sock == INVALID_SOCKET)
				m_Dump->Crash();

			// 논블락 소켓으로 변경
			ULONG on = 1;
			if (ioctlsocket(NowDummy->m_sock, FIONBIO, &on) == SOCKET_ERROR)
				m_Dump->Crash();

			SOCKADDR_IN clientaddr;
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			clientaddr.sin_family = AF_INET;
			clientaddr.sin_port = htons(NowDummy->m_iPort);
			InetPton(AF_INET, NowDummy->m_tcServerIP, &clientaddr.sin_addr.S_un.S_addr);

			// connect 시도
			int iReConnectCount = 0;
			while (1)
			{
				connect(NowDummy->m_sock, (SOCKADDR*)&clientaddr, sizeof(clientaddr));

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

					wset.fd_array[wset.fd_count] = NowDummy->m_sock;
					wset.fd_count++;

					exset.fd_array[exset.fd_count] = NowDummy->m_sock;
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

						closesocket(NowDummy->m_sock);
						m_Dump->Crash();
					}

					// 타임아웃 체크
					else if (retval == 0)
					{
						printf("Select Timeout..\n");

						// 5회 재시도한다.
						iReConnectCount++;
						if (iReConnectCount == 5)
						{
							closesocket(NowDummy->m_sock);
							m_Dump->Crash();
						}

						continue;
					}

					// 반응이 있다면, 예외셋에 반응이 왔는지 체크
					else if (exset.fd_count > 0)
					{
						//예외셋 반응이면 실패한 것.
						closesocket(NowDummy->m_sock);
						m_Dump->Crash();
					}

					// 쓰기셋에 반응이 왔는지 체크
					else if (wset.fd_count > 0)
					{
						break;
					}

					// 여기까지 오면 뭔지 모를 에러..
					else
					{
						printf("Select ---> UnknownError..(retval %d, LastError : %d)\n", retval, WSAGetLastError());

						closesocket(NowDummy->m_sock);
						m_Dump->Crash();
					}

				}
			}

			// 다시 소켓을 블락으로 변경
			on = 0;
			if (ioctlsocket(NowDummy->m_sock, FIONBIO, &on) == SOCKET_ERROR)
			{
				closesocket(NowDummy->m_sock);
				m_Dump->Crash();
			}

			// 접속 성공 시 flag 변경
			NowDummy->m_bMatchConnect = true;

		}

		// 3. Temp++ 후, Count만큼 만들었으면 break;
		Temp++;
		if (Temp == Count)
			break;
	}		
}

// 모든 더미의 SendQ에 로그인 패킷 Enqueue
//
// Parameter : 없음
// return : 없음
void cMatchTestDummy::MatchLogin()
{
	// 로컬로 받아두기
	int Count = m_iDummyCount;

	// Connect가 된 유저에 한해 로그인 패킷 보냄
	int Temp = 0;
	while (1)
	{
		// 1. 더미 검색
		UINT AccountNo = Temp + 1;
		stDummy* NowDummy = FindDummyFunc(AccountNo);
		if (NowDummy == nullptr)
			m_Dump->Crash();

		// 2. Connect 되었고, Login 안된 유저 대상
		if (NowDummy->m_bMatchConnect == true && 
			NowDummy->m_bMatchLogin == false)
		{
			// SendBuff 할당받기
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 보낼 패킷 셋팅
			WORD Type = en_PACKET_CS_MATCH_REQ_LOGIN;
			UINT VerCode = VERCODE;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&AccountNo, 8);
			SendBuff->PutData(NowDummy->m_cToekn, 64);
			SendBuff->PutData((char*)&VerCode, 4);

			// SendQ에 넣기
			SendPacket(NowDummy, SendBuff);
		}

		// 3. Temp++ 후, Count만큼 만들었으면 break;
		Temp++;
		if (Temp == Count)
			break;
	}

}


// 해당 유저의 SendQ에 패킷 넣기
//
// Parameter : stDummy*, CProtocolBuff_Net*
// return : 성공 시 true
//		  : 실패 시 false
bool cMatchTestDummy::SendPacket(stDummy* NowDummy, CProtocolBuff_Net* payload)
{
	// 1. 헤더를 넣어 패킷 완성
	payload->Encode(m_bCode, m_bXORCode_1, m_bXORCode_2);

	// 2. 인큐. 패킷의 "주소"를 인큐한다(8바이트)
	// 직렬화 버퍼 레퍼런스 카운트 1 증가
	payload->Add();
	NowDummy->m_SendQueue->Enqueue(payload);

	// 3. 직렬화 버퍼 레퍼런스 카운트 1 감소. 0 되면 메모리풀에 반환
	CProtocolBuff_Net::Free(payload);

	return true;
}



// ------------------------------------
// 외부에서 사용 가능 함수
// ------------------------------------

// 더미 생성 함수
//
// Parameter : 생성할 더미 수
// return : 없음
void cMatchTestDummy::CreateDummpy(int Count)
{
	// 입력받은 Count만큼 더미 생성
	m_iDummyCount = Count;
	int Temp = 0;
	while (1)
	{
		// 1. 더미 구조체 할당
		stDummy* NowDummy = m_DummyStructPool->Alloc();

		// 2. 값 셋팅
		NowDummy->m_ui64AccountNo = Temp + 1;

		TCHAR tcToken[65] = TOKEN_T;
		_tcscpy_s(NowDummy->m_tcToken, 64, tcToken);

		char cToken[65] = TOKEN_C;
		strcpy_s(NowDummy->m_cToekn, 64, cToken);

		// 3. umap에 추가
		if (InsertDummyFunc(Temp + 1, NowDummy) == false)
			m_Dump->Crash();

		// 4. Temp++ 후, Count만큼 만들었으면 break;
		Temp++;
		if (Temp == Count)
			break;
	}
}


// 매치메이킹 서버의 IP와 Port 알아오는 함수
// 각 더미가, 자신의 멤버로 Server의 IP와 Port를 들고있음.
//
// Parameter : 없음
// return : 없음
void cMatchTestDummy::MatchInfoGet()
{	
	// 로컬로 받아두기
	int Count = m_iDummyCount;
	HTTP_Exchange* pHTTP_Post = HTTP_Post;		

	// 더미 수 만큼 돌면서, 매치메이킹 서버의 IP 알아옴
	TCHAR RequestBody[2000];
	TCHAR Body[1000];
	int Temp = 0;

	while (1)
	{
		// 1. 더미 umap에서 stDummy* 알아오기
		stDummy* NowDummy = FindDummyFunc(Temp + 1);
		if (NowDummy == nullptr)
			m_Dump->Crash();

		// 2. Connect 되지 않은 더미만 매치메이킹의 정보를 알아온다.
		if (NowDummy->m_bMatchConnect == false)
		{
			// get_matchmaking.php에 http 통신.
			ZeroMemory(RequestBody, sizeof(RequestBody));
			ZeroMemory(Body, sizeof(Body));

			swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %d, \"sessionkey\" : \"%s\"}", Temp + 1, NowDummy->m_tcToken);
			if (pHTTP_Post->HTTP_ReqANDRes((TCHAR*)_T("Lobby/get_matchmaking.php"), Body, RequestBody) == false)
				m_Dump->Crash();


			// Json데이터 파싱하기
			GenericDocument<UTF16<>> Doc;
			Doc.Parse(RequestBody);


			// 결과 확인 
			int Result;
			Result = Doc[_T("result")].GetInt();

			// 결과가 1이 아니면 Crash()
			if (Result != 1)
				m_Dump->Crash();


			// P와 Port 저장
			const TCHAR* TempServerIP = Doc[_T("ip")].GetString();
			_tcscpy_s(NowDummy->m_tcServerIP, 20, TempServerIP);
			NowDummy->m_iPort = Doc[_T("port")].GetInt();
		}

		// 3. Temp++ 후, Count만큼 만들었으면 break;
		Temp++;
		if (Temp == Count)
			break;
	}

}

// 더미 Run 함수
// 1. 매치메이킹 서버의 IP와 Port 알아오기 (Connect 안된 유저 대상)
// 2. 매치메이킹 Connect (Connect 안된 유저 대상)
// 3. 로그인 패킷 Enqueue (Connect 되었으며, Login 안된 유저 대상)
// 4. Select 처리 (Connect 된 유저 대상)
// 5. Disconnect. (Connect 후, Login 된 유저 대상. 확률로 Disconnect)
//
// Parameter : 없음
// return : 없음
void cMatchTestDummy::DummyRun()
{
	// --------------------------------- 
	// 1. 매치메이킹의 주소 알아오기
	// --------------------------------- 
	MatchInfoGet();

	// --------------------------------- 
	// 2. 매치메이킹에 Connect
	// --------------------------------- 
	MatchConnect();

	// --------------------------------- 
	// 3. 매치메이킹에게 보낼 로그인 패킷 Enqueue
	// --------------------------------- 
	MatchLogin();

	// --------------------------------- 
	// 4. Select 처리
	// --------------------------------- 

}



// ------------------------------------
// 생성자, 소멸자
// ------------------------------------

// 생성자
cMatchTestDummy::cMatchTestDummy()
{
	// 더미 수 0으로 시작
	m_iDummyCount = 0;

	// m_uamp_Dummy 공간 할당
	m_uamp_Dummy.reserve(5000);

	// 더미 구조체 pool 동적할당
	m_DummyStructPool = new CMemoryPoolTLS<stDummy>(0, false);

	// 덤프 싱글톤 얻어오기
	m_Dump = CCrashDump::GetInstance();

	// HTTP 통신용 동적할당
	HTTP_Post = new HTTP_Exchange((TCHAR*)_T("127.0.0.1"), 80);

	// 헤더를 위한 Config들 
	m_bCode = 119;
	m_bXORCode_1 = 50;
	m_bXORCode_2 = 132;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		fputs("Winsock Init Fail...\n", stdout);
		m_Dump->Crash();
	}
}

// 소멸자
cMatchTestDummy::~cMatchTestDummy()
{
	// 더미 구조체 pool 동적해제
	delete m_DummyStructPool;

	// HTTP 통신용 동적해제
	delete HTTP_Post;

	// 윈속 해제
	WSACleanup();
}