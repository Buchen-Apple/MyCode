#ifndef __A_SEARCH_H__
#define __A_SEARCH_H__
//#include "DLinked_List.h"
#include "MapTile.h"

class CFindSearch
{
	enum Direction
	{
		NONE = 0, LL, LU, LD, RR, RU, RD, UU, DD
	};
public:
	struct stFindSearchNode
	{
		// 여기에 저장되는 x,y는 블록단위로 저장.
		int m_iX;
		int m_iY;

		// G : 출발지부터 현재 노드까지의 거리.
		// H : 현재 노드부터 도착지까지의 거리
		// F : G + H
		double m_fG;
		double m_fH;
		double m_fF;
		stFindSearchNode* stpParent;

		// 방향 지정
		Direction dir;
	};

private:
	Map(*stMapArrayCopy)[MAP_WIDTH];			// 맵 Array 사본

	//
	typedef std::list<stFindSearchNode*> LISTNODE;
	LISTNODE m_OpenList;
	LISTNODE m_CloseList;
	//
	//DLinkedList<stFindSearchNode*> m_OpenList;
	//DLinkedList<stFindSearchNode*> m_CloseList;

	stFindSearchNode* StartNode;
	stFindSearchNode* FinNode;

	HWND hWnd;

public:
	// 생성자
	CFindSearch(Map(*Copy)[MAP_WIDTH]);

	// 소멸자
	~CFindSearch();

	// 반복문 돌면서 Jump Point Search 진행
	void Jump_Point_Search_While();

	// 인자로 받은 노드를 기준으로, 어느 방향으로 이동할 것인지 결정.
	void CheckCreateJump(int X, int Y, stFindSearchNode* parent, Direction dir);

	// 인자로 받은 노드를 기준으로, 노드 생성 가능 여부 체크
	void CreateNodeCheck(stFindSearchNode* parent);

	// 지정된 방향으로 점프하면서 체크
	bool Jump(int X, int Y, int* OutX, int* OutY, Direction dir);

	// 시작점, 도착점 셋팅하기
	void Init(int StartX, int StartY, int FinX, int FinY, HWND hWnd);

	// 노드 생성 함수
	void CreateNode(int iX, int iY, stFindSearchNode* parent, Direction dir);

	// 그리기
	void Show(stFindSearchNode* Node = nullptr);

};

bool NodeCompare(CFindSearch::stFindSearchNode* _l, CFindSearch::stFindSearchNode* _r);

#endif // !__A_SEARCH_H__
