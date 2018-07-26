#include "AccountArray.h"
#include "BankingCommonDecl.h"

ArrayBoundCheck::ArrayBoundCheck(int len)
	:arrlen(len)
{
	arr = new ACOUNT_PTR[arrlen];
}

ACOUNT_PTR& ArrayBoundCheck::operator[] (int idx)
{
	if (idx < 0 || arrlen <= idx)
	{
		cout << "배열을 벗어남" << endl;
		exit(1);
	}

	return arr[idx];
}

ACOUNT_PTR ArrayBoundCheck::operator[] (int idx) const
{
	if (idx < 0 || arrlen <= idx)
	{
		cout << "배열을 벗어남" << endl;
		exit(1);
	}

	return arr[idx];
}

int ArrayBoundCheck::GetArrlen() const
{
	return arrlen;
}

ArrayBoundCheck::~ArrayBoundCheck()
{
	delete[] arr;
}