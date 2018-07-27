#include "stdafx.h"
#include "Map.h"
#include <cmath>



//*******************************************
// 싱글톤 생성 함수
// 맵의 전체 타일 갯수(넓이), 맵 전체 타일 갯수(높이), 윈도우 사이즈(넓이), 윈도우 사이즈(높이), 타일 1개의 사이즈를 입력받음.
//*******************************************

CMap* CMap::Getinstance(int TotalTileWidth, int TotalTileHeight, int WinWidth, int WinHeight, int TileSize)
{
	static CMap SingletonMap(TotalTileWidth, TotalTileHeight, WinWidth, WinHeight, TileSize);
	return &SingletonMap;
}

// 생성자
// 맵의 전체 타일 갯수(넓이), 맵 전체 타일 갯수(높이), 윈도우 사이즈(넓이), 윈도우 사이즈(높이), 타일 1개의 사이즈
CMap::CMap(int TotalTileWidth, int TotalTileHeight, int WinWidth, int WinHeight, int TileSize)
{
	// 1. 싱글톤으로 CSpritedib형 객체 얻어오기.
	g_cSpriteDib = CSpritedib::Getinstance(70, 0x00ffffff, 32);

	// 2. 맵 전체의 타일 갯수 저장.
	m_Height = TotalTileHeight;
	m_Width = TotalTileWidth;
	m_TileSize = TileSize;			// 타일 1개의 사이즈도 저장.

	// 3. 윈도우 사이즈 저장
	m_WindowHeight = WinHeight;
	m_WindowWidth = WinWidth;

	// 4. 윈도우 사이즈 기준, 한 화면에 표시되는 타일 갯수 저장
	m_HeightTileCount = round( (float)WinHeight / (float)TileSize ) + 1;
	m_WidthTileCount = round((float)WinWidth / (float)TileSize) + 1;

	// 5. 타일 갯수만큼 2차원 배열로 동적할당
	Tile = new MapTile*[TotalTileHeight];
	for (int i = 0; i < TotalTileHeight; ++i)
		Tile[i] = new MapTile[TotalTileWidth];

	// 6. 타일 갯수만큼 x,y,반지름, 해당 타일의 인덱스 설정(픽셀 좌표)
	int TempRadius = TileSize / 2;

	int SaveX = 0;
	int SaveY = 0;
	for (int i = 0; i < TotalTileHeight; ++i)
	{		
		for (int h = 0; h < TotalTileWidth; ++h)
		{
			Tile[i][h].m_x = SaveX + TempRadius;
			Tile[i][h].m_y = SaveY + TempRadius;
			Tile[i][h].m_redius = TempRadius;
			Tile[i][h].m_MapIndex = 0;

			SaveX += TempRadius * 2;
		}

		SaveX = 0;
		SaveY += TempRadius * 2;
	}
}

// 소멸자
CMap::~CMap()
{
	// 타일 동적해제
	for (int i = 0; i < m_Height; ++i)
		delete Tile[i];

	delete Tile;	
}


// 카메라 X좌표 얻기
int CMap::GetCameraPosX()
{
	return m_iCameraPosX;
}

// 카메라 Y좌표 얻기
int CMap::GetCameraPosY()
{
	return m_iCameraPosY;
}

// 카메라 좌표를 기준으로, 화면 좌표 알아내기 (X좌표)
int CMap::GetShowPosX(int NowX)
{		
	int temp = NowX - m_iCameraPosX;

	return temp;
}

// 카메라 좌표를 기준으로, 화면 좌표 알아내 (Y좌표)
int CMap::GetShowPosY(int NowY)
{
	int temp = NowY - m_iCameraPosY;

	return temp;
}

