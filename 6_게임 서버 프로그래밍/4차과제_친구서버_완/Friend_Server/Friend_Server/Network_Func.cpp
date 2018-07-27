#include "stdafx.h"
#include "Network_Func.h"
#include "RingBuff\RingBuff.h"

#include "rapidjson\document.h"
#include "rapidjson\writer.h"
#include "rapidjson\stringbuffer.h"

#include <list>
#include <map>

using namespace std;

// Accept 유저 구조체
struct stAcceptUser
{
	SOCKET m_sock;								// 해당 Accept 유저의 소켓
	CRingBuff m_RecvBuff;						// 리시브 버퍼
	CRingBuff m_SendBuff;						// 샌드 버퍼

	UINT64  m_AccountNo;						// Accept 유저와 연결된(로그인 된) 유저 회원 넘버. 0일 경우, 로그인 안된 Accept 유저

	TCHAR m_tIP[33];							// 해당 유저의 IP
	WORD  m_wPort;								// 해당 유저의 Port
};

// 회원 정보 구조체
struct stAccount
{
	UINT64  m_AccountNo;						// 유저 회원 넘버
	TCHAR	m_NickName[dfNICK_MAX_LEN];			// 닉네임

	UINT	m_FriendCount = 0;					// 내 친구 수 카운트
	UINT	m_RequestCount = 0;					// 내가 보낸 친구요청 수 카운트
	UINT	m_ReplyCount = 0;					// 내가 받은 친구요청 수 카운트
};

// 친구관계, 보낸 요청, 받은 요청 등을 저장하는 구조체
struct stFromTo
{
	UINT64 m_FromAccount;
	UINT64 m_ToAccount;
	UINT64 m_Time;
};

// 전역 변수
UINT64 g_AcceptCount = 0;							// Accept 한 유저 수
UINT64 g_uiAccountNo = 1;							// 유저 AccountNo를 체크하는 변수. 1부터 시작.
UINT64 g_uiFriendIndex = 1;							// map_FriendList전용 고유 Index (친구 목록 Index)
UINT64 g_uiRequestIndex = 1;						// map_FriendRequestList전용 고유 Index (보낸 친구요청 목록 Index)
UINT64 g_uiReplyIndex = 1;							// map_FriendReplyList전용 고유 Index (받은 친구요청 목록 Index)

// [Accept 유저 목록]
// Key : 소켓 값
map <SOCKET, stAcceptUser*> map_AcceptList;

// [회원 목록]
// Key : 회원 넘버.
map <UINT64, stAccount*> map_AccountList;

// [친구 목록]
// Key : map_FriendList 전용 고유Index(g_uiFriendIndex) / Value : stFromTo 구조체. 
// Value에 from친구, to친구, 시간 모두 들어있다.
map <UINT64, stFromTo*> map_FriendList;

// [보낸 친구요청 목록] 
// Key : map_FriendRequestList(g_uiRequestIndex) 전용 고유 Index / Value : stFromTo 구조체. 
// Value의 from은 보낸 유저, to는 받은 유저이다.
map <UINT64, stFromTo*> map_FriendRequestList;

// [받은 친구요청 목록] 
// Key : map_FriendReplyList 전용 고유 Index(g_uiReplyIndex) / Value : stFromTo 구조체. 
// Value의 from은 요청을 받은 유저, to는 보낸 유저이다.
map <UINT64, stFromTo*> map_FriendReplyList;

// 인자로 받은 회원No 값을 기준으로 [회원 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 회원 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stAccount* ClientSearch_AccountList(UINT64 AccountNo)
{
	stAccount* NowAccount;
	map <UINT64, stAccount*>::iterator iter;
	for (iter = map_AccountList.begin(); iter != map_AccountList.end(); ++iter)
	{
		if (iter->first == AccountNo)
		{
			NowAccount = iter->second;
			return NowAccount;
		}
	}

	return nullptr;
}

// 인자로 받은 Socket 값을 기준으로 [Accept 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stAcceptUser* ClientSearch_AcceptList(SOCKET sock)
{
	stAcceptUser* NowAccount;
	map <SOCKET, stAcceptUser*>::iterator iter;
	for (iter = map_AcceptList.begin(); iter != map_AcceptList.end(); ++iter)
	{
		if (iter->first == sock)
		{
			NowAccount = iter->second;
			return NowAccount;
		}
	}

	return nullptr;
}

// 네트워크 프로세스
// 여기서 false가 반환되면, 프로그램이 종료된다.
bool Network_Process(SOCKET* listen_sock)
{
	// 클라와 통신에 사용할 변수
	static FD_SET rset;
	static FD_SET wset;

	SOCKADDR_IN clinetaddr;
	map <SOCKET, stAcceptUser*>::iterator itor;
	TIMEVAL tval;
	tval.tv_sec = 0;
	tval.tv_usec = 0;

	itor = map_AcceptList.begin();

	DWORD dwFDCount;

	while (1)
	{
		// select 준비	
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		SOCKET* NowSock[FD_SETSIZE];
		dwFDCount = 0;

		// 리슨소켓 셋팅		
		FD_SET(*listen_sock, &rset);
		NowSock[dwFDCount] = listen_sock;

		// 모든 멤버를 순회하며, 해당 유저를 읽기 셋과 쓰기 셋에 넣는다.
		// 넣다가 64개가 되거나, end에 도착하면 break
		while (itor != map_AcceptList.end())
		{
			++dwFDCount;

			// 해당 클라이언트에게 받을 데이터가 있는지 체크하기 위해, 모든 클라를 rset에 소켓 설정
			FD_SET(itor->second->m_sock, &rset);
			NowSock[dwFDCount] = &itor->second->m_sock;

			// 만약, 해당 클라이언트의 SendBuff에 값이 있으면, wset에도 소켓 넣음.
			if (itor->second->m_SendBuff.GetUseSize() != 0)
				FD_SET(itor->second->m_sock, &wset);

			// 64개 꽉 찼는지, 끝에 도착했는지 체크		
			++itor;
			if (dwFDCount + 1 == FD_SETSIZE || itor == map_AcceptList.end())
				break;			
		}

		// Select()
		DWORD dCheck = select(0, &rset, &wset, 0, &tval);

		// Select()결과 처리
		if (dCheck == SOCKET_ERROR)
		{
			_tprintf(_T("select 에러(%d)\n"), WSAGetLastError());
			return false;
		}

		// select의 값이 0보다 크다면 뭔가 할게 있다는 것이니 로직 진행
		else if (dCheck > 0)
		{
			for (DWORD i = 0; i <= dwFDCount; ++i)
			{
				// 리슨 소켓 처리
				if (FD_ISSET(*NowSock[i], &rset) && (*NowSock[i] == *listen_sock))
				{
					int addrlen = sizeof(clinetaddr);
					SOCKET client_sock = accept(*listen_sock, (SOCKADDR*)&clinetaddr, &addrlen);

					// 에러가 발생하면, 그냥 그 유저는 소켓 등록 안함
					if (client_sock == INVALID_SOCKET)
						_tprintf(_T("accept 에러\n"));

					// 에러가 발생하지 않았다면, "로그인 요청" 패킷이 온 것. 이에 대해 처리
					else
						Accept(&client_sock, clinetaddr);	// Accept 처리.

					// 리슨 소켓이면 읽기/쓰기 셋으로 가면 안되니까 conitnue한다.
					continue;
				}

				// 읽기 셋 처리
				if (FD_ISSET(*NowSock[i], &rset))
				{
					// Recv() 처리
					// 만약, RecvProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					bool Check = RecvProc(*NowSock[i]);
					if (Check == false)
					{
						Disconnect(*NowSock[i]);
						continue;
					}
				}

				// 쓰기 셋 처리
				if (FD_ISSET(*NowSock[i], &wset))
				{
					// Send() 처리
					// 만약, SendProc()함수가 false가 리턴된다면, 해당 유저 접속 종료
					bool Check = SendProc(*NowSock[i]);
					if (Check == false)
						Disconnect(*NowSock[i]);
				}
			}
		}

		// 만약, 모든 Client에 대해 Select처리가 끝났으면, 이번 함수는 여기서 종료.
		if (itor == map_AcceptList.end())
			break;

	}

	return true;
}

// Accept 처리
void Accept(SOCKET* client_sock, SOCKADDR_IN clinetaddr)
{
	// 1. 인자로 받은 clinet_sock에 해당되는 유저 생성
	stAcceptUser* NewAccount = new stAcceptUser;

	// 2. sock 셋팅. (m_AccountNo는 0을 넣는다. 0이면, 로그인 되지 않은 상태이다)
	NewAccount->m_sock = *client_sock;
	NewAccount->m_AccountNo = 0;	

	// 3. 접속한 유저의 ip, port, 소켓 출력
	TCHAR Buff[33];
	InetNtop(AF_INET, &clinetaddr.sin_addr, Buff, sizeof(Buff));
	WORD port = ntohs(clinetaddr.sin_port);

	_tprintf(_T("Accept : [%s : %d / Socket : %lld]\n"), Buff, port, *client_sock);
		
	// 4. IP와 Port 셋팅
	_tcscpy_s(NewAccount->m_tIP, _countof(Buff), Buff);
	NewAccount->m_wPort = port;
	
	// 5. Accept한 유저만을 관리하는 map에 추가	
	typedef pair<SOCKET, stAcceptUser*> Itn_pair;
	map_AcceptList.insert(Itn_pair(*client_sock, NewAccount));

	g_AcceptCount++;
}

// Disconnect 처리
void Disconnect(SOCKET sock)
{
	// 인자로 받은 sock을 기준으로 Accept 목록에서 유저 알아오기.
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowAccount == nullptr)
	{
		_tprintf(_T("Disconnect(). Accept 중이 아닌 유저를 대상으로 삭제 시도.\n"));
		return;
	}	

	// 2. 해당 Accept 유저를 AcceptList에서 제거
	map_AcceptList.erase(sock);

	// 3. 접속 종료한 유저의 ip, port, AccountNo, socket 출력	
	_tprintf(_T("Disconnect : [%s : %d / AccountNo : %lld / Socket : %lld]\n"), NowAccount->m_tIP, NowAccount->m_wPort, NowAccount->m_AccountNo, NowAccount->m_sock);

	// 4. 해당 유저의 소켓 close
	closesocket(NowAccount->m_sock);

	// 5. 해당 Accept유저 동적 해제
	delete NowAccount;

	g_AcceptCount--;
}

