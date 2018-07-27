#include "stdafx.h"
#include "SpriteDIB.h"
#include <stdio.h>

// *******************************************
// 생성자, 파괴자 (싱글톤을 위해 생성자는 Private)
// *******************************************
CSpritedib::CSpritedib(int iMaxSprite, DWORD dwColorKey, int iColorBit)
{
	// 해당 클래스의 최대 스프라이트 수를 변수에 저장.	
	m_iMaxSprite = iMaxSprite;

	// 최대 스프라이트 수 만큼 미리 공간을 할당받는다.
	m_stpSprite = new st_SPRITE[m_iMaxSprite];

	// 컬러키를 저장한다. 해당 클래스에 저장되는 모든 스프라이트는 동일한 컬러키로 처리된다.
	m_dwColorKey = dwColorKey;

	// 해당 스프라이트의 컬러 비트 저장
	m_iColorByte = iColorBit / 8;
}

CSpritedib::~CSpritedib()
{
	// 전체를 돌면서 모든 스프라이트를 지운다.
	for (int i = 0; i < m_iMaxSprite; ++i)
		ReleseSprite(i);
}


// *******************************************
// 싱글톤 생성 함수
// *******************************************
CSpritedib* CSpritedib::Getinstance(int iMaxSprite, DWORD dwColorKey, int iColorBit)
{
	static CSpritedib cSpritedib(iMaxSprite, dwColorKey, iColorBit);
	return &cSpritedib;
}

//*******************************************
// 게임 초반 세팅 함수(현재는 모든 비트맵을 읽어온다
//*******************************************
void CSpritedib::GameInit()
{
	// 필요한 스프라이트 로딩해두기.
	BOOL iCheck = LoadDibSprite(0, "..\\SpriteData\\_Map.bmp", 0, 0);
	if (!iCheck)
		exit(-1);

	iCheck = LoadDibSprite(1, "..\\SpriteData\\Attack1_L_01.bmp", 71, 90);
	if (!iCheck)
		exit(-1);

	iCheck = LoadDibSprite(2, "..\\SpriteData\\Attack1_L_02.bmp", 71, 90);
	if (!iCheck)
		exit(-1);

	iCheck = LoadDibSprite(3, "..\\SpriteData\\Attack1_L_03.bmp", 71, 90);
	if (!iCheck)
		exit(-1);

	iCheck = LoadDibSprite(4, "..\\SpriteData\\Attack1_L_04.bmp", 71, 90);
	if (!iCheck)
		exit(-1);

}

//*******************************************
// 인자로 받은 컬러를 붉은 계열로 변경.
// 플레이어 캐릭터의 색 변경 시 사용 (구분을 위해)
//*******************************************
DWORD CSpritedib::PlayerColorRed(BYTE* SorRGB)
{
	DWORD TempRGB;

	memcpy(&TempRGB, SorRGB, m_iColorByte);

	BYTE r = GetRValue(TempRGB);
	BYTE g = GetGValue(TempRGB);
	BYTE b = GetBValue(TempRGB);

	g = g / 2;
	b = b / 2;

	TempRGB = RGB(b, g, r);	// 나는 4바이트씩 계산하기 때문에, ARGB 순서대로 되어있는 데이터가 필요하다.

	return TempRGB;
}

