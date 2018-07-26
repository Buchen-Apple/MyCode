#pragma once
#ifndef __TEST_PROTOCOL_H__
#define __TEST_PROTOCOL_H__

#include  <windows.h>
//--------------------------------------------------------------------------------------------
// DB 저장 메시지의 헤더
//--------------------------------------------------------------------------------------------

struct st_DBQUERY_HEADER
{
	WORD	Type;
};




//--------------------------------------------------------------------------------------------
//  DB 저장 메시지 - 회원가입
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_NEW_ACCOUNT		0

struct st_DBQUERY_MSG_NEW_ACCOUNT
{
	WORD	Type;	
	char	szID[20];
	char	szPassword[20];
	__int64	iAccountNo;
};




//--------------------------------------------------------------------------------------------
//  DB 저장 메시지 - 스테이지 클리어
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_STAGE_CLEAR		1


struct st_DBQUERY_MSG_STAGE_CLEAR
{
	WORD	Type;	
	int	iStageID;
	__int64	iAccountNo;
};




//--------------------------------------------------------------------------------------------
//  DB 저장 메시지 - Player
//--------------------------------------------------------------------------------------------
#define df_DBQUERY_MSG_PLAYER		2

struct st_DBQUERY_MSG_PLAYER
{
	WORD	Type;	
	int	iLevel;
	__int64	iAccountNo;
	__int64 iExp;
};


#endif // !__TEST_PROTOCOL_H__

