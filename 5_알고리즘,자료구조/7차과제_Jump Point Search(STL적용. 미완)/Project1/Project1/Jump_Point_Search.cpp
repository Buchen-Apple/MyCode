#include "stdafx.h"
#include "Jump_Point_Search.h"
#include <math.h>

// 생성자
CFindSearch::CFindSearch(Map(*Copy)[MAP_WIDTH])
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
	StartNode->dir = NONE;

	// 도착점의 G,H,F 셋팅
	FinNode->m_fG = 0;
	FinNode->m_fH = 0;
	FinNode->m_fF = 0;
	FinNode->stpParent = nullptr;
	FinNode->dir = NONE;
}

// 반복문 돌면서 점프 포인트 서치 진행
void CFindSearch::Jump_Point_Search_While()
{
	// 1. 시작 전, 시작점을 오픈리스트에 넣는다.
	//m_OpenList.Insert(StartNode);
	m_OpenList.push_back(StartNode);

	// 2. 반복 시작
	// 오픈 리스트가 빌 때까지 반복.
	//while (m_OpenList.GetCount() != 0)
	while (!m_OpenList.empty())
	{
		// 2-1. 오픈리스트가 비어있지 않다면, 오픈 리스트에서 노드 하나 얻어옴.
		//stFindSearchNode* Node = m_OpenList.GetListNode();
		stFindSearchNode* Node = m_OpenList.front();
		m_OpenList.pop_front();

		// 2-2. 꺼낸 노드가 목적지라면 break;
		if (Node->m_iX == FinNode->m_iX &&
			Node->m_iY == FinNode->m_iY)
		{
			// Node가 목적지이니, 여기부터 부모를 타고가면서 선을 긋는다.
			Show(Node);
			break;
		}

		// 2-3. 목적지 노드가 아니면 꺼낸 노드를 CloseList에 넣기
		//m_CloseList.Insert(Node);
		m_CloseList.push_back(Node);

		if (stMapArrayCopy[Node->m_iY][Node->m_iX].m_eTileType == OPEN_LIST)
			stMapArrayCopy[Node->m_iY][Node->m_iX].m_eTileType = CLOSE_LIST;

		// 2-4. 그리고 노드 생성 함수 호출
		// 이 안에서 상황에 따라 타고 들어간다.
		CreateNodeCheck(Node);
	}

	// 3. 다 끝나면, Open리스트랑 Close리스트 초기화
	//m_CloseList.Clear();
	//m_OpenList.Clear();
	LISTNODE::iterator iter;
	size_t Idx;
	size_t IdxMax;

	iter = m_CloseList.begin();
	Idx = 0;
	IdxMax = m_CloseList.size();
	while (Idx < IdxMax)
	{
		delete (*iter);
		iter = m_CloseList.erase(iter);

		++Idx;
	}

	iter = m_OpenList.begin();
	Idx = 0;
	IdxMax = m_OpenList.size();
	while (Idx < IdxMax)
	{
		delete (*iter);
		iter = m_OpenList.erase(iter);

		++Idx;
	}


	// 4. FinNode는 소멸 안됐으니 소멸시킨다.
	delete[] FinNode;
}

