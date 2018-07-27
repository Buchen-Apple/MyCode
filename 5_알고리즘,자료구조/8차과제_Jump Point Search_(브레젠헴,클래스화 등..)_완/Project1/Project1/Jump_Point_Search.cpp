#include "stdafx.h"
#include "Jump_Point_Search.h"
#include "Bresenham_Line.h"
#include <math.h>
#include <time.h>

#define CheckTile(X, Y)		\
	((&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType == OBSTACLE || X < 0 || X >= m_iWidth || Y < 0 || Y >= m_iHeight || iTempMaxSearchCount >= m_iMaxSearchCount)

// 생성자
CFindSearch::CFindSearch(Map* Copy, int Width, int Height, int Radius, int MaxSearchCount)
{
	// 직선 가중치, 대각선 가중치 값 셋팅
	m_dStraight_Line = 1;
	m_dDiagonal_Line = 2;

	m_iWidth = Width;
	m_iHeight = Height;
	m_iRadius = Radius;
	m_iMaxSearchCount = MaxSearchCount;

	StartNode = nullptr;
	FinNode = nullptr;

	// 맵의 사본을 보관한다.
	stMapArrayCopy = Copy;

	// Jump노드에 칠할 색 미리 셋팅
	Color1 = RGB(232, 217, 255);	// 보라 계열
	Color2 = RGB(250, 244, 192);	// 노란 계열
	Color3 = RGB(234, 234, 234);	// 회색 계열
	Color4 = RGB(255, 180, 180);	// 빨간 계열
	Color5 = RGB(119, 255, 112);	// 초록 계열
	Color6 = RGB(188, 229, 92);		// 초록 계열2
	Color7 = RGB(255, 178, 245);	// 분홍 계열
	Color8 = RGB(255, 166, 72);		// 주홍 계열
}

// 소멸자
CFindSearch::~CFindSearch()
{
	// 하는것 없음.
	DeleteObject(m_hMyBitmap);
	DeleteObject(m_hOldBitmap);
	DeleteDC(m_hMemDC);
}

// 윈도우 핸들을 전달받고 MemDC등을 셋팅하는 함수
void CFindSearch::DCSet(HWND hWnd)
{
	// 기존에 이미 호출된 적이 있으면, 기존에 할당되었던 MemDC 등을 Delete시켜준다.
	if (m_hWnd != NULL)
	{
		DeleteObject(m_hMyBitmap);
		DeleteObject(m_hOldBitmap);
		DeleteDC(m_hMemDC);
	}

	m_hWnd = hWnd;

	HDC hdc = GetDC(m_hWnd);
	m_hMemDC = CreateCompatibleDC(hdc);
	GetClientRect(m_hWnd, &rt);

	m_hMyBitmap = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);
	m_hOldBitmap = (HBITMAP)SelectObject(m_hMemDC, m_hMyBitmap);

	FillRect(m_hMemDC, &rt, (HBRUSH)GetStockObject(WHITE_BRUSH));
}

// 시작점, 도착점 셋팅하기, 열린/닫힌 리스트 초기화하기, 윈도우 핸들 기억하기.(그림그리기 위해)
void CFindSearch::Init(int StartX, int StartY, int FinX, int FinY)
{
	StartNode = new stFindSearchNode[sizeof(stFindSearchNode)];
	FinNode = new stFindSearchNode[sizeof(stFindSearchNode)];

	// x,y좌표 셋팅
	StartNode->m_iX = StartX;
	StartNode->m_iY = StartY;
	FinNode->m_iX = FinX;
	FinNode->m_iY = FinY;

	// 시작점의 G,H,F 셋팅
	StartNode->m_fG = 0;
	StartNode->m_fH = fabs((FinX - StartX) + fabs(FinY - StartY));
	StartNode->m_fF = StartNode->m_fG + StartNode->m_fH;
	StartNode->stpParent = nullptr;
	StartNode->dir = NONE;

	// 도착점의 G,H,F 셋팅
	FinNode->m_fG = 0;
	FinNode->m_fH = 0;
	FinNode->m_fF = 0;
	FinNode->stpParent = nullptr;
	FinNode->dir = NONE;

	// 열린 리스트, 닫힌 리스트 초기화
	m_CloseList.Clear();
	m_OpenList.Clear();
}

// 시작 전, 시작점을 오픈리스트에 넣는다.
void CFindSearch::StartNodeInsert()
{
	m_OpenList.Insert(StartNode);		
}

