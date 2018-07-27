#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>
#include "Protocol.h"
#include "ProtocolBuff\ProtocolBuff.h"
#include "RingBuff\RingBuff.h"

using namespace Library_Jingyu;

// 윈속 초기화, 소켓 연결, 커넥트 등 함수
bool Network_Init();

// 메뉴에 따른 액션 처리
bool PacketProc(int SelectNum);

// 패킷 헤더 조립
void CreateHeader(CProtocolBuff* headerBuff, WORD MsgType, WORD PayloadSize);

// 내 로그인정보 출력하는 함수
void LoginShow();




//////////////////////////
// 패킷 제작 함수들
/////////////////////////
// "회원 추가" 패킷 제작
bool Network_Res_AccountAdd();

// "로그인" 패킷 제작
bool Network_Res_Login();

// "회원목록 요청" 패킷 제작
bool Network_Res_AccountList();

// "친구목록 요청" 패킷 제작
bool Network_Res_FriendList();

// "받은 친구요청 보기" 패킷 제작
bool Network_Res_ReplyList();

// "보낸 친구요청 보기" 패킷 제작
bool Network_Res_RequestList();

// "친구요청 보내기" 패킷 제작
bool Network_Res_FriendRequest();

// "친구요청 취소" 패킷 제작
bool Network_Res_FriendCancel();

// "받은요청 수락" 패킷 제작
bool Network_Res_FriendAgree();

// "받은요청 거절" 패킷 제작
bool Network_Res_FriendDeny();

// "친구 끊기" 패킷 제작
bool Network_Res_FriendRemove();







//////////////////////////
// 받은 패킷 처리 함수들
/////////////////////////
// "회원 추가 결과" 패킷 처리
void Network_Req_AccountAdd(CProtocolBuff* payload);

// "로그인 결과" 패킷 처리
void Network_Req_Login(CProtocolBuff* payload);

// "회원목록 요청 결과" 패킷 처리
void Network_Req_AccountList(CProtocolBuff* payload);

// "친구목록 요청 결과" 패킷 처리
void Network_Req_FriendList(CProtocolBuff* payload);

// "받은 친구요청 보기 결과" 패킷 처리
void Network_Req_ReplyList(CProtocolBuff* payload);

// "보낸 친구요청 보기 결과" 패킷 처리
void Network_Req_RequestList(CProtocolBuff* payload);

// "친구요청 보내기 결과" 패킷 처리
void Network_Req_FriendRequest(CProtocolBuff* payload);

// "친구요청 취소 결과" 패킷 처리
void Network_Req_FriendCancel(CProtocolBuff* payload);

// "받은요청 수락 결과" 패킷 처리
void Network_Req_FriendAgree(CProtocolBuff* payload);

// "받은요청 거절 결과" 패킷 처리
void Network_Req_FriendDeny(CProtocolBuff* payload);

// "친구끊기 결과" 패킷 처리
void Network_Req_FriendRemove(CProtocolBuff* payload);









/////////////////////////
// recv 처리
/////////////////////////
// 리시브 처리
bool RecvProc(CProtocolBuff* RecvBuff, WORD MsgType);





/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff);

// Send버퍼의 데이터를 Send하기
bool SendProc();



#endif // !__NETWORK_FUNC_H__