// 인자로 받은 노드를 기준으로, 방향 지정
void CFindSearch::CreateNodeCheck(stFindSearchNode* Node)
{
	int Time = 500;

	// 1. 인자로 받은 노드가 시작점 노드라면, 8방향으로 Jump하면서 노드 생성 여부 체크
	if (Node->stpParent == nullptr)
	{
		// LL
		CheckCreateJump(Node->m_iX -1, Node->m_iY, Node, LL);
		Show();		
		Sleep(Time);		

		// RR
		CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, RR);
		Show();		
		Sleep(Time);

		// UU
		CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, UU);
		Show();	
		Sleep(Time);

		// DD
		CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, DD);
		Show();	
		Sleep(Time);


		// LU
		CheckCreateJump(Node->m_iX -1, Node->m_iY -1, Node, LU);
		Show();	
		Sleep(Time);

		// LD
		CheckCreateJump(Node->m_iX -1, Node->m_iY +1, Node, LD);	
		Show();	
		Sleep(Time);

		// RU
		CheckCreateJump(Node->m_iX +1, Node->m_iY -1, Node, RU);
		Show();	
		Sleep(Time);

		// RD
		CheckCreateJump(Node->m_iX +1, Node->m_iY +1, Node, RD);
		Show();
		Sleep(Time);
		
	}

	// 그게 아니라면, 직선 / 대각선에 따라 노드 생성 여부 체크
	else
	{
		// 상
		if (Node->dir == UU)
		{
			// 상 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, Node->dir);

			// 좌상 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, LU);

			// 우상 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, RU);

			Show();			
		}

		// 하
		else if (Node->dir == DD)
		{
			// 하 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, Node->dir);

			// 좌하 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, LD);

			// 우하 체크
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, RD);

			Show();	
		}

		// 좌
		else if (Node->dir == LL)
		{
			// 좌 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node, Node->dir);

			// 좌상 체크
			if (stMapArrayCopy[Node->m_iY -1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY -1][Node->m_iX -1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX -1, Node->m_iY -1, Node, LU);

			// 좌하 체크
			if (stMapArrayCopy[Node->m_iY +1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY +1][Node->m_iX -1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX -1, Node->m_iY +1, Node, LD);

			Show();			
		}

		// 우
		else if (Node->dir == RR)
		{
			// 우 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, Node->dir);

			// 우상 체크
			if (stMapArrayCopy[Node->m_iY - 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, RU);

			// 우하 체크
			if (stMapArrayCopy[Node->m_iY + 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, RD);

			Show();			
		}

		// 좌상
		else if (Node->dir == LU)
		{
			// 왼쪽 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node, LL);

			// 위쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, UU);

			// 좌상 대각선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, Node->dir);

			// 우상 대각선
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, RU);

			// 좌하 대각선 
			if (stMapArrayCopy[Node->m_iY + 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, LD);

			Show();		
		}

		// 좌하
		else if (Node->dir == LD)
		{
			// 왼쪽 직선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY, Node, LL);

			// 아래쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, DD);

			// 좌하 대각선
			CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, Node->dir);

			// 좌상 대각선
			if (stMapArrayCopy[Node->m_iY - 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, LU);

			// 우하 대각선 
			if (stMapArrayCopy[Node->m_iY][Node->m_iX + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, RD);

			Show();	
		}

		// 우상
		else if (Node->dir == RU)
		{
			// 오른쪽 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, RR);

			// 위쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY - 1, Node, UU);

			// 우상 대각선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, Node->dir);

			// 좌상 대각선
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY - 1, Node, LU);

			// 우하 대각선 
			if (stMapArrayCopy[Node->m_iY + 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, RD);

			Show();		
		}

		// 우하
		else if (Node->dir == RD)
		{
			// 오른쪽 직선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY, Node, RR);

			// 아래쪽 직선
			CheckCreateJump(Node->m_iX, Node->m_iY + 1, Node, DD);

			// 우하 대각선
			CheckCreateJump(Node->m_iX + 1, Node->m_iY + 1, Node, Node->dir);

			// 우상 대각선
			if (stMapArrayCopy[Node->m_iY - 1][Node->m_iX].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY - 1][Node->m_iX + 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX + 1, Node->m_iY - 1, Node, RU);

			// 좌하 대각선 
			if (stMapArrayCopy[Node->m_iY][Node->m_iX - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Node->m_iY + 1][Node->m_iX - 1].m_eTileType != OBSTACLE)
				CheckCreateJump(Node->m_iX - 1, Node->m_iY + 1, Node, LD);

			Show();					
		}

		Sleep(Time);
	}		
}

// 인자로 받은 방향으로 점프하면서, 생성할 노드가 있으면 생성한다.
void CFindSearch::CheckCreateJump(int X, int Y, stFindSearchNode* parent, Direction dir)
{
	int OutX = 0, OutY = 0;
	bool Check = Jump(X, Y, &OutX, &OutY, dir);
	
	if (Check == true)
		CreateNode(OutX, OutY, parent, dir);
}

// 점프
// 해당 방향에 생성할 노드가 있는지 체크(재귀)
bool CFindSearch::Jump(int X, int Y, int* OutX, int* OutY, Direction dir)
{
	// 1. 해당 좌표의 맵이 노드 생성 가능 좌표인지 체크 (맵 안인지, 장해물인지)
	if (stMapArrayCopy[Y][X].m_eTileType == OBSTACLE ||
		X < 0 || X >= MAP_WIDTH || Y < 0 || Y >= MAP_HEIGHT)
		return false;

	// 2. 방향에 따라 진행
	// 상	
	if (dir == UU)
	{
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 목적지가 아니라면, 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 코너가 아니라면, 해당 위치의 타일을 JUMP로 바꾼다. 확인했단는 의미로! 
		// 근데 시작점/도착점/오픈리스트/닫힌리스트에 있으면 안바꾼다. 
		// 그들은 고유하게 색이 유지되어야 한다.
		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}	

		Jump(X, Y - 1, OutX, OutY, dir);
	}

	// 하	
	else if (dir == DD)
	{

		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}	

		Jump(X, Y + 1, OutX, OutY, dir);
	}


	//좌	
	else if (dir == LL)
	{
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}
		

		Jump(X - 1, Y, OutX, OutY, dir);
	}

	// 우	
	else if (dir == RR)
	{
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 현재 노드가 코너인지 체크
		else if ((stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE) ||
				(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}
		

		Jump(X + 1, Y, OutX, OutY, dir);
	}

	// 좌상
	else if (dir == LU)
	{
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 왼쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X - 1, Y, OutX, OutY, LL) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 위 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y - 1, OutX, OutY, UU) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}
		

		Jump(X - 1, Y - 1, OutX, OutY, dir);
	}

	// 좌하
	else if (dir == LD)
	{
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y][X + 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 왼쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X - 1, Y, OutX, OutY, LL) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 아래 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y + 1, OutX, OutY, DD) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}
		

		Jump(X - 1, Y + 1, OutX, OutY, dir);
	}

	// 우상
	else if (dir == RU)
	{
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X - 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y + 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 오른쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X + 1, Y, OutX, OutY, RR) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 위 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y - 1, OutX, OutY, UU) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}
		

		Jump(X + 1, Y - 1, OutX, OutY, dir);
	}

	// 우하
	else if (dir == RD)
	{
		// 현재 노드가 목적지인지 체크
		if (FinNode->m_iX == X && FinNode->m_iY == Y)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 현재 위치가 코너인지 체크
		else if ((stMapArrayCopy[Y][X - 1].m_eTileType == OBSTACLE && stMapArrayCopy[Y + 1][X - 1].m_eTileType != OBSTACLE) ||
			(stMapArrayCopy[Y - 1][X].m_eTileType == OBSTACLE && stMapArrayCopy[Y - 1][X + 1].m_eTileType != OBSTACLE))
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 오른쪽 직선 거리에 코너가 있는지 체크
		else if (Jump(X + 1, Y, OutX, OutY, RR) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		// 아래 직선거리에 코너가 있는지 체크
		else if (Jump(X, Y + 1, OutX, OutY, DD) == true)
		{
			*OutX = X;
			*OutY = Y;
			return true;
		}

		if (stMapArrayCopy[Y][X].m_eTileType != START && stMapArrayCopy[Y][X].m_eTileType != FIN &&
			stMapArrayCopy[Y][X].m_eTileType != OPEN_LIST && stMapArrayCopy[Y][X].m_eTileType != CLOSE_LIST)
		{
			stMapArrayCopy[Y][X].m_eTileType = JUMP;
		}
		

		Jump(X + 1, Y + 1, OutX, OutY, dir);
	}
}

