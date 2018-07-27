#pragma once
#ifndef __SPRITEDIB_H__
#define __SPRITEDIB_H__

#include <Windows.h>

class CSpritedib
{

private:
	// *******************************************
	// 싱글톤을 위해 생성자는 Private로 선언
	// *******************************************
	CSpritedib(int iMaxSprite, DWORD dwColorKey, int iColorBit);

	// *******************************************
	// 전달된 스프라이트를 동적할당 해제 (파괴자에서 호출)
	// *******************************************
	void ReleseSprite(int iSpriteIndex);

	// *******************************************
	// DIB 스프라이트 구조체
	//
	// 스프라이트 이미지와 사이즈 정보를 갖는다.
	// *******************************************
	struct st_SPRITE
	{
		BYTE* bypImge;			// 스프라이트 이미지
		int iWidth;				// 넓이
		int iHeight;			// 높이
		int iPitch;				// 피치(실제 바이트 넓이)

		int iCenterPointX;		// 중점 X
		int iCenterPointY;		// 중점 Y
	};

	// *******************************************
	// 내 캐릭터는 빨간색으로 처리
	// *******************************************
	DWORD PlayerColorRed(BYTE* SorRGB);


public:
	//*******************************************
	// 게임 초반 세팅 함수(현재는 모든 비트맵을 읽어온다
	//*******************************************
	void GameInit();
	
	//*******************************************
	// 싱글톤 생성 함수
	//*******************************************
	static CSpritedib* Getinstance(int iMaxSprite, DWORD dwColorKey, int iColorBit);
		
	// *******************************************
	// 파괴자
	// *******************************************
	virtual ~CSpritedib();

	// *******************************************
	// BMP파일을 읽어서 하나의 스프라이트로 저장
	// *******************************************
	BOOL LoadDibSprite(int iSpriteIndex, char* szFileName, int iCenterPointX, int iCenterPointY);
	
	// *******************************************
	// 특정 메모리 위치에 스프라이트를 출력한다 (칼라키, 클리핑 처리)
	// *******************************************
	BOOL DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE* bypDest, int iDestWidth,
					int iDestHeight, int iDestPitch, int iDrawLen = 100);

	// *******************************************
	// 특정 메모리 위치에 스프라이트를 출력한다 (클리핑 처리만. 배경 찍을때 사용)
	// *******************************************
	BOOL DrawImage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE* bypDest, int iDestWidth,
					int iDestHeight, int iDestPitch, int iDrawLen = 100);


protected: 
	// 멤버 변수들

	// *******************************************
	// 최대 스프라이트 수, Sprite 배열 정보, 스프라이트 컬러
	// *******************************************
	int			m_iMaxSprite;
	st_SPRITE	*m_stpSprite;
	int			m_iColorByte;

	// *******************************************
	// 투명 색상으로 사용할 컬러
	// *******************************************
	DWORD m_dwColorKey;

};


#endif // !__SPRITEDIB_H__

