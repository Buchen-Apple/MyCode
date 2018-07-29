#include <windows.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = TEXT("SimplePaint");

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);    //(HBRUSH)(COLOR_WINDOW+1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	while (GetMessage(&Message, NULL, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return (int)Message.wParam;
}

enum{RD_1 = 101, RD_2, RD_3, CH_1 = 201, RE_1};
HWND r1, r2, r3, c1, empty;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	static bool bButtonCheck = false;
	static int x, y;
	static HPEN MyPen, OldPen;
	static COLORREF color = RGB(255, 0, 0);
	static int iPenWeight = 1;

	switch (iMessage)
	{
	case WM_CREATE:
		hWndMain = hWnd;
		CreateWindow(TEXT("button"), TEXT("Color"), WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, 5, 120, 110, hWnd, (HMENU)0, g_hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("Weight"), WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 145, 5, 120, 110, hWnd, (HMENU)0, g_hInst, NULL);

		// 선의 색상(라디오 버튼)
		r1 = CreateWindow(TEXT("button"), TEXT("Red"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON  | WS_GROUP , 10, 20, 100, 30, hWnd,(HMENU)RD_1, g_hInst, NULL);
		r2 = CreateWindow(TEXT("button"), TEXT("Blue"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 10, 50, 100, 30, hWnd, (HMENU)RD_2, g_hInst, NULL);
		r3 = CreateWindow(TEXT("button"), TEXT("Yellow"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 10, 80, 100, 30, hWnd, (HMENU)RD_3, g_hInst, NULL);
		CheckRadioButton(hWnd, RD_1, RD_3, RD_1);

		// 굵기 (체크박스)
		c1 = CreateWindow(TEXT("button"), TEXT("굵게"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 150, 20, 100, 30, hWnd, (HMENU)CH_1, g_hInst, NULL);

		// 다시 그리기 버튼. 화면 모두 지움
		empty = CreateWindow(TEXT("button"), TEXT("다시 그리기"), WS_CHILD | WS_VISIBLE , 280, 5, 100, 30, hWnd, (HMENU)RE_1, g_hInst, NULL);			
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case RD_1:
			color = RGB(255, 0, 0);
			break;

		case RD_2:
			color = RGB(0, 0, 255);
			break;

		case RD_3:
			color = RGB(255, 255, 0);
			break;

		case CH_1:
			if (SendMessage(c1, BM_GETCHECK, 0, 0) == BST_CHECKED)
				iPenWeight = 5;
			else if (SendMessage(c1, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
				iPenWeight = 1;
			break;

		case RE_1:
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		return 0;

	case WM_LBUTTONDOWN:
		bButtonCheck = true;
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		return 0;

	case WM_MOUSEMOVE:		
		if (bButtonCheck == true)
		{
			hdc = GetDC(hWnd);
			MyPen = CreatePen(PS_SOLID, iPenWeight, color);
			OldPen = (HPEN)SelectObject(hdc, MyPen);

			MoveToEx(hdc, x, y, NULL);
			x = LOWORD(lParam);
			y = HIWORD(lParam);					
			LineTo(hdc, x, y);	

		
			SelectObject(hdc, OldPen);
			DeleteObject(MyPen);
			ReleaseDC(hWnd, hdc);
		}		
		return 0;

	case WM_LBUTTONUP:
		bButtonCheck = false;
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);		
		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:		
		SelectObject(hdc, OldPen);
		DeleteObject(MyPen);		
		PostQuitMessage(0);
		return 0;
	}

	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}