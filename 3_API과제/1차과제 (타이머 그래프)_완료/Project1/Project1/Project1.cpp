// Project1.cpp: 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Project1.h"

#define QUEUE_LEN 100

// 전역 변수
HINSTANCE hInst; // 현재 인스턴스입니다.

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// 큐 모음
typedef struct
{
	int front;
	int rear;
	int Peek;
	int queArr[QUEUE_LEN];
}Queue;
void Init(Queue* pq);
int NextPos(int pos);
int Dequeue(Queue* pq);
void Enqueue(Queue* pq, int y);
bool FirstPeek(Queue* pq, int* Data);
bool NextPeek(Queue* pq, int* Data);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	// TODO: 여기에 코드를 입력합니다.

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
	wcex.lpszClassName = TEXT("1차과제(타이머 그래프)");
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassExW(&wcex);


	// 응용 프로그램 초기화를 수행합니다.

	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(TEXT("1차과제(타이머 그래프)"), TEXT("타이머 그래프"), WS_OVERLAPPEDWINDOW,
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
	static int x, y;		// x,y 좌표
	static int randCheck;	// y값을 더할것인지 뺼것인지 결정값을 저장하는 변수
	static RECT rt;		// 작업영역 저장
	static Queue queue;	// 큐

	switch (message)
	{
	case WM_CREATE:
		SetTimer(hWnd, 1, 40, NULL);	// 타이머 세팅
		GetClientRect(hWnd, &rt);		// 작업 영역 구한다.
		y = rt.bottom - (rt.bottom/2);			// 작업 영역 기준, 중간 즈음의 값을 y에 넣는다. 이는, 처음 시작 위치를 지정한다.
		Init(&queue);							// 큐 초기화.
		Enqueue(&queue, y);						// 세팅한 y(처음 시작 위치)를 큐에 넣는다.
		break;

	case WM_COMMAND:
	{
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
		GetClientRect(hWnd, &rt);		// 작업 영역을 다시 구한다.
		break;
	
	case WM_TIMER:
		randCheck = rand() % 2;	// 더할것인지 뺼것인지 둘 중하나 랜덤결정
		if (randCheck == 0)	// 0이면 뺸다.
		{
			y -= rand() % 30;
			while(rt.top > y || rt.bottom < y)	// 뻇는데, 화면을 벗어난다면, 더한다. 벗어나지 않을 때 까지.
				y += rand() % 30;
		}
		else  // 1이면 더한다.
		{
			y += rand() % 30;
			while (rt.top > y || rt.bottom < y)	//더했는데 화면을 벗어난다면, 뺸다. 벗어나지 않을 때 까지
				y -= rand() % 30;
		}
		Enqueue(&queue, y);	// 계산된 y를 인큐한다.
		InvalidateRect(hWnd, NULL, true);	// WM_PAINT 호출
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		int data;	// 디큐 시, 저장할 y값.
		if (FirstPeek(&queue,&data))	// 큐의 가장 처음에 있는 y값을 디큐한다.
		{
			x = 0;	// 매번 그릴 때 마다, 가장 좌측<<부터 그려야 하기 떄문에 0으로 만듦.
			MoveToEx(hdc, x, data, NULL);			// 처음 시작 위치를 지정.	
			while (NextPeek(&queue, &data))	// 다음 y값 디큐.
			{
				x += rt.right/150;	// 윈도우의 너비를 150등분으로 나눠서, 1등분 간격으로 찍는다.
				LineTo(hdc, x, data);	// 디큐한 y값을 사용해 이전 위치부터 긋는다.
			}
		}
		EndPaint(hWnd, &ps);
	}
	break;

	case WM_DESTROY:
		KillTimer(hWnd, 1);	// 사용한 타이머 제거. 안해도 되지만 안전하게..
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


void Init(Queue* pq)
{
	pq->front = 0;
	pq->rear = 0;
}

int NextPos(int pos)
{
	if (pos == QUEUE_LEN - 1)
		return 0;
	else
		return pos + 1;
}

int Dequeue(Queue* pq)
{
	pq->front = NextPos(pq->front);
	return pq->queArr[pq->front];
}

void Enqueue(Queue* pq, int y)
{
	if (pq->front == NextPos(pq->rear))
	{
		Dequeue(pq);
	}
	pq->rear = NextPos(pq->rear);
	pq->queArr[pq->rear] = y;
}

bool FirstPeek(Queue* pq, int* Data)
{
	if (pq->front == pq->rear)
		return false;

	pq->Peek = NextPos(pq->front);

	*Data = pq->queArr[pq->Peek];
	return true;

}

bool NextPeek(Queue* pq, int* Data)
{
	if (pq->Peek == pq->rear)
		return false;

	pq->Peek = NextPos(pq->Peek);

	*Data = pq->queArr[pq->Peek];
	return true;

}