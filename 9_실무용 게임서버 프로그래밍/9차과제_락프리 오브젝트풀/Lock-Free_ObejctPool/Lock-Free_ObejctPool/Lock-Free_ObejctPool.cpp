// Lock-Free_ObejctPool.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "ObjectPool\Object_Pool_LockFreeVersion.h"


using namespace Library_Jingyu;

class Test
{
public:
	ULONGLONG Addr = 0x0000000055555555;
	int Count = 0;
};

CMemoryPool<Test> g_MPool(1000, false);

int _tmain()
{
	Test* Test = g_MPool.Alloc();



    return 0;
}

