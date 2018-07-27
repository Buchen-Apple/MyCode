#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

// 테스트용으로 소켓 사이즈를 3으로 함.
#define FD_SETSIZE      3

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>
#include "Protocol.h"
#include "ProtocolBuff\ProtocolBuff.h"

using namespace Library_Jingyu;

// 구조체 전방선언
struct stClinet;
struct stRoom;

// 인자로 받은 UserID를 기준으로 클라이언트 목록에서 유저를 골라낸다.(검색)
// 성공 시, 해당 유저의 세션 구조체의 주소를 리턴 
// 실패 시 nullptr 리턴
stClinet* ClientSearch(DWORD UserID);

// 인자로 받은 RoomID를 기준으로 방 목록에서 방을 골라낸다.(검색)
// 성공 시, 해당 방의 세션 구조체의 주소를 리턴 
// 실패 시 nullptr 리턴
stRoom* RoomSearch(DWORD RoomID);

// 네트워크 프로세스
bool Network_Process(SOCKET* listen_sock);

// Accept 처리.
void Accept(SOCKET* client_sock, SOCKADDR_IN clinetaddr);

// Disconnect 처리
void Disconnect(DWORD UserID);

// 서버에 접속한 모든 유저의 SendBuff에 패킷 넣기
void BroadCast_All(CProtocolBuff* header, CProtocolBuff* Packet);
//void BroadCast_All(char* header, char* Packet);

// 같은 방에 있는 모든 유저의, SendBuff에 패킷 넣기
// UserID에 -1이 들어오면 방 안의 모든 유저 대상.
// UserID에 다른 값이 들어오면, 해당 ID의 유저는 제외.
void BroadCast_Room(CProtocolBuff* header, CProtocolBuff* Packet, DWORD RoomID, int UserID);
//void BroadCast_Room(char* header, char* Packet, DWORD RoomID, int UserID);


/////////////////////////
// Recv 처리 함수들
/////////////////////////
// Recv() 처리
bool RecvProc(DWORD UserID);

// 패킷 처리
bool PacketProc(WORD PacketType, DWORD UserID, char* Packet);

// "로그인 요청" 패킷 처리
bool Network_Req_Login(DWORD UserID, char* Packet);

// "방목록 요청" 패킷 처리
bool Network_Req_RoomList(DWORD UserID, char* Packet);

// "대화방 생성 요청" 패킷 처리
bool Network_Req_RoomCreate(DWORD UserID, char* Packet);

// "방입장 요청" 패킷 처리
bool Network_Req_RoomJoin(DWORD UserID, char* Packet);

// "채팅 요청" 패킷 처리
bool Network_Req_Chat(DWORD UserID, char* Packet);

// "방 퇴장 요청" 패킷 처리
bool Network_Req_RoomLeave(DWORD UserID, char* Packet);

// "방 삭제 결과"를 만들고 보낸다 
// 방에 사람이 있다가, 0명이 되는 순간 발생한다.
bool Network_Req_RoomDelete(DWORD RoomID);

// "타 사용자 입장" 패킷을 만들고 보낸다.
// 어떤 유저가 방에 접속 시, 해당 방에 있는 다른 유저에게 다른 사용자가 접속했다고 알려주는 패킷 (서->클 만 가능)
bool Network_Req_UserRoomJoin(DWORD JoinUserID);




/////////////////////////
// 패킷 제작 함수
/////////////////////////
// "로그인 요청 결과" 패킷 제작
void CreatePacket_Res_Login(char* header, char* Packet, int UserID, char result);

// "방 목록 요청 결과" 패킷 제작
void CreatePacket_Res_RoomList(char* header, char* Packet);

// "대화방 생성 요청 결과" 패킷 제작
void CreatePacket_Res_RoomCreate(char* header, char* Packet, char result, WORD RoomSize, char* cRoomName);

// "방입장 요청 결과" 패킷 제작
void CreatePacket_Res_RoomJoin(char* header, char* Packet, char result, DWORD RoomID);

// "채팅 요청 결과" 패킷 제작
void CreatePacket_Res_RoomJoin(char* header, char* Packet, WORD MessageSize, char* cMessage, DWORD UserID);

// "방 퇴장 요청 결과" 패킷 제작
void CreatePacket_Res_RoomLeave(char* header, char* Packet, DWORD UserID, DWORD RoomID);

// "방 삭제 결과" 패킷 제작
void CreatePacket_Res_RoomDelete(char* header, char* Packet, DWORD RoomID);

// "타 사용자 입장" 패킷 제작
void CreatePacket_Res_UserRoomJoin(char* header, char* Packet, char* cNickName, DWORD UserID);






/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(DWORD UserID, CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff);

// Send버퍼의 데이터를 Send하기
bool SendProc(DWORD UserID);


#endif // !__NETWORK_FUNC_H__
