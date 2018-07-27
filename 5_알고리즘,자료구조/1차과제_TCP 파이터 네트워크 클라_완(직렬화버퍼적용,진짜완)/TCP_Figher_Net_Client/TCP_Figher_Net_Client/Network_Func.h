#pragma once
#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#include "Network_Protocol.h"
#include "List_Template.h"
#pragma comment(lib, "ws2_32")
#include <WS2tcpip.h>
#pragma warning(disable:4996)

// 텍스트 아웃으로 에러 찍는 함수
void ErrorTextOut(const TCHAR*);

// 윈속 초기화, 소켓 연결, 커넥트 등 함수
BOOL NetworkInit(HWND hWNd, TCHAR tIP[30]);

// 윈속 해제, 소켓 해제 등 함수
void NetworkClose();

// 네트워크 처리 함수
BOOL NetworkProc(LPARAM);

// 네트워크 처리 중, Recv 처리 함수
BOOL RecvProc();

// 실제 Send 함수
BOOL SendProc();

// 패킷 타입에 따른 처리 함수
BOOL PacketProc(BYTE PacketType, char* Packet);



/////////////////////////
// Send 패킷 만들기 함수
/////////////////////////
// 이동 시작 패킷 만들기
void SendProc_MoveStart(char* header, char* packet, int dir, int x, int y);

// 정지 패킷 만들기
void SendProc_MoveStop(char* header, char* packet, int dir, int x, int y);

// 공격 패킷 만들기 (1번 공격)
void SendProc_Atk_01(char* header, char* packet, int dir, int x, int y);

// 공격 패킷 만들기 (2번 공격)
void SendProc_Atk_02(char* header, char* packet, int dir, int x, int y);

// 공격 패킷 만들기 (3번 공격)
void SendProc_Atk_03(char* header, char* packet, int dir, int x, int y);





/////////////////////////
// Send 큐에 데이터 넣기 함수
/////////////////////////
BOOL SendPacket(char* header, char* packet);



/////////////////////////
// Recv 데이터 처리 함수
/////////////////////////
// 내 캐릭터 생성 패킷 처리 함수
BOOL PacketProc_CharacterCreate_My(char* Packet);

// 다른사람 캐릭터 생성 패킷 처리 함수
BOOL PacketProc_CharacterCreate_Other(char* Packet);

// 캐릭터 삭제 패킷 처리 함수
BOOL PacketProc_CharacterDelete(char* Packet);

// 다른 캐릭터 이동 시작 패킷
BOOL PacketProc_MoveStart(char* Packet);

// 다른 캐릭터 이동 중지 패킷
BOOL PacketProc_MoveStop(char* Packet);

// 다른 캐릭터 공격 1번 패킷
BOOL PacketProc_Atk_01(char* Packet);

// 다른 캐릭터 공격 2번 패킷
BOOL PacketProc_Atk_02(char* Packet);

// 다른 캐릭터 공격 3번 패킷
BOOL PacketProc_Atk_03(char* Packet);

// 데미지 패킷
BOOL PacketProc_Damage(char* Packet);

#endif // !__NETWORK_FUNC_H__

