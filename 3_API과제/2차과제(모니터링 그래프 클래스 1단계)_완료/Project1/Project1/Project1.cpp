// Project1.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Project1.h"
#include "ChildWindow.h"

// 전역 변수
HINSTANCE g_hInst; // 현재 인스턴스입니다.
CMonitorGraphUnit *p1;
CMonitorGraphUnit *p2;
CMonitorGraphUnit *p3;
CMonitorGraphUnit *p4;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	// TODO: 여기에 코드를 입력합니다.

	// 윈도우 클래스 세팅
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECT1));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PROJECT1);
	wcex.lpszClassName = TEXT("2차과제(모니터링 1단계)");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);


	// 응용 프로그램 초기화를 수행합니다.

	g_hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(TEXT("2차과제(모니터링 1단계)"), TEXT("모니터링 1단계"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;

	// 기본 메시지 루프입니다.
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static RECT rt;
	static HBRUSH MyBrush;

	switch (message)
	{
	case WM_CREATE:
		GetClientRect(hWnd, &rt);
		MyBrush = CreateSolidBrush(RGB(150, 150, 150));
		p1 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(255, 255, 0), CMonitorGraphUnit::LINE_SINGLE, 10, 10, 200, 200);
		//p2 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(100, 100, 100), CMonitorGraphUnit::LINE_SINGLE, 220, 10, 200, 200);
		//p3 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(0, 255, 255), CMonitorGraphUnit::LINE_SINGLE, 430, 10, 400, 200);
		//p4 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(255, 255, 255), CMonitorGraphUnit::LINE_SINGLE, 10, 220, 300, 250);
		SetTimer(hWnd, 1, 40, NULL);	// 타이머 세팅
		break;
	case WM_COMMAND:
	{
		// 메뉴 선택을 구문 분석합니다.
		switch (LOWORD(wParam))
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;

	case WM_TIMER:
		p1->InsertData(rand() & 5);	// 모든 자식 윈도우에게 데이터 전달.
		//p2->InsertData(rand() & 5);
		//p3->InsertData(rand() & 5);
		//p4->InsertData(rand() & 5);
		break;


	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);		
		FillRect(hdc, &rt, MyBrush);		
		EndPaint(hWnd, &ps);
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		delete p1;
		delete p2;
		delete p3;
		delete p4;
		KillTimer(hWnd, 1);
		DeleteObject(MyBrush);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}