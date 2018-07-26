#pragma once
#ifndef __ACCOUNT_ARRAY_H__
#define __ACCOUNT_ARRAY_H__

#include "Account.h"

typedef Account* ACOUNT_PTR;

class ArrayBoundCheck
{
	ACOUNT_PTR *arr;
	int arrlen;
	ArrayBoundCheck(const ArrayBoundCheck& arr) {};
	ArrayBoundCheck& operator=(const ArrayBoundCheck& arr) {};

public:
	ArrayBoundCheck(int len = 100);

	ACOUNT_PTR& operator[] (int idx);
	ACOUNT_PTR operator[] (int idx) const;
	int GetArrlen() const;

	~ArrayBoundCheck();
};


#endif // !__ACCOUNT_ARRAY_H__

