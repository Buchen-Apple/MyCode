#ifndef __BRESENHAM_LINE_H__
#define __BRESENHAM_LINE_H__

class CBresenhamLine
{
private:
	int m_iStartX, m_iStartY, m_iFinX, m_iFinY;
	int m_iNowX, m_iNowY;

	int m_iError;

	int m_iAddX, m_iAddY;
	int m_iWidth, m_iHeight;	

public:
	// 생성자
	CBresenhamLine();

	// 초기화
	void Init(int StartX, int StartY, int FinX, int FinY);

	// 소멸자
	~CBresenhamLine();

	// 다음 위치 얻어오기
	bool GetNext(POINT* returnP);
	
};

#endif // !__BRESENHAM_LINE_H__
