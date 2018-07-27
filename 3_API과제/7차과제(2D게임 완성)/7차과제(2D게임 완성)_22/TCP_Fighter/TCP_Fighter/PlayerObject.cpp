#include "stdafx.h"
#include "PlayerObject.h"

//---------------------------
// 생성자
// --------------------------
PlayerObject::PlayerObject(int iObjectID, int iObjectType, int iCurX, int iCurY)
	:BaseObject(iObjectID, iObjectType, iCurX, iCurY)
{	
	m_dwActionCur =  100;
	m_dwActionOld = 100;
	m_iDirCur = dfDIRECTION_RIGHT;
	m_iDirOld = 0;
}

//---------------------------
// 소멸자
// --------------------------
PlayerObject::~PlayerObject()
{
	// 당장 할게 없음..
}

//---------------------------
// 실제 좌표 이동
// --------------------------
void PlayerObject::Action()
{
	m_dwActionCur = m_dwActionInput;

	// 애니메이션 다음 프레임으로 이동 ----------------
	NextFrame();

	// 좌표 이동, 애니메이션 처리 ---------------------
	ActionProc();
}	

//---------------------------
// 좌표이동과 애니메이션 처리
// --------------------------
void PlayerObject::ActionProc()
{
	static bool bAttackState = FALSE;	// 공격 중인지 체크하는 변수. FLASE면 공격 중 아님 / TRUE면 공격 중
	static bool bIdleState = FALSE;		// 대기 자세인지 체크하는 변수. FALSE면 대기중 아님 / TRUE면 대기중.

	// 내 캐릭터 일 때만 로직 처리
	if (m_bPlayerCharacter == TRUE)
	{
		// 공격 중이고, 마지막 프레임이 아니라면 메시지를 강제로 공격메시지로 변환. 아직은 공격 할 때이다.
		if (bAttackState == TRUE && Get_bGetEndFrame() == FALSE)
			m_dwActionCur = dfACTION_ATTACK_01;

		// 그게 아니라면 메시지 변환 안하고 공격모드 (bAttackState)를 FALSE로 만든다.
		else
			bAttackState = FALSE;

		// 공격 메시지 처리 ---------------------------------------
		switch (m_dwActionCur)
		{
		case dfACTION_ATTACK_01:
		case dfACTION_ATTACK_02:
		case dfACTION_ATTACK_03:
			if (bAttackState == FALSE)
			{
				bAttackState = TRUE;
				bIdleState = FALSE;
				if (m_dwActionCur == dfACTION_ATTACK_01)
				{
					if (m_iDirCur == dfDIRECTION_LEFT)
						SetSprite(CSpritedib::ePLAYER_ATTACK1_L01, 4, 3);
					else if (m_iDirCur == dfDIRECTION_RIGHT)
						SetSprite(CSpritedib::ePLAYER_ATTACK1_R01, 4, 3);
				}

				else if (m_dwActionCur == dfACTION_ATTACK_02)
				{
					if (m_iDirCur == dfDIRECTION_LEFT)
						SetSprite(CSpritedib::ePLAYER_ATTACK2_L01, 4, 4);
					else if (m_iDirCur == dfDIRECTION_RIGHT)
						SetSprite(CSpritedib::ePLAYER_ATTACK2_R01, 4, 4);
				}

				else if (m_dwActionCur == dfACTION_ATTACK_03)
				{
					if (m_iDirCur == dfDIRECTION_LEFT)
						SetSprite(CSpritedib::ePLAYER_ATTACK3_L01, 6, 4);
					else if (m_iDirCur == dfDIRECTION_RIGHT)
						SetSprite(CSpritedib::ePLAYER_ATTACK3_R01, 6, 4);
				}
			}

			break;
		}


		// IDLE 메시지와 이동 메시지 처리 ---------------------------------------
		switch (m_dwActionCur)
		{
		case dfACTION_IDLE:
			// 공격 상태 아니고, 대기 자세도 아니면 대기상태 시작.
			if (bAttackState == FALSE && bIdleState == FALSE)
			{
				if (m_iDirCur == dfDIRECTION_LEFT)
					SetSprite(CSpritedib::ePLAYER_STAND_L01, 5, 5);
				else if (m_iDirCur == dfDIRECTION_RIGHT)
					SetSprite(CSpritedib::ePLAYER_STAND_R01, 5, 5);
				bIdleState = TRUE;	// 대기상태를 TRUE로 만들면, 다음 로직에 대기자세 메시지가 또 들어와도 애니메이션을 처음부터 재생 안함.
			}
			break;

		default:
			bIdleState = FALSE;
			if (bAttackState == FALSE)
			{
				// 키 입력에 따른 좌표 이동 --------------------
				if (m_dwActionCur == dfACTION_MOVE_LL)			// LL
				{
					m_iCurX -= PLAYER_XMOVE_PIXEL;
					m_iDirCur = dfDIRECTION_LEFT;
				}

				else if (m_dwActionCur == dfACTION_MOVE_LU)		// LU
				{
					m_iCurX -= PLAYER_XMOVE_PIXEL;
					m_iCurY -= PLAYER_YMOVE_PIXEL;
					m_iDirCur = dfDIRECTION_LEFT;
				}

				else if (m_dwActionCur == dfACTION_MOVE_UU)		// UU
				{
					m_iCurY -= PLAYER_YMOVE_PIXEL;
				}

				else if (m_dwActionCur == dfACTION_MOVE_RU)		// RU
				{
					m_iCurX += PLAYER_XMOVE_PIXEL;
					m_iCurY -= PLAYER_YMOVE_PIXEL;
					m_iDirCur = dfDIRECTION_RIGHT;
				}

				else if (m_dwActionCur == dfACTION_MOVE_RR)		// RR
				{
					m_iCurX += PLAYER_XMOVE_PIXEL;
					m_iDirCur = dfDIRECTION_RIGHT;
				}

				else if (m_dwActionCur == dfACTION_MOVE_RD)		// RD
				{
					m_iCurX += PLAYER_XMOVE_PIXEL;
					m_iCurY += PLAYER_YMOVE_PIXEL;
					m_iDirCur = dfDIRECTION_RIGHT;
				}

				else if (m_dwActionCur == dfACTION_MOVE_DD)		// DD
				{
					m_iCurY += PLAYER_YMOVE_PIXEL;
				}

				else if (m_dwActionCur == dfACTION_MOVE_LD)		// LD
				{
					m_iCurX -= PLAYER_XMOVE_PIXEL;
					m_iCurY += PLAYER_YMOVE_PIXEL;
					m_iDirCur = dfDIRECTION_LEFT;
				}

				// 이전 상태와 현재 상태가 다르다면
				if (m_dwActionCur != m_dwActionOld)
				{
					// 무브 애니메이션 교체
					if (m_iDirCur != m_iDirOld ||	// 이전 방향과 현재 방향이 다르거나
						m_dwActionOld == dfACTION_IDLE ||	// 이전 상태가 대기였거나
						m_dwActionOld == dfACTION_ATTACK_01 || m_dwActionOld == dfACTION_ATTACK_02 || m_dwActionOld == dfACTION_ATTACK_03) // 이전 상태가 공격1, 공격2, 공격3 중 하나였다면
					{
						if (m_iDirCur == dfDIRECTION_LEFT)
							SetSprite(CSpritedib::ePLAYER_MOVE_L01, 12, 4);

						else if (m_iDirCur == dfDIRECTION_RIGHT)
							SetSprite(CSpritedib::ePLAYER_MOVE_R01, 12, 4);
					}
				}
			}
			break;
		}

		m_iDirOld = m_iDirCur;			// 방향 교체
		m_dwActionOld = m_dwActionCur;	// 상태 교체
	}
}

