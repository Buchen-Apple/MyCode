#ifndef __NETWORK_FUNC_H
#define __NETWORK_FUNC_H

// 테스트용으로 소켓 사이즈를 3으로 함.
//#define FD_SETSIZE      3

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>
#include "Protocol.h"
#include "ProtocolBuff\ProtocolBuff.h"
#include "RingBuff\RingBuff.h"

using namespace Library_Jingyu;

// 구조체 전방선언
struct stAccount;
struct stAcceptUser;

// 인자로 받은 Socket 값을 기준으로 [Accept 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stAcceptUser* ClientSearch_AcceptList(SOCKET sock);

// 인자로 받은 회원No 값을 기준으로 [회원 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 회원 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stAccount* ClientSearch_AccountList(UINT64 AccountNo);

// 네트워크 프로세스
bool Network_Process(SOCKET* listen_sock);

// Accept 처리.
void Accept(SOCKET* client_sock, SOCKADDR_IN clinetaddr);

// Disconnect 처리
void Disconnect(SOCKET sock);

// 패킷 헤더 조립
void CreateHeader(CProtocolBuff* headerBuff, WORD MsgType, WORD PayloadSize);




/////////////////////////////
// 디버깅용 함수
////////////////////////////

// 현재 커텍트한 유저 수 알아오기
UINT64 AcceptCount();

// TPS체크 (현재 Send횟수로 체크중)
UINT64 TPSCount();

// 초당 Send바이트 수 (Send Per Second)
UINT64 SPSCount();

// 초당 Recv바이트 수 (Recv Per Second)
UINT64 RPSCount();


/////////////////////////////
// Json 및 파일 입출력 함수
////////////////////////////
// 제이슨에 내보낼 데이터 셋팅하는 함수(UTF-16)
bool Json_Create();

// 파일 생성 후 데이터 쓰기 함수
bool FileCreate_UTF16(const TCHAR* FileName, const TCHAR* tpJson, size_t StringSize);

// 제이슨에서 데이터 읽어와 셋팅하는 함수(UTF-16)
bool Json_Get();

// 파일에서 데이터 읽어오는 함수
// pJson에 읽어온 데이터가 저장된다.
bool LoadFile_UTF16(const TCHAR* FileName, TCHAR** pJson);




/////////////////////////
// Recv 처리 함수들
/////////////////////////
// Recv() 처리
bool RecvProc(SOCKET sock);

// 패킷 처리
bool PacketProc(WORD PacketType, SOCKET sock, char* Packet);

// "회원가입 요청(회원목록 추가)" 패킷 처리
bool Network_Req_AccountAdd(SOCKET sock, char* Packet);

// "로그인 요청" 패킷 처리
bool Network_Req_Login(SOCKET sock, char* Packet);

// "회원 목록 요청" 패킷 처리
bool Network_Req_AccountList(SOCKET sock, char* Packet);

// "친구 목록 요청" 패킷 처리
bool Network_Req_FriendList(SOCKET sock, char* Packet);

// "보낸 친구요청 목록" 패킷 처리
bool Network_Req_RequestList(SOCKET sock, char* Packet);

// "받은 친구요청 목록" 패킷 처리
bool Network_Req_ReplytList(SOCKET sock, char* Packet);

// "친구끊기 요청" 패킷 처리
bool Network_Req_FriendRemove(SOCKET sock, char* Packet);

// "친구 요청" 패킷 처리
bool Network_Req_FriendRequest(SOCKET sock, char* Packet);

// "친구요청 취소" 패킷 처리
bool Network_Req_RequestCancel(SOCKET sock, char* Packet);

// "받은요청 거부" 패킷 처리
bool Network_Req_RequestDeny(SOCKET sock, char* Packet);

// "친구요청 수락" 패킷 처리
bool Network_Req_FriendAgree(SOCKET sock, char* Packet);

// 스트레스 테스트용 패킷 처리
bool Network_Req_StressTest(SOCKET sock, char* Packet);




/////////////////////////
// 패킷 제작 함수
/////////////////////////
// "회원가입 요청(회원목록 추가)" 패킷 제작
void Network_Res_AccountAdd(char* header, char* Packet, UINT64 AccountNo);

// "로그인 요청 결과" 패킷 제작
void Network_Res_Login(char* header, char* Packet, UINT64 AccountNo, TCHAR* Nick);

// "회원 목록 요청 결과" 패킷 제작
void Network_Res_AccountList(char* header, char* Packet);

// "친구 목록 요청 결과" 패킷 제작
void Network_Res_FriendList(char* header, char* Packet, UINT64 AccountNo, UINT FriendCount);

// "보낸 친구요청 목록 결과" 패킷 처리
void Network_Res_RequestList(char* header, char* Packet, UINT64 AccountNo, UINT RequestCount);

// "받은 친구요청 목록 결과" 패킷 처리
void Network_Res_ReplytList(char* header, char* Packet, UINT64 AccountNo, UINT ReplyCount);

// "친구끊기 요청 결과" 패킷 제작
void Network_Res_FriendRemove(char* header, char* Packet, UINT64 AccountNo, BYTE Result);

// "친구 요청 결과" 패킷 제작
void Network_Res_FriendRequest(char* header, char* Packet, UINT64 AccountNo, BYTE Result);

// "친구요청 취소 결과" 패킷 제작
void Network_Res_RequestCancel(char* header, char* Packet, UINT64 AccountNo, BYTE Result);

// "보낸요청 취소 결과" 패킷 제작
void Network_Res_RequestDeny(char* header, char* Packet, UINT64 AccountNo, BYTE Result);

// "친구요청 수락 결과" 패킷 제작
void Network_Res_FriendAgree(char* header, char* Packet, UINT64 AccountNo, BYTE Result);



/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(CRingBuff* SendBuff, CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff);

// Send버퍼의 데이터를 Send하기
bool SendProc(SOCKET sock);



#endif // !__NETWORK_FUNC_H

