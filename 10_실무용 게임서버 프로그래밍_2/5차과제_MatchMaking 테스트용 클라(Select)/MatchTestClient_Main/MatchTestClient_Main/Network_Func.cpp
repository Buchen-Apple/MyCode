#include "pch.h"

#include <Ws2tcpip.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

#include <strsafe.h>

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

#include "Network_Func.h"
#include "Protocol_Set/CommonProtocol_2.h"
#include "Parser/Parser_Class.h"

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
// SOCKET 기준, 더미 자료구조 관리용 함수
// ------------------------------------

// 더미 관리 자료구조에 더미 추가
// 현재 umap으로 관리중
// 
// Parameter : SOCKET, stDummy*
// return : 추가 성공 시, true
//		  : SOCKET이 중복될 시(이미 접속중인 유저) false
bool cMatchTestDummy::InsertDummyFunc_SOCKET(SOCKET sock, stDummy* insertDummy)
{
	// 1. umap에 추가		
	auto ret = m_uamp_Socket_and_AccountNo.insert(make_pair(sock, insertDummy));

	// 2. 중복된 키일 시 false 리턴.
	if (ret.second == false)
		return false;

	// 3. 아니면 true 리턴
	return true;
}

// 더미 관리 자료구조에서, 유저 검색
// 현재 map으로 관리중
// 
// Parameter : SOCKET
// return : 검색 성공 시, stDummy*
//		  : 검색 실패 시 nullptr
cMatchTestDummy::stDummy* cMatchTestDummy::FindDummyFunc_SOCKET(SOCKET sock)
{
	// 1. umap에서 검색
	auto FindPlayer = m_uamp_Socket_and_AccountNo.find(sock);

	// 2. 검색 실패 시 nullptr 리턴
	if (FindPlayer == m_uamp_Socket_and_AccountNo.end())
		return nullptr;

	// 3. 검색 성공 시, 찾은 stPlayer* 리턴
	return FindPlayer->second;

}

//더미 관리 자료구조에서, 더미 제거 (검색 후 제거)
// 현재 umap으로 관리중
// 
// Parameter : SOCKET
// return : 성공 시, 제거된 더미의 stDummy*
//		  : 검색 실패 시(접속중이지 않은 더미) nullptr
cMatchTestDummy::stDummy* cMatchTestDummy::EraseDummyFunc_SOCKET(SOCKET socket)
{
	// 1. umap에서 유저 검색
	auto FindPlayer = m_uamp_Socket_and_AccountNo.find(socket);
	if (FindPlayer == m_uamp_Socket_and_AccountNo.end())
	{
		return nullptr;
	}

	// 2. 존재한다면, 리턴할 값 받아두기
	stDummy* ret = FindPlayer->second;

	// 3. 맵에서 제거
	m_uamp_Socket_and_AccountNo.erase(FindPlayer);
	return ret;
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
	UINT64 TempStartNo = m_ui64StartAccountNo;

	// 더미 수 만큼 돌면서, Connect 시도
	int Temp = 0;
	while (1)
	{
		// 1. 더미 검색
		stDummy* NowDummy = FindDummyFunc(TempStartNo);
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

			// SOCKET용 더미 자료구조에 추가
			InsertDummyFunc_SOCKET(NowDummy->m_sock, NowDummy);
		}

		// 3. 마지막 체크
		++Temp;
		++TempStartNo;
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
	UINT64 TempStartNo = m_ui64StartAccountNo;

	// Connect가 된 유저에 한해 로그인 패킷 보냄
	int Temp = 0;
	while (1)
	{
		// 1. 더미 검색
		stDummy* NowDummy = FindDummyFunc(TempStartNo);
		if (NowDummy == nullptr)
			m_Dump->Crash();

		// 2. Connect 되었고, Login 안됐으며, 로그인 패킷을 안보낸 유저 대상
		if (NowDummy->m_bMatchConnect == true && 
			NowDummy->m_bMatchLogin == false && 
			NowDummy->m_bLoginPacketSend == false)
		{
			// SendBuff 할당받기
			CProtocolBuff_Net* SendBuff = CProtocolBuff_Net::Alloc();

			// 보낼 패킷 셋팅
			WORD Type = en_PACKET_CS_MATCH_REQ_LOGIN;
			UINT VerCode = VERCODE;

			SendBuff->PutData((char*)&Type, 2);
			SendBuff->PutData((char*)&NowDummy->m_ui64AccountNo, 8);
			SendBuff->PutData(NowDummy->m_cToekn, 64);
			SendBuff->PutData((char*)&VerCode, 4);

			// SendQ에 넣기
			SendPacket(NowDummy, SendBuff);

			// 로그인 패킷 보냄 상태로 변경
			NowDummy->m_bLoginPacketSend = true;

			// 직렬화 버퍼 레퍼런스 카운트 1 감소. 0 되면 메모리풀에 반환
			CProtocolBuff_Net::Free(SendBuff);
		}

		// 3. 마지막 체크
		++Temp;
		++TempStartNo;
		if (Temp == Count)
			break;
	}

}