//---------------------------
// 실제로 내 DC에 복사하는 함수
//--------------------------
void PlayerObject::Draw(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	// 내 그림자 
	BOOL iCheck = g_cSpriteDib->DrawSprite(CSpritedib::eSHADOW, m_iCurX, m_iCurY, bypDest, iDestWidth, iDestHeight, iDestPitch, isPlayer());
	if (!iCheck)
		exit(-1);

	// 내 캐릭터
	iCheck = g_cSpriteDib->DrawSprite(m_iSpriteNow, m_iCurX, m_iCurY, bypDest, iDestWidth, iDestHeight, iDestPitch, isPlayer());
	if (!iCheck)
		exit(-1);

	// 내 HP 게이지
	iCheck = g_cSpriteDib->DrawSprite(CSpritedib::eGUAGE_HP, m_iCurX - 35, m_iCurY + 9, bypDest, iDestWidth, iDestHeight, iDestPitch, isPlayer());
	if (!iCheck)
		exit(-1);
}

//---------------------------
// 최초 멤버변수 셋팅
//--------------------------
void PlayerObject::MemberSetFunc(BOOL bPlayerChar, int iHP)
{
	m_bPlayerCharacter = bPlayerChar;
	m_iHP = iHP;
}

//---------------------------
// m_iHP 게터
//--------------------------
int PlayerObject::GetHP()
{
	return m_iHP;
}

//---------------------------
// m_bPlayerCharacter 게터
//--------------------------
BOOL PlayerObject::isPlayer()
{
	return m_bPlayerCharacter;
}
