#include <iostream>

#include "My_New.h"

using namespace std;

int main()
{
	// NOALLOC을 위한 변수
	int *test;
	char *test2;

	// 동적할당	
	char *c1 = new char;
	short *s1 = new short[4];
	int *p1 = new int;
	long *l1 = new long;
	long long *dl1 = new long long[4];
	float *f1 = new float;
	double *d1 = new double[4];

	// 딜리트
	delete test;	// NOALLOC 발생
	//delete c1;	// LEAK 발생 (LEAK을 위해 잠시 주석처리)
	delete[] s1;
	delete[] p1;	// ARRAY 발생 (본래 일반 delete로 해제해야 하나, ARRAY를 보기 위해 배열 delete로 해제)
	delete test2;	// NOALLOC 발생
	delete l1;
	delete dl1;		// ARRAY 발생 (본래 배열 delete로 해제해야 하나, ARRAY를 보기 위해 일반 delete로 해제)
	delete f1;
	delete[] d1;

	return 0;
}