// 노드 생성 함수
void CFindSearch::CreateNode(int iX, int iY, stFindSearchNode* parent, Direction dir)
{	
	// 1. 해당 좌표의 노드가 Open 리스트에 있는지 체크
	//stFindSearchNode* SearchNode = m_OpenList.Search(iX, iY);
	LISTNODE::iterator iter;
	size_t Idx;
	size_t IdxMax;
	stFindSearchNode* SearchNode;

	SearchNode = NULL;
	iter = m_CloseList.begin();
	Idx = 0;
	IdxMax = m_CloseList.size();
	while (Idx < IdxMax)
	{
		if ((*iter)->m_iX == iX &&
			(*iter)->m_iY == iY)
		{
			SearchNode = (*iter);
			break;
		}

		++Idx;
		++iter;
	}

	if (SearchNode != nullptr)
	{
		// 있다면, 해당 노드의 G값과 내가 생성할 노드의 G값 체크.
		// 해당 노드의 G값이 더 크다면, 
		if (SearchNode->m_fG > (parent->m_fG + (fabs(parent->m_iX - iX) + fabs(parent->m_iY - iY))))
		{
			// parent를 부모 설정
			SearchNode->stpParent = parent;

			// 방향도 재설정
			SearchNode->dir = dir;

			// G값 재셋팅 (parent의 G값 + 부모부터 나까지의 거리)
			SearchNode->m_fG = parent->m_fG + (fabs(parent->m_iX - iX) + fabs(parent->m_iY - iY));

			// F값 재셋팅.
			SearchNode->m_fF = (SearchNode->m_fG + SearchNode->m_fH);

			// 정렬도 다시한다.
			//m_OpenList.Sort();
			m_OpenList.sort(NodeCompare);
		}

		return;
	}

	// 2. Close에 리스트에 있는지도 체크
	//SearchNode = m_CloseList.Search(iX, iY);
	SearchNode = NULL;
	iter = m_CloseList.begin();
	Idx = 0;
	IdxMax = m_CloseList.size();
	while (Idx < IdxMax)
	{
		if ((*iter)->m_iX == iX &&
			(*iter)->m_iY == iY)
		{
			SearchNode = (*iter);
			break;
		}

		++Idx;
		++iter;
	}

	if (SearchNode != nullptr)
	{
		// 있다면, 해당 노드의 G값과 내가 생성할 노드의 G값 체크.
		// 해당 노드의 G값이 더 크다면, 
		if (SearchNode->m_fG > (parent->m_fG + (fabs(parent->m_iX - iX) + fabs(parent->m_iY - iY))))
		{
			// parent를 부모 설정
			SearchNode->stpParent = parent;

			// 방향도 재설정
			SearchNode->dir = dir;

			// G값 재셋팅 (parent의 G값 + 부모부터 나까지의 거리)
			SearchNode->m_fG = parent->m_fG + (fabs(parent->m_iX - iX) + fabs(parent->m_iY - iY));

			// F값 재셋팅.
			SearchNode->m_fF = (SearchNode->m_fG + SearchNode->m_fH);

			// 정렬도 다시한다.
			//m_OpenList.Sort();
			m_OpenList.sort(NodeCompare);
		}

		return;
	}

	// 3. 둘 다에 없으면, 노드 생성 후, Open에 넣는다.
	stFindSearchNode* NewNode = new stFindSearchNode[sizeof(stFindSearchNode)];

	NewNode->stpParent = parent;
	NewNode->dir = dir;

	NewNode->m_iX = iX;
	NewNode->m_iY = iY;

	NewNode->m_fG = parent->m_fG + (fabs(parent->m_iX - iX) + fabs(parent->m_iY - iY));
	NewNode->m_fH = fabs(FinNode->m_iX - iX) + fabs(FinNode->m_iY - iY);
	NewNode->m_fF = NewNode->m_fG + NewNode->m_fH;

	// 해당 노드가 시작,도착, 장해물노드가 아니면, OPEN_LIST로 변경
	if (stMapArrayCopy[iY][iX].m_eTileType != START && stMapArrayCopy[iY][iX].m_eTileType != FIN && stMapArrayCopy[iY][iX].m_eTileType != OBSTACLE)
		stMapArrayCopy[iY][iX].m_eTileType = OPEN_LIST;

	//m_OpenList.Insert(NewNode);
	m_OpenList.push_back(NewNode);

	// 4. 정렬도 다시한다.
	//m_OpenList.Sort();
	m_OpenList.sort(NodeCompare);

	// 5. 출력한다.
	Show();
}

