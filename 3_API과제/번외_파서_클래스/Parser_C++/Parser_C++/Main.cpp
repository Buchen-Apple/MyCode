#include <iostream>
#include "Parser_Class.h"
#include <locale.h >

using namespace std;

// 몬스터 구조체
class Monster
{
public:
	TCHAR m_iName[30];
	int m_iHP;
	int m_iMP;
	int m_iAtack;
	int m_iArmor;

public:
	// 몬스터 구조체 정보 보기 함수
	void ShowMonster() const
	{
		_tprintf(_T("몬스터\n"));
		_tprintf(_T("이름 : %s \n"), m_iName);
		_tprintf(_T("체력 : %d \n"), m_iHP);
		_tprintf(_T("마력 : %d \n"), m_iMP);
		_tprintf(_T("공격력 : %d \n"), m_iAtack);
		_tprintf(_T("방어력 : %d \n\n"), m_iArmor);
	}
};

// 플레이어 구조체
class Player
{
public:
	TCHAR m_iName[30];
	int m_iHP;
	int m_iMP;
	int m_iAtack;
	int m_iArmor;

public:
	// 플레이어 구조체 정보 보기 함수
	void ShowMonster() const
	{	
		_tprintf(_T("플레이어\n"));
		_tprintf(_T("이름 : %s \n"),m_iName);
		_tprintf(_T("체력 : %d \n"), m_iHP);
		_tprintf(_T("마력 : %d \n"), m_iMP);
		_tprintf(_T("공격력 : %d \n"), m_iAtack);
		_tprintf(_T("방어력 : %d \n\n"), m_iArmor);		
	}
};

int main(void)
{
	_tsetlocale(LC_ALL, L"korean");

	Monster mon;
	Player ply;
	Parser parser;
	bool bParserAreaCheck = false;

	//----------------------
	//   파일 로드
	//----------------------
	try
	{
		parser.LoadFile(_T("Test_UNICODE.txt"));
	}
	catch (int expn)
	{
		if (expn == 1)
		{
			_tprintf(_T("파일 오픈 실패\n"));
			exit(-1);
		}
		else if (expn == 2)
		{
			_tprintf(_T("파일 읽어오기 실패\n"));
			exit(-1);
		}
	}
	

	//----------------------
	//    ply의 값 세팅 
	//----------------------
	// 구역 지정
	bParserAreaCheck = parser.AreaCheck(_T("PLAYER"));

	// 구역이 정상적으로 지정되었으면 아래 로직
	if (bParserAreaCheck)
	{
		// 원하는 값 찾기
		parser.GetValue_String(_T("m_iName"), ply.m_iName);
		parser.GetValue_Int(_T("m_iHP"), &ply.m_iHP);
		parser.GetValue_Int(_T("m_iMP"), &ply.m_iMP);
		parser.GetValue_Int(_T("m_iAtack"), &ply.m_iAtack);
		parser.GetValue_Int(_T("m_iArmor"), &ply.m_iArmor);
		// 확인하기
		ply.ShowMonster();
	}

	// 구역 지정 실패 시 아래 로직
	else
		_tprintf(_T("플레이어 구역 찾기 실패\n"));


	////----------------------
	////    mon의 값 세팅 
	////----------------------
	// 구역 지정
	bParserAreaCheck = parser.AreaCheck(_T("MONSTER"));

	// 구역이 정상적으로 지정되었으면 아래 로직
	if (bParserAreaCheck)
	{
		// 원하는 값 찾기
		parser.GetValue_String(_T("m_iName"), mon.m_iName);
		parser.GetValue_Int(_T("m_iHP"), &mon.m_iHP);
		parser.GetValue_Int(_T("m_iMP"), &mon.m_iMP);
		parser.GetValue_Int(_T("m_iAtack"), &mon.m_iAtack);
		parser.GetValue_Int(_T("m_iArmor"), &mon.m_iArmor);
		// 확인하기
		mon.ShowMonster();
	}

	// 구역 지정 실패 시 아래 로직
	else
		_tprintf(_T("몬스터 구역 찾기 실패\n"));


	return 0;
}