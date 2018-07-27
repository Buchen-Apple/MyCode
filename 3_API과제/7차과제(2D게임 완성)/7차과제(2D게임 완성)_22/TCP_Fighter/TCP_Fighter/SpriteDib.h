#pragma once
#ifndef __SPRITEDIB_H__
#define __SPRITEDIB_H__

#include <Windows.h>


class CSpritedib
{
public:
	enum e_SPRITE
	{
		eMAP = 0,

		ePLAYER_STAND_L01,
		ePLAYER_STAND_L02,
		ePLAYER_STAND_L03,
		ePLAYER_STAND_L04,
		ePLAYER_STAND_L05,

		ePLAYER_STAND_R01,
		ePLAYER_STAND_R02,
		ePLAYER_STAND_R03,
		ePLAYER_STAND_R04,
		ePLAYER_STAND_R05,

		ePLAYER_MOVE_L01,
		ePLAYER_MOVE_L02,
		ePLAYER_MOVE_L03,
		ePLAYER_MOVE_L04,
		ePLAYER_MOVE_L05,
		ePLAYER_MOVE_L06,
		ePLAYER_MOVE_L07,
		ePLAYER_MOVE_L08,
		ePLAYER_MOVE_L09,
		ePLAYER_MOVE_L10,
		ePLAYER_MOVE_L11,
		ePLAYER_MOVE_L12,

		ePLAYER_MOVE_R01,
		ePLAYER_MOVE_R02,
		ePLAYER_MOVE_R03,
		ePLAYER_MOVE_R04,
		ePLAYER_MOVE_R05,
		ePLAYER_MOVE_R06,
		ePLAYER_MOVE_R07,
		ePLAYER_MOVE_R08,
		ePLAYER_MOVE_R09,
		ePLAYER_MOVE_R10,
		ePLAYER_MOVE_R11,
		ePLAYER_MOVE_R12,

		ePLAYER_ATTACK1_L01,
		ePLAYER_ATTACK1_L02,
		ePLAYER_ATTACK1_L03,
		ePLAYER_ATTACK1_L04,

		ePLAYER_ATTACK1_R01,
		ePLAYER_ATTACK1_R02,
		ePLAYER_ATTACK1_R03,
		ePLAYER_ATTACK1_R04,

		ePLAYER_ATTACK2_L01,
		ePLAYER_ATTACK2_L02,
		ePLAYER_ATTACK2_L03,
		ePLAYER_ATTACK2_L04,

		ePLAYER_ATTACK2_R01,
		ePLAYER_ATTACK2_R02,
		ePLAYER_ATTACK2_R03,
		ePLAYER_ATTACK2_R04,

		ePLAYER_ATTACK3_L01,
		ePLAYER_ATTACK3_L02,
		ePLAYER_ATTACK3_L03,
		ePLAYER_ATTACK3_L04,
		ePLAYER_ATTACK3_L05,
		ePLAYER_ATTACK3_L06,

		ePLAYER_ATTACK3_R01,
		ePLAYER_ATTACK3_R02,
		ePLAYER_ATTACK3_R03,
		ePLAYER_ATTACK3_R04,
		ePLAYER_ATTACK3_R05,
		ePLAYER_ATTACK3_R06,

		eEFFECT_SPARK_01,
		eEFFECT_SPARK_02,
		eEFFECT_SPARK_03,
		eEFFECT_SPARK_04,

		eGUAGE_HP,
		eSHADOW,
		e_SPRITE_MAX,
	};

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


public:
	//*******************************************
	// 게임 초반 세팅 함수(현재는 모든 비트맵을 읽어온다
	//*******************************************
	void GameInit();

	//*******************************************
	// 싱글톤 생성 함수
	//*******************************************
	static CSpritedib* Getinstance(int iMaxSprite, DWORD dwColorKey, int iColorBit);

	//*******************************************
	// 스프라이트의 색 빨간색으로 처리
	//*******************************************
	void RedChange(int iSpriteIndex);

	// *******************************************
	// 파괴자
	// *******************************************
	~CSpritedib();

	// *******************************************
	// BMP파일을 읽어서 하나의 스프라이트로 저장
	// *******************************************
	BOOL LoadDibSprite(int iSpriteIndex, char* szFileName, int iCenterPointX, int iCenterPointY);

	// *******************************************
	// 특정 메모리 위치에 스프라이트를 출력한다 (칼라키, 클리핑 처리)
	// *******************************************
	BOOL DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE* bypDest, int iDestWidth,
		int iDestHeight, int iDestPitch, BOOL isPlayer, int iDrawLen = 100);

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

#pragma once
