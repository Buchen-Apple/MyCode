#include "stdafx.h"
#include "Jump_Point_Search.h"
#include <math.h>
#include <time.h>

// 인자로 받은 노드를 기준으로, 어느 방향으로 이동할 것인지 결정. CheckCreateJump()라는 함수를 썼었지만 매크로로 변경.
#define CheckCreateJump(X,Y,dir)	\
		if(Jump(X,Y,&OutX,&OutY,G,&OutG,colorRGB,dir))	\
		 { OutG += Node->m_fG; CreateNode(OutX, OutY, Node, OutG, dir); OutX = OutY; G = 0, OutG = Node->m_fG;}

// 생성자
CFindSearch::CFindSearch(Map(*Copy)[MAP_WIDTH])
{
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

}

// 시작점, 도착점, MemDC 셋팅하기
void CFindSearch::Init(int StartX, int StartY, int FinX, int FinY, HWND hWnd)
{
	this->hWnd = hWnd;

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

//  시작 전, 시작점을 오픈리스트에 넣는다.
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
		Show(Node);
		
		// 리스트 클리어, 타이머 종료 등 처리
		LastClear();
		return;
	}

	// 2-3. 목적지 노드가 아니면 꺼낸 노드를 CloseList에 넣기
	m_CloseList.Insert(Node);

	if (stMapArrayCopy[Node->m_iY][Node->m_iX].m_eTileType == OPEN_LIST)
		stMapArrayCopy[Node->m_iY][Node->m_iX].m_eTileType = CLOSE_LIST;

	// 2-4. 그리고 노드 생성 함수 호출
	// 이 안에서 상황에 따라 타고 들어간다.
	CreateNodeCheck(Node);
}

