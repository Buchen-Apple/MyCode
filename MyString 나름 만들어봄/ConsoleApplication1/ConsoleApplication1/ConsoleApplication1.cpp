// ConsoleApplication1.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <windows.h>

#include <iostream>
#include "MyStringClass.h"

int _tmain()
{	
	MyString Test1 = "SongJin";
	MyString Test2 = "Gyu";
	MyString Test3 = Test1 + Test2;
	Test1 += Test2;

	cout << Test1 << endl << Test2 << endl << Test3 << endl;
	
	wcout << "abcd" << endl;
	
    return 0;
}