// 패킷 헤더 조립
void CreateHeader(CProtocolBuff* headerBuff, WORD MsgType, WORD PayloadSize)
{
	BYTE byCode = dfPACKET_CODE;
	*headerBuff << byCode;
	*headerBuff << MsgType;
	*headerBuff << PayloadSize;
}

// 현재 커텍트한 유저 수 알아오기
UINT64 AcceptCount()
{
	return g_AcceptCount;
}




/////////////////////////////
// Json 및 파일 입출력 함수
////////////////////////////

// 제이슨에 데이터 셋팅하는 함수(UTF-16)
bool Json_Create()
{
	using namespace rapidjson;

	GenericStringBuffer<UTF16<>> StringJson;
	Writer< GenericStringBuffer<UTF16<>>, UTF16<>, UTF16<> > Writer(StringJson);

	Writer.StartObject();	

	// 회원 목록 
	map<UINT64, stAccount*>::iterator Accountitor;
	Writer.String(L"Account");
	Writer.StartArray();
	for (Accountitor = map_AccountList.begin(); Accountitor != map_AccountList.end(); ++Accountitor)
	{
		Writer.StartObject();

		// AccountNo		
		Writer.String(L"AccountNo");
		Writer.Uint64(Accountitor->first);

		// NickName
		Writer.String(L"NickName");
		Writer.String(Accountitor->second->m_NickName);
		
		// 회원의 친구 수
		Writer.String(L"FriendCount");
		Writer.Uint64(Accountitor->second->m_FriendCount);

		// 회원이 보낸 요청 수
		Writer.String(L"RequestCount");
		Writer.Uint64(Accountitor->second->m_RequestCount);

		// 회원이 받은 요청 수
		Writer.String(L"ReplyCount");
		Writer.Uint64(Accountitor->second->m_ReplyCount);

		Writer.EndObject();
	}
	Writer.EndArray();
	
	// 친구 목록 
	map<UINT64, stFromTo*>::iterator Frienditor;
	Writer.String(L"Friend");
	Writer.StartArray();
	for (Frienditor = map_FriendList.begin(); Frienditor != map_FriendList.end(); ++Frienditor)
	{
		Writer.StartObject();

		// FromAccountNo
		Writer.String(L"FromAccountNo");
		Writer.Uint64(Frienditor->second->m_FromAccount);

		// ToAccountNo
		Writer.String(L"ToAccountNo");
		Writer.Uint64(Frienditor->second->m_ToAccount);

		// Time
		Writer.String(L"Time");
		Writer.Uint64(Frienditor->second->m_Time);

		Writer.EndObject();

	}
	Writer.EndArray();

	// 보낸요청 목록
	map<UINT64, stFromTo*>::iterator Requestitor;
	Writer.String(L"FriendRequest");
	Writer.StartArray();
	for (Requestitor = map_FriendRequestList.begin(); Requestitor != map_FriendRequestList.end(); ++Requestitor)
	{
		Writer.StartObject();

		// FromAccountNo
		Writer.String(L"FromAccountNo");
		Writer.Uint64(Requestitor->second->m_FromAccount);

		// ToAccountNo
		Writer.String(L"ToAccountNo");
		Writer.Uint64(Requestitor->second->m_ToAccount);

		// Time
		Writer.String(L"Time");
		Writer.Uint64(Requestitor->second->m_Time);

		Writer.EndObject();

	}
	Writer.EndArray();


	// 받은요청 목록
	map<UINT64, stFromTo*>::iterator Replyitor;
	Writer.String(L"FriendReply");
	Writer.StartArray();
	for (Replyitor = map_FriendReplyList.begin(); Replyitor != map_FriendReplyList.end(); ++Replyitor)
	{
		Writer.StartObject();

		// FromAccountNo
		Writer.String(L"FromAccountNo");
		Writer.Uint64(Replyitor->second->m_FromAccount);

		// ToAccountNo
		Writer.String(L"ToAccountNo");
		Writer.Uint64(Replyitor->second->m_ToAccount);

		// Time
		Writer.String(L"Time");
		Writer.Uint64(Replyitor->second->m_Time);

		Writer.EndObject();

	}
	Writer.EndArray();
	Writer.EndObject();

	// pJson에는 UTF-16의 형태로 저장된다.
	const TCHAR* tpJson = StringJson.GetString();
	size_t Size = StringJson.GetSize();

	// 파일 생성 후 데이터 생성 및 쓰기
	if (FileCreate_UTF16(_T("FriendDB_Json.txt"), tpJson, Size) == false)
	{
		fputs("Json 파일 생성 실패...\n", stdout);
		return false;
	}

	fputs("Json 파일 생성 성공!\n", stdout);
	return true;
}

// 파일 생성 후 데이터 쓰기 함수
bool FileCreate_UTF16(const TCHAR* FileName, const TCHAR* tpJson, size_t StringSize)
{
	FILE* fp;			// 파일 스트림
	size_t iFileCheck;	// 파일 오픈 여부 체크, 파일의 사이즈 저장. 총 2개의 옹도

	// 파일 생성 (UTF-16 리틀엔디안)
	iFileCheck = _tfopen_s(&fp, FileName, _T("wt, ccs=UTF-16LE"));
	if (iFileCheck != 0)
	{
		fputs("fopen 에러발생!\n", stdout);
		return false;
	}

	// 파일에 데이터 쓰기
	iFileCheck = fwrite(tpJson, 1, StringSize, fp);
	if (iFileCheck != StringSize)
	{
		fputs("fwrite 에러발생!\n", stdout);
		return false;
	}

	fclose(fp);
	return true;

}

// 제이슨에서 데이터 읽어와 셋팅하는 함수(UTF-16)
bool Json_Get()
{
	using namespace rapidjson;

	TCHAR* tpJson = nullptr;

	// 파일에서 데이터 읽어오기
	if (LoadFile_UTF16(_T("FriendDB_Json.txt"), &tpJson) == false)
	{
		fputs("Json 파일 읽어오기 실패...\n", stdout);
		return false;
	}

	else if (tpJson == nullptr)
	{
		fputs("LoadFile() 함수에서 읽어온 제이슨 파일이 nullptr.\n", stdout);
		fputs("Json 파일 읽어오기 실패...\n", stdout);
		return false;
	}

	// Json데이터 파싱하기 (이미 UTF-16을 넣는중)
	GenericDocument<UTF16<>> Doc;
	Doc.Parse(tpJson);

	/////////////////////////////////
	// 회원 정보 셋팅
	////////////////////////////////
	// AccountArray에 Account관련 데이터 넣기
	GenericValue<UTF16<>> &AccountArray = Doc[_T("Account")];
	SizeType Size = AccountArray.Size();

	// 필요한 변수 선언
	UINT64  AccountNo;			// 유저 회원 넘버
	const TCHAR	*NickName;		// 닉네임

	UINT	FriendCount;		// 내 친구 수 카운트
	UINT	RequestCount;		// 내가 보낸 친구요청 수 카운트
	UINT	ReplyCount;			// 내가 받은 친구요청 수 카운트
	
	for (SizeType i = 0; i < Size; ++i)
	{		
		GenericValue<UTF16<>> &AccountObject = AccountArray[i];

		// AccountNo 빼오기
		AccountNo = AccountObject[_T("AccountNo")].GetInt64();

		// 닉네임 빼오기
		NickName = AccountObject[_T("NickName")].GetString();

		// FriendCount빼오기
		FriendCount = AccountObject[_T("FriendCount")].GetInt64();

		// RequestCount빼오기
		RequestCount = AccountObject[_T("RequestCount")].GetInt64();

		// ReplyCount 빼오기
		ReplyCount = AccountObject[_T("ReplyCount")].GetInt64();
		
		// 빼온 데이터를 회원 구조체에 셋팅
		stAccount* NewAccount = new stAccount;

		NewAccount->m_AccountNo = AccountNo;
		NewAccount->m_FriendCount = FriendCount;
		NewAccount->m_RequestCount = RequestCount;
		NewAccount->m_ReplyCount = ReplyCount;
		_tcscpy_s(NewAccount->m_NickName, dfNICK_MAX_LEN, NickName);

		// 셋팅한 회원을 [회원 목록]에 추가
		typedef pair<UINT64, stAccount*> Itn_pair;
		map_AccountList.insert(Itn_pair(AccountNo, NewAccount));
	}

	// 회원 고유 ID 재셋팅(이전 정보부터 쭉 이어가야하니까!)
	g_uiAccountNo = Size+1;


	/////////////////////////////////
	// 친구 목록 셋팅
	////////////////////////////////
	// FriendArray에 Friend관련 데이터 넣기
	GenericValue<UTF16<>> &FriendArray = Doc[_T("Friend")];
	Size = FriendArray.Size();

	// 필요한 변수 선언
	UINT64 FromAccountNo;
	UINT64 ToAccountNo;
	UINT64 Time;

	for (SizeType i = 0; i < Size; ++i)
	{
		GenericValue<UTF16<>> &FriendObject = FriendArray[i];

		// FromAccountNo빼오기
		FromAccountNo = FriendObject[_T("FromAccountNo")].GetInt64();

		// ToAccountNo빼오기
		ToAccountNo = FriendObject[_T("ToAccountNo")].GetInt64();

		// Time빼오기
		Time = FriendObject[_T("Time")].GetInt64();
		
		// 빼온 데이터를 stFromTo 구조체에 셋팅
		stFromTo* NewFriendList = new stFromTo;

		NewFriendList->m_FromAccount = FromAccountNo;
		NewFriendList->m_ToAccount = ToAccountNo;
		NewFriendList->m_Time = Time;

		// 셋팅한 데이터를 [친구 목록]에 추가
		// 셋팅한 회원을 [회원 목록]에 추가
		typedef pair<UINT64, stFromTo*> Itn_pair;
		map_FriendList.insert(Itn_pair(g_uiFriendIndex, NewFriendList));
		g_uiFriendIndex++;
	}



	/////////////////////////////////
	// 보낸요청 목록 셋팅
	////////////////////////////////
	// RequestArray에 Friend관련 데이터 넣기
	GenericValue<UTF16<>> &RequestArray = Doc[_T("FriendRequest")];
	Size = RequestArray.Size();

	for (SizeType i = 0; i < Size; ++i)
	{
		GenericValue<UTF16<>> &RequestObject = RequestArray[i];

		// FromAccountNo빼오기
		FromAccountNo = RequestObject[_T("FromAccountNo")].GetInt64();

		// ToAccountNo빼오기
		ToAccountNo = RequestObject[_T("ToAccountNo")].GetInt64();

		// Time빼오기
		Time = RequestObject[_T("Time")].GetInt64();

		// 빼온 데이터를 stFromTo 구조체에 셋팅
		stFromTo* NewRequestList = new stFromTo;

		NewRequestList->m_FromAccount = FromAccountNo;
		NewRequestList->m_ToAccount = ToAccountNo;
		NewRequestList->m_Time = Time;

		// 셋팅한 회원을 [회원 목록]에 추가
		typedef pair<UINT64, stFromTo*> Itn_pair;
		map_FriendRequestList.insert(Itn_pair(g_uiRequestIndex, NewRequestList));
		g_uiRequestIndex++;
	}



	/////////////////////////////////
	// 받은요청 목록 셋팅
	////////////////////////////////
	// ReplyArray에 Friend관련 데이터 넣기
	GenericValue<UTF16<>> &ReplyArray = Doc[_T("FriendReply")];
	Size = ReplyArray.Size();

	for (SizeType i = 0; i < Size; ++i)
	{
		GenericValue<UTF16<>> &ReplyObject = ReplyArray[i];

		// FromAccountNo빼오기
		FromAccountNo = ReplyObject[_T("FromAccountNo")].GetInt64();

		// ToAccountNo빼오기
		ToAccountNo = ReplyObject[_T("ToAccountNo")].GetInt64();

		// Time빼오기
		Time = ReplyObject[_T("Time")].GetInt64();

		// 빼온 데이터를 stFromTo 구조체에 셋팅
		stFromTo* NewReplyList = new stFromTo;

		NewReplyList->m_FromAccount = FromAccountNo;
		NewReplyList->m_ToAccount = ToAccountNo;
		NewReplyList->m_Time = Time;

		// 셋팅한 데이터를 [친구 목록]에 추가
		// 셋팅한 회원을 [회원 목록]에 추가
		typedef pair<UINT64, stFromTo*> Itn_pair;
		map_FriendReplyList.insert(Itn_pair(g_uiReplyIndex, NewReplyList));
		g_uiReplyIndex++;
	}

	fputs("제이슨 파일 셋팅 성공!\n", stdout);

	return true;
}

