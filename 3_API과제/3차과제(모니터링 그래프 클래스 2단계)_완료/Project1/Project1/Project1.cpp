// Project1.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Project1.h"
#include "ChildWindow.h"
#include <mmsystem.h>

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
	static HBRUSH MyBrush, RedBrush;
	static double a, b, c, d;
	static bool bBackCheck = false;
	static bool bSoundCheck = false;
	static HWND hChild_hWnd;

	switch (message)
	{
	case WM_CREATE:
		GetClientRect(hWnd, &rt);
		MyBrush = CreateSolidBrush(RGB(150, 150, 150));
		RedBrush = CreateSolidBrush(RGB(255, 0,0));

		// iMax값을, 현재 큐에 있는 가장 큰 값으로 지정하기. 가장 마지막에서 <<한칸 좌측 인자를 0으로 넣으면 된다.
		/*
		p1 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_SINGLE, 10, 10, 200, 200, L"CPU1", 0, 50);		
		p2 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 220, 10, 200, 200, L"CPU2", 0, 600);
		p3 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_SINGLE, 430, 10, 400, 200, L"CPU3", 0, 200);
		p4 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 10, 220, 300, 250, L"CPU4", 0, 150);
		*/

		// iMax값 지정하기.
		p1 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_SINGLE, 10, 10, 200, 200, L"CPU1", 100, 50);
		p2 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 220, 10, 200, 200, L"CPU2", 1200, 1250);
		p3 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_SINGLE, 430, 10, 400, 200, L"CPU3", 400, 450);
		p4 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 10, 220, 300, 250, L"CPU4", 300, 350);
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
		if (wParam == 1)
		{	
			// 모든 자식 윈도우에게 데이터 전달.
			
			// iMax값을, 현재 큐에 있는 가장 큰 값으로 지정하기.
			/* 
			p1->InsertData(rand() % 10, &a);
			p2->InsertData(rand() % 20, &a);
			p3->InsertData(rand() % 10, &a);
			p4->InsertData(rand() % 1000, &a);
			*/

			// iMax값 지정하기.
			p1->InsertData(rand() % 52, &a);
			p2->InsertData(rand() % 1200, &b);
			p3->InsertData(rand() % 400, &c);
			p4->InsertData(rand() % 300, &d);
		}
		else if (wParam == 2)
		{			
			KillTimer(hWnd, 2);	// 화면 빨갛게 만들기 종료.
			bBackCheck = false;
			InvalidateRect(hWnd, NULL, true);
		}
		else if (wParam == 3)
		{
			bSoundCheck = false;
			KillTimer(hWnd, 3);
		}
		break;

	case UM_PARENTBACKCHANGE:
		if (!bBackCheck)
		{
			hChild_hWnd = (HWND)wParam;	// 자식으로부터 메시지 도착. 화면을 빨갛게 만들라는 것.
			SetTimer(hWnd, 2, 200, NULL);
			bBackCheck = true;

			if (!bSoundCheck)
			{
				PlaySound(TEXT("SystemDefault"), 0, SND_ALIAS | SND_ASYNC);	// 화면을 빨갛게 해야 할 경우, 에러 사운드를 출력한다.
				bSoundCheck = true;
				SetTimer(hWnd, 3, 700, NULL);	// 사운드가 씹히는 현상을 막기 위해, 타이머를 이용해 강제 700밀리세컨드 시간동안 출력.
			}

			InvalidateRect(hWnd, NULL, true);
		}
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);			
		if (bBackCheck)	// 빨간색으로 칠하기
			FillRect(hdc, &rt, RedBrush);
		else  // 원래 색으로 칠하기
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
		DeleteObject(RedBrush);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}