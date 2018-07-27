#include "stdafx.h"
#include "A_Search.h"
#include <math.h>

// 생성자
CFindSearch::CFindSearch(Map (*Copy)[MAP_WIDTH])
{	
	StartNode = nullptr;
	FinNode = nullptr;

	// 맵의 사본을 보관한다.
	stMapArrayCopy = Copy;
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

	// 도착점의 G,H,F 셋팅
	FinNode->m_fG = 0;
	FinNode->m_fH = 0;
	FinNode->m_fF = 0;
	FinNode->stpParent = nullptr;
}

// 반복문 돌면서 에이스타 진행
void CFindSearch::A_Search_While()
{
	// 1. 시작 전, 시작점을 오픈리스트에 넣는다.
	m_OpenList.Insert(StartNode);

	// 2. 그리고 반복 시작
	// 오픈 리스트가 빌 때 까지 반복.
	while (m_OpenList.GetCount() != 0)
	{
		// 2-1. 오픈 리스트가 비어있지 않다면, 오픈 리스트에서 노드 하나 얻어옴.
		stFindSearchNode* Node = m_OpenList.GetListNode();

		// 2-2. 꺼낸 노드가 목적지라면 break;
		if (Node->m_iX == FinNode->m_iX &&
			Node->m_iY == FinNode->m_iY)
		{
			// Node가 목적지이니, 여기부터 부모를 타고가면서 선을 긋는다.
			Show(Node);			
			break;
		}

		// 2-3. 목적지 노드가 아니면 꺼낸 노드를 CloseList에 넣기.
		m_CloseList.Insert(Node);

		if(stMapArrayCopy[Node->m_iY][Node->m_iX].m_eTileType == OPEN_LIST)
			stMapArrayCopy[Node->m_iY][Node->m_iX].m_eTileType = CLOSE_LIST;

		// 2-4. 그리고 노드 생성함수 8개 호출(상하좌우,대각선까지 총 8개)
		// 상
		CreateNode(Node->m_iX, Node->m_iY - 1, Node);

		// 하
		CreateNode(Node->m_iX, Node->m_iY + 1, Node);

		// 좌
		CreateNode(Node->m_iX - 1, Node->m_iY, Node);

		// 우
		CreateNode(Node->m_iX + 1, Node->m_iY, Node);


		// 좌상
		CreateNode(Node->m_iX - 1, Node->m_iY - 1, Node, 1.0);

		// 좌하
		CreateNode(Node->m_iX - 1, Node->m_iY + 1, Node, 1.0);

		// 우상
		CreateNode(Node->m_iX + 1, Node->m_iY - 1, Node, 1.0);

		// 우하
		CreateNode(Node->m_iX + 1, Node->m_iY + 1, Node, 1.0);
	}

	// 다 끝나면, Open리스트랑 Close리스트 초기화
	m_CloseList.Clear();
	m_OpenList.Clear();

	// FinNode는 소멸 안됐으니 소멸시킨다.
	delete[] FinNode;

}

// 노드 생성 함수
void CFindSearch::CreateNode(int iX, int iY, stFindSearchNode* parent, double fWeight)
{
	// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지) 
	if (stMapArrayCopy[iY][iX].m_eTileType == OBSTACLE ||
		iX < 0 || iX >= MAP_WIDTH || iY < 0 || iY >= MAP_HEIGHT)
		return;

	// 2. 생성 가능한 위치라면, 해당 좌표의 노드가 Open 리스트에 있는지 체크
	stFindSearchNode* SearchNode = m_OpenList.Search(iX, iY);
	if (SearchNode != nullptr)
	{
		// 있다면, 해당 노드의 G값과 내가 생성할 노드의 G값 체크.
		// 해당 노드의 G값이 더 크다면, 
		if (SearchNode->m_fG > (parent->m_fG + 1 + fWeight))
		{
			// parent를 부모 설정
			SearchNode->stpParent = parent;

			// G값 재셋팅(parent의 G값 + 1)
			SearchNode->m_fG = parent->m_fG + 1 + fWeight;

			// F값 재셋팅.
			SearchNode->m_fF = (SearchNode->m_fG + SearchNode->m_fH);

			// 정렬도 다시한다.
			m_OpenList.Sort();
		}

		return;
	}

	// 3. Close에 리스트에 있는지도 체크
	SearchNode = m_CloseList.Search(iX, iY);
	if (SearchNode != nullptr)
	{
		// 있다면, 해당 노드의 G값과 내가 생성할 노드의 G값 체크.
		// 해당 노드의 G값이 더 크다면, 
		if (SearchNode->m_fG > (parent->m_fG + 1 + fWeight))
		{
			// parent를 부모 설정
			SearchNode->stpParent = parent;

			// G값 재셋팅(parent의 G값 + 1)
			SearchNode->m_fG = parent->m_fG + 1 + fWeight;

			// F값 재셋팅.
			SearchNode->m_fF = (SearchNode->m_fG + SearchNode->m_fH);

			// 정렬도 다시한다.
			m_OpenList.Sort();
		}

		return;
	}

	// 4. 둘 다에 없으면, 노드 생성 후, Open에 넣는다.
	stFindSearchNode* NewNode = new stFindSearchNode[sizeof(stFindSearchNode)];

	NewNode->stpParent = parent;

	NewNode->m_iX = iX;
	NewNode->m_iY = iY;

	NewNode->m_fG = parent->m_fG + 1 + fWeight;
	NewNode->m_fH = fabs(FinNode->m_iX - iX) + fabs(FinNode->m_iY - iY);
	NewNode->m_fF = NewNode->m_fG + NewNode->m_fH;

	if(stMapArrayCopy[iY][iX].m_eTileType == NONE)
		stMapArrayCopy[iY][iX].m_eTileType = OPEN_LIST;

	m_OpenList.Insert(NewNode);	

	// 5. 정렬도 다시한다.
	m_OpenList.Sort();

	// 6. 출력한다.
	Show();

}

// 그리기
void CFindSearch::Show(stFindSearchNode* Node)
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
				hMyBrush = CreateSolidBrush(RGB(125, 125, 125));
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

			// CLOSE_LIST는 연한 파란색
			else if (stMapArrayCopy[i][j].m_eTileType == CLOSE_LIST)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 0, 127));
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
	if(Node != nullptr)
	{
		MoveToEx(MemDC, stMapArrayCopy[Node->m_iY][Node->m_iX].m_iMapX,
			stMapArrayCopy[Node->m_iY][Node->m_iX].m_iMapY, NULL);

		HPEN hMyPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
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

	BitBlt(hdc, rt.left, rt.top, rt.right, rt.bottom, MemDC, rt.left, rt.top, SRCCOPY);

	DeleteObject(hMyBitmap);
	DeleteObject(hOldBitmap);
	DeleteDC(MemDC);
}