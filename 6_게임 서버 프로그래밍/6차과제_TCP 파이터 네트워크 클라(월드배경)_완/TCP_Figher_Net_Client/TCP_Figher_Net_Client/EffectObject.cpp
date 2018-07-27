#include "stdafx.h"
#include "EffectObject.h"
#include "PlayerObject.h"

#include "List_Template.h"

extern CList<BaseObject*> list;					// Main에서 할당한 객체 저장 리스트	

#define PLAYER_X	71
#define PLAYER_Y	90

// 생성자
EffectObject::EffectObject(int iObjectID, int iObjectType, int iCurX, int iCurY, int AtkType)
	:BaseObject(iObjectID, iObjectType, iCurX, iCurY)
{
	m_EffectSKipCheck = true;
	m_bEffectStart = FALSE;
	m_dwAttackID = AtkType;

	// 공격 타입에 따라 x,y 셋팅
	if (m_dwAttackID == dfACTION_ATTACK_01_LEFT || m_dwAttackID == dfACTION_ATTACK_02_LEFT || m_dwAttackID == dfACTION_ATTACK_03_LEFT)
		MoveCurXY(iCurX - 80, iCurY - 60);

	else if (m_dwAttackID == dfACTION_ATTACK_01_RIGHT || m_dwAttackID == dfACTION_ATTACK_02_RIGHT || m_dwAttackID == dfACTION_ATTACK_03_RIGHT)
		MoveCurXY(iCurX + 80, iCurY - 60);

	// 공격 타입에 따라, 대기하는 프레임 수 셋팅. 해당 프레임 수 만큼 대기한 후에 이펙트 재생이 시작된다.
	if (m_dwAttackID == dfACTION_ATTACK_03_LEFT || m_dwAttackID == dfACTION_ATTACK_03_RIGHT)
		m_StandByFrame = 10;

	else if (m_dwAttackID == dfACTION_ATTACK_01_LEFT || m_dwAttackID == dfACTION_ATTACK_01_RIGHT)
		m_StandByFrame = 0;

	else if (m_dwAttackID == dfACTION_ATTACK_02_LEFT || m_dwAttackID == dfACTION_ATTACK_02_RIGHT)
		m_StandByFrame = 4;
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

		// 이펙트 시작 시점에, 내가 표시되어야 하는 위치에 적이 있다면, Draw찍는다. false면 스킵을 안한다는 뜻이다.
		CList<BaseObject*>::iterator itor;
		for (itor = list.begin(); itor != list.end(); itor++)
		{
			// 해당 객체가 플레이어라면
			if ((*itor)->GetObjectType() == 1)
			{
				// 여기에는 비트맵 좌상단과 대응되는 월드 좌표가 저장된다.
				int WorldX = (*itor)->GetCurX() - PLAYER_X;
				int WorldY = (*itor)->GetCurY() - PLAYER_Y;

				//  해당 플레이어의 월드 좌표(중점이다)가 이펙트가 표시될 위치와 겹치는지 체크
				//	겹치는 플레이어가 발견되면, 스킵 여부를 false로 한다. 
				if (WorldX < m_iCurX && m_iCurX < WorldX + (PLAYER_X * 2) &&
					WorldY < m_iCurY && m_iCurY < WorldY + PLAYER_Y)
				{
					m_EffectSKipCheck = false;
					break;
				}					
			}
		}		
	}

	// m_StandByFrame가 0이 아니고, 이펙트 재생 플래그가 FALSE면, 아직 이펙트 재생할 때가 안된것이니 m_StandByFrame를 1 감소
	else if (m_StandByFrame != 0 && m_bEffectStart == FALSE)
		m_StandByFrame--;
}

// Draw
void EffectObject::Draw(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	// 실제 이펙트를 그린다.
	if (m_bEffectStart == TRUE && m_EffectSKipCheck == false)
	{
		// 카메라 좌표 기준, 내가 출력될 위치 구한다.
		CMap* SIngletonMap = CMap::Getinstance(0, 0, 0, 0, 0);
		int ShowX = SIngletonMap->GetShowPosX(m_iCurX);
		int ShowY = SIngletonMap->GetShowPosY(m_iCurY);

		/*
		if (ShowX < 0)
			ShowX = 0;

		if (ShowY < 0)
			ShowY = 0;
		*/

		int iCheck = g_cSpriteDib->DrawSprite(m_iSpriteNow, ShowX, ShowY, bypDest, iDestWidth, iDestHeight, iDestPitch, 0);
		if (!iCheck)
			exit(-1);
	}
}