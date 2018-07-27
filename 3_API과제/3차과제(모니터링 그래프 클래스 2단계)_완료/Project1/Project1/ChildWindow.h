#pragma once
#ifndef __CHILD_WINDOW_H__
#define __CHILD_WINDOW_H__

//// 자식 윈도우 클래스 헤더 ////////////////////////////
#include "Graph_Ypos_Queue.h"

#define dfMAXCHILD		100
#define UM_PARENTBACKCHANGE WM_USER+1

class CMonitorGraphUnit
{
public:

	enum TYPE
	{
		BAR_SINGLE_VERT,
		BAR_SINGLE_HORZ,
		BAR_COLUMN_VERT,
		BAR_COLUMN_HORZ,
		LINE_SINGLE,
		LINE_MULTI,
		PIE
	};

public:

	CMonitorGraphUnit(HINSTANCE hInstance, HWND hWndParent, COLORREF BackColor, TYPE enType, int iPosX, int iPosY, int iWidth, int iHeight, LPCTSTR str, int iMax, int AleCount);
	~CMonitorGraphUnit();

	/////////////////////////////////////////////////////////
	// 윈도우 프로시저
	/////////////////////////////////////////////////////////
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	/////////////////////////////////////////////////////////
	// 데이터 넣기.
	/////////////////////////////////////////////////////////
	void InsertData(int iData, double*);

private:
	//------------------------------------------------------
	// Paint함수
	//------------------------------------------------------
	void Paint_LINE_SINGLE();

	//------------------------------------------------------
	// 부모 배경 빨간색으로 칠하기 체크 함수
	//------------------------------------------------------
	void ParentBackCheck(int Data);

	//------------------------------------------------------
	// 기본 UI 세팅
	//------------------------------------------------------
	void UISet();

	//------------------------------------------------------
	// 부모 윈도우 핸들, 내 윈도우 핸들, 인스턴스 핸들
	//------------------------------------------------------
	HWND hWndParent;
	HWND hWnd;
	HINSTANCE hInstance;

	//------------------------------------------------------
	// 윈도우 위치,크기,색상, 그래프 타입 등.. 자료
	//------------------------------------------------------
	TYPE _enGraphType;
	RECT rt;
	RECT TitleRt;
	RECT LeftRt;
	RECT ClientRt;
	COLORREF BackColor;

	//------------------------------------------------------
	// 데이터
	//------------------------------------------------------	
	Queue queue;	// 큐
	TCHAR tWinName[20];
	double iAddY;
	int Greedint[4];

	//------------------------------------------------------
	// 그래프 Max값 , 경고 울리는 값
	//------------------------------------------------------
	bool MaxVariable;
	int iMax;
	double AleCount;
	bool bObjectCheck;

	//------------------------------------------------------
	// MemDC와 리소스
	//------------------------------------------------------
	HDC MemDC;
	HBRUSH BackBrush, TitleBrush;
	HBITMAP MyBitmap, OldBitmap;
	HFONT MyFont; 
	int iFontR, iFontG, iFontB;	// 폰트가 중간에 변경되기 때문에(빨간색으로) 원래 폰트색을 저장할 변수.
	HPEN GraphPen;
	

	// static 맴버 함수의 프로시저에서 This 포인터를 찾기 위한
	// HWND + Class Ptr 의 테이블
	static CMonitorGraphUnit *pThis;

	// 1씩 순차적으로 증가시키기 위한 임시 static
	double aaa;
};

#endif // !__CHILD_WINDOW_H__
