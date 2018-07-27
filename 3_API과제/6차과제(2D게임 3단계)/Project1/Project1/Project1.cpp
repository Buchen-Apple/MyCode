// Project1.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Project1.h"
#include "ScreenDib.h"
#include "SpriteDIB.h"
#include "windowsx.h"
#include "Player.h"
#include <stdio.h>

// 전역 변수
HINSTANCE hInst; // 현재 인스턴스입니다.
HWND g_hWnd;	 // 메인 윈도우 입니다.
RECT rt;	

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Update();

CScreenDib* g_cScreenDib = CScreenDib::Getinstance(640, 480, 32);	// 싱글톤으로 CScreenDib형 객체 하나 얻기.
CSpritedib* g_cSpriteDib = CSpritedib::Getinstance(5,0xffffffff, 32);	// 싱글톤으로 CSpritedib형 객체 하나 얻기
CBaseOBject*  g_cPlayer = new CPlayer;								// 플레이어 캐릭터 생성

int PlayerPosX = 300, PlayerPosY = 300;										// 플레이어 위치 x,y (중점 좌표와 맵핑되는 픽셀단위 위치)

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	// 전역 문자열을 초기화합니다.
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
	wcex.lpszClassName = TEXT("5차과제(2D게임 2단계)");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);


	// 응용 프로그램 초기화를 수행합니다.

	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	g_hWnd = CreateWindowW(TEXT("5차과제(2D게임 2단계)"), TEXT("2D게임"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!g_hWnd)
		return FALSE;

	GetClientRect(g_hWnd, &rt);

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	MSG msg;

	// 게임 초반 세팅 함수(현재는 모든 비트맵을 읽어온다
	g_cSpriteDib->GameInit();

	// 윈도우 크기를 640 X 480으로 변경시킨다.
	RECT WindowRect;
	WindowRect.top = 0;
	WindowRect.left = 0;
	WindowRect.right = 640;
	WindowRect.bottom = 480;

	AdjustWindowRectEx(&WindowRect, GetWindowStyle(g_hWnd), GetMenu(g_hWnd) != NULL, GetWindowExStyle(g_hWnd));	// 원하는 위치로 사이즈를 다시 잡아준다.
	MoveWindow(g_hWnd, 0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, TRUE);		// 윈도우를 이동시켜서 위치를 잡는다.

	// 게임용 기본 메시지 루프입니다.
	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// 게임 처리 함수 호출
			Update();
		}
	}

	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{		
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

		case WM_SIZE:
			GetClientRect(g_hWnd, &rt);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void Update()
{
	//---------------------------
	// 내 백버퍼의 버퍼 및 정보 얻음
	// --------------------------
	BYTE* bypDest = g_cScreenDib->GetDibBuffer();
	int iDestWidth = g_cScreenDib->GetWidth();
	int iDestHeight = g_cScreenDib->GetHeight();
	int iDestPitch = g_cScreenDib->GetPitch();	

	//---------------------------
	// 키보드 입력 파트 : 큐에 행동을 넣는다.
	// 액션 파트 : 큐의 행동을 꺼내서 행동한다. (이동 or 공격 로직)
	// --------------------------
	// 플레이어 좌표값 이동
	g_cPlayer->Action();	

	//---------------------------
	// 버퍼 포인터에 그림을 그린다.
	// 
	// 랜더링 부분
	// 
	// --------------------------
	// 배경 
	BOOL iCheck = g_cSpriteDib->DrawImage(0, 0, 0, bypDest, iDestWidth, iDestHeight, iDestPitch);
	if (!iCheck)
		exit(-1);

	// 플레이어
	g_cPlayer->Draw(2, bypDest, iDestWidth, iDestHeight, iDestPitch);


	//---------------------------
	// 버퍼의 내용을 화면으로 출력. Flip역활
	// --------------------------
	g_cScreenDib->DrawBuffer(g_hWnd, rt.left, rt.top);
}