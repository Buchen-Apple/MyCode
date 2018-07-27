// Project1.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include <stdio.h>
#include "Project1.h"

// 전역 변수:
HINSTANCE hInst;  // 현재 인스턴스입니다.
RECT rt;

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
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
	wcex.lpszClassName = TEXT("테스트");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);

    // 응용 프로그램 초기화를 수행합니다.
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(TEXT("테스트"), TEXT("타이틀"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	GetClientRect(hWnd, &rt);	// 윈도우의 작업 영역 구함. 
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

    MSG msg;
	

    // 기본 메시지 루프입니다.
    while (GetMessage(&msg, nullptr, 0, 0))
    {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}


//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static BYTE* Buffer;
	HANDLE fHandle;
	static bool Flag = true;
	static BITMAPFILEHEADER bFileHeader;
	static BITMAPINFOHEADER bInfoHeader;
	static int iFilePitch;

    switch (message)
    {
	case WM_CREATE:
		fHandle = CreateFile(TEXT("24bitBMP.bmp"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0 ,NULL);
		if (fHandle == INVALID_HANDLE_VALUE)
			Flag = false;

		else
		{
			int iSize;
			DWORD dResult;
			ReadFile(fHandle, &bFileHeader, sizeof(BITMAPFILEHEADER), &dResult, NULL);	// 파일 헤더 읽어오기
			if (bFileHeader.bfType != 0x4D42)
			{
				Flag = false;
				break;
			}
			ReadFile(fHandle, &bInfoHeader, sizeof(BITMAPINFOHEADER), &dResult, NULL);	// 정보 헤더 읽어오기
			
			iFilePitch = (bInfoHeader.biWidth + 3) & ~3;					// pitch 구하기
			int iRealWidth; iRealWidth = iFilePitch * (bInfoHeader.biBitCount / 8);			// 실제 가로 사이즈(byte)
			iSize = iRealWidth * bInfoHeader.biHeight;						// 실제 이미지 사이즈 구하기

			Buffer = (BYTE*)malloc(iSize);						// 실제 이미지 크기만큼 동적할당.
			ReadFile(fHandle, Buffer, iSize, &dResult, NULL);	// 실제 이미지 읽어오기

			CloseHandle(fHandle);	// 오픈한 파일 닫기.	
		}
		break;
	case WM_SIZE:
		GetClientRect(hWnd, &rt);	// 윈도우의 작업 영역 구함. 
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다.
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
			if (!Flag)	// Flag가 false라면, 파일 읽기 실패한 것. 실패했으면 텍스트 출력과 함께 WM_PAINT메시지 종료
			{
				TCHAR str[] = TEXT("WM_CREATE실패 혹은 비트맵파일이 아님");
				TextOut(hdc, 10, 10, str, lstrlen(str));
				break;
			}
			SetStretchBltMode(hdc, COLORONCOLOR);			// 이미지 축소 시 깨짐모드 방지. COLORONCOLOR : 논리연산 하지 않음. 컬러 비트맵에서 가장 무난한 방법이라고 함.
			StretchDIBits(hdc, 0, 0, rt.right, rt.bottom,	// 출력할 DC, 목적지(DC)의 x,y 좌표와 폭,높이.
						  0, 0, iFilePitch, bInfoHeader.biHeight,	// DIB의 x,y 좌표와 폭, 높이. 해당 DIB를 지정한 목적지 크기만큼 출력.
						  Buffer, (BITMAPINFO*)&bInfoHeader, DIB_RGB_COLORS, SRCCOPY);	// 실제 이미지가 저장되어 있는 포인터 시작점, BITMAPINFO정보를 넘겨야 하지만, 우린 팔레트 안쓰니 형변환
																						// DIB_RGB_COLORS는 RGB컬러를 쓸건지 팔레트를 쓸건지 선택, 마지막은 출력모드. 즉 우리는 복사.
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}