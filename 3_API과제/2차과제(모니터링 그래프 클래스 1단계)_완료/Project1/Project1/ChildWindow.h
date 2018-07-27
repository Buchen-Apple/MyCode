#pragma once
#ifndef __CHILD_WINDOW_H__
#define __CHILD_WINDOW_H__

//// 자식 윈도우 클래스 헤더 ////////////////////////////
#include "Graph_Ypos_Queue.h"

#define dfMAXCHILD		100

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

	typedef struct ST_HWNDtoTHIS
	{
		HWND			hWnd[dfMAXCHILD];
		CMonitorGraphUnit	*pThis[dfMAXCHILD];

	} stHWNDtoTHIS;

public:

	CMonitorGraphUnit(HINSTANCE hInstance, HWND hWndParent, COLORREF BackColor, TYPE enType, int iPosX, int iPosY, int iWidth, int iHeight);
	~CMonitorGraphUnit();

	/////////////////////////////////////////////////////////
	// 윈도우 프로시저
	/////////////////////////////////////////////////////////
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	/////////////////////////////////////////////////////////
	// 데이터 넣기.
	/////////////////////////////////////////////////////////
	void InsertData(int iData);

protected:

	//------------------------------------------------------
	// 윈도우 핸들, this 포인터 매칭 테이블 관리.
	//------------------------------------------------------
	BOOL				PutThis(void);
	static BOOL			RemoveThis(HWND hWnd);
	static CMonitorGraphUnit* GetThis(HWND hWnd);

private:

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
	COLORREF BackColor;

	//------------------------------------------------------
	// 윈도우 최초생성 체크
	//------------------------------------------------------
	static bool FirstCheck;

	//------------------------------------------------------
	// 데이터
	//------------------------------------------------------	
	Queue queue;	// 큐
	int y;			// 그래프 그릴 때 사용하는 x, y 변수
	HBRUSH MyBrush;		// 내 배경을 채울 브러시.

	// static 맴버 함수의 프로시저에서 This 포인터를 찾기 위한
	// HWND + Class Ptr 의 테이블
	static CMonitorGraphUnit *pThis;
	static stHWNDtoTHIS MyThisStruct;
};
#endif // !__CHILD_WINDOW_H__