// *******************************************
// BMP파일을 읽어서 하나의 스프라이트로 저장.
// *******************************************
BOOL CSpritedib::LoadDibSprite(int iSpriteIndex, char* szFileName, int iCenterPointX, int iCenterPointY)
{
	// 만약, 전달된 인덱스가 최대 스프라이트 개수보다 크다면 false 반환.
	if (m_iMaxSprite <= iSpriteIndex)
		return FALSE;

	size_t				dwRead;
	int					iPitch;
	int					iImageSize;
	BITMAPFILEHEADER	stFileHeader;
	BITMAPINFOHEADER	stInfoHeader;

	// 비트맵 헤더를 열어 BMP 파일 확인-------------
	FILE* fp;
	int iCheck = fopen_s(&fp, szFileName, "rb");
	if (iCheck != NULL)
		return FALSE;

	// 파일헤더, 인포헤더 읽어오기.----------------
	dwRead = fread(&stFileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	if (dwRead != 1)
		return FALSE;

	fread(&stInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	if (dwRead != 1)
		return FALSE;

	// 피치값 구하기--------------
	iPitch = (stInfoHeader.biWidth * (stInfoHeader.biBitCount / 8)) + 3 & ~3;	// 실제 가로 메모리 길이 (pitch는 바이트 단위 실제 가로 길이)	

	// 스프라이트 구조체에 필요한 값 저장.--------------
	m_stpSprite[iSpriteIndex].iCenterPointX = iCenterPointX;	// 중점 x 저장
	m_stpSprite[iSpriteIndex].iCenterPointY = iCenterPointY;	// 중점 y저장
	m_stpSprite[iSpriteIndex].iHeight = stInfoHeader.biHeight;	// 높이 저장
	m_stpSprite[iSpriteIndex].iPitch = iPitch;					// 계산된 pitch 저장
	m_stpSprite[iSpriteIndex].iWidth = stInfoHeader.biWidth;	// 넓이 저장.

	// 이미지 전체 크기 저장 후 스프라이트 버퍼에 메모리 동적할당.
	iImageSize = iPitch * stInfoHeader.biHeight;		
	m_stpSprite[iSpriteIndex].bypImge = new BYTE[iImageSize];

	// ---------------------------------------
	// 이미지를 스프라이트 버퍼로 읽어온다.
	// DIB는 뒤집혀져 있으니 임시 버퍼에 읽은 후, 뒤집으면서 내 스프라이트 버퍼에 복사한다.
	// ---------------------------------------
	BYTE* bypTempBuffer = new BYTE[iImageSize];
	fread(bypTempBuffer, iImageSize, 1, fp);
	if (dwRead != 1)
		exit(-1);
	
	// 비트맵 이미지의 높이만큼 반복하며 비트맵 하단부터 복사 (뒤집으며 복사)
	bypTempBuffer += (iImageSize - iPitch);
	for (int i = 0; i < stInfoHeader.biHeight; ++i)
	{
		memcpy(m_stpSprite[iSpriteIndex].bypImge, bypTempBuffer, iPitch);
		m_stpSprite[iSpriteIndex].bypImge += iPitch;
		bypTempBuffer -= iPitch;
	}

	// 내 스프라이트 버퍼의 주소를 증가시키면서 출력했으니 원래 위치로 이동시킨다.
	m_stpSprite[iSpriteIndex].bypImge -= (iPitch * stInfoHeader.biHeight);

	// 파일 스트림 닫고 다 쓴 핸들 클로즈-------------
	fclose(fp);
	return TRUE;
}

// *******************************************
// 전달된 Index의 스프라이트를 해제.
// *******************************************
void CSpritedib::ReleseSprite(int iSpriteIndex)
{
	// 전달된 스프라이트 Index가 최대 스프라이트 이상이라면 잘못된 전달이니 그냥 리턴.
	if (m_iMaxSprite <= iSpriteIndex)
		return;

	// 전달된 스프라이트 Index가 사용 중인 스프라이트라면, 그때 해제.
	else if (NULL != m_stpSprite[iSpriteIndex].bypImge)
	{
		delete[] m_stpSprite[iSpriteIndex].bypImge;		// 먼저, m_stpSprite에 할당된 Img동적할당 공간 해제.
		memset(&m_stpSprite[iSpriteIndex], 0, sizeof(st_SPRITE));	// 그리고, 해당 Index의 메모리를 0으로 리셋시킨다. 여기까지 하면 해제한 것.
	}
}

// *******************************************
// 특정 메모리 위치에 스프라이트를 출력한다 (칼라키/클리핑 처리, 캐릭터 찍을때 사용)
// *******************************************
BOOL CSpritedib::DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE* bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch, int iDrawLen)
{
	// 만약, 전달된 인덱스가 최대 스프라이트 개수보다 크다면 없는 스프라이트를 출력하라고 한 것이니, false 반환.
	if (m_iMaxSprite <= iSpriteIndex)
		return FALSE;

	BYTE* save = m_stpSprite[iSpriteIndex].bypImge;	// 다쓴 후 원복할 스프라이트 이미지의 시작 주소(0,0)

	// 해당 함수에서만 사용할, 변수들을 선언한다. 클리핑을 하기 때문에 로컬 변수가 필요하다.
	//int iSpriteWidth = m_stpSprite[iSpriteIndex].iWidth;		<<이거 가져왔는데 안써서 그냥 주석처리함. 나중에 필요하면 주석 해제하자.
	int iSpriteHeight = m_stpSprite[iSpriteIndex].iHeight;
	int iSpritePitch = m_stpSprite[iSpriteIndex].iPitch;

	// iDrawX, iDrawY는 스프라이트의 중점과 맵핑되는 좌표이다. 실제 비트맵을 찍어야 하는 내 백버퍼의 x,y 위치를 구한다. (iDestStartX, iDestStartY)
	// 아직 까지는 픽셀 단위로 계산이다.
	int iDestStartX = iDrawX - m_stpSprite[iSpriteIndex].iCenterPointX;
	int iDestStartY = iDrawY - m_stpSprite[iSpriteIndex].iCenterPointY;

	// 스프라이트 이미지의, 시작 위치를 구한다. 클리핑 처리를 하려면, 이미지 찍는 시작 위치를 이동시켜야 한다.
	int iSpriteStartX = 0;
	int iSpriteStartY = 0;
	
	// 클리핑 처리 -------------
	// 상
	if (iDestStartY < 0)
	{
		int iAddY = iDestStartY * -1;	// 위로 얼마나 올라갔는지 구함.
		iDestStartY = 0;				// 일단, Dest(내 백버퍼)의 y위치는 0이다.
		iSpriteStartY += iAddY;			// 스프라이트의 시작점 y 위치를 아래로 iAddY만큼 내린다.
		iSpriteHeight -= iAddY;			// 스프라이트의 높이를 iAddY만큼 줄인다. 
	}

	// 하
	if (iDestStartY + m_stpSprite[iSpriteIndex].iHeight > iDestHeight)
	{
		int iAddY = iDestStartY + m_stpSprite[iSpriteIndex].iHeight - iDestHeight;		// 아래로 얼마나 내려갔는지 구함	
		iSpriteHeight -= iAddY;															// 스프라이트의 높이를 iAddY만큼 줄인다. 		
	}

	// 좌
	int iAddX = 0;
	if (iDestStartX < 0)
	{
		iAddX = iDestStartX * -1;	// 좌로 얼마나 갔는지 구함.
		iDestStartX = 0;				// 일단, Dest(내 백버퍼)의 x위치는 0이다.
		iSpriteStartX += iAddX;			// 스프라이트의 시작점 x 위치를 우측으로 iAddX만큼 움직인다.
	}

	// 우
	if (iDestStartX + m_stpSprite[iSpriteIndex].iWidth > iDestWidth)
		iAddX = iDestStartX + m_stpSprite[iSpriteIndex].iWidth - iDestWidth;	// 우로 얼마나 갔는지 구함. 얘는 이러면 끝!

	// 시작 포인터 이동 -------------
	// Dest의 이미지의 시작 포인터 이동, 스프라이트의 시작 포인터 이동
	// 여기서 실제 바이트 단위로 계산한다.
	bypDest += (iDestStartX * m_iColorByte) + (iDestStartY * iDestPitch);
	m_stpSprite[iSpriteIndex].bypImge += (iSpriteStartX * m_iColorByte) + (iSpriteStartY * iSpritePitch);

	// 스프라이트 복사 -------------
	// 시작점부터 1줄씩 아래로 이동
	for (int i = 0; i < iSpriteHeight; ++i)
	{
		// 한 줄씩 아래로 이동하면서 픽셀단위로 >> 이동하며 컬러가 하얀색인지 체크한다.
		for (int j = 0; j < iSpritePitch - (iAddX*m_iColorByte); j += m_iColorByte)
		{
			// 해당 픽셀의 컬러가 하얀색이 아닐 때만 내 백버퍼에 찍는다. 하얀색이면 찍지 않고 다음 픽셀로 이동한다.
			// memcmp는 같으면 0반환. 다르면 1 반환. 즉, 하얀색이 아니면 1이 반환되어 참이 되어 로직 진행.
			if (memcmp(m_stpSprite[iSpriteIndex].bypImge + j, &m_dwColorKey, m_iColorByte))
			{		
				DWORD TempRGB;				

				//memcpy(&TempRGB, m_stpSprite[iSpriteIndex].bypImge + j, m_iColorByte);	
				//TempRGB = TempRGB & 0xffff0000;	// GB는 삭제.

				memcpy(&TempRGB, m_stpSprite[iSpriteIndex].bypImge + j, m_iColorByte);

				// GB를 /2해서 절반으로 낮춤.
				BYTE r = GetRValue(TempRGB);
				BYTE g = GetGValue(TempRGB);
				BYTE b = GetBValue(TempRGB);

				g = g / 2;
				b = b / 2;

				// 다시 합친다. 나는 4바이트씩 계산하기 때문에, ARGB 순서대로 되어있는 데이터가 필요하다.
				TempRGB = RGB(b, g, r);	
				memcpy(bypDest + j, &TempRGB, m_iColorByte);	// 1픽셀(4바이트)씩 카피한다.
			}
		}
		bypDest += iDestPitch;
		m_stpSprite[iSpriteIndex].bypImge += iSpritePitch;
	}

	// 내 스프라이트 버퍼의 주소를 증가시키면서 출력했으니 원래 위치로 이동시킨다.
	m_stpSprite[iSpriteIndex].bypImge = save;

	return TRUE;
}

// *******************************************
// 특정 메모리 위치에 스프라이트를 출력한다 (클리핑 처리만. 배경 찍을때 사용)
// *******************************************
BOOL CSpritedib::DrawImage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE* bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch, int iDrawLen)
{
	// 만약, 전달된 인덱스가 최대 스프라이트 개수보다 크다면 없는 스프라이트를 출력하라고 한 것이니, false 반환.
	if (m_iMaxSprite <= iSpriteIndex)
		return FALSE;

	BYTE* save = m_stpSprite[iSpriteIndex].bypImge;	// 다쓴 후 원복할 스프라이트 이미지의 시작 주소(0,0)

	// 해당 함수에서만 사용할, 변수들을 선언한다. 클리핑을 하기 때문에 로컬 변수가 필요하다.
	//int iSpriteWidth = m_stpSprite[iSpriteIndex].iWidth;		<<이거 가져왔는데 안써서 그냥 주석처리함. 나중에 필요하면 주석 해제하자.
	int iSpriteHeight = m_stpSprite[iSpriteIndex].iHeight;
	int iSpritePitch = m_stpSprite[iSpriteIndex].iPitch;

	// 스프라이트 이미지의, 시작 위치를 구한다. 클리핑 처리를 하려면, 이미지 찍는 시작 위치를 이동시켜야 한다.
	int iSpriteStartX = 0;
	int iSpriteStartY = 0;

	// 클리핑 처리 -------------
	// 상
	if (iDrawY < 0)
	{
		int iAddY = iDrawY * -1;	// 위로 얼마나 올라갔는지 구함.
		iDrawY = 0;				// 일단, Dest(내 백버퍼)의 y위치는 0이다.
		iSpriteStartY += iAddY;			// 스프라이트의 시작점 y 위치를 아래로 iAddY만큼 내린다.
		iSpriteHeight -= iAddY;			// 스프라이트의 높이를 iAddY만큼 줄인다. 
	}
	
	// 하
	if (iDrawY + m_stpSprite[iSpriteIndex].iHeight > iDestHeight)
	{
		int iAddY = iDrawY + m_stpSprite[iSpriteIndex].iHeight - iDestHeight;		// 아래로 얼마나 내려갔는지 구함	
		iSpriteHeight -= iAddY;															// 스프라이트의 높이를 iAddY만큼 줄인다. 		
	}

	// 좌
	int iAddX = 0;
	if (iDrawX < 0)
	{
		iAddX = iDrawX * -1;	// 좌로 얼마나 갔는지 구함.
		iDrawX = 0;				// 일단, Dest(내 백버퍼)의 x위치는 0이다.
		iDrawX += iAddX;			// 스프라이트의 시작점 x 위치를 우측으로 iAddX만큼 움직인다.
	}

	// 우
	if (iDrawX + m_stpSprite[iSpriteIndex].iWidth > iDestWidth)
		iAddX = iDrawX + m_stpSprite[iSpriteIndex].iWidth - iDestWidth;	// 우로 얼마나 갔는지 구함. 얘는 이러면 끝!

	// 시작 포인터 이동 -------------
	// 스프라이트의 시작 포인터 이동. Dest의 이미지의 시작 포인터는 이동시키기 않는다. 무조건 화면 좌상단부터 나와야 하기 때문에!
	// 여기서 실제 바이트 단위로 계산한다.
	m_stpSprite[iSpriteIndex].bypImge += (iSpriteStartX * m_iColorByte) + (iSpriteStartY * iSpritePitch);

	// 스프라이트 복사 -------------
	// 시작점부터 1줄씩 아래로 이동
	for (int i = 0; i < iSpriteHeight; ++i)
	{
		memcpy(bypDest, m_stpSprite[iSpriteIndex].bypImge, iSpritePitch - (iAddX*m_iColorByte));	// 컬러키 처리를 안하니, 1줄씩 복사한다.
		bypDest += iDestPitch;
		m_stpSprite[iSpriteIndex].bypImge += iSpritePitch;
	}

	// 내 스프라이트 버퍼의 주소를 증가시키면서 출력했으니 원래 위치로 이동시킨다.
	m_stpSprite[iSpriteIndex].bypImge = save;

	return TRUE;

}