// Select 처리
//
// Parameter : 없음
// return : 없음
void cMatchTestDummy::SelectFunc()
{
	// 통신에 사용할 변수
	FD_SET rset;
	FD_SET wset;

	unordered_map <UINT64, stDummy*>::iterator itor;
	unordered_map <UINT64, stDummy*>::iterator enditor;
	TIMEVAL tval;
	tval.tv_sec = 0;
	tval.tv_usec = 0;

	itor = m_uamp_Dummy.begin();

	// 리슨 소켓 셋팅. 
	rset.fd_count = 0;
	wset.fd_count = 0;

	while (1)
	{
		enditor = m_uamp_Dummy.end();

		// 모든 멤버를 순회하며, 해당 유저를 읽기 셋과 쓰기 셋에 넣는다.
		// 넣다가 64개가 되거나, end에 도착하면 break
		while (itor != enditor)
		{
			// 해당 클라이언트에게 받을 데이터가 있는지 체크하기 위해, 모든 클라를 rset에 소켓 설정
			rset.fd_array[rset.fd_count++] = itor->second->m_sock;

			// 만약, 해당 클라이언트의 SendBuff에 값이 있으면, wset에도 소켓 넣음.
			if (itor->second->m_SendBuff.GetUseSize() != 0)
				wset.fd_array[wset.fd_count++] = itor->second->m_sock;

			// 64개 꽉 찼는지, 끝에 도착했는지 체크		
			++itor;

			if (rset.fd_count == FD_SETSIZE || itor == enditor)
				break;

		}

		// Select()
		DWORD dCheck = select(0, &rset, &wset, 0, &tval);

		// Select()결과 처리
		if (dCheck == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAEINVAL && rset.fd_count != 0 && wset.fd_count != 0)
				_tprintf(_T("select 에러(%d)\n"), WSAGetLastError());

			m_Dump->Crash();
		}

		// select의 값이 0보다 크다면 뭔가 할게 있다는 것이니 로직 진행
		if (dCheck > 0)
		{
			DWORD rsetCount = 0;
			DWORD wsetCount = 0;

			while (1)
			{
				if (rsetCount < rset.fd_count)
				{
					stDummy* NowDummy = FindDummyFunc_SOCKET(rset.fd_array[rsetCount]);

					// Recv() 처리
					// 만약, RecvProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					if (RecvProc(NowDummy) == false)
						Disconnect(NowDummy);

					rsetCount++;
				}

				if (wsetCount < wset.fd_count)
				{
					stDummy* NowDummy = FindDummyFunc_SOCKET(wset.fd_array[wsetCount]);

					// Send() 처리
					// 만약, SendProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					if (SendProc(NowDummy) == false)
						Disconnect(NowDummy);

					wsetCount++;
				}

				if (rsetCount == rset.fd_count && wsetCount == wset.fd_count)
					break;

			}

		}

		// 만약, 모든 Client에 대해 Select처리가 끝났으면, 이번 함수는 여기서 종료.
		if (itor == enditor)
			break;

		// select 준비	
		rset.fd_count = 0;
		wset.fd_count = 0;
	}

}