// 파일에서 데이터 읽어오는 함수
// pJson에 읽어온 데이터가 저장된다.
bool LoadFile_UTF16(const TCHAR* FileName, TCHAR** pJson)
{
	FILE* fp;			// 파일 스트림
	size_t iFileCheck;	// 파일 오픈 여부 체크, 파일의 사이즈 저장. 총 2개의 옹도

	// 파일 열기
	iFileCheck = _tfopen_s(&fp, FileName, _T("rt, ccs=UTF-16LE"));
	if (iFileCheck != 0)
	{
		// 파일이 없을 경우, ENOENT(2) 에러가 뜨기도 함
		if (iFileCheck == ENOENT)
			fputs("제이슨 읽어올 파일이 없음!\n", stdout);

		// 파일이 있는데도 에러가 뜨는거면 그냥 에러임.
		else
			fputs("fopen 에러발생!\n", stdout);

		return false;
	}

	// 파일 읽어오기 위해 파일 크기만큼 동적할당
	fseek(fp, 0, SEEK_END);
	iFileCheck = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// BOM코드 가져오기. (2바이트)
	// UTF-16LE의 BOM은 HxD로 볼때는 FF FE이다.
	// 근데, 읽어오면 리틀엔디안으로 저장되기 때문에, BOMCheck에는 FE FF가 들어있다.
	TCHAR BOMCheck;
	if (fread_s(&BOMCheck, sizeof(TCHAR), 2, 1, fp) != 1)
	{
		fputs("fread_s BOM코드 읽어오다 문제생김!\n", stdout);
		return false;
	}

	// BOM코드 존재하지 않는다면, 파일지시자를 다시 처음으로 보내서, 처음부터 다시 읽어와야한다.
	if (BOMCheck != 0xfeff)
		fseek(fp, 0, SEEK_SET);

	// BOM코드가 존재한다면, 이미 그만큼 이동한 것이니, BOM만큼(2바이트)을 버리기 위해 iFileCheck 재조정
	else
		iFileCheck -= 2;

	*pJson = new TCHAR[iFileCheck];

	// 파일에서 데이터 읽어오기.
	size_t iSize = fread_s(*pJson, iFileCheck, 1, iFileCheck, fp);
	if (iSize != iFileCheck)
	{
		fputs("fread_s 에러발생!\n", stdout);
		return false;
	}

	fclose(fp);

	return true;
}




/////////////////////////
// Recv 처리 함수들
/////////////////////////
bool RecvProc(SOCKET sock)
{
	// 인자로 받은 sock을 기준으로 Accept 목록에서 유저 알아오기.
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 예외사항 체크
	// 1. 해당 유저가 로그인 중인 유저인가.
	// NowUser의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowAccount == nullptr)
	{
		_tprintf(_T("RecvProc(). 로그인 중이 아닌 유저를 대상으로 RecvProc 호출됨. 접속 종료\n"));
		return false;
	}

	////////////////////////////////////////////////////////////////////////
	// 소켓 버퍼에 있는 모든 recv 데이터를, 해당 유저의 recv링버퍼로 리시브.
	////////////////////////////////////////////////////////////////////////

	// 1. recv 링버퍼 포인터 얻어오기.
	char* recvbuff = NowAccount->m_RecvBuff.GetBufferPtr();

	// 2. 한 번에 쓸 수 있는 링버퍼의 남은 공간 얻기
	int Size = NowAccount->m_RecvBuff.GetNotBrokenPutSize();

	// 3. 한 번에 쓸 수 있는 공간이 0이라면
	if (Size == 0)
	{
		// rear 1칸 이동
		NowAccount->m_RecvBuff.MoveWritePos(1);

		// 그리고 한 번에 쓸 수 있는 위치 다시 알기.
		Size = NowAccount->m_RecvBuff.GetNotBrokenPutSize();
	}

	// 4. 그게 아니라면, 1칸 이동만 하기. 		
	else
	{
		// rear 1칸 이동
		NowAccount->m_RecvBuff.MoveWritePos(1);
	}

	// 5. 1칸 이동된 rear 얻어오기
	int* rear = (int*)NowAccount->m_RecvBuff.GetWriteBufferPtr();

	// 6. recv()
	int retval = recv(NowAccount->m_sock, &recvbuff[*rear], Size, 0);

	// 7. 리시브 에러체크
	if (retval == SOCKET_ERROR)
	{
		// WSAEWOULDBLOCK에러가 발생하면, 소켓 버퍼가 비어있다는 것. recv할게 없다는 것이니 함수 종료.
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return true;

		// 10054 에러는 상대방이 강제로 종료한 것.
		// 이 때는 그냥 return false로 나도 상대방과 연결을 끊는다.
		else if (WSAGetLastError() == 10054)
			return false;

		// WSAEWOULDBLOCK에러가 아니면 뭔가 이상한 것이니 접속 종료
		else
		{
			_tprintf(_T("RecvProc(). retval 후 우드블럭이 아닌 에러가 뜸(%d). 접속 종료\n"), WSAGetLastError());
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}
	}

	// 0이 리턴되는 것은 정상종료.
	else if(retval == 0)
		return false;

	// 8. 여기까지 왔으면 Rear를 이동시킨다.
	NowAccount->m_RecvBuff.MoveWritePos(retval - 1);


	//////////////////////////////////////////////////
	// 완료 패킷 처리 부분
	// RecvBuff에 들어있는 모든 완성 패킷을 처리한다.
	//////////////////////////////////////////////////
	while (1)
	{
		// 1. RecvBuff에 최소한의 사이즈가 있는지 체크. (조건 = 헤더 사이즈와 같거나 초과. 즉, 일단 헤더만큼의 크기가 있는지 체크)		
		st_PACKET_HEADER HeaderBuff;
		int len = sizeof(HeaderBuff);

		// RecvBuff의 사용 중인 버퍼 크기가 헤더 크기보다 작다면, 완료 패킷이 없다는 것이니 while문 종료.
		if (NowAccount->m_RecvBuff.GetUseSize() < len)
			break;

		// 2. 헤더를 Peek으로 확인한다.		
		// Peek 안에서는, 어떻게 해서든지 len만큼 읽는다. 버퍼가 꽉 차지 않은 이상!
		int PeekSize = NowAccount->m_RecvBuff.Peek((char*)&HeaderBuff, len);

		// Peek()은 버퍼가 비었으면 -1을 반환한다. 버퍼가 비었으니 일단 끊는다.
		if (PeekSize == -1)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 RecvBuff비었음. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 3. 헤더의 code 확인(정상적으로 온 데이터가 맞는가)
		if (HeaderBuff.byCode != dfPACKET_CODE)
		{
			_tprintf(_T("RecvProc(). 완료패킷 헤더 처리 중 PacketCode틀림. 접속 종료\n"));
			return false;	// FALSE가 리턴되면, 접속이 끊긴다.
		}

		// 4. 헤더의 Len값과 RecvBuff의 데이터 사이즈 비교. (완성 패킷 사이즈 = 헤더 사이즈 + 페이로드 Size )
		// 계산 결과, 완성 패킷 사이즈가 안되면 while문 종료.
		if (NowAccount->m_RecvBuff.GetUseSize() < (len + HeaderBuff.wPayloadSize))
			break;

		// 5. RecvBuff에서 Peek했던 헤더를 지우고 (이미 Peek했으니, 그냥 Remove한다)
		NowAccount->m_RecvBuff.RemoveData(len);

		// 6. RecvBuff에서 페이로드 Size 만큼 페이로드 임시 버퍼로 뽑는다. (디큐이다. Peek 아님)
		DWORD BuffArray = 0;
		CProtocolBuff PayloadBuff;
		DWORD PayloadSize = HeaderBuff.wPayloadSize;

		while (PayloadSize > 0)
		{
			int DequeueSize = NowAccount->m_RecvBuff.Dequeue(PayloadBuff.GetBufferPtr() + BuffArray, PayloadSize);

			// Dequeue()는 버퍼가 비었으면 -1을 반환. 버퍼가 비었으면 일단 끊는다.
			if (DequeueSize == -1)
			{
				_tprintf(_T("RecvProc(). 완료패킷 페이로드 처리 중 RecvBuff비었음. 접속 종료\n"));
				return false;	// FALSE가 리턴되면, 접속이 끊긴다.
			}

			PayloadSize -= DequeueSize;
			BuffArray += DequeueSize;
		}
		PayloadBuff.MoveWritePos(BuffArray);

		// 7. 헤더에 들어있는 타입에 따라 분기처리.
		bool check = PacketProc(HeaderBuff.wMsgType, sock, (char*)&PayloadBuff);
		if (check == false)
			return false;
	}


	return true;
}