// 호출될 때 마다 Jump Point Search 진행
void CFindSearch::Jump_Point_Search_While()
{
	if (m_OpenList.GetCount() == 0)
	{
		// 리스트 클리어, 타이머 종료 등 처리
		LastClear();
		return;
	}

	// 2-1. 오픈리스트가 비어있지 않다면, 오픈 리스트에서 노드 하나 얻어옴.
	stFindSearchNode* Node = m_OpenList.GetListNode();

	// 2-2. 꺼낸 노드가 목적지라면 break;
	if (Node->m_iX == FinNode->m_iX &&
		Node->m_iY == FinNode->m_iY)
	{
		// Node가 목적지이니, 여기부터 부모를 타고가면서 선을 긋는다.
		Line(Node);		
		
		// 리스트 클리어, 타이머 종료 등 처리
		int x = StartNode->m_iX, y = StartNode->m_iY, x2 = FinNode->m_iX, y2 = FinNode->m_iY;
		LastClear();

		// 외부로 반환하는 용도로 사용하는, 라이브러리용 함수(경로만 뱉어냄)를 다시 호출해서 라인을 다시 그려봄. 어긋나지않는지!
		POINT point[100];
		int Count = PathGet(x, y, x2, y2, point, true);
		Out_Line(point, Count, RGB(255, 187, 0));

		return;
	}

	// 2-3. 목적지 노드가 아니면 꺼낸 노드를 CloseList에 넣기
	m_CloseList.Insert(Node);

	if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + Node->m_iX)->m_eTileType == OPEN_LIST)
		(&stMapArrayCopy[Node->m_iY * m_iWidth] + Node->m_iX)->m_eTileType = CLOSE_LIST;

	// 2-4. 그리고 노드 생성 함수 호출
	// 이 안에서 상황에 따라 타고 들어간다.
	CreateNodeCheck(Node);
}