// Match 서버에 Disconnect. (확률에 따라)
// 
// Parameter : 없음
// return : 없음
void cMatchTestDummy::MatchDisconenct()
{
	// 로컬로 받아두기
	int Count = m_iDummyCount;
	UINT64 TempStartNo = m_ui64StartAccountNo;

	// Connect가 된 유저에 한해 확률에 따라 Disconenct()
	int Temp = 0;
	while (1)
	{
		// 1. 더미 검색
		stDummy* NowDummy = FindDummyFunc(TempStartNo);
		if (NowDummy == nullptr)
			m_Dump->Crash();

		// 2. 매치메이킹 서버에 로그인 된 유저에 한해 로직 진행
		if (NowDummy->m_bMatchLogin == true)
		{
			// 1. 확률 구하기
			int random = (rand() % 100) + 1;	// 1 ~ 100까지의 값 중 하나 결정

			// random 값이 설정한 확률보다 적은지 체크
			if (random <= DISCONNECT_RATE)	
			{
				// 여기 오면 확률에 당첨된 것. Disconenct 한다.
				Disconnect(NowDummy);
			}
		}

		// 3. 마지막 체크
		++Temp;
		++TempStartNo;
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

	// 2. 인큐. 페이로드의 데이터를 Enqueue한다.
	NowDummy->m_SendBuff.Enqueue(payload->GetBufferPtr(), payload->GetUseSize());	

	return true;
}


// 서버와 인자로 받은 더미 1개체와 연결을 끊는 함수.
//
// Parameter : stDummy*
// returb : 없음
void cMatchTestDummy::Disconnect(stDummy* NowDummy)
{
	SOCKET TempSocket = NowDummy->m_sock;

	// SOCKET 기준 자료구조에서 Erase
	// SOCKET은 새로 접속할 때 마다 변경되기 때문에 지워줘야 한다.
	// AccountNo로 관리되는 자료구조에서는 삭제하지 않음.
	if(EraseDummyFunc_SOCKET(TempSocket) == nullptr)
		m_Dump->Crash();

	// 정보 초기화	
	NowDummy->Reset();

	// 해당 유저의 소켓 close	
	closesocket(TempSocket);
}




// ------------------------------------
// 네트워크 처리 함수
// ------------------------------------

// Recv() 처리
//
// Parameter : stDummy*
// return : 접속이 끊겨야 하는 유저는, false 리턴
bool cMatchTestDummy::RecvProc(stDummy* NowDummy)
{
	////////////////////////////////////////////////////////////////////////
	// 소켓 버퍼에 있는 모든 recv 데이터를, 해당 유저의 recv링버퍼로 리시브.
	////////////////////////////////////////////////////////////////////////

	// 1. recv 링버퍼 포인터 얻어오기.
	char* recvbuff = NowDummy->m_RecvBuff.GetBufferPtr();

	// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = NowDummy->m_RecvBuff.GetNotBrokenPutSize();

	// 3. 한 번에 쓸 수 있는 공간이 0이라면
	if (Size == 0)
	{
		// rear 1칸 이동
		NowDummy->m_RecvBuff.MoveWritePos(1);

		// 그리고 한 번에 쓸 수 있는 위치 다시 알기.
		Size = NowDummy->m_RecvBuff.GetNotBrokenPutSize();
	}

	// 4. 그게 아니라면, 1칸 이동만 하기. 		
	else
	{
		// rear 1칸 이동
		NowDummy->m_RecvBuff.MoveWritePos(1);
	}

	// 5. 1칸 이동된 rear 얻어오기
	int* rear = (int*)NowDummy->m_RecvBuff.GetWriteBufferPtr();

	// 6. recv()
	int retval = recv(NowDummy->m_sock, &recvbuff[*rear], Size, 0);

	// 7. 리시브 에러체크
	if (retval == SOCKET_ERROR)
	{
		// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있다는 것. recv할게 없다는 것이니 함수 종료.
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;

		// 10053, 10054 둘다 어쩃든 연결이 끊어진 상태
		// WSAECONNABORTED(10053) :  프로토콜상의 에러나 타임아웃에 의해 연결(가상회선. virtual circle)이 끊어진 경우
		// WSAECONNRESET(10054) : 원격 호스트가 접속을 끊음.
		// 이 때는 그냥 return false로 나도 상대방과 연결을 끊는다.
		if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAECONNABORTED)
		{
			m_Dump->Crash();
			return false;
		}

		// WSAEWOULDBLOCK에러가 아니면 뭔가 이상한 것이니 접속 종료
		else
		{
			m_Dump->Crash();
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// 0이 리턴되는 것은 정상종료.
	else if (retval == 0)
	{
		m_Dump->Crash();
		return false;
	}

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	NowDummy->m_RecvBuff.MoveWritePos(retval - 1);
	   

	//////////////////////////////////////////////////
	// 완료 패킷 처리 부분
	// RecvBuff에 들어있는 모든 완성 패킷을 처리한다.
	//////////////////////////////////////////////////
	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)	
		stProtocolHead Header;
		int len = sizeof(Header);

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (NowDummy->m_RecvBuff.GetUseSize() < len)
			break;

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = NowDummy->m_RecvBuff.Peek((char*)&Header, len);

		// Peek()은 버퍼가 비었으면 -1을 반환한다. 버퍼가 비었으니 일단 끊는다.
		if (PeekSize == -1)
		{
			m_Dump->Crash();
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 4. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size)
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (NowDummy->m_RecvBuff.GetUseSize() < (len + Header.m_Len))
			break;

		// 4. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (Header.m_Code != m_bCode)
		{
			m_Dump->Crash();
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		NowDummy->m_RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		CProtocolBuff_Net* PayloadBuff = CProtocolBuff_Net::Alloc();
		WORD PayloadSize = Header.m_Len;

		// 7. RecvBuff에서 페이로드 Size 만큼 페이로드 직렬화 버퍼로 뽑는다. (디큐이다. Peek 아님)
		int DequeueSize = NowDummy->m_RecvBuff.Dequeue(PayloadBuff->GetBufferPtr(), PayloadSize);

		// 버퍼가 비어있거나, 내가 원하는만큼 데이터가 없었다면, 말이안됨. (위 if문에서는 있다고 했는데 여기오니 없다는것)
		// 로직문제로 보고 서버 종료.
		if ((DequeueSize == -1) || (DequeueSize != PayloadSize))
			m_Dump->Crash();	
		
		// 8. 읽어온 만큼 rear를 이동시킨다. 
		PayloadBuff->MoveWritePos(DequeueSize);

		// 9. 헤더 Decode
		if (PayloadBuff->Decode(PayloadSize, Header.m_RandXORCode, Header.m_Checksum, m_bXORCode_1, m_bXORCode_2) == false)
		{
			m_Dump->Crash();
		}

		// 7. 헤더에 들어있는 타입에 따라 분기처리.
		Network_Packet(NowDummy, PayloadBuff);

		// 8. 직렬화 버퍼 해제
		CProtocolBuff_Net::Free(PayloadBuff);
		
	}

	return true;
}


// SendBuff의 데이터를 Send하기
//
// Parameter : stDummy*
// return : 접속이 끊겨야 하는 유저는, false 리턴
bool cMatchTestDummy::SendProc(stDummy* NowDummy)
{
	// 1. SendBuff에 보낼 데이터가 있는지 확인.
	if (NowDummy->m_SendBuff.GetUseSize() == 0)
		return true;

	// 2. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = NowDummy->m_SendBuff.GetBufferPtr();

	// 3. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 4. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (NowDummy->m_SendBuff.GetUseSize() == 0)
			break;

		// 5. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = NowDummy->m_SendBuff.GetNotBrokenGetSize();

		// 6. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 7. front 포인터 얻어옴
		int *front = (int*)NowDummy->m_SendBuff.GetReadBufferPtr();

		// 8. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = NowDummy->m_SendBuff.NextIndex(*front, 1);

		// 9. Send()
		int SendSize = send(NowDummy->m_sock, &sendbuff[BuffArrayCheck], Size, 0);

		// 10. 에러 체크. 우드블럭이면 더 이상 못보내니 while문 종료. 다음 Select때 다시 보냄
		if (SendSize == SOCKET_ERROR)
		{
			DWORD Error = WSAGetLastError();

			if (Error == WSAEWOULDBLOCK ||
				Error == 10054)
				break;

			else
			{				
				m_Dump->Crash();
				return false;
			}

		}

		// 11. 보낸 사이즈가 나왔으면, 그 만큼 remove
		NowDummy->m_SendBuff.RemoveData(SendSize);
	}

	return true;
}


