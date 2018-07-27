#include <iostream>
#include "CList_Template.h"

// 테스트 클래스
class Test
{
	int num;
	char str[10];

public:
	Test(int num, const char* str)
		:num(num)
	{
		strcpy_s(this->str, sizeof(str), str);
	}
	friend ostream& operator<< (ostream& os, const Test& ref);
};

ostream& operator<< (ostream& os, const Test& ref)
{
	os << ref.num << " " << ref.str;
	return os;
}

// 리스트의 정보를 보여주는 함수
template <class T>
void ShowList(CList<Test*>, CList<Test*>::iterator);

int main()
{

	CList<Test*> list;

	CList<Test*>::iterator itor;

	Test ts(1, "송진규");
	Test ts2(2, "김범규");
	Test ts3(3, "변영준");

	list.push_front(&ts);
	list.push_back(&ts2);
	list.push_back(&ts3);

	ShowList<Test*>(list, itor);	// 리스트 출력
	itor = list.begin();

	if (list.is_empty(itor))
		cout << "비어있다" << endl;
	else
		cout << "비어있지 않다." << endl;

	
	// 노드 삭제.	
	try
	{
		itor = list.erase(itor);
	}
	catch (int expn)
	{
		if(expn == 1)
			cout << endl <<  "헤드 더미 or 꼬리 더미는 삭제 불가" << endl;
	}
	
	// 다시 >> 위치로 이동시켜 위에서 한 노드삭제 예외를 원복
	itor++;	
	cout << endl;
	ShowList<Test*>(list, itor);	// 리스트 출력

	// 리스트 클리어
	list.clear();
	cout << endl;
	ShowList<Test*>(list, itor);	// 리스트 출력


	return 0;
}

template <class T>
void ShowList(CList<Test*> list, CList<Test*>::iterator itor) 
{
	cout << "CList 사이즈: " << list.size() << endl;

	for (itor = list.begin(); itor != list.end();)
	{
		cout << *(*itor) << endl;
		itor++;
	}
}

