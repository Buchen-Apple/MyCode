#include "stdafx.h"
#include "ChildWindow.h"

//// 자식 윈도우 클래스 CPP ////////////////////////////

// CMonitorGraphUnit 클래스의 static들.
CMonitorGraphUnit::stHWNDtoTHIS CMonitorGraphUnit::MyThisStruct;
bool CMonitorGraphUnit::FirstCheck = true;
CMonitorGraphUnit *CMonitorGraphUnit::pThis;

// 생성자
CMonitorGraphUnit::CMonitorGraphUnit(HINSTANCE hInstance, HWND hWndParent, COLORREF BackColor, TYPE enType, int iPosX, int iPosY, int iWidth, int iHeight)
{
	if (FirstCheck)	// 어차피 이름 다 똑같으니..(lpszClassName가 동일) 최초 1회만 세팅.
	{
		FirstCheck = false;
		for (int i = 0; i < dfMAXCHILD; ++i)	// 최초 1회에는 모든 MyThisStruct를 NULL로 초기화. NULL일 경우 빈 공간으로 판단.
		{
			MyThisStruct.hWnd[i] = NULL;
		}

		// 윈도우 클래스 세팅
		WNDCLASSEXW wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CMonitorGraphUnit::WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = TEXT("자식윈도우");
		wcex.hIconSm = NULL;

		RegisterClassExW(&wcex);		
	}

	// 윈도우 세팅
	HWND hWnd = CreateWindowW(TEXT("자식윈도우"), TEXT("자식윈도우임다"),  WS_CAPTION | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		iPosX, iPosY, iWidth, iHeight, hWndParent, nullptr, hInstance, nullptr);

	// 작업 영역을 얻어온다.
	GetClientRect(hWnd, &rt);

	// 각종 필요한 값을 클래스 멤버변수에 넣는다.
	_enGraphType = enType;
	this->BackColor = BackColor;
	this->hWndParent = hWndParent;
	this->hWnd = hWnd;
	this->hInstance = hInstance;
	MyBrush = CreateSolidBrush(BackColor);
	
	Init(&queue);								// 큐 초기화
	y = rt.bottom - (rt.bottom / 2);			// 작업 영역 기준, 중간 즈음의 값을 y에 넣는다. 이는, 처음 시작 위치를 지정한다.
	Enqueue(&queue, y);						    // 세팅한 y(처음 시작 위치)를 큐에 넣는다.

	// 나의 this를 위한 세팅
	bool Check = PutThis();

	if (!Check)	// 만약, 더 이상 윈도우를 생성할 수 없다면 그냥 종료.
		exit(0);

	UpdateWindow(hWnd);
}

// 소멸자
CMonitorGraphUnit::~CMonitorGraphUnit()
{
	// 현재는 할게 없다...일단 만들어두자
}

// 자식들의 윈도우 프로시저
LRESULT CALLBACK CMonitorGraphUnit::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	pThis = GetThis(hWnd);	// 현재 hWnd의 this 얻어오기.

	switch (message)
	{
	case WM_PAINT:
	{		
		int x;
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);	
		HDC MemDC = CreateCompatibleDC(hdc);
		HBITMAP MyBitmap = CreateCompatibleBitmap(hdc, pThis->rt.right, pThis->rt.bottom);
		HBITMAP OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

		FillRect(MemDC, &pThis->rt, pThis->MyBrush);
		int data;	// 디큐 시, 저장할 y값.
		if (FirstPeek(&pThis->queue, &data))	// 큐의 가장 처음에 있는 y값을 디큐한다.
		{
			x = 0;	// 매번 그릴 때 마다, 가장 좌측<<부터 그려야 하기 떄문에 0으로 만든다.
			MoveToEx(MemDC, x, data, NULL);			// 처음 시작 위치 지정.	
			while (NextPeek(&pThis->queue, &data))	// 다음 y값 디큐.
			{
				x += pThis->rt.right / 100;;	// 윈도우의 너비를 100등분으로 나눠서, 1등분 간격으로 찍는다.
				LineTo(MemDC, x, data);	// 디큐한 y값을 사용해 이전 위치부터 긋는다.
			}
		}
		BitBlt(hdc, pThis->rt.left, pThis->rt.top, pThis->rt.right, pThis->rt.bottom, MemDC, pThis->rt.left, pThis->rt.top, SRCCOPY);
		
		SelectObject(MemDC, OldBitmap);
		DeleteObject(MyBitmap);
		DeleteDC(MemDC);

		EndPaint(hWnd, &ps);
	}
	break;

	case WM_DESTROY:	
		DeleteObject(pThis->MyBrush);
		PostQuitMessage(0);		
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 데이터 넣기.
void CMonitorGraphUnit::InsertData(int iData)
{
	int randCheck = rand() % 2;	// 더할것인지 뺼것인지 둘 중하나 랜덤결정

	if (randCheck == 0)	// 0이면 뺸다.
	{
		y -= iData;
		while (rt.top > y || rt.bottom < y)	// 뻇는데, 화면을 벗어난다면, 더한다. 벗어나지 않을 때 까지.
			y += iData;
	}
	else  // 1이면 더한다.
	{
		y += iData;
		while (rt.top > y || rt.bottom < y)	//더했는데 화면을 벗어난다면, 뺸다. 벗어나지 않을 때 까지
			y -= iData;
	}
	Enqueue(&queue, y);	// 계산된 y를 인큐한다.
	InvalidateRect(hWnd, NULL, false);	// WM_PAINT 호출
}

//------------------------------------------------------
// 윈도우 핸들, this 포인터 매칭 테이블 관리.
//------------------------------------------------------
BOOL CMonitorGraphUnit::PutThis(void)
{
	for (int i = 0; i < dfMAXCHILD; ++i)
	{
		// hWnd가 NULL인 공간을 찾으면, 해당 공간에 핸들이랑 this 세팅
		if (MyThisStruct.hWnd[i] == NULL)
		{
			MyThisStruct.hWnd[i] = hWnd;
			MyThisStruct.pThis[i] = this;
			return true;
		}
	}
	
	return false;
}
BOOL CMonitorGraphUnit::RemoveThis(HWND hWnd)
{
	for (int i = 0; i < dfMAXCHILD; ++i)
	{
		// 인자로 받은 hWnd와 같은 공간을 찾으면, 해당 공간을 NULL로 만들어서 빈 공간 취급.
		if (MyThisStruct.hWnd[i] == hWnd)
		{
			MyThisStruct.hWnd[i] = NULL;
			return true;
		}
	}

	return false;
}

CMonitorGraphUnit *CMonitorGraphUnit::GetThis(HWND hWnd)
{
	// 필요한 This 얻어오기.
	for (int i = 0; i < dfMAXCHILD; ++i)
		if (MyThisStruct.hWnd[i] == hWnd)
			return MyThisStruct.pThis[i];
}