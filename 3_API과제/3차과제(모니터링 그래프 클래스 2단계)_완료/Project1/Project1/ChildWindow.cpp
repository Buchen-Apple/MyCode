#include "stdafx.h"
#include "ChildWindow.h"

//// 자식 윈도우 클래스 CPP ////////////////////////////
CMonitorGraphUnit *CMonitorGraphUnit::pThis;

// 생성자
CMonitorGraphUnit::CMonitorGraphUnit(HINSTANCE hInstance, HWND hWndParent, COLORREF BackColor, TYPE enType, int iPosX, int iPosY, int iWidth, int iHeight, LPCTSTR str, int iMax, int AleCount)
{

	/////////////////////////////////////////////////////////
	// 윈도우 클래스 세팅
	/////////////////////////////////////////////////////////
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

	HWND hWnd = CreateWindowW(TEXT("자식윈도우"), TEXT("자식윈도우임다"),   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		iPosX, iPosY, iWidth, iHeight, hWndParent, nullptr, hInstance, nullptr);
	
	/////////////////////////////////////////////////////////
	//  윈도우 영역 세팅
	/////////////////////////////////////////////////////////
	// 작업 영역을 구한다.
	GetClientRect(hWnd, &rt);

	// 화면 상단 타이틀과 화면 좌측 공간을 지정한다.
	TitleRt = rt;
	TitleRt.bottom = 30;

	LeftRt = rt;
	LeftRt.top = TitleRt.bottom;
	LeftRt.right = 40;

	// 공간을 제외한 실제 그래프가 그려질 공간 지정
	ClientRt = rt;
	ClientRt.left = LeftRt.right;
	ClientRt.top = TitleRt.bottom;

	/////////////////////////////////////////////////////////
	//  클래스 멤버변수에 필요한 값 세팅
	/////////////////////////////////////////////////////////
	_enGraphType = enType;
	this->BackColor = BackColor;
	this->hWndParent = hWndParent;
	this->hWnd = hWnd;
	this->hInstance = hInstance;
	wcscpy_s(tWinName, _countof(tWinName), str);
	this->iMax = iMax;
	if (iMax == 0)
		MaxVariable = true;
	else
		MaxVariable = false;
	
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

	/////////////////////////////////////////////////////////
	//  그래프 Max값 세팅
	/////////////////////////////////////////////////////////
	if (!MaxVariable)
	{
		// 작업 영역 기준, y 1칸 이동 시, 실제 이동해야하는 위치 계산
		iAddY = (double)(ClientRt.bottom - ClientRt.top) / iMax;

		Greedint[0] = iMax / 4;							// 첫 그리드에 표시될 숫자
		Greedint[1] = Greedint[0] + Greedint[0];		// 두 번째 그리드에 표시될 숫자
		Greedint[2] = Greedint[1] + Greedint[0];		// 세 번째 그리드에 표시될 숫자
		Greedint[3] = iMax;								// 네 번째 그리드에 표시될 숫자
	}

	this->AleCount = ClientRt.bottom - (AleCount * iAddY);	// 알림이 울릴 값 세팅

	/////////////////////////////////////////////////////////
	//  MemDC와 리소스 세팅
	/////////////////////////////////////////////////////////
	HDC hdc = GetDC(hWnd);
	MemDC = CreateCompatibleDC(hdc);

	// 비트맵
	MyBitmap = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);	
	OldBitmap = (HBITMAP)SelectObject(MemDC, MyBitmap);

	// 브러시
	BackBrush = CreateSolidBrush(BackColor);
	int r, g, b;
	r = GetRValue(BackColor);
	g = GetGValue(BackColor);
	b = GetBValue(BackColor);
	r /= 2;
	g /= 2;
	b /= 2;
	TitleBrush = CreateSolidBrush(RGB(r, g, b));

	// 펜
	r = GetRValue(BackColor);
	g = GetGValue(BackColor);
	b = GetBValue(BackColor);
	r *= 3;
	g *= 3;
	b *= 3;
	GraphPen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
	SelectObject(MemDC, GraphPen);
	
	// 폰트
	iFontR = r, iFontG = g, iFontB = b;
	MyFont = CreateFont(17, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0,
		VARIABLE_PITCH | FF_ROMAN, L"Arial"); 

	SetBkMode(MemDC, TRANSPARENT);	

	ReleaseDC(hWnd, hdc);	

	/////////////////////////////////////////////////////////
	//  큐 세팅
	/////////////////////////////////////////////////////////
	Init(&queue);			
	Enqueue(&queue, 0);

	UpdateWindow(hWnd);
}

