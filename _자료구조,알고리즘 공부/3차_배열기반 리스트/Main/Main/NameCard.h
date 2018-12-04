#ifndef __NAME_CARD_H__
#define __NAME_CARD_H__

#define NAME_LEN	30
#define PHONE_LEN	30

class NameCard
{
	char m_cName[NAME_LEN];		// 이름
	char m_cPhone[PHONE_LEN];	// 이름에 대응되는 폰 번호

public:
	// 이름, 폰번호 출력
	void ShowInfo();

	// 이름 비교
	// 같으면 0, 다르면 0이 아닌 값 리턴
	int NameCompare(const char* Name);

	// 전화번호 변경
	void ChangePhoneNum(const char* Phone);

	// 생성자
	NameCard(const char* Name, const char* Phone);
};


// NameCard 동적할당 및 이름,폰번호 셋팅
NameCard* CreateNameCard(const char* Name, const char* Phone);

#endif // !__NAME_CARD_H__
