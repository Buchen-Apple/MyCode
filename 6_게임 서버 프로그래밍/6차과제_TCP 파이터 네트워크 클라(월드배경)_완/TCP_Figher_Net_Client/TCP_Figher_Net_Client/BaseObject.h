#pragma once
#ifndef __BASE_OBEJCT_H__
#define __BASE_OBEJCT_H__

#include <Windows.h>
#include "SpriteDib.h"


class BaseObject
{
private:
	bool m_bEndFrame;			// 애니메이션 플레이 종료 여부. true면 종료, false면 진행 중
	int m_iDelayCount;			// 
	int m_iFrameDelay;			// 프레임의 딜레이. 애니메이션 별로 딜레이가 정해져 있다.
	int m_iObjectID;			// 오브젝트의 ID
	int m_iObjectType;			// 오브젝트의 타입. 플레이어/이펙트 구분 (1 : 플레이어 2 : 이펙트)

protected:
	DWORD m_dwActionInput;		// 큐 대용이다. 키 누르면 그 키의 값이 저장된다.
	CSpritedib* g_cSpriteDib;	// 스프라이트를 저장할 객체.
	int m_iCurX;				// 현재 X좌표 위치 (픽셀 기준)
	int m_iCurY;				// 현재 Y좌표 위치 (픽셀 기준)
	int m_iSpriteStart;			// 재생을 시작할 스프라이트의 번호
	int m_iSpriteNow;			// 현재 진행 중인 스프라이트 번호
	int m_iSpriteEnd;			// 재생 완료된 스프라이트 번호

public:

	BaseObject(int iObjectID, int iObjectType, int iCurX, int iCurY);

	virtual void Action() = 0;
	virtual ~BaseObject();
	virtual void Draw(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch) = 0;

	void ActionInput(int MoveXY);
	void NextFrame();
	void SetSprite(int iStartSprite, int iSpriteCount, int iFrameDelay);
	void MoveCurXY(int X, int Y);

	int GetCurX();
	int GetCurY();
	int GetObjectID();
	int GetSprite();
	int GetObjectType();
	bool Get_bGetEndFrame();

};


#endif // !__BASE_OBEJCT_H__
