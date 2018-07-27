#ifndef __DEFINE__
#define __DEFINE__


struct st_SESSION
{
	int SessionID;
};


struct st_PLAYER
{
	int SessionID = -1;	// -1이면 빈 공간이다.
	int Content[3];

	bool UseFlag = false;	// 사용중 여부. true면 사용 중. false면 사용 중 아님
};


#define dfTHREAD_NUM	3
#endif