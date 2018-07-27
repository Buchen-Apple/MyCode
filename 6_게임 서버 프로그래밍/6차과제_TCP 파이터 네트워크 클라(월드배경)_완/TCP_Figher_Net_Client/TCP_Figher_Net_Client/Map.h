#ifndef __MAP_H__
#define __MAP_H__
#include "SpriteDib.h"

class BaseOBject;

class CMap
{
	friend class BaseOBject;

	//*******************************************
	// 프라이빗 함수
	//*******************************************
private:
	// 싱글톤을 위해 생성자는 Private으로 선언
	CMap(int TotalTileWidth, int TotalTileHeight, int WinWidth, int WinHeight, int TileSize);

	// 타일 구조체
	struct MapTile
	{
		// 픽셀 단위 x,y,반지름
		int m_x;		
		int m_y;
		int m_MapIndex;		// 표시할 타일의 인덱스. 지금은 다 0이다.
		int m_redius;
	};

	//*******************************************
	// 계속 변경되는 프라이빗 변수
	//*******************************************
private:
	// 카메라 좌표. 월드좌표(픽셀) 중 X. 좌상단
	int m_iCameraPosX;

	// 카메라 좌표. 월드좌표(픽셀) 중 Y. 좌상단
	int m_iCameraPosY;


	//*******************************************
	// 최초 셋팅 후 변경되지 않는 변수
	//*******************************************
private:
	// 타일 1개의 사이즈
	int m_TileSize;

	// 윈도우 사이즈 높이
	int m_WindowHeight;

	// 윈도우 사이즈 넓이
	int m_WindowWidth;

	// 저장하고있는 인덱스의 갯수 (세로)
	int m_Height;

	// 저장하고있는 인덱스의 갯수 (가로)
	int m_Width;

	// 싱글톤으로 CSpritedib형 객체
	CSpritedib* g_cSpriteDib;

	// MapTile 구조체
	MapTile** Tile;

	// 윈도우 크기 기준, 한 화면에 표시되는 타일의 갯수
	// 640x480기준, 높이는 (480/64 = 7.5(반올림해서 8) + 1 = 9개) / 넓이는 (640/64 =  10 + 1 = 11개) 이다.
	int m_HeightTileCount;
	int m_WidthTileCount;

public:

	// 싱글톤 생성 함수
	static CMap* Getinstance(int TotalTileWidth, int TotalTileHeight, int WinWidth, int WinHeight, int TileSize);

	// 소멸자
	~CMap();

	// 카메라 X좌표 얻기
	int GetCameraPosX();

	// 카메라 Y좌표 얻기
	int GetCameraPosY();
	
	// 맵을 드로우하는 함수
	bool MapDraw(BYTE* bypDest, int DestWidth, int DestHeight, int DestPitch);

	// 카메라 좌표 셋팅
	void SetCameraPos(int PlayerX, int PlayerY, int NowSpriteIndex);

	// 카메라 좌표를 기준으로, 내가 출력되어야 하는 월드좌표 알아내기 (X좌표)
	int GetShowPosX(int NowX);

	// 카메라 좌표를 기준으로, 내가 출력되어야 하는 월드좌표 알아내기 (Y좌표)
	int GetShowPosY(int NowY);

};

#endif // !__MAP_H__
