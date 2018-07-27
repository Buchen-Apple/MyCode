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
CMonitorGraphUnit *p5;

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
	wcex.lpszClassName = TEXT("4차과제(모니터링 3단계)");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);


	// 응용 프로그램 초기화를 수행합니다.

	g_hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(TEXT("4차과제(모니터링 3단계)"), TEXT("모니터링 3단계"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
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
	static bool bBackCheck = false;
	static bool bSoundCheck = false;
	static HWND hChild_hWnd;
	static int SendVal[10] = { 0, };

	switch (message)
	{
	case WM_CREATE:
		GetClientRect(hWnd, &rt);
		MyBrush = CreateSolidBrush(RGB(150, 150, 150));
		RedBrush = CreateSolidBrush(RGB(255, 0,0));

		// 생성자 호출.
		p1 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::BAR_SINGLE_VERT, 10, 10, 200, 200, L"CPU");
		p2 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::LINE_SINGLE, 220, 10, 200, 200, L"메모리");
		p3 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::LINE_MULTI, 430, 10, 600, 200, L"동시접속자");
		p4 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(18, 52, 120), CMonitorGraphUnit::BAR_COLUMN_VERT, 10, 220, 600, 400, L"DB 버퍼");
		p5 = new CMonitorGraphUnit(g_hInst, hWnd, RGB(62, 62, 62), CMonitorGraphUnit::PIE, 620, 220, 410, 400, L"동접 비율");

		// 추가 정보 세팅
		// 순서대로 [Max값, 알람 울리는 수치, 표시할 값의 단위]. 
		// Max 값이 0이면, 현재 큐의 값을 가장 큰 값으로 사용
		// 알람 값이 0이면 알람 울리지 않음.
		// 표시할 값의 단위가 L"NULL" 이면, 단위 표시하지 않음.
		p1->AddData(100, 80, L"%");
		p2->AddData(0, 1000, L"MB");
		p3->AddData(0, 0, L"명");
		p4->AddData(0, 0, L"NULL");
		p5->AddData(0, 0, L"%");

		// 컬럼 최초 정보 세팅
		{
			int p1_ColumnCount = 1;
			int p1_ServerID[1] = { SERVER_1 };
			int p1_DataType[1] = { DATA_TYPE_CPU_USE };
			TCHAR p1_DataName[1][10] = { L"CPU 사용량" };

			int p2_ColumnCount = 1;
			int p2_ServerID[1] = { SERVER_2 };
			int p2_DataType[1] = { DATA_TYPE_MEMORY_USE };
			TCHAR p2_DataName[1][10] = { L"메모리 사용량" };

			int p3_ColumnCount = 4;
			int p3_ServerID[4] = { SERVER_1, SERVER_2, SERVER_3, SERVER_4 };
			int p3_DataType[4] = { DATA_TYPE_CCU, DATA_TYPE_CCU, DATA_TYPE_CCU, DATA_TYPE_CCU };
			TCHAR p3_DataName[4][10] = { L"1섭동접", L"2섭동접", L"3섭동접", L"4섭동접" };

			int p4_ColumnCount = 4;
			int p4_ServerID[4] = { SERVER_1, SERVER_2, SERVER_3, SERVER_4 };
			int p4_DataType[4] = { DATA_TYPE_DB_BUFFER, DATA_TYPE_DB_BUFFER, DATA_TYPE_DB_BUFFER, DATA_TYPE_DB_BUFFER };
			TCHAR p4_DataName[4][10] = { L"1섭DB", L"2섭DB", L"3섭DB", L"4섭DB" };

			int p5_ColumnCount = 4;
			int p5_ServerID[4] = { SERVER_1, SERVER_2, SERVER_3, SERVER_4 };
			int p5_DataType[4] = { DATA_TYPE_CCU, DATA_TYPE_CCU, DATA_TYPE_CCU, DATA_TYPE_CCU };
			TCHAR p5_DataName[4][10] = { L"1섭동접", L"2섭동접", L"3섭동접", L"4섭동접" };

			p1->SetColumnInfo(1, p1_ServerID, p1_DataType, p1_DataName);
			p2->SetColumnInfo(1, p2_ServerID, p2_DataType, p2_DataName);
			p3->SetColumnInfo(4, p3_ServerID, p3_DataType, p3_DataName);
			p4->SetColumnInfo(4, p4_ServerID, p4_DataType, p4_DataName);
			p5->SetColumnInfo(4, p5_ServerID, p5_DataType, p5_DataName);
		}


		SetTimer(hWnd, 1, 100, NULL);	// 타이머 세팅
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

	case WM_SIZE:
		GetClientRect(hWnd, &rt);
		break;

	case WM_TIMER:

		// 모든 자식 윈도우에게 데이터 전달하는 타이머
		if (wParam == 1)
		{			
			/////////////////////////////////////////////////////////
			// 전달할 데이터 세팅
			/////////////////////////////////////////////////////////
			int a[10];

			// 1번 서버의 CPU
			a[0] = rand() % 5;
			// 2번 서버의 메모리 사용량
			a[1] = rand() % 80;
			// 1번 서버 동접자 수
			a[2] = rand() % 40;
			// 2번 서버 동접자 수
			a[3] = rand() % 38;
			// 3번 서버 동접자 수
			a[4] = rand() % 36;
			// 4번 서버 동접자 수
			a[5] = rand() % 80;
			// 1번 서버 DB 사용량 
			a[6] = rand() % 20;	
			// 2번 서버 DB 사용량 
			a[7] = rand() % 3;
			// 3번 서버 DB 사용량 
			a[8] = rand() % 5;
			// 4번 서버 DB 사용량 
			a[9] = rand() % 4;
			
			/////////////////////////////////////////////////////////
			// 전달할 데이터 가공
			/////////////////////////////////////////////////////////
			// 첫 번째 그래프는 바 그래프. 해당 그래프는 %로 표시하기 때문에(임시로) 100 이상을 넘지 않도록 강제한다.
			// 전달할 값을, +할지 -할지 결정한다.
			int randCheck = rand() % 2;

			if (randCheck == 0)	// 0이면 뺸다.
			{
				if (0 > (SendVal[0] - a[0]))	// 빼야하는데 뺸 후의 값이 -라면 더한다.
					SendVal[0] += a[0];
				else
					SendVal[0] -= a[0];
			}
			else   // 1이면 더한다.
			{
				if (100 < (SendVal[0] + a[0]))	// 더해야 하는데, 더한 후의 값이 100을 넘어가면 뺀다.
					SendVal[0] -= a[0];
				else
					SendVal[0] += a[0];	
			}

			// 2~10번 정보들 처리
			for (int i = 1; i < 10; ++i)
			{
				// 전달할 값을, +할지 -할지 결정한다.
				randCheck = rand() % 2;

				if (randCheck == 0)	// 0이면 뺸다.
				{
					if( 0 > (SendVal[i] - a[i]))	// 빼야하는데 뺸 값이 -라면 더한다.
						SendVal[i] += a[i];
					else
						SendVal[i] -= a[i];
				}
				else   // 1이면 더한다.
				{
					SendVal[i] += a[i];
				}
			}

			/////////////////////////////////////////////////////////
			// 데이터 모두 전달하기. 
			/////////////////////////////////////////////////////////
			// 자신의 것만 잘 구별해서 받는지 확인한다.

			int i = 0;
			// 1번 서버의 CPU 사용량 (p1에만 표시되어야 함)
			p1->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CPU_USE);	//<<이놈 전용
			p2->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CPU_USE);
			p3->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CPU_USE);
			p4->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CPU_USE);
			p5->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CPU_USE);
			

			// 2번 서버의 메모리 사용량	(p2에만 표시되어야 함)
			i++;
			p1->InsertData(SendVal[i], SERVER_2, DATA_TYPE_MEMORY_USE);
			p2->InsertData(SendVal[i], SERVER_2, DATA_TYPE_MEMORY_USE);	//<<이놈 전용
			p3->InsertData(SendVal[i], SERVER_2, DATA_TYPE_MEMORY_USE);
			p4->InsertData(SendVal[i], SERVER_2, DATA_TYPE_MEMORY_USE);
			p5->InsertData(SendVal[i], SERVER_2, DATA_TYPE_MEMORY_USE);

			// 1번 서버 동접자 수	(p3, p5의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CCU);
			p2->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CCU);
			p3->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CCU);//<<이놈은 이걸 Line 그래프로 표시
			p4->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CCU);
			p5->InsertData(SendVal[i], SERVER_1, DATA_TYPE_CCU);//<<이놈은 이걸 Pie 그래프로 표시

			// 2번 서버 동접자 수	(p3, p5의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_2, DATA_TYPE_CCU);
			p2->InsertData(SendVal[i], SERVER_2, DATA_TYPE_CCU);
			p3->InsertData(SendVal[i], SERVER_2, DATA_TYPE_CCU);//<<이놈은 이걸 Line 그래프로 표시
			p4->InsertData(SendVal[i], SERVER_2, DATA_TYPE_CCU);
			p5->InsertData(SendVal[i], SERVER_2, DATA_TYPE_CCU);//<<이놈은 이걸 Pie 그래프로 표시

			// 3번 서버 동접자 수	(p3, p5의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_3, DATA_TYPE_CCU);
			p2->InsertData(SendVal[i], SERVER_3, DATA_TYPE_CCU);
			p3->InsertData(SendVal[i], SERVER_3, DATA_TYPE_CCU);//<<이놈은 이걸 Line 그래프로 표시
			p4->InsertData(SendVal[i], SERVER_3, DATA_TYPE_CCU);
			p5->InsertData(SendVal[i], SERVER_3, DATA_TYPE_CCU);//<<이놈은 이걸 Pie 그래프로 표시

			// 4번 서버 동접자 수	(p3, p5의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_4, DATA_TYPE_CCU);
			p2->InsertData(SendVal[i], SERVER_4, DATA_TYPE_CCU);
			p3->InsertData(SendVal[i], SERVER_4, DATA_TYPE_CCU);//<<이놈은 이걸 Line 그래프로 표시
			p4->InsertData(SendVal[i], SERVER_4, DATA_TYPE_CCU);
			p5->InsertData(SendVal[i], SERVER_4, DATA_TYPE_CCU);//<<이놈은 이걸 Pie 그래프로 표시

			 // 1번 서버 DB 사용량	(p4의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_1, DATA_TYPE_DB_BUFFER);
			p2->InsertData(SendVal[i], SERVER_1, DATA_TYPE_DB_BUFFER);
			p3->InsertData(SendVal[i], SERVER_1, DATA_TYPE_DB_BUFFER);
			p4->InsertData(SendVal[i], SERVER_1, DATA_TYPE_DB_BUFFER);//<<이놈 컬럼 중 하나
			p5->InsertData(SendVal[i], SERVER_1, DATA_TYPE_DB_BUFFER);

			// 2번 서버 DB 사용량 	(p4의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_2, DATA_TYPE_DB_BUFFER);
			p2->InsertData(SendVal[i], SERVER_2, DATA_TYPE_DB_BUFFER);
			p3->InsertData(SendVal[i], SERVER_2, DATA_TYPE_DB_BUFFER);
			p4->InsertData(SendVal[i], SERVER_2, DATA_TYPE_DB_BUFFER);//<<이놈 컬럼 중 하나
			p5->InsertData(SendVal[i], SERVER_2, DATA_TYPE_DB_BUFFER);

			// 3번 서버 DB 사용량 	(p4의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_3, DATA_TYPE_DB_BUFFER);
			p2->InsertData(SendVal[i], SERVER_3, DATA_TYPE_DB_BUFFER);
			p3->InsertData(SendVal[i], SERVER_3, DATA_TYPE_DB_BUFFER);
			p4->InsertData(SendVal[i], SERVER_3, DATA_TYPE_DB_BUFFER);//<<이놈 컬럼 중 하나
			p5->InsertData(SendVal[i], SERVER_3, DATA_TYPE_DB_BUFFER);

			// 4번 서버 DB 사용량 	(p4의 컬럼 중 하나)
			i++;
			p1->InsertData(SendVal[i], SERVER_4, DATA_TYPE_DB_BUFFER);
			p2->InsertData(SendVal[i], SERVER_4, DATA_TYPE_DB_BUFFER);
			p3->InsertData(SendVal[i], SERVER_4, DATA_TYPE_DB_BUFFER);
			p4->InsertData(SendVal[i], SERVER_4, DATA_TYPE_DB_BUFFER);//<<이놈 컬럼 중 하나
			p5->InsertData(SendVal[i], SERVER_4, DATA_TYPE_DB_BUFFER);

		}

		// 화면 빨갛게 만들기 종료 타이머
		else if (wParam == 2)
		{			
			KillTimer(hWnd, 2);
			bBackCheck = false;
			InvalidateRect(hWnd, NULL, true);
		}

		// 알람 사운드를 끄는 타이머
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