#ifndef  __PROTOCOL_STRUCT_H__
#define __PROTOCOL_STRUCT_H__

#include <windows.h>

////////////////////////////////////////////////////////
//
//	NetServer & ChatServer
//
////////////////////////////////////////////////////////

// 유저 접속 (OnClientJoin)
struct st_Protocol_NetChat_OnClientJoin
{
	ULONGLONG	SessionID;
	WORD		Type;
};

// 유저 종료 (OnClientLeave)
struct st_Protocol_NetChat_OnClientLeave
{
	ULONGLONG	SessionID;
	WORD		Type;
};


////////////////////////////////////////////////////////
//
//	Client & Server Protocol
//
////////////////////////////////////////////////////////

// 채팅서버 섹터 이동 요청
#pragma pack(push, 1)
struct st_Protocol_CS_CHAT_REQ_SECTOR_MOVE
{
	ULONGLONG	SessionID;
	WORD		Type;

	INT64		AccountNo;
	WORD		SectorX;
	WORD		SectorY;
};
#pragma pack(pop)


// 채팅서버 섹터 이동 결과
#pragma pack(push, 1)
struct st_Protocol_CS_CHAT_RES_SECTOR_MOVE
{
	ULONGLONG	SessionID;
	WORD		Type;

	INT64		AccountNo;
	WORD		SectorX;
	WORD		SectorY;
};
#pragma pack(pop)


// 채팅서버 채팅 보내기 요청
#define CHAT_MAX_SIZE	512
#pragma pack(push, 1)
struct st_Protocol_CS_CHAT_REQ_MESSAGE
{
	ULONGLONG	SessionID;
	WORD		Type;

	INT64		AccountNo;
	WORD		MessageLen;
	WCHAR		Message[CHAT_MAX_SIZE+2];	// null 미포함
};
#pragma pack(pop)


// 채팅서버 채팅 보내기 응답
// 현재 메모리풀에서 다루는 구조체.
#pragma pack(push, 1)
struct st_Protocol_CS_CHAT_RES_MESSAGE
{
	ULONGLONG	SessionID;
	WORD		Type;

	INT64		AccountNo;

	WCHAR		ID[20];						// null 포함
	WCHAR		Nickname[20];				// null 포함
	WORD		MessageLen;
	WCHAR		Message[CHAT_MAX_SIZE + 2];	// null 미포함
};
#pragma pack(pop)


// 하트비트
#pragma pack(push, 1)
struct st_Protocol_CS_CHAT_REQ_HEARTBEAT
{
	ULONGLONG	SessionID;
	WORD		Type;
};
#pragma pack(pop)


// 메시지 중 가장 큰 메시지 구조체. define 되어있음
#define BINGNODE	st_Protocol_CS_CHAT_RES_MESSAGE


#endif // ! __PROTOCOL_STRUCT_H__
