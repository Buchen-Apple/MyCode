#ifndef __A_SEARCH_H__
#define __A_SEARCH_H__
#include "DLinked_List.h"

struct stFindSearchNode;

// 맵 타일의 타입 enum
enum Tile_Type
{
	NONE = 0, START, FIN, OBSTACLE, OPEN_LIST, CLOSE_LIST, JUMP, TEST
};

// 맵 데이터 구조체.
struct Map
{
	// 여기에 저장되는 X,Y는 실제 픽셀 좌표
	int m_iMapX;
	int m_iMapY;
	Tile_Type m_eTileType;
	COLORREF m_coColorRGB;
};




class CFindSearch
{
public:
	// 생성자
	CFindSearch(Map* Copy, int Width, int Height, int Radius, int MaxSearchCount);

	// 소멸자
	~CFindSearch();

	// 노드의 방향 enum
	enum Direction
	{
		NONE = 0, LL, LU, LD, RR, RU, RD, UU, DD
	};

	// 노드 데이터 구조체
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

//////////////////////////
// 클래스 변수
/////////////////////////
private:
	// Jump로 체크한 타일을 칠할 색. 8개를 만들어두고 순서대로 돌려쓴다.
	COLORREF Color1;
	COLORREF Color2;
	COLORREF Color3;
	COLORREF Color4;
	COLORREF Color5;
	COLORREF Color6;
	COLORREF Color7;
	COLORREF Color8;
	
	// 맵 Array 포인터
	Map* stMapArrayCopy;		

	// 열린 리스트, 닫힌 리스트
	DLinkedList<stFindSearchNode*> m_OpenList;
	DLinkedList<stFindSearchNode*> m_CloseList;

	// 시작점, 도착점
	stFindSearchNode* StartNode;
	stFindSearchNode* FinNode;

	// 그리기용 변수들 (DC용)
	HWND m_hWnd;
	HDC m_hMemDC;
	HBITMAP m_hMyBitmap, m_hOldBitmap;
	RECT rt;

	// 현재 사용중인 컬러가 몇 번인지 저장. Color1 ~ Color8
	int m_iColorKey = 1;

	// 맵의 넓이, 높이, 타일1개의 반지름, 몇개의 타일까지 Jump할 것인지 저장하는 변수.
	int m_iWidth, m_iHeight, m_iRadius, m_iMaxSearchCount;

	// G값 계산 시 직선 가중치, 대각선 가중치
	double m_dStraight_Line;
	double m_dDiagonal_Line;

//////////////////////////
// JPS 로직처리용 함수들
/////////////////////////
private:	

	// 인자로 받은 노드 체크. 노드의 속성에 따라 CheckCreateJump() 함수에 인자 전달.
	void CreateNodeCheck(stFindSearchNode* parent, bool ShowCheck = true);

	// 인자로 받은 노드 기준, 어느 방향으로 이동할 것인지 결정. 후 그 방향의 Jump??() 함수 호출
	void CheckCreateJump(int X, int Y, stFindSearchNode* parent, COLORREF colorRGB, Direction dir);	

	// CheckCreateJump()함수에서 호출된다. 지정된 방향으로 점프하면서 노드 생성 위치 체크
	bool JumpUU(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);
	bool JumpDD(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);
	bool JumpRR(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);
	bool JumpLL(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);
	bool JumpLU(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);
	bool JumpLD(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);
	bool JumpRU(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);
	bool JumpRD(int X, int Y, int* OutX, int* OutY, double G, double* OutG, COLORREF colorRGB, Direction dir);

	// Jump??() 함수에서 true가 호출되면 CheckCreateJump()함수가 해당 함수 호출. 
	// 노드 생성 함수
	void CreateNode(int iX, int iY, stFindSearchNode* parent, double G, Direction dir);

	// Jump_Point_Search_While()가 완료될 때, 오픈/닫힌 리스트 정리 등 하기
	void LastClear();

public:
	// 처음 시작 노드를 오픈리스트에 넣는 용도.
	void StartNodeInsert();

	// 시작점, 도착점 셋팅하기
	void Init(int StartX, int StartY, int FinX, int FinY);

	// 호출될 때 마다 Jump Point Search 진행
	void Jump_Point_Search_While();


//////////////////////////
// 그리기용 함수
/////////////////////////
private:	
	// 내부에서 선긋기용
	void Line(stFindSearchNode* Node, COLORREF rgb = -1);

public:
	// 윈도우 핸들을 전달받고 MemDC등을 셋팅하는 함수
	void DCSet(HWND hWnd);

	// 그리드 그리기
	void GridShow();

	// 외부에서 POINT인자를 받아서 선 긋기
	void Out_Line(POINT* p, int Count, COLORREF rgb = -1);


//////////////////////////
// 브레젠헴 알고리즘 관련 함수
/////////////////////////
private:
	// 브레젠헴 실제 체크
	void Bresenham_Line(stFindSearchNode* FinNode);	

public:
	// 밖에서 시작점, 출발점과 Map 구조체를 받으면, 경로를 POINT구조체 안에 넣어주는 함수. 
	// POINT구조체 배열 0번부터 채워준다.
	// Use_Bresenham변수에 true를 넣으면 브레젠헴 알고리즘으로 길찾는다.
	// 배열의 수를 리턴한다.
	int PathGet(int StartX, int StartY, int FinX, int FinY, POINT* Outmap, bool Use_Bresenham);

	// 브레젠헴 테스트 함수(타일 색칠하기. 지금 Main함수에서 호출중)
	void Bresenham_Line_Test(int StartX, int StartY, int FinX, int FinY);
};


#endif // !__A_SEARCH_H__