// 인자로 받은 노드를 기준으로, 방향 지정
void CFindSearch::CreateNodeCheck(stFindSearchNode* Node)
{
	// 이번 노드의 컬러 지정
	COLORREF colorRGB;
	switch (ColorKey)
	{
	case 1:
		colorRGB = Color1;
		ColorKey++;
		break;

	case 2:
		colorRGB = Color2;
		ColorKey++;
		break;

	case 3:
		colorRGB = Color3;
		ColorKey++;
		break;

	case 4:
		colorRGB = Color4;
		ColorKey++;
		break;

	case 5:
		colorRGB = Color5;
		ColorKey++;
		break;

	case 6:
		colorRGB = Color6;
		ColorKey++;
		break;

	case 7:
		colorRGB = Color7;
		ColorKey++;
		break;

	case 8:
		colorRGB = Color8;
		ColorKey = 1;
		break;

	default:
		break;
	}	

	// CheckCreateJump 매크로 함수에서 Jump와 CreateNode를 호출하는데, 그 때 사용할 변수들 선언.
	int OutX = 0, OutY = 0;
	double G = 0, OutG = Node->m_fG;

	// 1. 인자로 받은 노드가 시작점 노드라면, 8방향으로 Jump하면서 노드 생성 여부 체크
	if (Node->stpParent == nullptr)
	{
		// LL
		CheckCreateJump(Node->m_iX - 1, Node->m_iY, LL);

		// RR
		CheckCreateJump(Node->m_iX + 1, Node->m_iY, RR);

		// UU
		CheckCreateJump(Node->m_iX, Node->m_iY - 1, UU);

		// DD
		CheckCreateJump(Node->m_iX, Node->m_iY + 1, DD);

		// LU
		CheckCreateJump(Node->m_iX -1, Node->m_iY -1, LU);

		// LD
		CheckCreateJump(Node->m_iX -1, Node->m_iY +1, LD);

		// RU
		CheckCreateJump(Node->m_iX +1, Node->m_iY -1, RU);

		// RD
		CheckCreateJump(Node->m_iX +1, Node->m_iY +1, RD);
	}

	// 그게 아니라면, 직선 / 대각선에 따라 노드 생성 여부 체크
	else
	{
		// 상
		if (Node->dir == UU)
		{
			// 상 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node->dir);

			// 좌상 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, LU);

			// 우상 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, RU);
		}

		// 하
		else if (Node->dir == DD)
		{
			// 하 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node->dir);

			// 좌하 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, LD);

			// 우하 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, RD);
		}

		// 좌
		else if (Node->dir == LL)
		{
			// 좌 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node->dir);

			// 좌상 체크
			if (stMapArrayCopy[Node->m_iY -1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY -1][Node->m_iX -1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX -1, Node->m_iY -1, LU);

			// 좌하 체크
			if (stMapArrayCopy[Node->m_iY +1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY +1][Node->m_iX -1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX -1, Node->m_iY +1, LD);
		}

		// 우
		else if (Node->dir == RR)
		{
			// 우 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node->dir);

			// 우상 체크
			if (stMapArrayCopy[Node->m_iY - 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, RU);

			// 우하 체크
			if (stMapArrayCopy[Node->m_iY + 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, RD);
		}

		// 좌상
		else if (Node->dir == LU)
		{
			// 왼쪽 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, LL);

			// 위쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, UU);

			// 좌상 대각선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node->dir);

			// 우상 대각선
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, RU);

			// 좌하 대각선 
			if (stMapArrayCopy[Node->m_iY + 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, LD);
		}

		// 좌하
		else if (Node->dir == LD)
		{
			// 왼쪽 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, LL);

			// 아래쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, DD);

			// 좌하 대각선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node->dir);

			// 좌상 대각선
			if (stMapArrayCopy[Node->m_iY - 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, LU);

			// 우하 대각선 
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, RD);
		}

		// 우상
		else if (Node->dir == RU)
		{
			// 오른쪽 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, RR);

			// 위쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, UU);

			// 우상 대각선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node->dir);

			// 좌상 대각선
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, LU);

			// 우하 대각선 
			if (stMapArrayCopy[Node->m_iY + 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, RD);
		}

		// 우하
		else if (Node->dir == RD)
		{
			// 오른쪽 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, RR);

			// 아래쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, DD);

			// 우하 대각선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node->dir);

			// 우상 대각선
			if (stMapArrayCopy[Node->m_iY - 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, RU);

			// 좌하 대각선 
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, LD);
		}			
	}	
	Show();
}

// 점프
// 해당 방향에 생성할 노드가 있는지 체크(재귀)
bool CFindSearch::Jump(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir)
{
	// 직선 가중치, 대각선 가중치
	static double dStraight_Line = 1;
	static double dDiagonal_Line = 2;

	// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
	if (stMapArrayCopy[Y][X].m_eTileType == OBSTACLE ||
		X < 0 || X >= MAP_WIDTH || Y < 0 || Y >= MAP_HEIGHT)
		return false;

	// 2. 방향에 따라 진행
	// 상	
	if (dir == UU)
	{
		G += dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 목적지가 아니라면, 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 코너가 아니라면, 해당 위치의 타일을 JUMP로 바꾼다. 확인했단는 의미로! 
		// 근데 시작점/도착점/오픈리스트/닫힌리스트에 있으면 안바꾼다. 
		// 그들은 고유하게 색이 유지되어야 한다.
		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}

		Jump(X, Y - 1, OutX, OutY, G, OutG, colorRGB, dir);
	}

	// 하	
	else if (dir == DD)
	{
		G += dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}
		Jump(X, Y + 1, OutX, OutY, G, OutG, colorRGB, dir);
	}

	//좌	
	else if (dir == LL)
	{
		G += dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}

		Jump(X - 1, Y, OutX, OutY, G, OutG, colorRGB, dir);
	}

	// 우	
	else if (dir == RR)
	{
		G += dStraight_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}

		Jump(X + 1, Y, OutX, OutY, G, OutG, colorRGB, dir);
	}

	// 좌상
	else if (dir == LU)
	{
		G += dDiagonal_Line;
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 왼쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X - 1, Y, OutX, OutY, G, OutG, colorRGB, LL) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 위 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y - 1, OutX, OutY, G, OutG, colorRGB, UU) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}
		Jump(X - 1, Y - 1, OutX, OutY, G, OutG, colorRGB, dir);
	}

	// 좌하
	else if (dir == LD)
	{
		G += dDiagonal_Line;
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 왼쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X - 1, Y, OutX, OutY, G, OutG, colorRGB, LL) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 아래 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y + 1, OutX, OutY, G, OutG, colorRGB, DD) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}

		Jump(X - 1, Y + 1, OutX, OutY, G, OutG, colorRGB, dir);
	}

	// 우상
	else if (dir == RU)
	{
		G += dDiagonal_Line;
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 오른쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X + 1, Y, OutX, OutY, G, OutG, colorRGB, RR) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 위 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y - 1, OutX, OutY, G, OutG, colorRGB, UU) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}

		Jump(X + 1, Y - 1, OutX, OutY, G, OutG, colorRGB, dir);
	}

	// 우하
	else if (dir == RD)
	{
		G += dDiagonal_Line;

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 오른쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X + 1, Y, OutX, OutY, G, OutG, colorRGB, RR) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		// 아래 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y + 1, OutX, OutY, G, OutG, colorRGB, DD) == true)
		{
			*OutX = X;
			*OutY = Y;
			*OutG = G;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
			stMapArrayCopy[Y][X].colorRGB = colorRGB;
		}

		Jump(X + 1, Y + 1, OutX, OutY, G, OutG, colorRGB, dir);
	}
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
		//if (SearchNode->m_fG > (parent->m_fG + (fabs(parent->m_iX - iX) + fabs(parent->m_iY - iY))))
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
	if (stMapArrayCopy[iY][iX].m_eTileType != START && stMapArrayCopy[iY][iX].m_eTileType != FIN && stMapArrayCopy[iY][iX].m_eTileType != OBSTACLE)
		stMapArrayCopy[iY][iX].m_eTileType = OPEN_LIST;

	m_OpenList.Insert(NewNode);

	// 4. 정렬도 다시한다.
	m_OpenList.Sort();
}

