#include <windows.h>
#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = TEXT("GDIObject");

HACCEL hAccel;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);//(HBRUSH)GetStockObject(WHITE_BRUSH);   
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	while (GetMessage(&Message, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccel, &Message))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}

	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	static int x, y, oldx, oldy;
	static int iCheck = 1;
	static bool bNowDraw = false;
	HBRUSH MyBrush, OldBrush;


	switch (iMessage)
	{
	case WM_CREATE:
		hWndMain = hWnd;
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_40001:	// 자유 곡선 모드
			iCheck = 1;
			break;
		case ID_40002:	// 선 모드
			iCheck = 2;
			break;
		case ID_40003:	// 원 모드
			iCheck = 3;
			break;
		case ID_40004:	// 사각형 모드
			iCheck = 4;
			break;
		}
		return 0;

	case WM_LBUTTONDOWN:
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		oldx = x;
		oldy = y;
		bNowDraw = true;
		return 0;

	case WM_MOUSEMOVE:
		if (bNowDraw)
		{
			hdc = GetDC(hWnd);
			if (iCheck == 1)	// 자유 곡선
			{
				oldx = LOWORD(lParam);
				oldy = HIWORD(lParam);
				MoveToEx(hdc, x, y, NULL);
				LineTo(hdc, oldx, oldy);
				x = oldx;
				y = oldy;
			}
			else if (iCheck == 2)	// 선 
			{				
				SetROP2(hdc, R2_NOT);
				MoveToEx(hdc, x, y, NULL);
				LineTo(hdc, oldx, oldy);
				oldx = LOWORD(lParam);
				oldy = HIWORD(lParam);
				MoveToEx(hdc, x, y, NULL);
				LineTo(hdc, oldx, oldy);				
			}
			else if (iCheck == 3)	// 원
			{
				MyBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
				OldBrush = (HBRUSH)SelectObject(hdc, MyBrush);
				SetROP2(hdc, R2_NOT);
				Ellipse(hdc, x, y, oldx, oldy);
				oldx = LOWORD(lParam);
				oldy = HIWORD(lParam);
				Ellipse(hdc, x, y, oldx, oldy);
				SelectObject(hdc, OldBrush);
				DeleteObject(MyBrush);
			}
			else if (iCheck == 4)	// 사각형
			{
				MyBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
				OldBrush = (HBRUSH)SelectObject(hdc, MyBrush);
				SetROP2(hdc, R2_NOT);
				Rectangle(hdc, x, y, oldx, oldy);
				oldx = LOWORD(lParam);
				oldy = HIWORD(lParam);
				Rectangle(hdc, x, y, oldx, oldy);	
				SelectObject(hdc, OldBrush);
				DeleteObject(MyBrush);
			}
			ReleaseDC(hWnd, hdc);

		}
		return 0;

	case WM_LBUTTONUP:
		bNowDraw = false;
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}