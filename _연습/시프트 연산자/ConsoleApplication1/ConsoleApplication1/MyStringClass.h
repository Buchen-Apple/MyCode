#ifndef __MY_STRING_CLASS_H__
#define __MY_STRING_CLASS_H__


#include <iostream>

using namespace std;

class MyString
{
	char* m_String = nullptr;

	// 널문자 제외 길이
	size_t m_len;

public:

	// ---------------
	// 생성자 파트
	// ---------------
	// 보이드 생성자
	MyString()
		:m_len(0), m_String(nullptr)
	{}

	// 값 입력받는 생성자
	MyString(const char* str);

	// 복사 생성자 (깊은 복사)
	MyString(const MyString& copy);

	// ---------------
	// 연산자 오버로딩 파트
	// ---------------
	// 대입 연산자 (깊은 복사)
	MyString& operator=(const MyString& copy);

	// + 연산자 오버로딩.
	// A+B 문자열의 결과를 객체로 반환
	MyString operator+(const MyString& str);

	// += 연산자 오버로딩
	// A 문자열에 B를 붙인다.
	MyString& operator+=(const MyString& ref);

	// == 연산자 오버로딩
	// A와 B 비교
	// 같으면 true, 다르면 false 반환
	bool operator==(const MyString& ref);

	// << 연산자 오버로딩
	// 화면 출력용 
	friend ostream& operator<<(ostream& os, const MyString& ref);

	// >> 연산자 오버로딩
	// 입력받기 용 
	friend istream& operator<<(istream& os, const MyString& ref);


	~MyString();
};


// << 연산자 오버로딩
// 화면 출력용 
ostream& operator<<(ostream& os, const MyString& ref);

// >> 연산자 오버로딩
// 입력받기 용 
istream& operator<<(istream& is, const MyString& ref);


#endif // !__MY_STRING_CLASS_H__