// 소멸자
CMonitorGraphUnit::~CMonitorGraphUnit()
{
	SelectObject(MemDC, OldBitmap);
	DeleteObject(MyBitmap);

	DeleteObject(BackBrush);
	DeleteObject(TitleBrush);

	SelectObject(MemDC, GetStockObject(WHITE_PEN));
	DeleteObject(GraphPen);

	SelectObject(MemDC, GetStockObject(DEFAULT_GUI_FONT));
	DeleteObject(MyFont);

	DeleteDC(MemDC);
}

// 자식들의 윈도우 프로시저
LRESULT CALLBACK CMonitorGraphUnit::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static double yTemp;
	static double Temp;
	static TCHAR GreedText[20];

	pThis = (CMonitorGraphUnit*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_PAINT:
	{		
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);	

		// 그리드, 배경채우기 등 기본 UI 세팅
		pThis->UISet();

		// 실제 출력
		switch (pThis->_enGraphType)
		{
		case BAR_SINGLE_VERT:
			break;

		case BAR_SINGLE_HORZ:
			break;

		case BAR_COLUMN_VERT:
			break;

		case  BAR_COLUMN_HORZ:
			break;

		case LINE_SINGLE:
			pThis->Paint_LINE_SINGLE();
			break;

		case LINE_MULTI:
			break;

		case PIE:
			break;

		}		
		BitBlt(hdc, pThis->rt.left, pThis->rt.top, pThis->rt.right, pThis->rt.bottom, pThis->MemDC, pThis->rt.left, pThis->rt.top, SRCCOPY);
		
		EndPaint(hWnd, &ps);
	}
	break;
	
	case WM_DESTROY:	
		DeleteObject(pThis->BackBrush);
		PostQuitMessage(0);		
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// 과거 데이터 넣기

//void CMonitorGraphUnit::InsertData(int iData, double* a)
//{
//	int randCheck = rand() % 2;	// 더할것인지 뺼것인지 둘 중하나 랜덤결정
//	double yTemp = y;
//
//	if (randCheck == 0)	// 0이면 뺸다.
//	{
//		yTemp -= (iAddY * iData);
//		if (ClientRt.top > yTemp || ClientRt.bottom < yTemp)	// 뻇는데, 화면을 벗어난다면, 더한다. 벗어나지 않을 때 까지.
//		{
//			yTemp = y;
//			yTemp += (iAddY * iData);
//		}
//	}
//	else  // 1이면 더한다.
//	{
//		yTemp += (iAddY * iData);
//		if (ClientRt.top > yTemp || ClientRt.bottom < yTemp)	//더했는데 화면을 벗어난다면, 뺸다. 벗어나지 않을 때 까지
//		{
//			yTemp = y;
//			yTemp -= (iAddY * iData);
//		}			
//	}
//	y = yTemp;
//	*a =  (ClientRt.bottom - y) / iAddY;	// ClientRt.bottom - y는 내가 지정한 논리 1칸이다.
//											// 여기에 iAddY를 나누면, (ClientRt.bottom - y) 안에 iAddY가 몇개가 있는지 알 수 있다.
//											// 즉, 실제 수를 카운트 할 수 있다.
//	Enqueue(&queue, (int)y);	// 계산된 y를 인큐한다.	
//	InvalidateRect(hWnd, NULL, false);	// WM_PAINT 호출	
//}

// 데이터 넣기.
void CMonitorGraphUnit::InsertData(int iData, double* a)
{	
	// 1씩 증가하기 할 경우, 사용하는 로직.	
	//aaa += iData;	
	//if (aaa >= iMax)
		//aaa = 0;
	//Enqueue(&queue, aaa);	

	Enqueue(&queue, iData);	// iData를 인큐한다.	

	// 인큐 한 데이터를 기준으로 부모의 백그라운드 빨간색으로 칠하기 체크
	//ParentBackCheck(ClientRt.bottom - (aaa * iAddY));
	ParentBackCheck(ClientRt.bottom - (iData * iAddY));

	InvalidateRect(hWnd, NULL, false);	// WM_PAINT 호출	
}

// LINE_SINGLE 출력
void CMonitorGraphUnit::Paint_LINE_SINGLE(void)
{
	double x, y;
	int data;	// 디큐 시, 저장할 y값.
	if (FirstPeek(&queue, &data))	// 큐의 가장 처음에 있는 y값을 디큐한다.
	{
		x = ClientRt.left;	// 매번 그릴 때 마다, 가장 좌측<<부터 그려야 하기 떄문에 0으로 만든다.
		y = ClientRt.bottom - (iAddY * data);			// 디큐한 값을, 내가 지정한 논리 y좌표 1로 계산한 후, 시작 위치에서 뺀다. 
		MoveToEx(MemDC, x, y, NULL);			// 처음 시작 위치 지정.	
		while (NextPeek(&queue, &data))	// 다음 y값 디큐.
		{
			x += (double)(ClientRt.right - ClientRt.left) / 100;	// 작업 영역의 너비를 100등분으로 나눠서, 1등분 간격으로 찍는다.
			y = ClientRt.bottom - (iAddY * data);		// 디큐한 값을 기준으로, 내가 지정한 논리 좌표만큼 다시 이동
			LineTo(MemDC, x, y);	// 그리고 그 값을 이전 라인부터 긋는다.
		}
	}
}