// Recv 한 데이터 처리 부분
//
// Parameter : stDummy*, 페이로드(CProtocolBuff_Net*)
// return : 없음
void cMatchTestDummy::Network_Packet(stDummy* NowDummy, CProtocolBuff_Net* payload)
{
	// 타입 분리
	WORD Type;
	payload->GetData((char*)&Type, 2);
	
	// 타입에 따라 분기 처리
	try
	{
		switch (Type)
		{
			// 로그인 요청 결과
		case en_PACKET_CS_MATCH_RES_LOGIN:
			PACKET_Login_Match(NowDummy, payload);
			break;

		default:
			TCHAR ErrStr[1024];
			StringCchPrintf(ErrStr, 1024, _T("Network_Packet(). TypeError. Type : %d, AccountNo : %lld"),
				Type, NowDummy->m_ui64AccountNo);

			throw CException(ErrStr);
		}

	}
	catch (CException& exc)
	{
		// 로그 찍기 (로그 레벨 : 에러)
		m_Log->LogSave(L"TestClinet", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s",
			(TCHAR*)exc.GetExceptionText());

		// Crash
		m_Dump->Crash();
	}
}





// ------------------------------------
// 패킷 처리 함수
// ------------------------------------

// Match서버로 보낸 로그인 패킷의 응답 처리
//
// Parameter : stDummy*, 페이로드(CProtocolBuff_Net*)
// return : 없음
void cMatchTestDummy::PACKET_Login_Match(stDummy* NowDummy, CProtocolBuff_Net* payload)
{
	// 1. 마샬링
	BYTE Status;
	payload->GetData((char*)&Status, 1);

	// 2. 성공이 아니라면 외부로 Throw 던짐
	if (Status != 1)
	{
		TCHAR ErrStr[1024];
		StringCchPrintf(ErrStr, 1024, _T("PACKET_Login_Match(). Login Response Status Error : %d(AccountNo : %lld)"),
			Status, NowDummy->m_ui64AccountNo);

		throw CException(ErrStr);		
	}

	// 3. 성공이라면 Flag 변경
	NowDummy->m_bMatchLogin = true;		// 매치메이킹에 로그인 상태로 변경	
	NowDummy->m_bLoginPacketSend = false;	// 매치메이킹에 로그인패킷 안보낸 상태로 변경
}







