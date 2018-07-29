#include "stdafx.h"
#include "MyStringClass.h"


// ---------------
// 생성자 파트
// ---------------
// 값 입력받는 생성자
MyString::MyString(const char* str)
{
	m_len = strlen(str);

	m_String = new char[m_len + 1];

	strcpy_s(m_String, m_len + 1, str);
}

// 복사 생성자 (깊은 복사)
MyString::MyString(const MyString& copy)
{
	// 새로운 문자열로 동적할당.
	m_len = strlen(copy.m_String);

	m_String = new char[m_len + 1];

	strcpy_s(m_String, m_len + 1, copy.m_String);

}

// ---------------
// 연산자 오버로딩 파트
// ---------------
// 대입 연산자 (깊은 복사)
MyString& MyString::operator=(const MyString& copy)
{
	// 기존에 문자열이 있는 경우, 할당 해제
	if (m_String != nullptr)
		delete[] m_String;

	// 새로운 문자열로 동적할당.
	m_len = copy.m_len;

	m_String = new char[m_len + 1];

	strcpy_s(m_String, m_len + 1, copy.m_String);

	return *this;
}

// + 연산자 오버로딩.
// A+B 문자열의 결과를 객체로 반환
MyString MyString::operator+(const MyString& str)
{
	size_t Templen = m_len + strlen(str.m_String);

	char* Temp = new char[Templen + 1];

	strcpy_s(Temp, m_len + 1, m_String);
	strcat_s(Temp, Templen + 1, str.m_String);

	MyString retobj = Temp;

	delete[] Temp;

	return retobj;
}

// += 연산자 오버로딩
// A 문자열에 B를 붙인다.
MyString& MyString::operator+=(const MyString& ref)
{
	size_t Templen = m_len + strlen(ref.m_String);

	char* Temp = new char[Templen + 1];

	strcpy_s(Temp, m_len + 1, m_String);
	strcat_s(Temp, Templen + 1, ref.m_String);

	delete[] m_String;


	m_len = Templen;
	m_String = new char[m_len + 1];
	strcpy_s(m_String, m_len + 1, Temp);

	delete[] Temp;

	return *this;
}

// == 연산자 오버로딩
// A와 B 비교
// 같으면 true, 다르면 false 반환
bool MyString::operator==(const MyString& ref)
{
	if (strcmp(m_String, ref.m_String) == 0)
		return true;

	else
		return false;
}

// 소멸자
MyString::~MyString()
{
	delete[] m_String;
}



// << 연산자 오버로딩
// 화면 출력용 
ostream& operator<<(ostream& os, const MyString& ref)
{
	os << ref.m_String;

	return os;
}

// >> 연산자 오버로딩
// 입력받기 용 
istream& operator<<(istream& is, const MyString& ref)
{
	is >> ref.m_String;

	return is;
}