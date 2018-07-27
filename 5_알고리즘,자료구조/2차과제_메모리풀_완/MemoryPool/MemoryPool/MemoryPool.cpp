#include <stdio.h>
#include "MemoryPool_Class.h"

class CTest
{
public:

	CTest()
	{
		a = 10;
		b = 10;
		printf("持失切!\n");
	}
	void show()
	{
		printf("%d %d\n", a, b);
	}
	~CTest()
	{
		printf("社瑚切!\n");
	}
	virtual void Set(int a, int b)
	{
		this->a = a;
		this->b = b;
	}

private:
	int a;
	int b;
};

int main()
{

	CMemoryPool<CTest> objpool(0, false);

	CTest* p1 = objpool.Alloc();
	CTest* p2 = objpool.Alloc();

	objpool.Free(p1);
	objpool.Free(p2);	

	return 0;
}