// ------------------------------------
// 외부에서 사용 가능 함수
// ------------------------------------


// 매치메이킹 서버의 IP와 Port 알아오는 함수
// 각 더미가, 자신의 멤버로 Server의 IP와 Port를 들고있음.
//
// Parameter : 없음
// return : 없음
void cMatchTestDummy::MatchInfoGet()
{
	// 로컬로 받아두기
	int Count = m_iDummyCount;
	UINT64 TempStartNo = m_ui64StartAccountNo;
	HTTP_Exchange* pHTTP_Post = HTTP_Post;

	// 더미 수 만큼 돌면서, 매치메이킹 서버의 IP 알아옴
	TCHAR RequestBody[2000];
	TCHAR Body[1000];

	int Temp = 0;
	while (1)
	{
		// 1. 더미 umap에서 stDummy* 알아오기
		stDummy* NowDummy = FindDummyFunc(TempStartNo);
		if (NowDummy == nullptr)
			m_Dump->Crash();

		// 2. Connect 되지 않은 더미만 매치메이킹의 정보를 알아온다.
		if (NowDummy->m_bMatchConnect == false)
		{
			// get_matchmaking.php에 http 통신.
			ZeroMemory(RequestBody, sizeof(RequestBody));
			ZeroMemory(Body, sizeof(Body));

			swprintf_s(Body, _Mycountof(Body), L"{\"accountno\" : %lld, \"sessionkey\" : \"%s\"}", TempStartNo, NowDummy->m_tcToken);
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


		// 3. 마지막 체크
		++Temp;
		++TempStartNo;
		if (Temp == Count)
			break;
	}

}


// 더미 생성 함수
//
// Parameter : 생성할 더미 수, 시작 AccountNo
// return : 없음
void cMatchTestDummy::CreateDummpy(int Count, int StartAccountNo)
{
	// 입력받은 Count만큼 더미 생성
	m_iDummyCount = Count;
	m_ui64StartAccountNo = StartAccountNo;

	int Temp = 0;
	while (1)
	{
		// 1. 더미 구조체 할당
		stDummy* NowDummy = m_DummyStructPool->Alloc();

		// 2. 값 셋팅
		NowDummy->m_ui64AccountNo = StartAccountNo;

		TCHAR tcToken[65] = TOKEN_T;
		_tcscpy_s(NowDummy->m_tcToken, 64, tcToken);

		char cToken[65] = TOKEN_C;
		strcpy_s(NowDummy->m_cToekn, 64, cToken);

		// 3. umap에 추가
		if (InsertDummyFunc(StartAccountNo, NowDummy) == false)
			m_Dump->Crash();

		// 4. Temp++ 후, Count만큼 만들었으면 break;
		Temp++;
		StartAccountNo++;
		if (Temp == Count)
			break;
	}
}

// 더미 Run 함수
// 1. 매치메이킹 Connect (Connect 안된 유저 대상)
// 2. 로그인 패킷 Enqueue (Connect 되었으며, Login 안된 유저, 그리고 로그인 패킷도 보내지 않은 유저 대상)
// 3. Select 처리 (Connect 된 유저 대상)
// 4. Disconnect. (Connect 후, Login 된 유저 대상. 확률로 Disconnect)
//
// Parameter : 없음
// return : 없음
void cMatchTestDummy::DummyRun()
{
	// --------------------------------- 
	// 1. 매치메이킹에 Connect
	// --------------------------------- 
	MatchConnect();

	// --------------------------------- 
	// 2. 매치메이킹에게 보낼 로그인 패킷 Enqueue
	// --------------------------------- 
	MatchLogin();

	// --------------------------------- 
	// 3. Select 처리
	// --------------------------------- 
	SelectFunc();

	// --------------------------------- 
	// 4. Disconnect 처리
	// --------------------------------- 
	MatchDisconenct();
}



// ------------------------------------
// 생성자, 소멸자
// ------------------------------------

// 생성자
cMatchTestDummy::cMatchTestDummy()
{
	// srand 설정 (m_Log 변수의 주소를 그냥 Seed 값으로 사용)
	srand((UINT)m_Log);

	// 로그 받아오기
	m_Log = CSystemLog::GetInstance();

	// ------------------- 로그 저장할 파일 셋팅
	m_Log->SetDirectory(L"TestClinet");
	m_Log->SetLogLeve((CSystemLog::en_LogLevel)0);

	// 더미 수 0으로 시작
	m_iDummyCount = 0;

	// m_uamp_Dummy 공간 할당
	m_uamp_Dummy.reserve(5000);
	m_uamp_Socket_and_AccountNo.reserve(5000);

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