// 그리기
void CFindSearch::Show(stFindSearchNode* Node, int StartX, int StartY, int FinX, int FinY)
{
	HDC hdc = GetDC(hWnd);
	HDC MemDC = CreateCompatibleDC(hdc);
	RECT rt;
	GetClientRect(hWnd, &rt);

	HBITMAP hMyBitmap = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(MemDC, hMyBitmap);

	FillRect(MemDC, &rt, (HBRUSH)GetStockObject(WHITE_BRUSH));

	// TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다.
	int iRadius = MAP_TILE_RADIUS;

	for (int i = 0; i < MAP_HEIGHT; ++i)
	{
		for (int j = 0; j < MAP_WIDTH; ++j)
		{
			HBRUSH hMyBrush = NULL;
			HBRUSH hOldBrush = NULL;
			bool bBrushCheck = false;

			// 해당 맵 타일의 상태에 따라서 색을 설정한다.
			// 출발지는 초록색
			if (stMapArrayCopy[i][j].m_eTileType == START)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 255, 0));
				hOldBrush = (HBRUSH)SelectObject(MemDC, hMyBrush);
				bBrushCheck = true;
			}

			// 목적지는 빨간색
			else if (stMapArrayCopy[i][j].m_eTileType == FIN)
			{
				hMyBrush = CreateSolidBrush(RGB(255, 0, 0));
				hOldBrush = (HBRUSH)SelectObject(MemDC, hMyBrush);
				bBrushCheck = true;
			}

			// 장해물은 회색
			else if (stMapArrayCopy[i][j].m_eTileType == OBSTACLE)

			{
				hMyBrush = CreateSolidBrush(RGB(80, 80, 80));
				hOldBrush = (HBRUSH)SelectObject(MemDC, hMyBrush);
				bBrushCheck = true;
			}

			// OPEN_LIST는 파란색
			else if (stMapArrayCopy[i][j].m_eTileType == OPEN_LIST)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 0, 255));
				hOldBrush = (HBRUSH)SelectObject(MemDC, hMyBrush);
				bBrushCheck = true;
			}

			// CLOSE_LIST는 어두운 파란색
			else if (stMapArrayCopy[i][j].m_eTileType == CLOSE_LIST)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 0, 127));
				hOldBrush = (HBRUSH)SelectObject(MemDC, hMyBrush);
				bBrushCheck = true;
			}

			// JUMP면 타일에 지정된 색 사용
			else if (stMapArrayCopy[i][j].m_eTileType == JUMP)
			{
				hMyBrush = CreateSolidBrush(stMapArrayCopy[i][j].colorRGB);
				hOldBrush = (HBRUSH)SelectObject(MemDC, hMyBrush);
				bBrushCheck = true;
			}

			Rectangle(MemDC, stMapArrayCopy[i][j].m_iMapX - iRadius, stMapArrayCopy[i][j].m_iMapY - iRadius, stMapArrayCopy[i][j].m_iMapX + iRadius, stMapArrayCopy[i][j].m_iMapY + iRadius);

			if (bBrushCheck == true)
			{
				SelectObject(MemDC, hOldBrush);
				DeleteObject(hMyBrush);
				DeleteObject(hOldBrush);
			}
		}
	}

	// 선긋기
	if (Node != nullptr)
	{
		MoveToEx(MemDC, stMapArrayCopy[Node->m_iY][Node->m_iX].m_iMapX,
			stMapArrayCopy[Node->m_iY][Node->m_iX].m_iMapY, NULL);

		HPEN hMyPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
		HPEN OldPen = (HPEN)SelectObject(MemDC, hMyPen);

		stFindSearchNode* ParentNode = Node->stpParent;
		while (1)
		{
			if (ParentNode->m_iX == StartNode->m_iX && ParentNode->m_iY == StartNode->m_iY)
			{
				LineTo(MemDC, stMapArrayCopy[ParentNode->m_iY][ParentNode->m_iX].m_iMapX,
					stMapArrayCopy[ParentNode->m_iY][ParentNode->m_iX].m_iMapY);
				break;
			}

			LineTo(MemDC, stMapArrayCopy[ParentNode->m_iY][ParentNode->m_iX].m_iMapX,
				stMapArrayCopy[ParentNode->m_iY][ParentNode->m_iX].m_iMapY);

			ParentNode = ParentNode->stpParent;
		}

		DeleteObject(hMyPen);
		DeleteObject(OldPen);
	}

	 //브레젠험 알고리즘.
	if (StartX != -1)
	{
		// 시작 X,Y위치 셋팅
		int X = StartX, Y = StartY;

		// 차후, Y 혹은 X를 증가시킬 기준 변수.
		int Error = 0;

		// X,Y를 증가시킬 값.
		int AddX, AddY;

		// 시작점부터 높이점까지의 넓이와 높이.
		int Width = FinX - StartX;
		int Height = FinY - StartY;

		//  AddX, AddY 셋팅
		if (Width < 0)
		{
			Width = -Width;
			AddX = -1;
		}
		else
			AddX = 1;

		if (Height < 0)
		{
			Height = -Height;
			AddY = -1;
		}
		else
			AddY = 1;

		
		// 넓이가 높이보다, 크거나 같을 경우
		if (Width >= Height)
		{		
			for (int i = 0; i < Width; ++i)
			{
				X += AddX;
				Error += Height;

				if (Error >= Width)
				{
					Y += AddY;
					Error -= Width;
				}

				SetPixel(MemDC, X, Y, RGB(0, 0, 0));
			}
		}		

		// 높이가 넓이보다, 클 경우
		else
		{		
			for (int i = 0; i < Height; ++i)
			{
				Y += AddY;
				Error += Width;

				if (Error >= Height)
				{
					X += AddX;
					Error -= Height;
				}

				SetPixel(MemDC, X, Y, RGB(0, 0, 0));
			}
		}
	}

	BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, MemDC, rt.left, rt.top, SRCCOPY);

	DeleteObject(hMyBitmap);
	DeleteObject(hOldBitmap);
	DeleteDC(MemDC);
	ReleaseDC(hWnd, hdc);
}

// Jump_Point_Search_While()가 완료될 때, 오픈/닫힌 리스트, 타이머 등 정리하기
void CFindSearch::LastClear()
{
	// 1. 다 끝나면, Open리스트랑 Close리스트 초기화
	//m_CloseList.Clear();
	//m_OpenList.Clear();

	// 2. FinNode는 소멸 안됐으니 소멸시킨다.
	delete[] FinNode;

	// 타이머도 끝낸다.
	KillTimer(hWnd, 1);

	ColorKey = 1;
}