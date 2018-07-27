#include <iostream>
#include "Parser_Class.h"

using namespace std;
using namespace Library_Jingyu;

// 몬스터 구조체
class Monster
{
public:
	char m_iName[30];
	int m_iHP;
	int m_iMP;
	int m_iAtack;
	int m_iArmor;

public:
	// 몬스터 구조체 정보 보기 함수
	void ShowMonster() const
	{
		cout << "몬스터!" << endl;
		cout << "이름 : " << m_iName << endl;
		cout << "체력 : " << m_iHP << endl;
		cout << "마력 : " << m_iMP << endl;
		cout << "공격력 : " << m_iAtack << endl;
		cout << "방어력 : " << m_iArmor << endl;
	}
}; // 몬스터 구조체

// 플레이어 구조체
class Player
{
public:
	char m_iName[30];
	int m_iHP;
	int m_iMP;
	int m_iAtack;
	int m_iArmor;

public:
	// 플레이어 구조체 정보 보기 함수
	void ShowMonster() const
	{
		cout << "플레이어!" << endl;
		cout << "이름 : " << m_iName << endl;
		cout << "체력 : " << m_iHP << endl;
		cout << "마력 : " << m_iMP << endl;
		cout << "공격력 : " << m_iAtack << endl;
		cout << "방어력 : " << m_iArmor << endl;
	}
};

int main(void)
{
	Monster mon;
	Player ply;
	Parser parser;
	bool bParserAreaCheck = false;

	// 파일 로드
	try
	{
		parser.LoadFile("Test.txt");
	}
	catch (int expn)
	{
		if (expn == 1)
		{
			cout << "파일 오픈 실패" << endl;
			exit(-1);
		}
		else if (expn == 2)
		{
			cout << "파일 읽어오기 실패" << endl;
			exit(-1);
		}
	}	

	//----------------------
	//    ply의 값 세팅 
	//----------------------
	// 구역 지정
	bParserAreaCheck = parser.AreaCheck("PLAYER");

	if (bParserAreaCheck)
	{
		// 원하는 값 찾기
		parser.GetValue_String("m_iName", ply.m_iName);
		parser.GetValue_Int("m_iHP", &ply.m_iHP);
		parser.GetValue_Int("m_iMP", &ply.m_iMP);
		parser.GetValue_Int("m_iAtack", &ply.m_iAtack);
		parser.GetValue_Int("m_iArmor", &ply.m_iArmor);
		// 확인하기
		ply.ShowMonster();
		cout << endl;
	}
	else
		cout << "플레이어 찾는 구역이 없습니다." << endl;

	//----------------------
	//    mon의 값 세팅 
	//----------------------
	
	// 구역 지정
	bParserAreaCheck = parser.AreaCheck("MONSTER");

	if (bParserAreaCheck)
	{
		// 원하는 값 찾기
		parser.GetValue_String("m_iName", mon.m_iName);
		parser.GetValue_Int("m_iHP", &mon.m_iHP);
		parser.GetValue_Int("m_iMP", &mon.m_iMP);
		parser.GetValue_Int("m_iAtack", &mon.m_iAtack);
		parser.GetValue_Int("m_iArmor", &mon.m_iArmor);
		// 확인하기
		mon.ShowMonster();
	}
	else
		cout << "몬스터 찾는 구역이 없습니다." << endl; 

	return 0;
}