// 인자로 받은 노드를 기준으로, 방향 지정
void CFindSearch::CreateNodeCheck(stFindSearchNode* Node, bool ShowCheck)
{
	// 이번 노드의 컬러 지정
	COLORREF colorRGB;
	switch (m_iColorKey)
	{
	case 1:
		colorRGB = Color1;
		m_iColorKey++;
		break;

	case 2:
		colorRGB = Color2;
		m_iColorKey++;
		break;

	case 3:
		colorRGB = Color3;
		m_iColorKey++;
		break;

	case 4:
		colorRGB = Color4;
		m_iColorKey++;
		break;

	case 5:
		colorRGB = Color5;
		m_iColorKey++;
		break;

	case 6:
		colorRGB = Color6;
		m_iColorKey++;
		break;

	case 7:
		colorRGB = Color7;
		m_iColorKey++;
		break;

	case 8:
		colorRGB = Color8;
		m_iColorKey = 1;
		break;

	default:
		break;
	}

	// 1. 인자로 받은 노드가 시작점 노드라면, 8방향으로 Jump하면서 노드 생성 여부 체크
	if (Node->stpParent == nullptr)
	{
		// LL
		CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node, colorRGB, LL);

		// RR
		CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, colorRGB, RR);

		// UU
		CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, colorRGB, UU);

		// DD
		CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, colorRGB, DD);

		// LU
		CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, colorRGB, LU);

		// LD
		CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, colorRGB, LD);

		// RU
		CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, colorRGB, RU);

		// RD
		CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, colorRGB, RD);
	}

	// 그게 아니라면, 직선 / 대각선에 따라 노드 생성 여부 체크
	else
	{
		// 상
		if (Node->dir == UU)
		{
			// 상 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, colorRGB, Node->dir);

			// 좌상 체크
			if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + (Node->m_iX - 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, colorRGB, LU);

			// 우상 체크
			if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + (Node->m_iX + 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, colorRGB, RU);
		}

		// 하
		else if (Node->dir == DD)
		{
			// 하 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, colorRGB, Node->dir);

			// 좌하 체크
			if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + (Node->m_iX - 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, colorRGB, LD);

			// 우하 체크
			if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + (Node->m_iX + 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, colorRGB, RD);
		}

		// 좌
		else if (Node->dir == LL)
		{
			// 좌 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node, colorRGB, Node->dir);

			// 좌상 체크
			if ((&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, colorRGB, LU);

			// 좌하 체크
			if ((&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, colorRGB, LD);
		}

		// 우
		else if (Node->dir == RR)
		{
			// 우 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, colorRGB, Node->dir);

			// 우상 체크

			if ((&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, colorRGB, RU);

			// 우하 체크
			if ((&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, colorRGB, RD);
		}

		// 좌상
		else if (Node->dir == LU)
		{
			// 왼쪽 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node, colorRGB, LL);

			// 위쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, colorRGB, UU);

			// 좌상 대각선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, colorRGB, Node->dir);

			// 우상 대각선
			if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + (Node->m_iX + 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, colorRGB, RU);

			// 좌하 대각선 
			if ((&stMapArrayCopy[(Node->m_iY + 1)* m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1)* m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, colorRGB, LD);
		}

		// 좌하
		else if (Node->dir == LD)
		{
			// 왼쪽 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node, colorRGB, LL);

			// 아래쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, colorRGB, DD);

			// 좌하 대각선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, colorRGB, Node->dir);

			// 좌상 대각선

			if ((&stMapArrayCopy[(Node->m_iY - 1)* m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1)* m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, colorRGB, LU);

			// 우하 대각선 
			if ((&stMapArrayCopy[Node->m_iY* m_iWidth] + (Node->m_iX + 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1)* m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, colorRGB, RD);
		}

		// 우상
		else if (Node->dir == RU)
		{
			// 오른쪽 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, colorRGB, RR);

			// 위쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, colorRGB, UU);

			// 우상 대각선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, colorRGB, Node->dir);

			// 좌상 대각선
			if ((&stMapArrayCopy[Node->m_iY* m_iWidth] + (Node->m_iX - 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, colorRGB, LU);

			// 우하 대각선 
			if ((&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, colorRGB, RD);
		}

		// 우하
		else if (Node->dir == RD)
		{
			// 오른쪽 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, colorRGB, RR);

			// 아래쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, colorRGB, DD);

			// 우하 대각선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, colorRGB, Node->dir);

			// 우상 대각선
			if ((&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + Node->m_iX)->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY - 1) * m_iWidth] + (Node->m_iX + 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, colorRGB, RU);

			// 좌하 대각선 
			if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + (Node->m_iX - 1))->m_eTileType == OBSTACLE &&
				(&stMapArrayCopy[(Node->m_iY + 1) * m_iWidth] + (Node->m_iX - 1))->m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, colorRGB, LD);
		}
	}
	if(ShowCheck)
		GridShow();
}

// 인자로 받은 노드를 기준으로, 어느 방향으로 이동할 것인지 결정 후, 결과에 따라 노드 생성 함수까지 호출.
void CFindSearch::CheckCreateJump(int X, int Y, stFindSearchNode* parent, COLORREF colorRGB, Direction dir)
{
	int OutX = 0, OutY = 0;
	double G = 0, OutG = 0;

	switch (dir)
	{
	case CFindSearch::LL:
		if (JumpLL(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);
		}

		break;

	case CFindSearch::LU:
		if (JumpLU(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);
		}

		break;

	case CFindSearch::LD:
		if (JumpLD(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);
		}

		break;

	case CFindSearch::RR:
		if (JumpRR(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);
		}

		break;

	case CFindSearch::RU:
		if (JumpRU(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);
		}

		break;

	case CFindSearch::RD:
		if (JumpRD(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);
		}

		break;

	case CFindSearch::UU:
		if (JumpUU(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);		
		}				

		break;

	case CFindSearch::DD:
		if (JumpDD(X, Y, &OutX, &OutY, G, &OutG, colorRGB, dir) == true)
		{
			OutG += parent->m_fG;
			CreateNode(OutX, OutY, parent, OutG, dir);
		}

		break;
	}
}

// 상 점프
bool CFindSearch::JumpUU(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{	
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		if (CheckTile(X, Y))
			return false;

		G += m_dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 목적지가 아니라면, 현재 노드가 코너인지 체크
		else if (((&stMapArrayCopy[Y * m_iWidth] + (X - 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) * m_iWidth] + (X - 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[Y * m_iWidth] + (X + 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) * m_iWidth] + (X + 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 코너가 아니라면, 해당 위치의 타일을 JUMP로 바꾼다. 확인했단는 의미로! 
		// 근데 시작점/도착점/오픈리스트/닫힌리스트에 있으면 안바꾼다. 
		// 그들은 고유하게 색이 유지되어야 한다.
		if ((&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y * m_iWidth] + X)->m_coColorRGB = colorRGB;
		}
		
		Y--;
		iTempMaxSearchCount++;
	}
	

	return true;
}

// 하 점프
bool CFindSearch::JumpDD(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		if (CheckTile(X, Y))
			return false;

		G += m_dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 현재 노드가 코너인지 체크
		else if (((&stMapArrayCopy[Y * m_iWidth] + (X - 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) * m_iWidth] + (X - 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[Y *  m_iWidth] + (X + 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) * m_iWidth] + (X + 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		if ((&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y * m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y * m_iWidth] + X)->m_coColorRGB = colorRGB;
		}

		Y++;
		iTempMaxSearchCount++;

	}
	
	return true;
}

// 좌 점프
bool CFindSearch::JumpLL(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		if (CheckTile(X, Y))
			return false;

		G += m_dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 현재 노드가 코너인지 체크
		else if (((&stMapArrayCopy[(Y - 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) *  m_iWidth] + (X - 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[(Y + 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) *  m_iWidth] + (X - 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		if ((&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_coColorRGB = colorRGB;
		}

		X--;
		iTempMaxSearchCount++;
	}

	return true;
}

// 우 점프
bool CFindSearch::JumpRR(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		if (CheckTile(X, Y))
			return false;

		G += m_dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 현재 노드가 코너인지 체크
		else if (((&stMapArrayCopy[(Y - 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) *  m_iWidth] + (X + 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[(Y + 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) *  m_iWidth] + (X + 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		if ((&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_coColorRGB = colorRGB;
		}

		X++;
		iTempMaxSearchCount++;

	}

	return true;
}

// 좌상 점프
bool CFindSearch::JumpLU(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		if (CheckTile(X, Y))
			return false;

		G += m_dDiagonal_Line;
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if (((&stMapArrayCopy[Y *  m_iWidth] + (X + 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) *  m_iWidth] + (X + 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[(Y + 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) *  m_iWidth] + (X - 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 왼쪽 직선 거리에 코너가 있는지 체크
		else if (JumpLL(X - 1, Y, OutX, OutY, G, OutG, colorRGB, LL) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 위 직선거리에 코너가 있는지 체크
		else if (JumpUU(X, Y - 1, OutX, OutY, G, OutG, colorRGB, UU) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		if ((&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_coColorRGB = colorRGB;
		}

		X--;
		Y--;
		iTempMaxSearchCount++;
	}
	return true;
}

// 좌하 점프
bool CFindSearch::JumpLD(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		if (CheckTile(X, Y))
			return false;

		G += m_dDiagonal_Line;
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 현재 위치가 코너인지 체크
		else if (((&stMapArrayCopy[(Y - 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) *  m_iWidth] + (X - 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[Y *  m_iWidth] + (X + 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) *  m_iWidth] + (X + 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 왼쪽 직선 거리에 코너가 있는지 체크
		else if (JumpLL(X - 1, Y, OutX, OutY, G, OutG, colorRGB, LL) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 아래 직선거리에 코너가 있는지 체크
		else if (JumpDD(X, Y + 1, OutX, OutY, G, OutG, colorRGB, DD) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		if ((&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_coColorRGB = colorRGB;
		}

		X--;
		Y++;
		iTempMaxSearchCount++;
	}

	return true;
}

// 우상 점프
bool CFindSearch::JumpRU(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		if (CheckTile(X, Y))
			return false;

		G += m_dDiagonal_Line;
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 현재 위치가 코너인지 체크
		else if (((&stMapArrayCopy[Y *  m_iWidth] + (X - 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) *  m_iWidth] + (X - 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[(Y + 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) *  m_iWidth] + (X + 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 오른쪽 직선 거리에 코너가 있는지 체크
		else if (JumpRR(X + 1, Y, OutX, OutY, G, OutG, colorRGB, RR) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 위 직선거리에 코너가 있는지 체크
		else if (JumpUU(X, Y - 1, OutX, OutY, G, OutG, colorRGB, UU) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		if ((&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_coColorRGB = colorRGB;
		}

		X++;
		Y--;
		iTempMaxSearchCount++;
	}

	return true;
}

// 우하
bool CFindSearch::JumpRD(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	int iTempMaxSearchCount = 0;

	while (1)
	{
		// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
		// true가 리턴되면, 장애물이거나 맵 밖의 좌표인 것.
		if (CheckTile(X, Y))
			return false;

		G += m_dDiagonal_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 현재 위치가 코너인지 체크
		else if (((&stMapArrayCopy[Y *  m_iWidth] + (X - 1))->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y + 1) *  m_iWidth] + (X - 1))->m_eTileType != OBSTACLE) ||
			((&stMapArrayCopy[(Y - 1) *  m_iWidth] + X)->m_eTileType == OBSTACLE && (&stMapArrayCopy[(Y - 1) *  m_iWidth] + (X + 1))->m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 오른쪽 직선 거리에 코너가 있는지 체크
		else if (JumpRR(X + 1, Y, OutX, OutY, G, OutG, colorRGB, RR) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		// 아래 직선거리에 코너가 있는지 체크
		else if (JumpDD(X, Y + 1, OutX, OutY, G, OutG, colorRGB, DD) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			break;
		}

		if ((&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != START && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != FIN &&
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != OPEN_LIST && (&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType != CLOSE_LIST)
		{
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_eTileType = JUMP;
			(&stMapArrayCopy[Y *  m_iWidth] + X)->m_coColorRGB = colorRGB;
		}

		X++;
		Y++;
		iTempMaxSearchCount++;
	}

	return true;
}

// 노드 생성 함수
void CFindSearch::CreateNode(int iX, int iY, stFindSearchNode* parent, double G, Direction dir)
{	
	// 1. 해당 좌표의 노드가 Open 리스트에 있는지 체크
	stFindSearchNode* SearchNode = m_OpenList.Search(iX, iY);
	if (SearchNode != nullptr)
	{
		// 있다면, 해당 노드의 G값과 내가 생성할 노드의 G값 체크.
		// 해당 노드의 G값이 더 크다면, 
		if (SearchNode->m_fG > G)
		{
			// parent를 부모 설정
			SearchNode->stpParent = parent;

			// 방향도 재설정
			SearchNode->dir = dir;

			// G값 재셋팅 (parent의 G값 + 부모부터 나까지의 거리)
			SearchNode->m_fG = G;

			// F값 재셋팅.
			SearchNode->m_fF = (SearchNode->m_fG + SearchNode->m_fH);

			// 정렬도 다시한다.
			m_OpenList.Sort();
		}
		return;
	}

	// 2. Close에 리스트에 있는지도 체크
	SearchNode = m_CloseList.Search(iX, iY);
	if (SearchNode != nullptr)
	{
		// 있다면, 해당 노드의 G값과 내가 생성할 노드의 G값 체크.
		// 해당 노드의 G값이 더 크다면, 
		if (SearchNode->m_fG > G)
		{
			// parent를 부모 설정
			SearchNode->stpParent = parent;

			// 방향도 재설정
			SearchNode->dir = dir;

			// G값 재셋팅 (parent의 G값 + 부모부터 나까지의 거리)
			SearchNode->m_fG = G;

			// F값 재셋팅.
			SearchNode->m_fF = (SearchNode->m_fG + SearchNode->m_fH);

			// 정렬도 다시한다.
			m_OpenList.Sort();
		}

		return;
	}

	// 3. 둘 다에 없으면, 노드 생성 후, Open에 넣는다.
	stFindSearchNode* NewNode = new stFindSearchNode[sizeof(stFindSearchNode)];

	NewNode->stpParent = parent;
	NewNode->dir = dir;

	NewNode->m_iX = iX;
	NewNode->m_iY = iY;

	NewNode->m_fG = G;
	NewNode->m_fH = fabs(FinNode->m_iX - iX) + fabs(FinNode->m_iY - iY);
	NewNode->m_fF = NewNode->m_fG + NewNode->m_fH;

	// 해당 노드가 시작,도착, 장해물노드가 아니면, OPEN_LIST로 변경
	if ((&stMapArrayCopy[iY *  m_iWidth] + iX)->m_eTileType != START && (&stMapArrayCopy[iY *  m_iWidth] + iX)->m_eTileType != FIN && (&stMapArrayCopy[iY *  m_iWidth] + iX)->m_eTileType != OBSTACLE)
		(&stMapArrayCopy[iY *  m_iWidth] + iX)->m_eTileType = OPEN_LIST;

	m_OpenList.Insert(NewNode);

	// 4. 정렬도 다시한다.
	m_OpenList.Sort();
}

// 그리드 그리기
void CFindSearch::GridShow()
{	
	HDC hdc = GetDC(m_hWnd);
	
	// 그리드 그리기

	for (int i = 0; i < m_iHeight; ++i)
	{
		for (int j = 0; j < m_iWidth; ++j)
		{
			HBRUSH hMyBrush = NULL;
			HBRUSH hOldBrush = NULL;
			bool bBrushCheck = false;

			// 해당 맵 타일의 상태에 따라서 색을 설정한다.
			// 출발지는 초록색
			if ((&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType == START)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 255, 0));
				hOldBrush = (HBRUSH)SelectObject(m_hMemDC, hMyBrush);
				bBrushCheck = true;
			}

			// 목적지는 빨간색
			else if ((&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType == FIN)
			{
				hMyBrush = CreateSolidBrush(RGB(255, 0, 0));
				hOldBrush = (HBRUSH)SelectObject(m_hMemDC, hMyBrush);
				bBrushCheck = true;
			}

			// 장해물은 회색
			else if ((&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType == OBSTACLE)

			{
				hMyBrush = CreateSolidBrush(RGB(80, 80, 80));
				hOldBrush = (HBRUSH)SelectObject(m_hMemDC, hMyBrush);
				bBrushCheck = true;
			}

			// OPEN_LIST는 파란색
			else if ((&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType == OPEN_LIST)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 0, 255));
				hOldBrush = (HBRUSH)SelectObject(m_hMemDC, hMyBrush);
				bBrushCheck = true;
			}

			// CLOSE_LIST는 어두운 파란색
			else if ((&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType == CLOSE_LIST)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 0, 127));
				hOldBrush = (HBRUSH)SelectObject(m_hMemDC, hMyBrush);
				bBrushCheck = true;
			}

			// JUMP면 타일에 지정된 색 사용
			else if ((&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType == JUMP)
			{
				hMyBrush = CreateSolidBrush((&stMapArrayCopy[i *  m_iWidth] + j)->m_coColorRGB);
				hOldBrush = (HBRUSH)SelectObject(m_hMemDC, hMyBrush);
				bBrushCheck = true;
			}

			// TEST면 타일에 지정된 색 사용 후, 속성을 NONE으로 변경
			else if ((&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType == TEST)
			{
				hMyBrush = CreateSolidBrush((&stMapArrayCopy[i *  m_iWidth] + j)->m_coColorRGB);
				hOldBrush = (HBRUSH)SelectObject(m_hMemDC, hMyBrush);
				bBrushCheck = true;
				(&stMapArrayCopy[i *  m_iWidth] + j)->m_eTileType = Tile_Type::NONE;
			}

			Rectangle(m_hMemDC, (&stMapArrayCopy[i *  m_iWidth] + j)->m_iMapX - m_iRadius, (&stMapArrayCopy[i *  m_iWidth] + j)->m_iMapY - m_iRadius, (&stMapArrayCopy[i *  m_iWidth] + j)->m_iMapX + m_iRadius, (&stMapArrayCopy[i *  m_iWidth] + j)->m_iMapY + m_iRadius);

			if (bBrushCheck == true)
			{
				SelectObject(m_hMemDC, hOldBrush);
				DeleteObject(hMyBrush);
				DeleteObject(hOldBrush);
			}
		}
	}

	BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, m_hMemDC, rt.left, rt.top, SRCCOPY);

	ReleaseDC(m_hWnd, hdc);
}

// 선긋기
void CFindSearch::Line(stFindSearchNode* Node, COLORREF rgb)
{
	HDC hdc = GetDC(m_hWnd);

	MoveToEx(m_hMemDC, (&stMapArrayCopy[(Node->m_iY) *  m_iWidth] + Node->m_iX)->m_iMapX,
		(&stMapArrayCopy[(Node->m_iY) *  m_iWidth] + Node->m_iX)->m_iMapY, NULL);

	HPEN hMyPen, OldPen;

	if (rgb == -1)
	{
		hMyPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
		OldPen = (HPEN)SelectObject(m_hMemDC, hMyPen);
	}
	else
	{
		hMyPen = CreatePen(PS_SOLID, 3, rgb);
		OldPen = (HPEN)SelectObject(m_hMemDC, hMyPen);
	}

	stFindSearchNode* ParentNode = Node->stpParent;
	while (1)
	{
		if (ParentNode->m_iX == StartNode->m_iX && ParentNode->m_iY == StartNode->m_iY)
		{
			LineTo(m_hMemDC, (&stMapArrayCopy[(ParentNode->m_iY) *  m_iWidth] + ParentNode->m_iX)->m_iMapX,
				(&stMapArrayCopy[(ParentNode->m_iY) *  m_iWidth] + ParentNode->m_iX)->m_iMapY);
			break;
		}

		LineTo(m_hMemDC, (&stMapArrayCopy[(ParentNode->m_iY) *  m_iWidth] + ParentNode->m_iX)->m_iMapX,
			(&stMapArrayCopy[(ParentNode->m_iY) *  m_iWidth] + ParentNode->m_iX)->m_iMapY);

		ParentNode = ParentNode->stpParent;
	}

	SelectObject(m_hMemDC, OldPen);
	DeleteObject(hMyPen);
	DeleteObject(OldPen);

	BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, m_hMemDC, rt.left, rt.top, SRCCOPY);

	ReleaseDC(m_hWnd, hdc);
}

// Jump_Point_Search_While()가 완료될 때, 오픈/닫힌 리스트, 타이머 등 정리하기
void CFindSearch::LastClear()
{
	// 1. FinNode는 소멸 안됐으니 소멸시킨다.
	delete[] FinNode;

	// 2. 타이머도 끝낸다.
	KillTimer(m_hWnd, 1);

	m_iColorKey = 1;
}


////////////////////////////////
// 외부로 X,Y를 반환하는 함수
///////////////////////////////
// 밖에서 시작점, 출발점과 Map 구조체를 받으면, 경로를 POINT구조체 안에 넣어주는 함수. 
// POINT구조체 배열 0번부터 parent를 하면서 가면 된다.
// Use_Bresenham변수에 true를 넣으면 브레젠헴 알고리즘으로 길찾는다.
// 몇 번 배열까지 찼는지 리턴한다.
int CFindSearch::PathGet(int StartX, int StartY, int FinX, int FinY, POINT* Outmap, bool Use_Bresenham)
{
	StartNode = new stFindSearchNode[sizeof(stFindSearchNode)];
	FinNode = new stFindSearchNode[sizeof(stFindSearchNode)];

	// x,y좌표 셋팅
	StartNode->m_iX = StartX;
	StartNode->m_iY = StartY;
	FinNode->m_iX = FinX;
	FinNode->m_iY = FinY;

	// 시작점의 G,H,F 셋팅
	StartNode->m_fG = 0;
	StartNode->m_fH = fabs((FinX - StartX) + fabs(FinY - StartY));
	StartNode->m_fF = StartNode->m_fG + StartNode->m_fH;
	StartNode->stpParent = nullptr;
	StartNode->dir = NONE;

	// 도착점의 G,H,F 셋팅
	FinNode->m_fG = 0;
	FinNode->m_fH = 0;
	FinNode->m_fF = 0;
	FinNode->stpParent = nullptr;
	FinNode->dir = NONE;

	// 열린 리스트, 닫힌 리스트 초기화
	m_CloseList.Clear();
	m_OpenList.Clear();

	// 오픈리스트에 시작 위치 넣기.
	m_OpenList.Insert(StartNode);

	// while문 돌면서 경로 구하기.
	while (1)
	{
		if (m_OpenList.GetCount() == 0)
		{
			// 1. FinNode는 소멸 안됐으니 소멸시킨다.
			delete[] FinNode;
			m_iColorKey = 1;

			return 0;
		}

		// 2-1. 오픈리스트가 비어있지 않다면, 오픈 리스트에서 노드 하나 얻어옴.
		stFindSearchNode* Node = m_OpenList.GetListNode();

		// 2-2. 꺼낸 노드가 목적지라면 break;
		if (Node->m_iX == FinNode->m_iX &&
			Node->m_iY == FinNode->m_iY)
		{	
			// m_bUse_Bresenham변수(생성자에서 지정)가 true라면 브레젠헴 알고리즘 사용하겠다는 것.
			if (Use_Bresenham)
			{
				Bresenham_Line(Node);
			}

			// 배열 채우기
			int Array = 0;
			while (1)
			{
				Outmap[Array].x = Node->m_iX;
				Outmap[Array].y = Node->m_iY;

				if (Node->stpParent == nullptr)
					break;								

				Node = Node->stpParent;
				Array++;
			}

			// FinNode는 소멸 안됐으니 소멸시킨다.
			delete[] FinNode;
			m_iColorKey = 1;

			return Array;
		}

		// 2-3. 목적지 노드가 아니면 꺼낸 노드를 CloseList에 넣기
		m_CloseList.Insert(Node);

		if ((&stMapArrayCopy[Node->m_iY * m_iWidth] + Node->m_iX)->m_eTileType == OPEN_LIST)
			(&stMapArrayCopy[Node->m_iY * m_iWidth] + Node->m_iX)->m_eTileType = CLOSE_LIST;

		// 2-4. 그리고 노드 생성 함수 호출
		// 이 안에서 상황에 따라 타고 들어간다.
		CreateNodeCheck(Node, false);
	}
}

// 외부에서 POINT인자를 받아서 선 긋기
void CFindSearch::Out_Line(POINT* p, int Count, COLORREF rgb)
{
	HDC hdc = GetDC(m_hWnd);

	MoveToEx(m_hMemDC, (&stMapArrayCopy[(p[0].y) *  m_iWidth] + p[0].x)->m_iMapX,
		(&stMapArrayCopy[(p[0].y) *  m_iWidth] + p[0].x)->m_iMapY, NULL);

	HPEN hMyPen, OldPen;

	if (rgb == -1)
	{
		hMyPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
		OldPen = (HPEN)SelectObject(m_hMemDC, hMyPen);
	}
	else
	{
		hMyPen = CreatePen(PS_SOLID, 3, rgb);
		OldPen = (HPEN)SelectObject(m_hMemDC, hMyPen);
	}

	for (int i = 1; i <= Count; ++i)
	{
		LineTo(m_hMemDC, (&stMapArrayCopy[(p[i].y) *  m_iWidth] + p[i].x)->m_iMapX,
			(&stMapArrayCopy[(p[i].y) *  m_iWidth] + p[i].x)->m_iMapY);
	}

	SelectObject(m_hMemDC, OldPen);
	DeleteObject(hMyPen);
	DeleteObject(OldPen);

	BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, m_hMemDC, rt.left, rt.top, SRCCOPY);

	ReleaseDC(m_hWnd, hdc);
}


////////////////////////////////
// 브레젠헴 알고리즘 관련 함수
///////////////////////////////
// 브레젠헴 알고리즘 테스트용 색칠하기
void CFindSearch::Bresenham_Line_Test(int StartX, int StartY, int FinX, int FinY)
{
    FillRect(m_hMemDC, &rt, (HBRUSH)GetStockObject(WHITE_BRUSH));
	GridShow();

	CBresenhamLine test;
	test.Init(StartX, StartY, FinX, FinY);
	POINT p;

	HDC hdc = GetDC(m_hWnd);	

	while (test.GetNext(&p))
	{
		(&stMapArrayCopy[p.y *  m_iWidth] + p.x)->m_eTileType = TEST;
		(&stMapArrayCopy[p.y *  m_iWidth] + p.x)->m_coColorRGB = RGB(152,0,0);		
	}

	MoveToEx(m_hMemDC, (&stMapArrayCopy[StartY *  m_iWidth] + StartX)->m_iMapX, (&stMapArrayCopy[StartY *  m_iWidth] + StartX)->m_iMapY, NULL);
	LineTo(m_hMemDC, (&stMapArrayCopy[FinY *  m_iWidth] + FinX)->m_iMapX, (&stMapArrayCopy[FinY *  m_iWidth] + FinX)->m_iMapY);

	BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, m_hMemDC, rt.left, rt.top, SRCCOPY);

	ReleaseDC(m_hWnd, hdc);
}

// 브레젠헴 실제 체크
void CFindSearch::Bresenham_Line(stFindSearchNode* FinNode)
{
	// 1. 브레젠헴 객체 선언
	CBresenhamLine Bresenham;

	// 2. 포인터 객체 선언
	POINT p;

	// 3. while문을 돌면서, 도착지점까지, 브레젠헴 알고리즘을 이용해 최단경로 체크.
	stFindSearchNode* NowNode = FinNode;
	stFindSearchNode* Now_Next = NowNode->stpParent;
	stFindSearchNode* Now_Next_Next = Now_Next->stpParent;	
	
	if (Now_Next_Next == nullptr)
		return;

	// 4. 브레젠헴 객체 초기화
	Bresenham.Init(NowNode->m_iX, NowNode->m_iY, Now_Next_Next->m_iX, Now_Next_Next->m_iY);

	// 5. 브레젠헴 알고리즘 시작
	while (1)
	{
		// 1. Next 경로 알아오기.
		Bresenham.GetNext(&p);

		// 2. Next 경로가, 지정한 목적지라면, 목표지점에 도착한 것이니 아래 로직 진행
		if (p.x == Now_Next_Next->m_iX && p.y == Now_Next_Next->m_iY)
		{
			// 2-1. NowNode의 부모를 Now_Next에서 Now_Next_Next로 지정 (즉, Now_Next를 버린다)
			NowNode->stpParent = Now_Next_Next;

			// 2-2. Now_Next_Next를 NowNode로 설정
			//NowNode = Now_Next_Next;

			// 2-3. 만약, NowNode가 시작 노드거나, Now_Next_Next가 nullptre이라면, 브레젠헴 알고리즘을 끝낼 시점임.
			if (NowNode == StartNode || NowNode->stpParent->stpParent == nullptr)
				break;

			// 2-4. 새로 지정된 NowNode기준, Now_Next / Now_Next_Next를 재정의.
			Now_Next = NowNode->stpParent;
			Now_Next_Next = Now_Next->stpParent;		

			// 2-5. 브레젠헴 객체 재초기화
			Bresenham.Init(NowNode->m_iX, NowNode->m_iY, Now_Next_Next->m_iX, Now_Next_Next->m_iY);
		}

		// 3. Next 경로가, 지정한 목적지가 아니라면, 현재 경로가 장애물인지 체크
		else
		{
			// 3-1. 현재 경로가 장애물이면 Now_Next를 NowNode로 설정 후 다시 진행.
			// 위의 Now_Next, Now_Next_Next와는 관계 없다. 브레젠헴 알고리즘으로 나온 경로의 Next이다.
			if ((&stMapArrayCopy[p.y *  m_iWidth] + p.x)->m_eTileType == OBSTACLE)
			{
				// 1. Now_Next를 NowNode로 설정
				NowNode = Now_Next;

				// 2. 만약, NowNode가 시작 노드거나, Now_Next_Next가 nullptre이라면, 브레젠헴 알고리즘을 끝낼 시점임.
				if (Now_Next == StartNode || NowNode->stpParent->stpParent == nullptr)
					break;

				// 3. 새로 지정된 NowNode기준, Now_Next / Now_Next_Next를 재정의.
				Now_Next = NowNode->stpParent;					
				Now_Next_Next = Now_Next->stpParent;

				// 2-5. 브레젠헴 객체 재초기화
				Bresenham.Init(NowNode->m_iX, NowNode->m_iY, Now_Next_Next->m_iX, Now_Next_Next->m_iY);
			}
		}
	}
}

