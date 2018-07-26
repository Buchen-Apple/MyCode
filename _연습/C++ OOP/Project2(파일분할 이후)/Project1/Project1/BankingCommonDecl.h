/*
* 업데이트 정보 : [2018년 1월 13일] 버전 0.7
*/

#pragma once
#ifndef __BANKING_SYSTEM_MAIN_H__
#define __BANKING_SYSTEM_MAIN_H__
#define ACCOUNT_COUNT 50

#include <iostream>
#include "conio.h"
#include <cstring>
#include <windows.h>

using namespace std;

enum
{
	CREATE = 1, INSERT, OUT_MONEY, ALL_SHOW, EXIT
};

enum CREDIT
{
	A = 7, B = 4, C = 2
};


#endif // !__BANKING_SYSTEM_MAIN_H__