// 카메라 좌표 셋팅
void  CMap::SetCameraPos(int PlayerX, int PlayerY, int NowSpriteIndex)
{	
	// 입력받은 x,y는 플레이어의 중점 기준 월드좌표이다. (비트맵 좌상단 아님!)

	// 1. 카메라 좌표 셋팅 전, 대략적으로 화면의 중심? 개념의 좌표를 계산한다.
	// X는 상관없고, Y만구한다. 중점이 약간 아래에 있어서 받은 좌표 그대로 카메라 위치를 구하면 정중앙이 아닐수도 있다.
	// 이 좌표를 기준으로 카메라 위치를 구한다.
	int TempCenterX = PlayerX;
	int TempCenterY = PlayerY;

	// 2. 카메라 위치 셋팅.
	m_iCameraPosX = TempCenterX - (m_WindowWidth / 2);
	m_iCameraPosY = TempCenterY - (m_WindowHeight / 2);

	// 3. 만약, 카메라 위치가 - 라면 0으로 바꿔준다.
	if (m_iCameraPosX < 0)
		m_iCameraPosX = 0;

	if (m_iCameraPosY < 0)
		m_iCameraPosY = 0;

	// 3. 만약, 카메라 위치가 >> 끝이라면 x 좌표는 고정된다.
	if (m_iCameraPosX + m_WindowWidth > m_TileSize * m_Width)
		m_iCameraPosX = (m_TileSize * m_Width) - m_WindowWidth;

	// 3. 만약 카메라 위치가 아래 끝이라면, y좌표는 고정된다.
	if (m_iCameraPosY + m_WindowHeight > m_TileSize * m_Height)
		m_iCameraPosY = (m_TileSize * m_Height) - m_WindowHeight;


}

