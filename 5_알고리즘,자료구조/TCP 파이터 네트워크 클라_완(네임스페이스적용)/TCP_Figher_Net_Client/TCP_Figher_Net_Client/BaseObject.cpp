#include "stdafx.h"
#include "BaseObject.h"

//---------------------------
// 생성자
// --------------------------
BaseObject::BaseObject(int iObjectID, int iObjectType, int iCurX, int iCurY)
{
	m_bEndFrame = false;
	m_dwActionInput = 0;
	m_iCurX = iCurX;
	m_iCurY = iCurY;
	m_iDelayCount = 0;
	m_iFrameDelay = 0;
	m_iObjectID = iObjectID;
	m_iObjectType = iObjectType;
	m_iSpriteStart = 0;
	m_iSpriteNow = 0;
	m_iSpriteEnd = 0;
	g_cSpriteDib = CSpritedib::Getinstance(31, 0xffffffff, 32);	// 싱글톤으로 CSpritedib형 객체 하나 얻기
}

//---------------------------
// 소멸자
// --------------------------
BaseObject::~BaseObject()
{
	// 당장 할게 없음
}

//---------------------------
// 스프라이트 애니메이션 셋팅
// --------------------------
void BaseObject::SetSprite(int iStartSprite, int iSpriteCount, int iFrameDelay)
{
	m_iSpriteStart = iStartSprite;
	m_iSpriteEnd = iStartSprite + (iSpriteCount - 1);
	m_iFrameDelay = iFrameDelay;

	m_iSpriteNow = iStartSprite;
	m_iDelayCount = 0;
	m_bEndFrame = FALSE;
}


//---------------------------
// 외부로부터의 키입력 체크
// --------------------------
void BaseObject::ActionInput(int MoveXY)
{
	m_dwActionInput = MoveXY;
}

//---------------------------
// 프레임 다음으로 넘기기 함수
// --------------------------
void BaseObject::NextFrame()
{
	if (0 > m_iSpriteStart)		// 0보다 작은 값이 시작값이면 리턴.
		return;

	// 프레임 딜레이 값을 넘어야 다음 프레임으로 넘어간다.
	m_iDelayCount++;

	if (m_iDelayCount >= m_iFrameDelay)
	{
		m_iDelayCount = 0;
		m_iSpriteNow++;

		if (m_iSpriteNow > m_iSpriteEnd)
		{
			m_iSpriteNow = m_iSpriteStart;
			m_bEndFrame = TRUE;
		}
	}
}

//---------------------------
// X,Y 세팅 함수 (네트워크용)
// --------------------------
void BaseObject::MoveCurXY(int X, int Y)
{
	m_iCurX = X;
	m_iCurY = Y;
}

//---------------------------
// 각종 게터 함수
// --------------------------
int BaseObject::GetCurX()
{
	return m_iCurX;
}

int BaseObject::GetCurY()
{
	return m_iCurY;
}

int BaseObject::GetObjectID()
{
	return m_iObjectID;
}

int BaseObject::GetSprite()
{
	return m_iSpriteNow;
}

int BaseObject::GetObjectType()
{
	return m_iObjectType;
}

bool BaseObject::Get_bGetEndFrame()
{
	return m_bEndFrame;
}

