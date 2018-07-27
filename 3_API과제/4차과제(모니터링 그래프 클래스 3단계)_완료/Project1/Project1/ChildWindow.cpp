#include "stdafx.h"
#include "ChildWindow.h"

//// 자식 윈도우 클래스 CPP ////////////////////////////
CMonitorGraphUnit *CMonitorGraphUnit::pThis;

// 생성자
CMonitorGraphUnit::CMonitorGraphUnit(HINSTANCE hInstance, HWND hWndParent, COLORREF BackColor, TYPE enType, int iPosX, int iPosY, int iWidth, int iHeight, LPCTSTR str)
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

	// 만약 enType(윈도우 타입)이 [LINE_MULTI] 라면, 화면 우측 공간 세팅. 그리고 Client영역도 다시 세팅.
	if (enType == LINE_MULTI)
	{
		RightRt = rt;
		RightRt.top = TitleRt.bottom;
		RightRt.left = ClientRt.right - 100;

		ClientRt.right = RightRt.left;
	}

	// 만약 enType(윈도우 타입)이[BAR_COLUMN_VERT] 라면, 화면 하단 공간 세팅. Client영역도 다시 세팅.
	else if (enType == BAR_COLUMN_VERT)
	{
		BottomRt = rt;
		BottomRt.top = ClientRt.bottom - 40;
		BottomRt.left = LeftRt.right;

		ClientRt.bottom = BottomRt.top;
	}

	/////////////////////////////////////////////////////////
	//  클래스 멤버변수에 필요한 값 세팅
	/////////////////////////////////////////////////////////
	_enGraphType = enType;
	this->BackColor = BackColor;
	this->hWndParent = hWndParent;
	this->hWnd = hWnd;
	this->hInstance = hInstance;
	wcscpy_s(tWinName, _countof(tWinName), str);
	
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

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

	// 펜. 총 5개의 펜을 만들어둔다. 5개를 동적할당한다.
	r = GetRValue(BackColor);
	g = GetGValue(BackColor);
	b = GetBValue(BackColor);
	r *= 3;
	g *= 3;
	b *= 3;

	GraphPen = new HPEN[5];	
	GraphPen[0] = CreatePen(PS_SOLID, 1, RGB(r, g, b));			// 내가 쓰는 기본 펜 색
	GraphPen[1] = CreatePen(PS_SOLID, 1, RGB(0, 84, 255));		// 파란 계열
	GraphPen[2] = CreatePen(PS_SOLID, 1, RGB(209, 178, 255));	// 보라 계열
	GraphPen[3] = CreatePen(PS_SOLID, 1, RGB(255, 228, 0));		// 노란 계열
	GraphPen[4] = CreatePen(PS_SOLID, 1, RGB(255, 94, 0));		// 빨간 계열

	SelectObject(MemDC, GraphPen[0]);	// 펜은 기본 0번펜을 사용한다.
	
	// 폰트
	NormalFontColor = RGB(r, g, b);
	MyFont = CreateFont(17, 0, 0, 0, 0, 0, 0, 0,
		DEFAULT_CHARSET, 0, 0, 0,
		VARIABLE_PITCH | FF_ROMAN, L"Arial"); 

	SetBkMode(MemDC, TRANSPARENT);	
	NowFontColor = NormalFontColor;
	SetTextColor(MemDC, NowFontColor);

	ReleaseDC(hWnd, hdc);	

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
	for(int i=0; i<5; ++i)
		DeleteObject(GraphPen[i]);

	SelectObject(MemDC, GetStockObject(DEFAULT_GUI_FONT));
	DeleteObject(MyFont);

	DeleteDC(MemDC);
}