// 부모의 백그라운드 빨간색으로 칠하기
void CMonitorGraphUnit::ParentBackCheck(int data)
{
	// 마지막에 디큐한 Data가 알람보다 작다면, 부모에게 화면 빨갛게 하라고 무조건 메시지를 보낸다. 
	// 이미 빨간색인지 아닌지는 부모가 체크
	if (data < AleCount)
	{
		SendMessage(hWndParent, UM_PARENTBACKCHANGE, 0, 0);	// 부모에게 화면 빨갛게 하라고 전달.

		// 부모에게 빨갛게 하라고 전달한 후, 폰트의 색 변경. 
		// 현재 알람 수치보다 적은 값일 경우, 폰트 색 빨간색으로 변경.
		// 즉, 모든 윈도우가 폰트 빨간색으로 될 수도 있다.
		// 이미 전달했다면(bObjectCheck가 true) if문 작동 안함.
		if (!bObjectCheck)
		{
			bObjectCheck = true;
			SetTextColor(MemDC, RGB(255, 0, 0));
		}
	}

	// Data가 알람보다 크다면, 다시 내 폰트를 원래대로 복구. 
	else
	{
		bObjectCheck = false;
		SetTextColor(MemDC, RGB(iFontR, iFontG, iFontB));
	}

}

// 기본 UI 세팅 함수
void CMonitorGraphUnit::UISet()
{
	static double yTemp;
	static double Temp;
	static TCHAR GreedText[20];
	int data;

	// 타이틀, 화면좌측, 작업공간 채우기
	FillRect(MemDC, &TitleRt, TitleBrush);
	FillRect(MemDC, &LeftRt,BackBrush);
	FillRect(MemDC, &ClientRt, BackBrush);

	// 타이틀 텍스트 출력
	SelectObject(MemDC, MyFont);
	TextOut(MemDC, TitleRt.top + 5, TitleRt.left + 5, tWinName, lstrlen(tWinName));

	//  Max가 가변인 경우, 그래프 Max값을 매번 세팅
	if (MaxVariable == true)
	{
		// 일단, 모든 큐를 Peek 하면서 가장 큰 값 1개를 찾아낸다.
		if (FirstPeek(&queue, &data))
		{
			iMax = data;
			while (NextPeek(&queue, &data))
			{
				if(data > iMax)
					iMax = data;
			}
		}
		// 여기까지 오면 iMax에는 현재 큐의 가장 큰 값이 들어 있다.
		
		// 만약, iMax가 10보다 작다면, 그냥 10으로 통일.. 그리드에 표시할 숫자가 애매해짐.
		if (iMax < 10)
			iMax = 10;

		// 그리드에 표시할 숫자 지정
		Greedint[0] = iMax / 4;							// 첫 그리드에 표시될 숫자
		Greedint[1] = Greedint[0] + Greedint[0];		// 두 번째 그리드에 표시될 숫자
		Greedint[2] = Greedint[1] + Greedint[0];		// 세 번째 그리드에 표시될 숫자
		Greedint[3] = iMax;								// 네 번째 그리드에 표시될 숫자

		// 작업 영역 기준, y 1칸 이동 시, 실제 이동해야하는 위치 계산
		iAddY = (double)(ClientRt.bottom - ClientRt.top) / iMax;

		// 알림이 울릴 값 다시 세팅. 그냥 작업 영역의 절반으로 했다.
		AleCount = (ClientRt.bottom - ClientRt.top) / 2;	
	}

	// 그리드 선 긋기, 텍스트 출력하기
	yTemp = ClientRt.bottom;
	Temp = (double)(ClientRt.bottom - ClientRt.top) / 4;
	SelectObject(MemDC, MyFont);
	for (int i = 0; i < 4; ++i)
	{
		yTemp -= Temp;
		MoveToEx(MemDC, ClientRt.left, (int)yTemp, NULL);
		LineTo(MemDC, ClientRt.right, (int)yTemp);

		// 그리드 별 수치값 출력하기
		_itow_s(Greedint[i], GreedText, _countof(GreedText), 10);
		TextOut(MemDC, LeftRt.left + 5, (int)yTemp - 6, GreedText, lstrlen(GreedText));
	}

}