// 그리기
void CFindSearch::Show(stFindSearchNode* Node)
{
	static stFindSearchNode* SaveNode = nullptr;

	if(Node!= nullptr)
		SaveNode = Node;

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

			// CLOSE_LIST는 어두운 파란색
			else if (stMapArrayCopy[i][j].m_eTileType == CLOSE_LIST)
			{
				hMyBrush = CreateSolidBrush(RGB(0, 0, 127));
				hOldBrush = (HBRUSH)SelectObject(MemDC, hMyBrush);
				bBrushCheck = true;
			}

			// JUMP는 연한 노란색
			else if (stMapArrayCopy[i][j].m_eTileType == JUMP)
			{
				hMyBrush = CreateSolidBrush(RGB(255, 255, 0));
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
	if (SaveNode != nullptr)
	{
		MoveToEx(MemDC, stMapArrayCopy[SaveNode->m_iY][SaveNode->m_iX].m_iMapX,
			stMapArrayCopy[SaveNode->m_iY][SaveNode->m_iX].m_iMapY, NULL);

		HPEN hMyPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
		HPEN OldPen = (HPEN)SelectObject(MemDC, hMyPen);

		stFindSearchNode* ParentNode = SaveNode->stpParent;
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
	ReleaseDC(hWnd, hdc);
}
//
bool NodeCompare(CFindSearch::stFindSearchNode* _l, CFindSearch::stFindSearchNode* _r)
{
	return _l->m_fF < _r->m_fF;
}