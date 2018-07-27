// Project1.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Project1.h"
#include "ScreenDib.h"
#include <stdio.h>

// 전역 변수
HINSTANCE hInst; // 현재 인스턴스입니다.
HWND g_hWnd;	 // 메인 윈도우 입니다.
RECT rt;	

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Update();

CScreenDib *g_cScreenDIb = CScreenDib::Getinstance(640, 480, 32);	// 싱글톤으로 CScreenDib형 객체 하나 얻기.

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
	wcex.lpszClassName = TEXT("4차과제(내 백버퍼 제작)");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);


	// 응용 프로그램 초기화를 수행합니다.

	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	g_hWnd = CreateWindowW(TEXT("4차과제(내 백버퍼 제작)"), TEXT("백버퍼 제작"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!g_hWnd)
		return FALSE;

	GetClientRect(g_hWnd, &rt);

	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);

	MSG msg;

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
	// 출력 버퍼 포인터 및 정보 얻음
	// --------------------------
	BYTE* bypDest = g_cScreenDIb->GetDibBuffer();
	int iDestWidth = g_cScreenDIb->GetWidth();
	int iDestHeight = g_cScreenDIb->GetHeight();
	int iDestPitch = g_cScreenDIb->GetPitch();

	//---------------------------
	// 버퍼 포인터에 그림을 그린다.
	//
	// 스프라이트 출력부
	// 
	// --------------------------
	// 배경 그라데이션 찍기 ---------------------
	BYTE byGrayColor = 255;
	BYTE* reset = bypDest;
	for (int iCount = 0; iCount < iDestHeight; iCount++)
	{
		memset(bypDest, byGrayColor, iDestPitch);
		bypDest += iDestPitch;
		byGrayColor++;
	}

	// 바이트 단위 접근해 점찍어서 선 긋기. ---------------------
	// 바이트 단위로 하나씩 접근할 때는 이처럼 리틀엔디안으로 BGRA로 취급된다.
	// memset은 픽셀이 아니라 바이트 단위이기 때문에, bypDest의 위치를 1씩 증가시켜줘야한다.
	// 32비트에서 1픽셀은 32비트 (4바이트)이기 때문에 1칸씩 >>로 이동하면서 4번 찍어줘야한다.
	bypDest = reset + 999;
	for (int i = 0; i < iDestHeight; ++i)
	{
		memset(bypDest + 1, 0x00, 1);	// B		byGrayColor대신 색을 직접 넣어도 된다. 참고로 0xffffffff은 255(하얀색)이다.
		memset(bypDest + 2, 0x00, 1);	// G
		memset(bypDest + 3, 0xff, 1);	// R
		memset(bypDest + 4, 0x00, 1);	// A

		bypDest += iDestPitch;	// 다음 줄로 이동.
	}

	// 비트맵 이미지 읽어오기 ---------------------
	FILE* fp;
	int iCheck = fopen_s(&fp, "Attack1_L_01.bmp", "rb");
	if (iCheck != NULL)
		exit(-1);

	BITMAPFILEHEADER FileHeader;
	BITMAPINFOHEADER InfoHeader;

	fread(&FileHeader, sizeof(BITMAPFILEHEADER), 1, fp);	// 파일헤더 읽어오기.
	fread(&InfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);	// 인포헤더 읽어오기. 

	int iBitmapPitch = (InfoHeader.biWidth * (InfoHeader.biBitCount / 8)) + 3 & ~3;	// 실제 가로 메모리 길이 (pitch는 바이트 단위 실제 가로 길이)
	int iImgSize = iBitmapPitch * InfoHeader.biHeight;		// 실제 이미지 크기 (바이트 단위). 

	// 이미지 크기만큼 동적할당.
	BYTE* BitmapBuffer = new BYTE[iImgSize];

	// 동적할당 한 크기만큼 이미지를 읽어온다.
	fread(BitmapBuffer, iImgSize, 1, fp);

	// 파일 읽기 끝났으니 스트림 닫기
	fclose(fp);


	// 읽어온 비트맵 이미지를 내 버퍼에 찍기 ---------------------
	// 컬러 키 기술을 적용해 하얀색은 찍지 않는다. 픽셀을 하나하나 비교한다.
	UINT Colorkey = 0xffffffff;		// 컬러키를 저장해둘 변수
		
	bypDest = reset + 380500;		// 비트맵 출력 시작 좌표.

	// 비트맵 이미지의 높이만큼 반복한다
	for (int i = 0; i < InfoHeader.biHeight; ++i)	
	{
		// 높이를 반복하면서, >>로 한칸씩 이동하며 픽셀단위로 컬러가 하얀색인지 체크한다.
		for (int j = 0; j < iBitmapPitch; j+=4)	// 
		{
			// 해당 픽셀의 컬러가 하얀색이 아닐 때만 내 백버퍼에 찍는다. 하얀색이면 찍지 않고 다음 픽셀로 이동한다.
			if(memcmp(BitmapBuffer+j, &Colorkey, 4))
				memcpy(bypDest + j, BitmapBuffer + j, 4);	// 1픽셀(4바이트)씩 카피한다.
		}	
		bypDest += iDestPitch;
		BitmapBuffer += iBitmapPitch;	
	}
	

	//---------------------------
	// 버퍼의 내용을 화면으로 출력. Flip역활
	// --------------------------
	g_cScreenDIb->DrawBuffer(g_hWnd, rt.left, rt.top);	
}