#ifndef __Header_h__
#define __Header_h__

#define NICK_MAX_LEN 30
#include <windows.h>

struct stUser
{
	// "AccountNo"
	UINT64 m_AccountID;
	// "NickName"
	TCHAR m_NickName[NICK_MAX_LEN];
};

#endif // !__Header_h__
