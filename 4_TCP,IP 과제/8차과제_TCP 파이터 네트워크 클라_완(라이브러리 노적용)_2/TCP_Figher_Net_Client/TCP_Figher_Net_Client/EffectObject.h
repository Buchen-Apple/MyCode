#pragma once
#ifndef __EFFECT_OBJECT_H__
#define __EFFECT_OBJECT_H__

#include "BaseObject.h"

class EffectObject :public BaseObject
{
private:
	bool m_bEffectStart;	// 시작된 이펙트가 맞는지 체크
	DWORD m_dwAttackID;		// 어떤 공격인지. (공격 1,2,3) 공격에 따라 어떤 스프라이트 프레임에 이펙트를 터트려야 하는지 찾는다.
	DWORD m_StandByFrame;	// 이펙트가 생성되고, 이 변수의 값 만큼 대기한 후에 이펙트 재생 시작

public:
	// 생성자
	EffectObject(int iObjectID, int iObjectType, int iCurX, int iCurY, int AtkType);

	// 소멸자
	~EffectObject();

	// 액션
	virtual void Action();

	// Draw
	virtual void Draw(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch);
};


#endif // !__EFFECT_OBJECT_H__