// 패킷 처리
// PacketProc() 함수에서 false가 리턴되면 해당 유저는 접속이 끊긴다.
bool PacketProc(WORD PacketType, SOCKET sock, char* Packet)
{
	_tprintf(_T("PacketRecv [Socket : %lld / TypeID : %d]\n"), sock, PacketType);

	bool check = true;

	try
	{
		switch (PacketType)
		{

		// 회원 가입 요청
		case df_REQ_ACCOUNT_ADD:
		{
			check = Network_Req_AccountAdd(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 회원 로그인 요청
		case df_REQ_LOGIN:
		{
			check = Network_Req_Login(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;


		// 회원 목록 요청
		case df_REQ_ACCOUNT_LIST:
		{
			check = Network_Req_AccountList(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 내 친구 목록 요청
		case df_REQ_FRIEND_LIST:
		{
			check = Network_Req_FriendList(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 보낸 친구요청 목록 요청
		case df_REQ_FRIEND_REQUEST_LIST:
		{
			check = Network_Req_RequestList(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;


		// 받은 친구요청 목록 요청
		case df_REQ_FRIEND_REPLY_LIST:
		{
			check = Network_Req_ReplytList(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 친구 끊기
		case df_REQ_FRIEND_REMOVE:
		{
			check = Network_Req_FriendRemove(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 친구 요청
		case df_REQ_FRIEND_REQUEST:
		{
			check = Network_Req_FriendRequest(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 보낸 친구요청 취소
		case df_REQ_FRIEND_CANCEL:
		{
			check = Network_Req_RequestCancel(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 받은 친구요청 거절
		case df_REQ_FRIEND_DENY:
		{
			check = Network_Req_RequestDeny(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 친구요청 수락
		case df_REQ_FRIEND_AGREE:
		{
			check = Network_Req_FriendAgree(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		// 스트레스 테스트용 에코
		case df_REQ_STRESS_ECHO:
		{
			check = Network_Req_StressTest(sock, Packet);
			if (check == false)
				return false;	// 해당 유저 접속 끊김
		}
		break;

		default:
			_tprintf(_T("이상한 패킷입니다.\n"));
			return false;
		}


	}
	catch (CException exc)
	{
		TCHAR* text = (TCHAR*)exc.GetExceptionText();
		_tprintf(_T("%s.\n"), text);
		return false;

	}

	return true;
}

// "회원가입 요청(회원목록 추가)" 패킷 처리
bool Network_Req_AccountAdd(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 Accept한 회원 알아오기
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (NowAccount == nullptr)
	{
		_tprintf(_T("Network_Req_AccountAdd(). Accept하지 않은 유저가 회원가입(회원목록추가)요청. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}	

	// 3. 닉네임 골라내기
	CProtocolBuff* AccountAddPacket = (CProtocolBuff*)Packet;
	TCHAR NickName[dfNICK_MAX_LEN];
	*AccountAddPacket >> NickName;	

	// 4. 생성할 회원 정보 셋팅 (닉네임 AccountNo)
	stAccount* NewUser = new stAccount;
	_tcscpy_s(NewUser->m_NickName, _countof(NickName), NickName);
	NewUser->m_AccountNo = g_uiAccountNo;
	g_uiAccountNo++;

	// 5. "회원가입 요청(회원목록 추가) 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_AccountAdd((char*)&header, (char*)&Payload, NewUser->m_AccountNo);

	// 6. 해당 유저를 서버 내 [회원 목록] map에 추가
	typedef pair<UINT64, stAccount*> Itn_Pair;
	map_AccountList.insert(Itn_Pair(NewUser->m_AccountNo, NewUser));
	
	// 7. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// "로그인 요청" 패킷 처리
bool Network_Req_Login(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 Accept한 회원 알아오기
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (NowAccount == nullptr)
	{
		_tprintf(_T("Network_Req_Login(). Accept하지 않은 유저가 로그인 요청함. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. 로그인 목표 AccountNo를 알아온다.
	CProtocolBuff* LoginPacket = (CProtocolBuff*)Packet;
	UINT64 AcoNo;

	*LoginPacket >> AcoNo;

	// 4. 해당 AccountNo가 [회원 목록]에 있는지 체크
	stAccount* NowUser = nullptr;
	map<UINT64, stAccount*>::iterator itor;
	for (itor = map_AccountList.begin(); itor != map_AccountList.end(); ++itor)
	{
		if (itor->first == AcoNo)
		{
			NowUser = itor->second;
			break;
		}
	}

	// 5-1 만약, 해당 AccountNo가 서버 내에 없다면, 실패 메시지를 보내기 위해 AcoNo에 0을 넣는다.
	TCHAR NickName[dfNICK_MAX_LEN];

	if (NowUser == nullptr)
		AcoNo = 0;

	// 5-2. 만약, 해당 AccountNo가 서버 내에 있다면 해당 AccountNo 회원의 닉네임 셋팅
	// 그리고, 해당 Accept유저가 로그인 한 유저 셋팅
	else
	{
		_tcscpy_s(NickName, _countof(NickName), NowUser->m_NickName);
		NowAccount->m_AccountNo = NowUser->m_AccountNo;
	}

	// 6. "로그인 요청 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_Login((char*)&header, (char*)&Payload, AcoNo, NickName);

	// 7. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// "회원 목록 요청" 패킷 처리
bool Network_Req_AccountList(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 Accept한 회원인지 알아오기
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (NowAccount == nullptr)
	{
		_tprintf(_T("Network_Req_AccountList(). Accept하지 않은 유저가 회원 목록요청. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. "회원 목록 요청 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_AccountList((char*)&header, (char*)&Payload);
	
	// 4. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// "친구 목록 요청" 패킷 처리
bool Network_Req_FriendList(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 Accept한 회원인지 알아오기
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (NowAccount == nullptr)
	{
		_tprintf(_T("Network_Req_AccountList(). Accept하지 않은 유저가 회원 목록요청. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. "친구 목록 요청 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	stAccount* NowUser = ClientSearch_AccountList(NowAccount->m_AccountNo);

	// 아직 로그인한 유저가 아니라면, 친구 카운트로 0 설정
	UINT FriendCount;
	if (NowUser == nullptr)
		FriendCount = 0;
	else
		FriendCount = NowUser->m_FriendCount;

	Network_Res_FriendList((char*)&header, (char*)&Payload, NowAccount->m_AccountNo, FriendCount);

	// 4. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;

}

// "보낸 친구요청 목록" 패킷 처리
bool Network_Req_RequestList(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 Accept한 회원인지 알아오기
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (NowAccount == nullptr)
	{
		_tprintf(_T("Network_Req_RequestList(). Accept하지 않은 유저가 [보낸 친구요청 목록] 요청. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. "보낸 친구요청 목록 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	stAccount* NowUser = ClientSearch_AccountList(NowAccount->m_AccountNo);

	// 아직 로그인한 유저가 아니라면, 보낸 요청 수로 0 설정
	UINT RequestCount;
	if (NowUser == nullptr)
		RequestCount = 0;
	else
		RequestCount = NowUser->m_RequestCount;

	Network_Res_RequestList((char*)&header, (char*)&Payload, NowAccount->m_AccountNo, RequestCount);

	// 4. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;

}

// "받은 친구요청 목록" 패킷 처리
bool Network_Req_ReplytList(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 Accept한 회원인지 알아오기
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (NowAccount == nullptr)
	{
		_tprintf(_T("Network_Req_ReplytList(). Accept하지 않은 유저가 [받은 친구요청 목록] 요청. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. "받은 친구요청 목록 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	stAccount* NowUser = ClientSearch_AccountList(NowAccount->m_AccountNo);

	// 아직 로그인한 유저가 아니라면, 받은 요청 수 0 설정
	UINT ReplyCount;
	if (NowUser == nullptr)
		ReplyCount = 0;
	else
		ReplyCount = NowUser->m_ReplyCount;

	Network_Res_ReplytList((char*)&header, (char*)&Payload, NowAccount->m_AccountNo, ReplyCount);

	// 4. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// "친구끊기 요청" 패킷 처리
bool Network_Req_FriendRemove(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 Accept한 회원인지 알아오기
	stAcceptUser* AcceptUser = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (AcceptUser == nullptr)
	{
		_tprintf(_T("Network_Req_FriendRemove(). Accept하지 않은 유저가 [친구 끊기] 요청. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. 친구를 끊을 회원의 AccountNo
	CProtocolBuff* RemovePacekt = (CProtocolBuff*)Packet;

	UINT64 RemoveAccountNo;
	*RemovePacekt >> RemoveAccountNo;

	// 4. 친구를 끊을 회원이 존재하는지 체크
	stAccount* NowAccount = ClientSearch_AccountList(RemoveAccountNo);

	// 5. 친구를 끊을 회원이 내 친구인지 체크
	// 끊을 회원이 나랑 친구일 경우, Frienditor가 end()가 아님
	map<UINT64, stFromTo*>::iterator Frienditor;
	for (Frienditor = map_FriendList.begin(); Frienditor != map_FriendList.end(); ++Frienditor)
		if (Frienditor->second->m_FromAccount == AcceptUser->m_AccountNo)	// 나와 친구인 누군가를 찾았고			
			if (Frienditor->second->m_ToAccount == RemoveAccountNo)			// 그 누군가가 이번에 친구를 끊을 회원이라면, 나와 친구인 유저인 것. for문 종료
				break;
	

	// 6. 결과 셋팅
	BYTE result;

	// 6-1. Accept 유저가 로그인 중이 아니라면, 로그인 중 아니라고 결과 셋팅
	if (AcceptUser->m_AccountNo == 0)
		result = df_RESULT_FRIEND_REMOVE_FAIL;

	// 6-2. 친구 요청을 받을 회원이 없거나, 나 자신이 나에게 보낸 친구요청(?)을 거절하는 경우, 회원 없다고 결과 셋팅
	// 나 자신이 나에게 친구요청을 보낸 경우는 애초에 친구요청할때 막지만, 뭔일이 발생할지 모르니 여기서도 예외처리.
	else if (NowAccount == nullptr || RemoveAccountNo == AcceptUser->m_AccountNo)
		result = df_RESULT_FRIEND_REMOVE_NOTFRIEND;

	// 6-3. 친구가 아닌 유저와 친구끊기를 하려고 하는 경우, 회원 없다고 셋팅
	else if (Frienditor == map_FriendList.end())
		result = df_RESULT_FRIEND_REMOVE_NOTFRIEND;

	// 6-4. 모두 해당되지 않는다면, 정상 결과 셋팅
	// 그리고, [친구 목록]에서 서로 관계를 해제한다.
	else
	{
		result = df_RESULT_FRIEND_REMOVE_OK;
		stAccount* MyAccount = ClientSearch_AccountList(AcceptUser->m_AccountNo);

		// [친구 목록]에서 서로 관계 해제
		map <UINT64, stFromTo*>::iterator itor;
		itor = map_FriendList.begin();

		while (itor != map_FriendList.end())
		{
			// 내 친구를 찾았는데
			if (itor->second->m_FromAccount == AcceptUser->m_AccountNo)
			{				
				if (itor->second->m_ToAccount == RemoveAccountNo)			// 그 친구가 이번에 삭제한 친구라면
				{
					itor = map_FriendList.erase(itor);						// 해당 행을 삭제
					MyAccount->m_FriendCount--;								// 그리고 나의 친구 수 1 감소
				}
											
				else														 // 누군가가, 내가 이번에 삭제한 친구가 아니라면
					++itor;													// 이터레이터 다음으로 이동
			}
 
			// 상대의 친구를 찾았는데
			else if (itor->second->m_FromAccount == RemoveAccountNo)
			{				
				if (itor->second->m_ToAccount == AcceptUser->m_AccountNo)		// 그게 나라면		
				{
					itor = map_FriendList.erase(itor);							// 해당 행을 삭제
					NowAccount->m_FriendCount--;								// 그리고 상대의 친구 수 1 감소
				}

				else															// 누군가가 내가 아니라면
					++itor;														// 이터레이터 다음으로 이동
			}

			// itor->first가 나도아니고 상대도 아니라면 바로 다음 이터레이터로 이동
			else
				++itor;
		}		
	}

	// 6. ""친구끊기 요청 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_FriendRemove((char*)&header, (char*)&Payload, RemoveAccountNo, result);

	// 7. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// "친구 요청" 패킷 처리
bool Network_Req_FriendRequest(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 친구 요청을 보낸 유저 알아오기
	stAcceptUser* AcceptUser = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (AcceptUser == nullptr)
	{
		_tprintf(_T("Network_Req_FriendRequest(). Accept하지 않은 유저가 친구요청을 보냄. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. 친구 요청을 받을 회원의 AccountNo 알아내기
	CProtocolBuff* RequestPacekt = (CProtocolBuff*)Packet;

	UINT64 RequestAccountNo;
	*RequestPacekt >> RequestAccountNo;

	// 4. 친구 요청을 받을 회원이 존재하는지 체크
	stAccount* NowAccount = ClientSearch_AccountList(RequestAccountNo);

	// 5. 이미 친구요청을 보낸 회원인지 체크
	// 이미 친구요청을 보낸 회원이라면, FriendRequestitor가 end가 아니다.
	map<UINT64, stFromTo*>::iterator FriendRequestitor;
	for (FriendRequestitor = map_FriendRequestList.begin(); FriendRequestitor != map_FriendRequestList.end(); ++FriendRequestitor)
		if (FriendRequestitor->second->m_FromAccount == AcceptUser->m_AccountNo)			// 내가 보낸 친구요청을 찾았다고
			if (FriendRequestitor->second->m_ToAccount == RequestAccountNo)					// 요청을 받은 상대방이 이번에 친구요청을 보내려는 유저라면 for문 종료
				break;
	

	// 6. 이미 친구인 회원인지 체크
	// 이미 친구인 회원이라면, Frienditor가 end가 아니다
	map<UINT64, stFromTo*>::iterator Frienditor;
	for (Frienditor = map_FriendList.begin(); Frienditor != map_FriendList.end(); ++Frienditor)
		if (Frienditor->second->m_FromAccount == AcceptUser->m_AccountNo)	// 내 친구를 찾았는데
			if (Frienditor->second->m_ToAccount == RequestAccountNo)		// 그 친구가, 이번에 친구요청을 보내려는 유저라면 for문 종료
				break;

	// 7. 결과 셋팅
	BYTE result;

	// 7-1. Accept 유저가 로그인 중이 아니라면, 로그인 중 아니라고 결과 셋팅
	if (AcceptUser->m_AccountNo == 0)
		result = df_RESULT_FRIEND_REQUEST_AREADY;

	// 7-2. 로그인은 되어있지만 친구 요청을 받을 회원이 없거나, 나 자신에게 친구요청을 보낸 경우, 회원 없다고 결과 셋팅
	else if (NowAccount == nullptr || RequestAccountNo == AcceptUser->m_AccountNo)
		result = df_RESULT_FRIEND_REQUEST_NOTFOUND;

	// 7-3. 이미 친구 요청을 보낸 유저에게 또 친구요청을 보낸 경우, 실패 메시지 셋팅
	else if(FriendRequestitor != map_FriendRequestList.end())
		result = df_RESULT_FRIEND_REQUEST_NOTFOUND;

	// 7-4. 이미 친구인 유저에게 또 친구요청을 보낸 경우, 실패 메시지 셋팅
	else if (Frienditor != map_FriendList.end())
		result = df_RESULT_FRIEND_REQUEST_NOTFOUND;
	
	// 7-5. 모두 해당되지 않는다면, 정상 결과 셋팅
	// 그리고, [받은 친구요청 목록], [보낸 친구요청 목록]에 추가
	else
	{
		result = df_RESULT_FRIEND_REQUEST_OK;

		typedef pair<UINT64, stFromTo*> Itn_pair;

		// [보낸 친구요청 목록]에 추가
		stFromTo* MyRequest = new stFromTo;
		MyRequest->m_FromAccount = AcceptUser->m_AccountNo;
		MyRequest->m_ToAccount = RequestAccountNo;
		MyRequest->m_Time = 0;
		map_FriendRequestList.insert(Itn_pair(g_uiRequestIndex, MyRequest));
		g_uiRequestIndex++;

		// "나"의 보낸 친구요청 카운트 증가
		stAccount* MyAccount = ClientSearch_AccountList(AcceptUser->m_AccountNo);
		MyAccount->m_RequestCount++;

		// [받은 친구요청 목록]에 추가
		stFromTo* OtherRequest = new stFromTo;
		OtherRequest->m_FromAccount = RequestAccountNo;
		OtherRequest->m_ToAccount = AcceptUser->m_AccountNo;
		OtherRequest->m_Time = 0;
		map_FriendReplyList.insert(Itn_pair(g_uiRequestIndex, OtherRequest));
		g_uiRequestIndex++;

		// "상대"의 받은 친구요청 카운트 증가
		NowAccount->m_ReplyCount++;
	}

	// 8. "친구 요청 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_FriendRequest((char*)&header, (char*)&Payload, RequestAccountNo, result);

	// 8. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// "친구요청 취소" 패킷 처리
bool Network_Req_RequestCancel(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 친구 요청을 보낸 유저 알아오기
	stAcceptUser* AcceptUser = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (AcceptUser == nullptr)
	{
		_tprintf(_T("Network_Req_RequestCancel(). Accept하지 않은 유저가 친구요청 취소 보냄. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. 보낸 친구 요청을 취소할 회원의 AccountNo 알아내기
	CProtocolBuff* CancelPacekt = (CProtocolBuff*)Packet;

	UINT64 CancelAccountNo;
	*CancelPacekt >> CancelAccountNo;

	// 4. 내가 친구요청을 보냈던 유저가 아직 회원으로 존재하는지 체크
	// 로그인이 없어서..큰 의미가 없는것 같긴 한데 그냥 해봄.
	stAccount* NowAccount = ClientSearch_AccountList(CancelAccountNo);

	// 5. 이번에 취소하려는 요청이 [친구요청 목록]에 존재하는지 체크
	// 목록에 존재하면, Requestitor가 end()가 아님.
	map<UINT64, stFromTo*>::iterator Requestitor;
	for (Requestitor = map_FriendRequestList.begin(); Requestitor != map_FriendRequestList.end(); ++Requestitor)
		if (Requestitor->second->m_FromAccount == AcceptUser->m_AccountNo)			// 내가 보낸 요청을 찾았고
			if (Requestitor->second->m_ToAccount == CancelAccountNo)				// 상대가 이번에 삭제하려는 유저No라면, [친구요청 목록]에 존재하는 것. break;
				break;

	// 6. 혹시 이번에 취소하려는 유저와 이미 친구가 되었는지 체크
	// 거의 발생하지 않지만.. 혹시 패킷 처리가 느리다면 이미 친구가 되어있는 경우도 있을듯.
	// 친구 목록에 존재하면, Friendito가 end()가 아님
	map<UINT64, stFromTo*>::iterator Frienditor;
	for (Frienditor = map_FriendList.begin(); Frienditor != map_FriendList.end(); ++Frienditor)
		if (Frienditor->second->m_FromAccount == AcceptUser->m_AccountNo)				// 내 친구를 찾았고
			if (Frienditor->second->m_ToAccount == CancelAccountNo)						// 상대가 이번에 삭제하려는 유저라면, [친구 목록]에 있는 것. break;
				break;


	// 7. 결과 셋팅
	BYTE result;

	// 7-1. Accept 유저가 로그인 중이 아니라면, Fail결과 셋팅
	if (AcceptUser->m_AccountNo == 0)
		result = df_RESULT_FRIEND_CANCEL_FAIL;

	// 7-2. 로그인은 되어있지만 친구 요청을 받았던 회원이 없거나, 나 자신에게 보낸(?) 친구요청을 취소하려는 경우, 회원 없다고 결과 셋팅
	else if (NowAccount == nullptr || CancelAccountNo == AcceptUser->m_AccountNo)
		result = df_RESULT_FRIEND_REQUEST_NOTFOUND;

	// 7-3. 이번에 취소하려는 요청이 [친구요청 목록]에 존재하는지 않는다면, 회원 없다고 결과 셋팅
	else if (Requestitor == map_FriendRequestList.end())
		result = df_RESULT_FRIEND_REQUEST_NOTFOUND;

	// 7-4. 위 상황에 해당되지 않는다면, 정상 결과 셋팅
	else
	{
		stAccount* MyAccount = ClientSearch_AccountList(AcceptUser->m_AccountNo);

		// 1. 근데 만약, 뭔가 버그가 발생해서 이미 친구인 유저에게 보냈던 요청이 아직 남아있다면, 클라에는 에러를 보내고 [보낸 친구요청 목록]에서 삭제한다.
		if (Frienditor != map_FriendList.end())
			result = df_RESULT_FRIEND_CANCEL_FAIL;

		// 이게 아니라면 정말로 정상 결과 셋팅
		else
			result = df_RESULT_FRIEND_CANCEL_OK;

		// 2. [보낸요청 목록]에서 요청 삭제 ("나"의 보낸 요청쪽 삭제)
		map<UINT64, stFromTo*>::iterator itor;
		itor = map_FriendRequestList.begin();

		while (itor != map_FriendRequestList.end())
		{
			// 내가 보낸 요청을 찾았는데
			if (itor->second->m_FromAccount == AcceptUser->m_AccountNo)
			{
				// 상대가 이번에 요청을 취소하는 상대라면
				if (itor->second->m_ToAccount == CancelAccountNo)
				{
					// 해당 요청 삭제
					itor = map_FriendRequestList.erase(itor);

					// 그리고 내가 보낸 요청 수 1감소
					MyAccount->m_RequestCount--;
				}

				// 이번 취소 상대가 아니라면 itor 증가
				else
					++itor;
			}

			// 내가 보낸 요청이 아니면, itor 증가
			else
				++itor;
		}

		// 3. [받은요청 목록]에서 요청 삭제 ("상대"의 받은 요청쪽 삭제)
		itor = map_FriendReplyList.begin();
		while (itor != map_FriendReplyList.end())
		{
			// 상대가 받은 요청을 찾았는데
			if (itor->second->m_FromAccount == CancelAccountNo)
			{
				// 내가 보냈던 요청이라면
				if (itor->second->m_ToAccount == AcceptUser->m_AccountNo)
				{
					// 해당 요청 삭제
					itor = map_FriendReplyList.erase(itor);

					// 상대의 받은 요청 수 1감소
					NowAccount->m_ReplyCount--;
				}

				// 내가 보냈던 요청이 아니면 itor 증가
				else
					++itor;
			}

			// 상대가 받은 요청이 아니면, itor증가
			else
				++itor;
		}

	}

	// 6. "친구요청 취소 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_RequestCancel((char*)&header, (char*)&Payload, CancelAccountNo, result);

	// 7. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;

}

// "받은요청 거부" 패킷 처리
bool Network_Req_RequestDeny(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 친구 요청을 보낸 유저 알아오기
	stAcceptUser* AcceptUser = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (AcceptUser == nullptr)
	{
		_tprintf(_T("Network_Req_RequestDeny(). Accept하지 않은 유저가 받은요청 거부 보냄. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. 받은 요청을 거절할 회원의 AccountNo 알아내기
	CProtocolBuff* DenyPacekt = (CProtocolBuff*)Packet;

	UINT64 DenyAccountNo;
	*DenyPacekt >> DenyAccountNo;

	// 4. 나에게 친구요청을 보냈던 유저가 아직 회원으로 존재하는지 체크
	// 회원 탈퇴가 없어서..큰 의미가 없는것 같긴 한데 그냥 해봄.
	stAccount* NowAccount = ClientSearch_AccountList(DenyAccountNo);

	// 5. 이번에 거절하려는 요청이 [받은요청 목록]에 존재하는지 체크
	// 목록에 존재하면, Denyitor가 end()이 아님.
	map<UINT64, stFromTo*>::iterator Denyitor;
	for (Denyitor = map_FriendReplyList.begin(); Denyitor != map_FriendReplyList.end(); ++Denyitor)
		if (Denyitor->second->m_FromAccount == AcceptUser->m_AccountNo)			// 내가 받은 요청을 찾았고
			if (Denyitor->second->m_ToAccount == DenyAccountNo)					// 상대가 이번에 요청을 거절하려는 유저No라면, [받은요청 목록]에 존재하는 것. break;
				break;

	// 6. 혹시 이번에 취소하려는 유저와 이미 친구가 되었는지 체크
	// 거의 발생하지 않지만.. 혹시 패킷 처리가 느리다면 이미 친구가 되어있는 경우도 있을듯.
	// 친구 목록에 존재하면, Friendito가 end()가 아님
	map<UINT64, stFromTo*>::iterator Frienditor;
	for (Frienditor = map_FriendList.begin(); Frienditor != map_FriendList.end(); ++Frienditor)
		if (Frienditor->second->m_FromAccount == AcceptUser->m_AccountNo)				// 내 친구를 찾았고
			if (Frienditor->second->m_ToAccount == DenyAccountNo)						// 상대가 이번에 거절하려는 유저라면, [친구 목록]에 있는 것. break;
				break;

	// 7. 결과 셋팅
	BYTE result;

	// 7-1. Accept 유저가 로그인 중이 아니라면, Fail결과 셋팅
	if (AcceptUser->m_AccountNo == 0)
		result = df_RESULT_FRIEND_DENY_FAIL;

	// 7-2. 로그인은 되어있지만 친구 요청을 받았던 회원이 없거나, 내가 나에게 보낸(?) 친구 요청을 거절하려는 경우, 회원 없다고 결과 셋팅
	else if (NowAccount == nullptr || DenyAccountNo == AcceptUser->m_AccountNo)
		result = df_RESULT_FRIEND_DENY_NOTFRIEND;

	// 7-3. 이번에 거절하려는 요청이 [받은요청 목록]에 존재하는지 않는다면, 회원 없다고 결과 셋팅
	else if (Denyitor == map_FriendRequestList.end())
		result = df_RESULT_FRIEND_DENY_NOTFRIEND;

	// 7-4. 위 상황에 해당되지 않는다면, 정상 결과 셋팅
	else
	{
		stAccount* MyAccount = ClientSearch_AccountList(AcceptUser->m_AccountNo);

		// 1. 근데 만약, 뭔가 버그가 발생해서 이미 친구인 유저에게 보냈던 요청이 아직 남아있었다면, 클라에는 에러를 보내고 [받은 친구요청 목록]에서 삭제한다.
		if (Frienditor != map_FriendList.end())
			result = df_RESULT_FRIEND_DENY_FAIL;

		// 이게 아니라면 정말로 정상 결과 셋팅
		else
			result = df_RESULT_FRIEND_DENY_OK;

		// 3. [받은요청 목록]에서 요청 삭제 ("나"의 받은 요청쪽 삭제)
		map<UINT64, stFromTo*>::iterator itor;
		itor = map_FriendReplyList.begin();

		while (itor != map_FriendReplyList.end())
		{
			// 내가 받은 요청을 찾았는데
			if (itor->second->m_FromAccount == AcceptUser->m_AccountNo)
			{
				// 이번에 거절하는 상대가 보냈던 요청이라면
				if (itor->second->m_ToAccount == DenyAccountNo)
				{
					// 해당 요청 삭제
					itor = map_FriendRequestList.erase(itor);

					// 나의 받은 요청 수 1감소
					MyAccount->m_ReplyCount--;
				}

				// 상대가 보냈던 요청이 아니면 itor 증가
				else
					++itor;
			}

			// 내가 받은 요청이 아니면, itor증가
			else
				++itor;
		}


		// 2. [보낸요청 목록]에서 요청 삭제 ("상대"의 보낸 요청쪽 삭제)
		itor = map_FriendRequestList.begin();

		while (itor != map_FriendRequestList.end())
		{
			// 상대가 보낸 요청을 찾았는데
			if (itor->second->m_FromAccount == DenyAccountNo)
			{
				// 나에게 보낸 요청이라면
				if (itor->second->m_ToAccount == AcceptUser->m_AccountNo)
				{
					// 해당 요청 삭제
					itor = map_FriendRequestList.erase(itor);

					// 그리고 상대의 보낸 요청 수 1감소
					NowAccount->m_RequestCount--;
				}

				// 나에게 보낸 요청이 아니라면 itor 증가
				else
					++itor;
			}

			// 상대가 보낸 요청이 아니라면, itor 증가
			else
				++itor;
		}		
	}

	// 7. "보낸요청 취소 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_RequestDeny((char*)&header, (char*)&Payload, DenyAccountNo, result);

	// 8. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// "친구요청 수락" 패킷 처리
bool Network_Req_FriendAgree(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 친구 요청을 보낸 유저 알아오기
	stAcceptUser* AcceptUser = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (AcceptUser == nullptr)
	{
		_tprintf(_T("Network_Req_Login(). Accept하지 않은 유저가 로그인요청. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. 친구요청을 수락할 회원의 AccountNo 알아내기
	CProtocolBuff* AgreePacekt = (CProtocolBuff*)Packet;

	UINT64 AgreePacektAccountNo;
	*AgreePacekt >> AgreePacektAccountNo;

	// 4. 나에게 친구 요청을 보낸 회원이 존재하는지 체크 (발생할 일이 있을까 싶지만.. 혹시몰라서 해둠)
	stAccount* NowAccount = ClientSearch_AccountList(AgreePacektAccountNo);

	// 5. 나에게 온 친구요청을 수락한게 맞는지 체크
	// 맞다면, ReplyListitor가 end가 아님.
	map<UINT64, stFromTo*>::iterator ReplyListitor;
	for (ReplyListitor = map_FriendReplyList.begin(); ReplyListitor != map_FriendReplyList.end(); ++ReplyListitor)
		if (ReplyListitor->second->m_FromAccount == AcceptUser->m_AccountNo)	// 내가 받은 요청중에
			if (ReplyListitor->second->m_ToAccount == AgreePacektAccountNo)		// AgreePacektAccountNo 유저가 보낸 요청이 있으면, 나한테 온 친구요청에 대해 수락을 한게 맞는것. for문 break;
				break;


	// 6. 결과 셋팅
	BYTE result;

	// 6-1. Accept 유저가 로그인 중이 아니라면, Fail 결과 셋팅
	if (AcceptUser->m_AccountNo == 0)
		result = df_RESULT_FRIEND_AGREE_FAIL;

	// 6-2. 혹은, 로그인은 되어있지만 친구요청 수락할 회원이 없는 회원이거나, 나 자신이(?) 나에게 친구요청을 보낸 경우, 회원 없다고 결과 셋팅
	// 나 자신이 나에게 친구요청을 보낸 경우는 애초에 친구요청할때 막지만, 뭔일이 발생할지 모르니 여기서도 예외처리.
	else if (NowAccount == nullptr || AgreePacektAccountNo == AcceptUser->m_AccountNo)
		result = df_RESULT_FRIEND_AGREE_NOTFRIEND;

	// 6-3. 나에게 온 친구요청을 수락한게 아니라면, Fail결과 셋팅
	else if(ReplyListitor == map_FriendReplyList.end())
		result = df_RESULT_FRIEND_AGREE_FAIL;

	// 6-3. 모두 해당되지 않는다면, 정상 결과 셋팅
	// 그리고, [친구 목록]에 추가, [보낸 친구요청 목록]에서 삭제, [받은 친구요청 목록]에서 삭제
	else
	{
		result = df_RESULT_FRIEND_AGREE_OK;
		stAccount* MyAccount = ClientSearch_AccountList(AcceptUser->m_AccountNo);

		// 1. [친구 목록]에 추가
		// 주의할 점은, [1. A->B가 친구다. 2. B->A가 친구다] 둘 다 추가해야함
		typedef pair<UINT64, stFromTo*> Itn_pair;

		stFromTo* MyFriend = new stFromTo;
		MyFriend->m_FromAccount = AcceptUser->m_AccountNo;
		MyFriend->m_ToAccount = AgreePacektAccountNo;
		MyFriend->m_Time = 0;
		map_FriendList.insert(Itn_pair(g_uiFriendIndex, MyFriend));
		g_uiFriendIndex++;

		// 나의 친구 수 증가
		MyAccount->m_FriendCount++;

		stFromTo* OtherFriend = new stFromTo;
		OtherFriend->m_FromAccount = AgreePacektAccountNo;
		OtherFriend->m_ToAccount = AcceptUser->m_AccountNo;
		OtherFriend->m_Time = 0;
		map_FriendList.insert(Itn_pair(g_uiFriendIndex, OtherFriend));
		g_uiFriendIndex++;

		// 상대의 친구 수 증가
		NowAccount->m_FriendCount++;

		// 2. [받은 친구요청 목록]에서 삭제
		map<UINT64, stFromTo*>::iterator itor;
		itor = map_FriendReplyList.begin();

		while (itor != map_FriendReplyList.end())
		{
			// 내가 받은 요청을 찾았는데
			if (itor->second->m_FromAccount == AcceptUser->m_AccountNo)
			{
				// 상대가, 이번에 친구로 추가된 유저라면
				if (itor->second->m_ToAccount == AgreePacektAccountNo)
				{
					// 해당 행을 [받은 친구요청 목록]에서 삭제.
					itor = map_FriendReplyList.erase(itor);

					// 내 받은요청 수 감소
					MyAccount->m_ReplyCount--;
				}

				// 이번에 추가된 유저가 아니라면, itor 다음으로 증가.
				else
					itor++;

			}

			// 상대가 받은 요청을 찾았는데
			else if (itor->second->m_FromAccount == AgreePacektAccountNo)
			{
				// 내가 보낸 요청이었다면
				if (itor->second->m_ToAccount == AcceptUser->m_AccountNo)
				{
					// 해당 행을 [받은 친구요청 목록]에서 삭제.
					itor = map_FriendReplyList.erase(itor);

					// 상대의 받은요청 수 감소
					NowAccount->m_ReplyCount--;
				}

				// 나 아닌 다른사람이 보낸 요청이었다면 itor 다음으로 증가
				else
					itor++;
			}

			// 내가 받은 요청, 상대가 받은 요청 둘 다 아니라면 itor 다음으로 증가
			else
				itor++;
		}

		// 3. [보낸 친구요청 목록]에서 삭제
		itor = map_FriendRequestList.begin();
		while (itor != map_FriendRequestList.end())
		{
			// 내가 보낸 요청을 찾았는데
			if (itor->second->m_FromAccount == AcceptUser->m_AccountNo)
			{
				// 상대가, 이번에 친구로 추가된 유저라면
				if (itor->second->m_ToAccount == AgreePacektAccountNo)
				{
					// 해당 행을 [보낸 친구요청 목록]에서 삭제.
					itor = map_FriendRequestList.erase(itor);

					// 나의 보낸 요청 수 감소
					MyAccount->m_RequestCount--;
				}

				// 이번에 추가된 유저가 아니라면, itor 다음으로 증가.
				else
					itor++;
			}

			// 상대가 보낸 요청을 찾았는데
			else if (itor->second->m_FromAccount == AgreePacektAccountNo)
			{
				// 상대가, 나라면
				if (itor->second->m_ToAccount == AcceptUser->m_AccountNo)
				{
					// 해당 행을 [보낸 친구요청 목록]에서 삭제.
					itor = map_FriendRequestList.erase(itor);

					// 상대의 보낸 요청 수 감소
					NowAccount->m_RequestCount--;
				}

				// 상대가 내가 아니라면, itor 다음으로 증가.
				else
					itor++;
			}

			// 내가 보낸 요청, 상대가 보낸 요청 둘 다 아니라면 itor 다음으로 증가
			else
				itor++;
		}
	}

	// 7. "친구요청 수락 결과" 패킷 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_FriendAgree((char*)&header, (char*)&Payload, AgreePacektAccountNo, result);

	// 8. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	return true;
}

// 스트레스 테스트용 패킷 처리
bool Network_Req_StressTest(SOCKET sock, char* Packet)
{
	// 1. 인자로 받은 sock을 이용해 접속중(Accept)인 유저인지 체크
	stAcceptUser* AcceptUser = ClientSearch_AcceptList(sock);

	// 2. 에러 처리
	// 없는 유저일 경우(여기서 없는 유저는, Accept를 안한 유저), 연결을 끊는다.
	// 이런상황이 발생할지는 모르겠지만..혹시모르니 처리함
	if (AcceptUser == nullptr)
	{
		_tprintf(_T("Network_Req_StressTest(). Accept하지 않은 유저가 스트레스 테스트 패킷 보냄. (SOCKET : %lld). 접속 종료\n"), sock);
		return false;
	}

	// 3. 받은 문자열 꺼내기
	CProtocolBuff* StressTestBuff = (CProtocolBuff*)Packet;

	// 일단 문자열 사이즈 얻어오기 (바이트 단위)
	WORD wStringSize;
	*StressTestBuff >> wStringSize;

	// 사이즈 만큼 문자열 가져오기
	TCHAR* TestString = new TCHAR[(wStringSize / 2) + 1];
	for(int i=0; i<wStringSize/2; ++i)
		*StressTestBuff >> TestString[i];
	
	// 3. "스트레스 테스트용 패킷 결과" 제작
	CProtocolBuff header(dfHEADER_SIZE);
	CProtocolBuff Payload;

	Network_Res_StressTest((char*)&header, (char*)&Payload, wStringSize, TestString);
	
	// 4. 만든 패킷을 해당 유저의, SendBuff에 넣기
	bool check = SendPacket(sock, &header, &Payload);
	if (check == false)
		return false;

	delete TestString;

	return true;
}




/////////////////////////
// 패킷 제작 함수
/////////////////////////
// "회원가입 요청(회원목록 추가) 결과" 패킷 제작
void Network_Res_AccountAdd(char* header, char* Packet, UINT64 AccountNo)
{
	// 1. 페이로드 조립
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;
	*Payload << AccountNo;

	// 2. 헤더 조립
	CreateHeader((CProtocolBuff*)header, df_RES_ACCOUNT_ADD, Payload->GetUseSize());
}

// "로그인 요청 결과" 패킷 제작
void Network_Res_Login(char* header, char* Packet, UINT64 AccountNo, TCHAR* Nick)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;
	*Payload << AccountNo;

	for(int i=0; i<dfNICK_MAX_LEN; ++i)
		*Payload << Nick[i];

	// 2. 헤더 조립
	CreateHeader((CProtocolBuff*)header, df_RES_LOGIN, Payload->GetUseSize());
}

// "회원 목록 요청 결과" 패킷 제작
void Network_Res_AccountList(char* header, char* Packet)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// 서버 내의 회원 수 알아오기
	UINT AccountCount = (UINT)map_AccountList.size();
	*Payload << AccountCount;

	// 서버 내 모든 유저의 AccountNo, 닉네임 추가
	map<UINT64, stAccount*>::iterator itor;
	for (itor = map_AccountList.begin(); itor != map_AccountList.end(); ++itor)
	{
		// AccountNo 추가
		*Payload << itor->first;

		// 닉네임 추가
		for (int i = 0; i < dfNICK_MAX_LEN; ++i)
			*Payload << itor->second->m_NickName[i];
	}

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_ACCOUNT_LIST, Payload->GetUseSize());
}

// "친구 목록 요청 결과" 패킷 제작
void Network_Res_FriendList(char* header, char* Packet, UINT64 AccountNo, UINT FriendCount)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// 2. 내 친구 수 셋팅
	*Payload << FriendCount;

	// 내 친구 수가 0이 아닐 경우,
	// 처음부터 끝까지 [친구 목록]을 돌면서, 나와 친구인 유저의 AccountNo, 닉네임 셋팅
	map <UINT64, stFromTo*>::iterator itor;
	if (FriendCount != 0)
	{
		for (itor = map_FriendList.begin(); itor != map_FriendList.end(); ++itor)
		{
			// 내 친구를 찾았으면
			if (itor->second->m_FromAccount == AccountNo)
			{
				// 내 친구의 정보를 가져온다.
				map <UINT64, stAccount*>::iterator Accountitor;
				Accountitor = map_AccountList.find(itor->second->m_ToAccount);

				// 상대의 AccountNo, 닉네임 셋팅
				*Payload << Accountitor->second->m_AccountNo;

				for (int i = 0; i<dfNICK_MAX_LEN; ++i)
					*Payload << Accountitor->second->m_NickName[i];

			}
		}
	}

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_LIST, Payload->GetUseSize());
}

// "보낸 친구요청 목록 결과" 패킷 제작
void Network_Res_RequestList(char* header, char* Packet, UINT64 AccountNo, UINT ReplyCount)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// 보낸 친구요청 수(카운트) 셋팅	
	*Payload << ReplyCount;

	// 내가 보낸 친구요청 수가 0이 아닐 경우,
	// 처음부터 끝까지 [보낸 친구요청 목록]을 돌면서, 내가 친구요청을 보낸 유저의 AccountNo, 닉네임 셋팅
	map <UINT64, stFromTo*>::iterator itor;
	if (ReplyCount != 0)
	{
		for (itor = map_FriendRequestList.begin(); itor != map_FriendRequestList.end(); ++itor)
		{
			// 내가 보낸 요청을 찾았으면
			if (itor->second->m_FromAccount == AccountNo)
			{
				// 요청을 받은 상대의 정보를 가져온다.
				map <UINT64, stAccount*>::iterator Accountitor;
				Accountitor = map_AccountList.find(itor->second->m_ToAccount);

				// 상대의 AccountNo, 닉네임 셋팅
				*Payload << Accountitor->second->m_AccountNo;

				for (int i = 0; i<dfNICK_MAX_LEN; ++i)
					*Payload << Accountitor->second->m_NickName[i];

			}
		}
	}

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_REQUEST_LIST, Payload->GetUseSize());
}

// "받은 친구요청 목록 결과" 패킷 처리
void Network_Res_ReplytList(char* header, char* Packet, UINT64 AccountNo, UINT ReplyCount)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	//  받은 친구요청 수(카운트) 셋팅	
	*Payload << ReplyCount;

	// 내가 받은 친구요청 수가 0이 아닐 경우,
	// 처음부터 끝까지 [받은 친구요청 목록]을 돌면서, 나한테 친구요청을 보낸 유저의 AccountNo, 닉네임 셋팅
	multimap <UINT64, stFromTo*>::iterator itor;
	if (ReplyCount != 0)
	{
		for (itor = map_FriendReplyList.begin(); itor != map_FriendReplyList.end(); ++itor)
		{
			// 나한테 보낸 요청을 찾았으면
			if (itor->second->m_FromAccount == AccountNo)
			{
				// 요청을 보낸 상대의 정보를 가져온다.
				map <UINT64, stAccount*>::iterator Accountitor;
				Accountitor = map_AccountList.find(itor->second->m_ToAccount);

				// 상대의 AccountNo, 닉네임 셋팅
				*Payload << Accountitor->second->m_AccountNo;

				for (int i = 0; i<dfNICK_MAX_LEN; ++i)
					*Payload << Accountitor->second->m_NickName[i];

			}
		}
	}

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_REPLY_LIST, Payload->GetUseSize());
}

// "친구끊기 요청 결과" 패킷 제작
void Network_Res_FriendRemove(char* header, char* Packet, UINT64 AccountNo, BYTE Result)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// AccountNo, Result 셋팅
	*Payload << AccountNo;
	*Payload << Result;

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_REMOVE, Payload->GetUseSize());

}

// "친구 요청 결과" 패킷 제작
void Network_Res_FriendRequest(char* header, char* Packet, UINT64 AccountNo, BYTE Result)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// AccountNo, Result 셋팅
	*Payload << AccountNo;
	*Payload << Result;

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_REQUEST, Payload->GetUseSize());
}

// "친구요청 취소 결과" 패킷 제작
void Network_Res_RequestCancel(char* header, char* Packet, UINT64 AccountNo, BYTE Result)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// AccountNo, Result 셋팅
	*Payload << AccountNo;
	*Payload << Result;

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_CANCEL, Payload->GetUseSize());
}

// "보낸요청 취소 결과" 패킷 제작
void Network_Res_RequestDeny(char* header, char* Packet, UINT64 AccountNo, BYTE Result)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// AccountNo, Result 셋팅
	*Payload << AccountNo;
	*Payload << Result;

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_DENY, Payload->GetUseSize());

}

// "친구요청 수락 결과" 패킷 제작
void Network_Res_FriendAgree(char* header, char* Packet, UINT64 AccountNo, BYTE Result)
{
	// 1. 페이로드 제작
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	// AccountNo, Result 셋팅
	*Payload << AccountNo;
	*Payload << Result;

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_FRIEND_AGREE, Payload->GetUseSize());
}

// "스트레스 테스트 결과" 패킷 제작
void Network_Res_StressTest(char* header, char* Packet, WORD StringSize, TCHAR* TestString)
{
	// 1. 페이로드 조립
	CProtocolBuff* Payload = (CProtocolBuff*)Packet;

	*Payload << StringSize;

	for (int i = 0; i < StringSize / 2; ++i)
		*Payload << TestString[i];

	// 2. 헤더 제작
	CreateHeader((CProtocolBuff*)header, df_RES_STRESS_ECHO, Payload->GetUseSize());
}




/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(SOCKET sock, CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff)
{
	// 인자로 받은 sock을 기준으로 Accept 유저 목록에서 유저 알아오기.
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 예외사항 체크
	// 1. 해당 유저가 Accept 중인 유저인가.
	// NowAccount의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowAccount == nullptr)
	{
		_tprintf(_T("SendPacket(). 로그인 중이 아닌 유저를 대상으로 함수가 호출됨. 접속 종료\n"));
		return false;
	}

	// 1. 샌드 q에 헤더 넣기
	int Size = headerBuff->GetUseSize();
	DWORD BuffArray = 0;
	int a = 0;
	while (Size > 0)
	{
		int EnqueueCheck = NowAccount->m_SendBuff.Enqueue(headerBuff->GetBufferPtr() + BuffArray, Size);
		if (EnqueueCheck == -1)
		{
			_tprintf(_T("SendPacket(). 헤더 넣는 중 샌드큐 꽉참. 접속 종료\n"));
			return false;
		}

		Size -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}

	// 2. 샌드 q에 페이로드 넣기
	WORD PayloadLen = payloadBuff->GetUseSize();
	BuffArray = 0;
	while (PayloadLen > 0)
	{
		int EnqueueCheck = NowAccount->m_SendBuff.Enqueue(payloadBuff->GetBufferPtr() + BuffArray, PayloadLen);
		if (EnqueueCheck == -1)
		{
			_tprintf(_T("SendPacket(). 페이로드 넣는 중 샌드큐 꽉참. 접속 종료\n"));
			return false;
		}

		PayloadLen -= EnqueueCheck;
		BuffArray += EnqueueCheck;
	}


	return true;
}

// Send버퍼의 데이터를 Send하기
bool SendProc(SOCKET sock)
{
	// 인자로 받은 sock을 기준으로 Accept 유저 목록에서 유저 알아오기.
	stAcceptUser* NowAccount = ClientSearch_AcceptList(sock);

	// 예외사항 체크
	// 1. 해당 유저가 Accept 중인 유저인가.
	// NowAccount의 값이 nullptr이면, 해당 유저를 못찾은 것. false를 리턴한다.
	if (NowAccount == nullptr)
	{
		_tprintf(_T("SendProc(). 로그인 중이 아닌 유저를 대상으로 SendProc이 호출됨. 접속 종료\n"));
		return false;
	}

	// 2. SendBuff에 보낼 데이터가 있는지 확인.
	if (NowAccount->m_SendBuff.GetUseSize() == 0)
		return true;

	// 3. 샌드 링버퍼의 실질적인 버퍼 가져옴
	char* sendbuff = NowAccount->m_SendBuff.GetBufferPtr();

	// 4. SendBuff가 다 빌때까지 or Send실패(우드블럭)까지 모두 send
	while (1)
	{
		// 5. SendBuff에 데이터가 있는지 확인(루프 체크용)
		if (NowAccount->m_SendBuff.GetUseSize() == 0)
			break;

		// 6. 한 번에 읽을 수 있는 링버퍼의 남은 공간 얻기
		int Size = NowAccount->m_SendBuff.GetNotBrokenGetSize();

		// 7. 남은 공간이 0이라면 1로 변경. send() 시 1바이트라도 시도하도록 하기 위해서.
		// 그래야 send()시 우드블럭이 뜨는지 확인 가능
		if (Size == 0)
			Size = 1;

		// 8. front 포인터 얻어옴
		int *front = (int*)NowAccount->m_SendBuff.GetReadBufferPtr();

		// 9. front의 1칸 앞 index값 알아오기.
		int BuffArrayCheck = NowAccount->m_SendBuff.NextIndex(*front, 1);

		// 10. Send()
		int SendSize = send(NowAccount->m_sock, &sendbuff[BuffArrayCheck], Size, 0);

		// 11. 에러 체크. 우드블럭이면 더 이상 못보내니 while문 종료. 다음 Select때 다시 보냄
		if (SendSize == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;

			else
			{
				_tprintf(_T("SendProc(). Send중 에러 발생. 접속 종료\n"));
				return false;
			}
		}

		// 12. 보낸 사이즈가 나왔으면, 그 만큼 remove
		NowAccount->m_SendBuff.RemoveData(SendSize);
	}

	return true;
}
