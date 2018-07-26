#pragma once
#ifndef __TEST_PROTOCOL_H__
#define __TEST_PROTOCOL_H__

#include  <windows.h>
//--------------------------------------------------------------------------------------------
// DB 저장 메시지의 헤더
//--------------------------------------------------------------------------------------------
#pragma pack(push,1)
struct st_DBQUERY_HEADER
{
	WORD	Type;
	WORD	Size;
};
#pragma pack(pop)
#define df_HEADER_SIZE	4



//--------------------------------------------------------------------------------------------
//  DB 저장 메시지 - 회원가입
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_NEW_ACCOUNT		0

#pragma pack(push,1)
struct st_DBQUERY_MSG_NEW_ACCOUNT
{
	__int64	iAccountNo;
	char	szID[20];
	char	szPassword[20];
};
#pragma pack(pop)



//--------------------------------------------------------------------------------------------
//  DB 저장 메시지 - 스테이지 클리어
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_STAGE_CLEAR		1

#pragma pack(push,1)
struct st_DBQUERY_MSG_STAGE_CLEAR
{
	__int64	iAccountNo;
	int	iStageID;
};
#pragma pack(pop)



//--------------------------------------------------------------------------------------------
//  DB 저장 메시지 - Player
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_PLAYER		2

#pragma pack(push,1)
struct st_DBQUERY_MSG_PLAYER
{
	__int64	iAccountNo;
	int	iLevel;
	__int64 iExp;
};
#pragma pack(pop)


#endif // !__TEST_PROTOCOL_H__