// 자식들의 윈도우 프로시저
LRESULT CALLBACK CMonitorGraphUnit::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	pThis = (CMonitorGraphUnit*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (message)
	{
	case WM_PAINT:
	{		
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);	

		// 그리드 등 기본 UI 세팅
		pThis->UISet();

		// 실제 출력
		switch (pThis->_enGraphType)
		{
		case BAR_SINGLE_VERT:
			pThis->Paint_BAR_SINGLE_VERT();
			break;

		case BAR_SINGLE_HORZ:
			break;

		case BAR_COLUMN_VERT:
			pThis->Paint_BAR_COLUMN_VERT();
			break;

		case  BAR_COLUMN_HORZ:
			break;

		case LINE_SINGLE:
			pThis->Paint_LINE_SINGLE();
			break;

		case LINE_MULTI:
			pThis->Paint_LINE_MULTI();
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

// 데이터 넣기.
void CMonitorGraphUnit::InsertData(int iData, int ServerID, int DataType)
{	
	for (int i = 0; i < iColumnCount; ++i)
	{
		// for문을 돌면서, ServerID,DataType 둘 다 동일한 컬럼이 있는 경우 데이터를 넣고 로직 진행
		if (CoInfo[i].ServerID == ServerID && CoInfo[i].DataType == DataType)
		{
			// 가장 마지막에 들어온 값을 저장해 둔다.
			// 캡션바에 표시될 값.
			CoInfo[i].iLastVal = iData;

			// iData를 인큐한다.
			Enqueue(&CoInfo[i].queue, iData);

			// true면 부모의 화면 빨갛게 칠하기를 체크 한다. false면 안함.
			if (AleOnOff == true)
				ParentBackCheck(iData);
		}
	}

	// 자신(자식)의 WM_PAINT 호출
	InvalidateRect(hWnd, NULL, false);
}

// 추가 데이터 세팅. 순서대로 [Max값 , 알람 울리는 값, 표시할 단위]
void CMonitorGraphUnit::AddData(int iMax, int AleCount, LPCTSTR Unit)
{
	this->iMax = iMax;

	// iMax가 0이면 iMax 가 고정이 아니라는 것.
	if (iMax == 0)
		MaxVariable = true;
	else
		MaxVariable = false;

	/////////////////////////////////////////////////////////
	//  그래프 Max값 세팅
	/////////////////////////////////////////////////////////
	// Max가 0이 아니라면, 값 세팅. 0이라면 가변이라는 뜻이기 때문에 매번 체크해줘야 한다.
	if (!MaxVariable)
	{
		// 작업 영역 기준, y 1칸 이동 시, 실제 이동해야하는 위치 계산
		iAddY = (double)(ClientRt.bottom - ClientRt.top) / iMax;

		Greedint[0] = iMax / 4;							// 첫 그리드에 표시될 숫자
		Greedint[1] = Greedint[0] + Greedint[0];		// 두 번째 그리드에 표시될 숫자
		Greedint[2] = Greedint[1] + Greedint[0];		// 세 번째 그리드에 표시될 숫자
		Greedint[3] = iMax;								// 네 번째 그리드에 표시될 숫자
	}

	/////////////////////////////////////////////////////////
	// 알림이 울릴 값 세팅
	/////////////////////////////////////////////////////////
	// AleCount값이 0이면 알람 울리지 않음.
	if (AleCount == 0)
		AleOnOff = false;
	else
	{
		AleOnOff = true;
		this->AleCount = AleCount;
	}

	/////////////////////////////////////////////////////////
	// 표시할 단위 값 저장.
	/////////////////////////////////////////////////////////
	// Unit의 문자가 NULL이라면, 해당 윈도우는 단위를 사용하지 않는다는 의미. 
	// 그래서 Unit의 문자가 NULL이 아닐 때만 값을 복사한다.
	// 값을 복사하지 않으면 디폴트로 tUnit에는 값이 비어있다.
	if (_tcscmp(Unit, L"NULL"))
		_tcscpy_s(tUnit, _countof(tUnit), Unit);
		
}

// 컬럼 정보 최초 세팅
void CMonitorGraphUnit::SetColumnInfo(int iColumnCount, int ServerID[], int DataType[], TCHAR DataName[][10])
{	
	// 컬럼 카운트 저장	
	this->iColumnCount = iColumnCount;

	// 컬럼 하나당 [서버 ID, 데이터 타입, 데이터 이름, 큐] 총 4개 정보가 할당된다.
	CoInfo = new ColumnInfo[iColumnCount];

	// 컬럼 카운트 만큼 반복하면서 정보를 채운다.
	for (int i = 0; i < iColumnCount; ++i)
	{
		CoInfo[i].ServerID = ServerID[i];	// 서버 ID 채우기
		CoInfo[i].DataType = DataType[i];	// 데이터 타입 채우기
		_tcscpy_s(CoInfo[i].DataName, _countof(CoInfo[i].DataName), DataName[i]);// 데이터 이름 채우기
		Init(&CoInfo[i].queue);			// 큐 초기화
		Enqueue(&CoInfo[i].queue, 0);
	}

}

// LINE_SINGLE 출력
void CMonitorGraphUnit::Paint_LINE_SINGLE(void)
{
	double x, y;
	int data;	// 디큐 시, 저장할 y값.	
	TCHAR tTitleText[30];	// 완성된 타이틀 이름.

	// 타이틀 텍스트 출력
	// 윈도우 이름 : 큐에서 가장 큰 값, 표시단위 형태로 문자열 완성
	swprintf_s(tTitleText, _countof(tTitleText), _T("%s : %d %s"), tWinName, CoInfo[0].iLastVal, tUnit);
	SelectObject(MemDC, MyFont);
	TextOut(MemDC, TitleRt.top + 5, TitleRt.left + 5, tTitleText, lstrlen(tTitleText));

	// 그래프 출력
	if (FirstPeek(&CoInfo[0].queue, &data))	// 큐의 가장 처음에 있는 y값을 디큐한다.
	{
		x = ClientRt.left;	// 매번 그릴 때 마다, 가장 좌측<<부터 그려야 하기 떄문에 0으로 만든다.
		y = ClientRt.bottom - (iAddY * data);			// 디큐한 값을, 내가 지정한 논리 y좌표 1로 계산한 후, 시작 위치에서 뺀다. 
		MoveToEx(MemDC, x, y, NULL);			// 처음 시작 위치 지정.	
		while (NextPeek(&CoInfo[0].queue, &data))	// 다음 y값 디큐.
		{
			x += (double)(ClientRt.right - ClientRt.left) / 100;	// 작업 영역의 너비를 100등분으로 나눠서, 1등분 간격으로 찍는다.
			y = ClientRt.bottom - (iAddY * data);		// 디큐한 값을 기준으로, 내가 지정한 논리 좌표만큼 다시 이동
			LineTo(MemDC, x, y);	// 그리고 그 값을 이전 라인부터 긋는다.
		}
	}
}

// Paint_LINE_MULTI 출력
void CMonitorGraphUnit::Paint_LINE_MULTI()
{
	double x, y;
	int data;	// 디큐 시, 저장할 y값.	
	TCHAR tTitleText[30];	// 완성된 타이틀 이름.
	int TextLineY = 20;

	// 화면 우측 영역을 칠한다. 여기서 하는 이유는, 화면 우측 영역은 LINE_MULTI에서만 쓰기 때문에.
	FillRect(MemDC, &RightRt, BackBrush);

	// 타이틀 텍스트 출력
	// 윈도우 이름 :  표시단위 형태로 문자열 완성
	swprintf_s(tTitleText, _countof(tTitleText), _T("%s (%s)"), tWinName, tUnit);
	SelectObject(MemDC, MyFont);
	TextOut(MemDC, TitleRt.top + 5, TitleRt.left + 5, tTitleText, lstrlen(tTitleText));

	// 그래프 출력
	for (int i = 0; i < iColumnCount; ++i)
	{
		if (FirstPeek(&CoInfo[i].queue, &data))	// 큐의 가장 처음에 있는 y값을 디큐한다.
		{
			x = ClientRt.left;	// 매번 그릴 때 마다, 가장 좌측<<부터 그려야 하기 떄문에 0으로 만든다.
			y = ClientRt.bottom - (iAddY * data);			// 디큐한 값을, 내가 지정한 논리 y좌표 1로 계산한 후, 시작 위치에서 뺀다. 
			MoveToEx(MemDC, x, y, NULL);			// 처음 시작 위치 지정.	
			while (NextPeek(&CoInfo[i].queue, &data))	// 다음 y값 디큐.
			{
				x += (double)(ClientRt.right - ClientRt.left)/ 100;	// 작업 영역의 너비를 100등분으로 나눠서, 1등분 간격으로 찍는다.
				y = ClientRt.bottom - (iAddY * data);		// 디큐한 값을 기준으로, 내가 지정한 논리 좌표만큼 다시 이동
				LineTo(MemDC, x, y);	// 그리고 그 값을 이전 라인부터 긋는다.
			}
		}

		MoveToEx(MemDC, RightRt.left + 10, RightRt.top + TextLineY, NULL);	// 화면 우측에 그래프 정보를 알려주는 라인을 긋는다.
		LineTo(MemDC, RightRt.left +25, RightRt.top + TextLineY);

		//SetTextAlign(MemDC, TA_CENTER);
		TextOut(MemDC, RightRt.left + 29, RightRt.top + TextLineY - 8, CoInfo[i].DataName, lstrlen(CoInfo[i].DataName));
		//SetTextAlign(MemDC, TA_TOP | TA_LEFT);

		TextLineY += 30;	// 다음 그래프 라인 위치를 위해 위치 이동

		SelectObject(MemDC, GraphPen[i+1]);	// 펜을, 다음 그래프의 색으로 변경
	}

	SelectObject(MemDC, GraphPen[0]);

}

// BAR_SINGLE_VERT 출력
void CMonitorGraphUnit::Paint_BAR_SINGLE_VERT()
{
	TCHAR tNowVal[20];
	TCHAR tTitleText[30];	// 완성된 타이틀 이름.

	// 타이틀 텍스트 출력 (윈도우 이름 : 값, 타입)
	swprintf_s(tTitleText, _countof(tTitleText), _T("%s : %d %s"), tWinName, CoInfo[0].iLastVal, tUnit);
	SelectObject(MemDC, MyFont);
	TextOut(MemDC, TitleRt.top + 5, TitleRt.left + 5, tTitleText, lstrlen(tTitleText));

	/////////////////////////////////////////////////////////
	//  세로 바 출력
	/////////////////////////////////////////////////////////
	// 세로 바는 가장 최근의 값을 기준으로 사각형을 그리면 된다.	

	HBRUSH BarBrush = CreateSolidBrush(RGB(255, 228, 0));		// Bar를 칠할 노란색 브러시 제작
	HBRUSH OldBrush = (HBRUSH)SelectObject(MemDC, BarBrush);	// 노란색 브러시 적용. 이전 브러시는 OldBrush에 들어있다.
	HPEN OldPen = (HPEN)SelectObject(MemDC, GetStockObject(NULL_PEN));	// 펜을 투명한 것으로 세팅.

	// Ractangle 할 사각형의 왼쪽 , 오른쪽 위치를 구한다.
	int RectLx = ClientRt.left + 20;
	int RectRx = ClientRt.right - 20;

	Rectangle(MemDC, RectLx, ClientRt.bottom - (CoInfo[0].iLastVal *iAddY), RectRx, ClientRt.bottom);	// 새로 바 그리기

	SelectObject(MemDC, OldBrush);	// 노란색 브러시 선택 해제.
	SelectObject(MemDC, OldPen);	// 투명 펜 선택 해제.

	DeleteObject(BarBrush);	//노란색 브러시 삭제. 투명 펜은 GetStockObject에서 가져다 썻으니 안지워도 됨.

	/////////////////////////////////////////////////////////
	//  세로 바의 가운데에 현재 값 출력.	
	/////////////////////////////////////////////////////////
	SetTextColor(MemDC, RGB(0, 0, 0));	// 폰트 색을 검정색으로 변경 
	HFONT BarFont = CreateFont(30, 0, 0, 0, 0, 0, 0, 0,	// Bar에 현재값을 표시할 커다란 폰트 제작.
		DEFAULT_CHARSET, 0, 0, 0,
		VARIABLE_PITCH | FF_ROMAN, L"Arial");	

	SelectObject(MemDC, BarFont);		// 만든 커다란 폰트 적용

	_itot_s(CoInfo[0].iLastVal, tNowVal, _countof(tNowVal), 10);		// 현재 값을 TCHAR형으로 변경	
	
	SetTextAlign(MemDC, TA_CENTER);	// 텍스트 출력 모드를 TA_CENTER로 변경. TextOut에서 지정하는 좌표가 중앙 위치가 된다.
	TextOut(MemDC, RectLx + ((RectRx - RectLx) / 2), ClientRt.bottom - (CoInfo[0].iLastVal *iAddY) / 2, tNowVal, lstrlen(tNowVal));	// 텍스트 출력
	SetTextAlign(MemDC, TA_TOP | TA_LEFT);	// 텍스트 출력 모드를 다시 디폴트로 변경. (디폴트는 TA_TOP | TA_LEFT 이다)
		
	SetTextColor(MemDC, NowFontColor);	// 폰트 색 쓰던 색으로 변경
	SelectObject(MemDC, MyFont);	// 본래 폰트(MyFont) 적용.
	DeleteObject(BarFont);			// 다 쓴 커다란 폰트 삭제
}

// BAR_COLUMN_VERT 출력
void CMonitorGraphUnit::Paint_BAR_COLUMN_VERT()
{
	TCHAR tNowVal[20];
	TCHAR tTitleText[30];	// 완성된 타이틀 이름.

	// 화면 하단 영역을 칠한다. 여기서 하는 이유는, 화면 하단 영역은 BAR_COLUMN_VERT에서만 쓰기 때문에.
	FillRect(MemDC, &BottomRt, BackBrush);

	// 타이틀 텍스트 출력 (윈도우 이름만 표시)
	swprintf_s(tTitleText, _countof(tTitleText), _T("%s"), tWinName);
	SelectObject(MemDC, MyFont);
	TextOut(MemDC, TitleRt.top + 5, TitleRt.left + 5, tTitleText, lstrlen(tTitleText));

	/////////////////////////////////////////////////////////
	//  세로 바 출력
	/////////////////////////////////////////////////////////
	// BAR는 가장 최근의 값을 기준으로 사각형을 그리면 된다.	

	HBRUSH BarBrush = CreateSolidBrush(RGB(255, 228, 0));		// Bar를 칠할 노란색 브러시 제작
	HBRUSH OldBrush = (HBRUSH)SelectObject(MemDC, BarBrush);	// 노란색 브러시 적용. 이전 브러시는 OldBrush에 들어있다.
	HPEN OldPen = (HPEN)SelectObject(MemDC, GetStockObject(NULL_PEN));	// 펜을 투명한 것으로 세팅.
	HFONT BarFont = CreateFont(30, 0, 0, 0, 0, 0, 0, 0,	// Bar에 현재값을 표시할 커다란 폰트 제작.
		DEFAULT_CHARSET, 0, 0, 0,
		VARIABLE_PITCH | FF_ROMAN, L"Arial");
	SelectObject(MemDC, BarFont);		// 만든 커다란 폰트 적용
	
	// Ractangle 할 최초 사각형의 왼쪽 , 오른쪽 위치를 구한다.
	int RectLx = ClientRt.left + 20;
	int RectRx = ClientRt.left + 120;

	SetTextColor(MemDC, RGB(0, 0, 0));	// 폰트 색을 검정색으로 변경 	
	SetTextAlign(MemDC, TA_CENTER);	// 텍스트 출력 모드를 TA_CENTER로 변경. TextOut에서 지정하는 좌표가 중앙 위치가 된다.

	// 컬럼 수 만큼 반복하면서 사각형을 그리기 / 바에 값 텍스트 출력
	for (int i = 0; i < iColumnCount; ++i)
	{
		Rectangle(MemDC, RectLx, ClientRt.bottom - (CoInfo[i].iLastVal *iAddY), RectRx, ClientRt.bottom);	// 새로 바 그리기
		_itot_s(CoInfo[i].iLastVal, tNowVal, _countof(tNowVal), 10);		// 현재 큐의 마지막 값을 TCHAR형으로 변경
		TextOut(MemDC, RectLx + ((RectRx - RectLx) / 2), ClientRt.bottom - (CoInfo[i].iLastVal *iAddY) / 2, tNowVal, lstrlen(tNowVal));	// Ractangle의 한 가운데에 텍스트 출력.

		RectLx = RectRx + 20;
		RectRx = RectLx + 120;
	}

	// 다시 컬럼 수 만큼 반복하면서 화면 하단에 데이터 타입 이름 출력하기. 사각형 그리기와 따로하는 이유는 텍스트 색과 폰트가 다르기 때문이다.
	// 계속 SelectObject와 SetTextColor를 반복하는 것 보다는 따로 빼는게 더 낫겠다는 판단이다.
	SetTextColor(MemDC, NowFontColor);	// 폰트 색 쓰던 색으로 변경
	SelectObject(MemDC, MyFont);	// 본래 폰트(MyFont) 적용.
	RectLx = ClientRt.left + 20;
	RectRx = ClientRt.left + 120;
	for (int i = 0; i < iColumnCount; ++i)
	{
		TextOut(MemDC, RectLx + ((RectRx - RectLx) / 2), BottomRt.bottom - 20, CoInfo[i].DataName, _tcslen(CoInfo[i].DataName));	// 화면 하단에 데이터 타입 이름 출력
		RectLx = RectRx + 20;
		RectRx = RectLx + 120;
	}

	SetTextAlign(MemDC, TA_TOP | TA_LEFT);	// 텍스트 출력 모드를 다시 디폴트로 변경. (디폴트는 TA_TOP | TA_LEFT 이다)	
	DeleteObject(BarFont);			// 다 쓴 커다란 폰트 삭제

	SelectObject(MemDC, OldBrush);	// 노란색 브러시 선택 해제.
	SelectObject(MemDC, OldPen);	// 투명 펜 선택 해제.
	DeleteObject(BarBrush);	//노란색 브러시 삭제. 투명 펜은 GetStockObject에서 가져다 썻으니 안지워도 됨.
}

// 부모의 백그라운드 빨간색으로 칠하기
void CMonitorGraphUnit::ParentBackCheck(int data)
{
	// 마지막에 디큐한 Data가 알람보다 작다면, 부모에게 화면 빨갛게 하라고 무조건 메시지를 보낸다. 
	// 이미 빨간색인지 아닌지는 부모가 체크
	if (data > AleCount)
	{
		SendMessage(hWndParent, UM_PARENTBACKCHANGE, 0, 0);	// 부모에게 화면 빨갛게 하라고 전달.

		// 부모에게 빨갛게 하라고 전달한 후, 폰트의 색 변경. 
		// 현재 알람 수치보다 적은 값일 경우, 폰트 색 빨간색으로 변경.
		// 즉, 모든 윈도우가 폰트 빨간색으로 될 수도 있다.
		// 이미 전달했다면(bObjectCheck가 true) if문 작동 안함.
		if (!bObjectCheck)
		{
			bObjectCheck = true;
			NowFontColor = RGB(255, 0, 0);
			SetTextColor(MemDC, NowFontColor);
			// Timer를 세팅한다. 빨간색 폰트 표시 시간을 위해.
		}
	}

	// Data가 알람보다 작다면, 다시 내 폰트를 원래대로 복구. 
	else
	{
		bObjectCheck = false;
		NowFontColor = NormalFontColor;
		SetTextColor(MemDC, NowFontColor);
	}
}

// 기본 UI 세팅 함수
void CMonitorGraphUnit::UISet()
{
	double yTemp;
	double Temp;
	TCHAR GreedText[20];
	int data;

	// 타이틀, 화면좌측, 작업공간 색 채우기
	FillRect(MemDC, &TitleRt, TitleBrush);
	FillRect(MemDC, &LeftRt, BackBrush);
	FillRect(MemDC, &ClientRt, BackBrush);

	//  Max가 가변인 경우, 그래프 Max값을 매번 세팅
	if (MaxVariable == true)
	{
		int iMax = 0;
		// 일단, 모든 큐를 Peek 하면서 가장 큰 값 1개를 찾아낸다.
		for (int i = 0; i < iColumnCount; ++i)
		{
			if (FirstPeek(&CoInfo[i].queue, &data))
			{
				if(iMax < data)
					iMax = data;
				while (NextPeek(&CoInfo[i].queue, &data))
				{
					if (data > iMax)
						iMax = data;
				}
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