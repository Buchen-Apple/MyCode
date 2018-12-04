#ifndef __POINT_H__
#define __POINT_H__

class Point
{
	int m_iXpos;
	int m_iYpos;

public:
	// X,Y 값 설정
	//
	// Parameter : 셋팅할 x, y 값
	void SetPointPos(int x, int y);

	// X,Y 값 출력
	void ShowPointPos();

	// 두 Point변수 비교
	// 
	// Parameter : 비교할 Point*
	// return 0 : 둘이 x,y가 완전히 같음
	// return 1 : x만 같음
	// return 2 : y만 같음
	// return -1 : 모두 다름
	int Comp(Point* Comp);
};

#endif // !__POINT_H__
