#include "stdafx.h"
#include "EffectObject.h"
#include "PlayerObject.h"

// 생성자
EffectObject::EffectObject(int iObjectID, int iObjectType, int iCurX, int iCurY, int AtkType)
	:BaseObject(iObjectID, iObjectType, iCurX, iCurY)
{
	m_bEffectStart = FALSE;
	m_dwAttackID = AtkType;

	// 공격 타입에 따라 x,y 셋팅
	if (m_dwAttackID == dfACTION_ATTACK_01_LEFT || m_dwAttackID == dfACTION_ATTACK_02_LEFT || m_dwAttackID == dfACTION_ATTACK_03_LEFT)
		MoveCurXY(iCurX - 60, iCurY - 60);

	else if (m_dwAttackID == dfACTION_ATTACK_01_RIGHT || m_dwAttackID == dfACTION_ATTACK_02_RIGHT || m_dwAttackID == dfACTION_ATTACK_03_RIGHT)
		MoveCurXY(iCurX + 60, iCurY - 60);

	// 공격 타입에 따라, 대기하는 프레임 수 셋팅. 해당 프레임 수 만큼 대기한 후에 이펙트 재생이 시작된다.
	if (m_dwAttackID == dfACTION_ATTACK_03_LEFT || m_dwAttackID == dfACTION_ATTACK_03_RIGHT)
		m_StandByFrame = 10;

	else if (m_dwAttackID == dfACTION_ATTACK_01_LEFT || m_dwAttackID == dfACTION_ATTACK_01_RIGHT || m_dwAttackID == dfACTION_ATTACK_02_LEFT || m_dwAttackID == dfACTION_ATTACK_02_RIGHT)
		m_StandByFrame = 0;
}

// 소멸자
EffectObject::~EffectObject()
{
	// 할게 없음
}

// 액션
void EffectObject::Action()
{	
	if (m_bEffectStart == TRUE)
	{
		// 애니메이션 다음 프레임으로 이동 ----------------
		NextFrame();
	}

	// m_StandByFrame가 0이고(대기 프레임 수), 이펙트 재생 플래그가 FALSE면, 이펙트 재생 시작 플래그를 TRUE로 만든다.
	if (m_StandByFrame == 0 && m_bEffectStart == FALSE)
	{
		m_bEffectStart = TRUE;		
	}

	// m_StandByFrame가 0이 아니고, 이펙트 재생 플래그가 FALSE면, 아직 이펙트 재생할 때가 안된것이니 m_StandByFrame를 1 감소
	else if(m_StandByFrame != 0 && m_bEffectStart == FALSE)
		m_StandByFrame--;
}

// Draw
void EffectObject::Draw(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	// 실제 이펙트를 그린다.
	if (m_bEffectStart == TRUE)
	{		
		int iCheck = g_cSpriteDib->DrawSprite(m_iSpriteNow, m_iCurX, m_iCurY, bypDest, iDestWidth, iDestHeight, iDestPitch, 0);
		if (!iCheck)
			exit(-1);
	}
}