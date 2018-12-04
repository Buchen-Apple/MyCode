#include "pch.h"
#include "NameCard.h"

using namespace std;

// 이름, 폰번호 출력
void NameCard::ShowInfo()
{
	cout << "[" << m_cName << " : " << m_cPhone << "]" << endl;
	
}

// 이름 비교
// 같으면 0, 다르면 0이 아닌 값 리턴
int NameCard::NameCompare(const char* Name)
{
	if (strcmp(m_cName, Name) == 0)
		return 0;

	return 1;
}

// 전화번호 변경
void NameCard::ChangePhoneNum(const char* Phone)
{
	strcpy_s(m_cPhone, Phone);
}

// 생성자
NameCard::NameCard(const char* Name, const char* Phone)
{
	strcpy_s(m_cName, Name);
	strcpy_s(m_cPhone, Phone);
}



// NameCard 동적할당 및 이름,폰번호 셋팅
NameCard* CreateNameCard(const char* Name, const char* Phone)
{
	return new NameCard(Name, Phone);
}