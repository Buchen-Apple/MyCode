#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#include "Protocol.h"
#include "ProtocolBuff\ProtocolBuff.h"

using namespace Library_Jingyu;

#define WM_SOCK			WM_USER+1
#define ROOMNAME_SIZE	30
#define CHAT_SIZE		30

// 각종 정보 전달받기. (로비 다이얼로그 핸들(메인), 인스턴스 등..)
void InfoGet(HWND g_LobbyhWnd, HWND* g_RoomhWnd, HINSTANCE hInst, int cmdShow);

// 윈속 초기화, 소켓 연결, 커넥트 등 함수
BOOL NetworkInit(TCHAR tIP[30], TCHAR tNickName[dfNICK_MAX_LEN]);

// 윈속 해제, 소켓 해제 등 함수
void NetworkClose();

// 네트워크 처리 함수
BOOL NetworkProc(LPARAM);

// 헤더 조립하기
void CreateHeader(CProtocolBuff* header, CProtocolBuff* payload, WORD Type);

// 내 채팅을 만든 후 보내기. 그리고 내 에디터에 출력하기까지 하는 함수
void ChatLogic();



/////////////////////////
// 패킷 제작 함수
/////////////////////////
// "로그인 요청" 패킷 제작
void CreatePacket_Req_Login(char* header, char* Packet);

// "대화방 목록 요청" 패킷 제작
void CreatePacket_Req_RoomList(char* header);

// "방 생성 요청" 패킷 제작
void CreatePacket_Req_RoomCreate(char* header, char* Packet, TCHAR RoomName[ROOMNAME_SIZE]);

// "방 입장 요청" 패킷 제작
void CreatePacket_Req_RoomJoin(char* header, char* Packet, DWORD RoomNo);

// "채팅 송신" 패킷 제작
void CreatePacket_Req_ChatSend(char* header, char* Packet, DWORD MessageSize, TCHAR Chat[CHAT_SIZE]);

// "방 퇴장 요청" 패킷 제작
void CreatePacket_Req_RoomLeave(char* header);



/////////////////////////
// Send 처리
/////////////////////////
// 내 SendBuff에 데이터 넣기
BOOL SendPacket(CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff);

// SendBuff의 데이터를 Send하기
BOOL SendProc();


/////////////////////////
// Recv 처리 함수들
/////////////////////////
//  Recv() 처리 함수
BOOL RecvProc();

// 패킷 타입에 따른 패킷처리 함수
BOOL PacketProc(WORD PacketType, char* Packet);

// "로그인 요청 결과" 패킷 처리
BOOL Network_Res_Login(char* Packet);

// "대화방 목록 요청 결과" 패킷 처리
BOOL Network_Res_RoomList(char* Packet);

// "방 생성 요청 결과" 패킷 처리 (내가 안보낸것도 옴)
BOOL Network_Res_RoomCreate(char* Packet);

// "방 입장 요청 결과" 패킷 처리
BOOL Network_Res_RoomJoin(char* Packet);

// "채팅 결과" 패킷 처리 (내가 보낸건 안옴. 나와 같은 방에 있는 다른사람의 채팅 처리하기)
BOOL Network_Res_ChatRecv(char* Packet);

// "타 사용자 입장 결과" 패킷 처리 (내가 안보낸것도 옴)
BOOL Network_Res_UserEnter(char* Packet);

// "방 퇴장 요청 결과" 패킷 처리 (내가 안보낸것도 옴)
BOOL Network_Res_RoomLeave(char* Packet);

// "방 삭제 결과" 패킷 처리
BOOL Network_Res_RoomDelete(char* Packet);






#endif // !__NETWORK_FUNC_H__
