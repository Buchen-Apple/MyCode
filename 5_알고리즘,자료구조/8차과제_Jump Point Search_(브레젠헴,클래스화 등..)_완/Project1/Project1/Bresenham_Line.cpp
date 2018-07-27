#include "stdafx.h"
#include "Bresenham_Line.h"

// 생성자
CBresenhamLine::CBresenhamLine()
{	
	// 하는거 없음
}

// 초기화
void CBresenhamLine::Init(int StartX, int StartY, int FinX, int FinY)
{
	m_iStartX = StartX;
	m_iStartY = StartY;

	m_iFinX = FinX;
	m_iFinY = FinY;

	m_iNowX = StartX;
	m_iNowY = StartY;

	m_iError = 0;

	m_iWidth = FinX - StartX;
	m_iHeight = FinY - StartY;

	// AddX, AddY 셋팅
	if (m_iWidth < 0)
	{
		m_iWidth = -m_iWidth;
		m_iAddX = -1;
	}
	else
		m_iAddX = 1;

	if (m_iHeight < 0)
	{
		m_iHeight = -m_iHeight;
		m_iAddY = -1;
	}
	else
		m_iAddY = 1;
}

// 소멸자
CBresenhamLine::~CBresenhamLine()
{}

// 다음 위치 얻어오기
bool CBresenhamLine::GetNext(POINT* returnP)
{
	// 도착점에 도착하면 더 이상 진행 불가.
	if (m_iNowX == m_iFinX && m_iNowY == m_iFinY)
		return false;

	// 넓이가 높이보다, 크거나 같을 경우
	if (m_iWidth >= m_iHeight)
	{
		m_iNowX += m_iAddX;
		m_iError += m_iHeight * 2;

		if (m_iError >= m_iWidth)
		{
			m_iNowY += m_iAddY;
			m_iError -= m_iWidth * 2;
		}
	}

	// 높이가 넓이보다, 클 경우
	else
	{
		m_iNowY += m_iAddY;
		m_iError += m_iWidth * 2;

		if (m_iError >= m_iHeight)
		{
			m_iNowX += m_iAddX;
			m_iError -= m_iHeight * 2;
		}		
	}

	returnP->x = m_iNowX;
	returnP->y = m_iNowY;

	return true;
}