// 맵을 드로우하는 함수
bool CMap::MapDraw(BYTE* bypDest, int DestWidth, int DestHeight, int DestPitch)
{
	// Dest의 시작 시작 위치 저장해두기 ---------
	BYTE* FirstDestSave = bypDest;

	// 카메라 좌표 기준, 이번 프레임에 출력할, 화면 좌상단 타일 index 구하기
	int NowTileY = m_iCameraPosY / m_TileSize;
	int NowTileX = m_iCameraPosX / m_TileSize;	

	// 만약, 인덱스가 - 라면 0으로 바꿔준다.
	if (NowTileX < 0)
		NowTileX = 0;

	if (NowTileY < 0)
		NowTileY = 0;

	// 유저가 오른쪽 끝에 도착했으면 더 이상 스크롤되면 안된다.
	// 타일의 X인덱스를 89(100기준)으로 고정.
	if (NowTileX > m_Width - m_WidthTileCount)
		NowTileX = m_Width - m_WidthTileCount;

	// 유저가 아래 끝에 도착했으면, 더 이상 스크롤되면 안된다.
	if (NowTileY > m_Height - m_HeightTileCount)
		NowTileY = m_Height - m_HeightTileCount;

	int i;
	int h;
	int AddTileX = 0;
	int AddTileY = 0;
	// m_HeightTileCount
	for (i = 0; i < m_HeightTileCount; ++i)
	{
		BYTE* SecondDestSave = bypDest;
		int TempHeight = 0;

		for (h = 0; h < m_WidthTileCount; ++h)
		{
			BYTE* save = g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].bypImge;	// 다쓴 후 원복할 타일의 시작 주소(0,0)

			// 해당 함수에서만 사용할, 변수들을 선언한다. 클리핑을 하기 때문에 로컬 변수가 필요하다.
			int iSpriteWidth = g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].iWidth;
			int iSpriteHeight = g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].iHeight;
			int iSpritePitch = g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].iPitch;	

			// 해당 타일의 좌상단 알아오기 저장.(화면 좌표)
			int iSpriteShowX = GetShowPosX(Tile[NowTileY][NowTileX].m_x - Tile[NowTileY][NowTileX].m_redius);
			int iSpriteShowY = GetShowPosY(Tile[NowTileY][NowTileX].m_y - Tile[NowTileY][NowTileX].m_redius);

			// 스프라이트 시작 위치 저장.
			int iSpriteStartX = 0;
			int iSpriteStartY = 0;	
						
			// 클리핑 처리 -------------
			// 상
			if (iSpriteShowY < 0)
			{
				int iAddY = iSpriteShowY * -1;				// 위로 얼마나 올라갔는지 구함.
				iSpriteStartY += iAddY;						// 스프라이트의 시작점 y 위치를 아래로 iAddY만큼 내린다.
				iSpriteHeight -= iAddY;						// 스프라이트의 높이를 iAddY만큼 줄인다. 

				// 높이가 -가 나왔다면 높이를 0으로 만든다.
				if (iSpriteHeight < 0)
					iSpriteHeight = 0;
			}

			 // 하
			if (iSpriteShowY + m_TileSize > m_WindowHeight)
			{
				int iAddY = (iSpriteShowY + m_TileSize) - m_WindowHeight;		// 아래로 얼마나 내려갔는지 구함	
				iSpriteHeight -= iAddY;											// 스프라이트의 높이를 iAddY만큼 줄인다. 	

				// 높이가 -가 나왔다면 높이를 0으로 만든다.
				if (iSpriteHeight < 0)
					iSpriteHeight = 0;						 		
			}

			// 좌
			int iAddX = 0;
			if (iSpriteShowX < 0)
			{
				iAddX = iSpriteShowX *-1;				// 좌로 얼마나 갔는지 구함.
				iSpriteStartX += iAddX;					// 스프라이트의 시작점 x 위치를 우측으로 iAddX만큼 움직인다.
				iSpriteWidth -= iAddX;					// 넓이도 그만큼 줄인다.

				// 넓이가 -가 나왔으면 넓이를 0으로 만든다.
				if (iSpriteWidth < 0)
					iSpriteWidth = 0;
			}

			// 우 (>>화면 끝에 도착하면 스크롤 되지 않음)
			if ((iSpriteShowX + m_TileSize > m_WindowWidth))
			{
				iAddX = iSpriteShowX + m_TileSize - m_WindowWidth;	// 우로 얼마나 갔는지 구함. 얘는 이러면 끝!
				iSpriteWidth -= iAddX;					// 넓이도 그만큼 줄인다.
			}
			

			// 스프라이트의 시작 포인터 이동. ----------
			// 여기서 실제 바이트 단위로 계산한다.
			g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].bypImge += (iSpriteStartX * g_cSpriteDib->m_iColorByte) + (iSpriteStartY * iSpritePitch);

			// Dest의 시작 시작 위치 저장해두기 ---------
			BYTE* DestSave = bypDest;

			// 스프라이트 복사 -------------
			// 시작점부터 1줄씩 아래로 이동
			int cpySize = iSpriteWidth *  g_cSpriteDib->m_iColorByte;
			for (int aa = 0; aa < iSpriteHeight; ++aa)
			{				
				memcpy(bypDest, g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].bypImge, cpySize);
				bypDest += DestPitch;
				g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].bypImge += iSpritePitch;
			}

			// Dest의 시작 시작 위치 원복 ---------
			bypDest = DestSave;			

			// 내 스프라이트 버퍼의 주소를 증가시키면서 출력했으니 원래 위치로 이동시킨다.
			g_cSpriteDib->m_stpSprite[Tile[NowTileY][NowTileX].m_MapIndex].bypImge = save;

			TempHeight = iSpriteHeight;

			// 타일 Index의 X위치를 >>로 1칸 이동
			if (NowTileX + 1 == m_Width)
				break;

			NowTileX++;
			AddTileX++;

			// 백버퍼의 위치를 1칸 >>로 이동(타일 기준 1칸)
			bypDest += cpySize;			
		}

		// 백버퍼의 위치를 원복 후, 타일 1개만큼 아래로 내린다.
		bypDest = SecondDestSave;
		bypDest += (DestPitch * TempHeight);

		// 타일 Index의 x위치를 처음 위치로 이동
		NowTileX -= AddTileX;
		AddTileX = 0;

		// 타일 Index의 y위치를 1줄 아래로 이동
		if(NowTileY + 1 != m_Height)
			NowTileY++;

	}

	bypDest = FirstDestSave;

	return TRUE;	
}