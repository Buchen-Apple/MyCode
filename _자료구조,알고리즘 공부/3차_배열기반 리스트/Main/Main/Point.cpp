#include "pch.h"
#include "Point.h"

using namespace std;

// X,Y 값 설정
//
// Parameter : 셋팅할 x, y 값
void Point::SetPointPos(int x, int y)
{
	m_iXpos = x;
	m_iYpos = y;
}

// X,Y 값 출력
void Point::ShowPointPos()
{
	cout << "[" << m_iXpos << "," << m_iYpos << "]" << endl;
}

// 두 Point변수 비교
// 
// return 0 : 둘이 x,y가 완전히 같음
// return 1 : x만 같음
// return 2 : y만 같음
// return -1 : 모두 다름
int Point::Comp(Point* Comp)
{
	if (m_iXpos == Comp->m_iXpos &&
		m_iYpos == Comp->m_iYpos)
	{
		return 0;
	}

	else if (m_iXpos == Comp->m_iXpos)
	{
		return 1;
	}

	else if (m_iYpos == Comp->m_iYpos)
	{
		return 1;
	}

	else
	{
		return -1;
	}
}