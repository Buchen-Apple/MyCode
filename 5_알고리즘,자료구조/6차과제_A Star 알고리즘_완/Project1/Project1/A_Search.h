#ifndef __A_SEARCH_H__
#define __A_SEARCH_H__
#include "DLinked_List.h"
#include "MapTile.h"

class CFindSearch
{
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
	};

private:
	Map (*stMapArrayCopy)[MAP_WIDTH];			// 맵 Array 사본
	DLinkedList<stFindSearchNode*> m_OpenList;
	DLinkedList<stFindSearchNode*> m_CloseList;

	stFindSearchNode* StartNode;
	stFindSearchNode* FinNode;

	HWND hWnd;

public:
	// 생성자
	CFindSearch(Map (*Copy)[MAP_WIDTH]);

	// 소멸자
	~CFindSearch();

	// 반복문 돌면서 에이스타 진행
	void A_Search_While();

	// 시작점, 도착점 셋팅하기
	void Init(int StartX, int StartY, int FinX, int FinY, HWND hWnd);

	// 노드 생성 함수
	void CreateNode(int iX, int iY, stFindSearchNode* parent, double fWeight = 0);

	// 그리기
	void Show(stFindSearchNode* Node = nullptr);

};


#endif // !__A_SEARCH_H__
