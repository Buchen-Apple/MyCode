#include "stdafx.h"
#include "PlayerObject.h"
#include "Network_Func.h"
#include "EffectObject.h"

extern CList<BaseObject*> list;					// Main에서 할당한 객체 저장 리스트	

//---------------------------
// 생성자
// --------------------------
PlayerObject::PlayerObject(int iObjectID, int iObjectType, int iCurX, int iCurY, int iDirCur)
	:BaseObject(iObjectID, iObjectType, iCurX, iCurY)
{
	m_dwActionCur = 100;
	m_dwActionOld = 100;
	m_iDirCur = iDirCur;
	m_iDirOld = 0;
	m_bAttackState = FALSE;
	m_bIdleState = FALSE;
	m_dwTargetID = NULL;
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
	 //공격 중이고, 마지막 프레임이 아니라면 메시지를 강제로 이전 메시지로 변환. 즉, 공격 중에는 다른 액션 모두 무시
	if (m_bAttackState == TRUE && Get_bGetEndFrame() == FALSE)
	{
		// 근데, 내 캐릭터일때만 적용. 다른 사람 캐릭터라면, 그냥 오는 족족 행동해야 함.
		if (m_bPlayerCharacter == TRUE)
			m_dwActionCur = m_dwActionOld;
	}

	// 공격 중이고, 마지막 프레임이라면, 공격 모드를 해제하고 현재 메시지를 아이들로 변경.
	else if(m_bAttackState == TRUE && Get_bGetEndFrame() == TRUE)
	{
		m_bAttackState = FALSE;
		m_dwActionCur = dfACTION_IDLE;
		m_dwNowAtkType = 0;

		// 다른 사람 캐릭터라면, 해당 유저의 키보드 입력을 IDLE로 변경. 그래야 쭉 IDLE이 나오니까!
		if (m_bPlayerCharacter == FALSE)
			m_dwActionInput = dfACTION_IDLE;
	}

	// 공격 메시지 처리 ---------------------------------------
	switch (m_dwActionCur)
	{
	case dfACTION_ATTACK_01_LEFT:
	case dfACTION_ATTACK_02_LEFT:
	case dfACTION_ATTACK_03_LEFT:
		if (m_bAttackState == FALSE)
		{
			m_bAttackState = TRUE;
			m_bIdleState = FALSE;

			// 공격모션 셋팅
			if (m_dwActionCur == dfACTION_ATTACK_01_LEFT)
			{
				if (m_iDirCur == dfDIRECTION_LEFT)
				{
					SetSprite(CSpritedib::ePLAYER_ATTACK1_L01, 4, 3);
					m_dwNowAtkType = dfACTION_ATTACK_01_LEFT;					
				}
				else if (m_iDirCur == dfDIRECTION_RIGHT)
				{
					SetSprite(CSpritedib::ePLAYER_ATTACK1_R01, 4, 3);
					m_dwNowAtkType = dfACTION_ATTACK_01_RIGHT;
				}
			}

			else if (m_dwActionCur == dfACTION_ATTACK_02_LEFT)
			{
				if (m_iDirCur == dfDIRECTION_LEFT)
				{
					SetSprite(CSpritedib::ePLAYER_ATTACK2_L01, 4, 4);
					m_dwNowAtkType = dfACTION_ATTACK_02_LEFT;
				}
				else if (m_iDirCur == dfDIRECTION_RIGHT)
				{
					SetSprite(CSpritedib::ePLAYER_ATTACK2_R01, 4, 4);
					m_dwNowAtkType = dfACTION_ATTACK_02_RIGHT;
				}
			}

			else if (m_dwActionCur == dfACTION_ATTACK_03_LEFT)
			{
				if (m_iDirCur == dfDIRECTION_LEFT)
				{
					SetSprite(CSpritedib::ePLAYER_ATTACK3_L01, 6, 4);
					m_dwNowAtkType = dfACTION_ATTACK_03_LEFT;
				}
				else if (m_iDirCur == dfDIRECTION_RIGHT)
				{
					SetSprite(CSpritedib::ePLAYER_ATTACK3_R01, 6, 4);
					m_dwNowAtkType = dfACTION_ATTACK_03_RIGHT;
				}
			}							


			// 네트워크. 
			// 내 캐릭터일 경우 패킷 보냄
			if (m_bPlayerCharacter == TRUE)
			{
				st_NETWORK_PACKET_HEADER header;

				// 이전 행동이 대기, 공격이었으면 STOP패킷 보내지 않음. 이미 멈춰있는 상태이기 때문에!
				if (m_dwActionOld != dfACTION_IDLE && m_dwActionOld != dfACTION_ATTACK_01_LEFT && m_dwActionOld != dfACTION_ATTACK_02_LEFT && m_dwActionOld != dfACTION_ATTACK_03_LEFT)
				{
					// 이전 행동이 이동 중이었으면, Stop 패킷을 Send해서 일단 멈춤상태로 만든다.						
					stPACKET_CS_MOVE_STOP packet;

					SendProc_MoveStop(&header, &packet, m_iDirCur, m_iCurX, m_iCurY);
					SendPacket(&header, (char*)&packet);
				}

				// 공격 패킷 Send
				ZeroMemory(&header, sizeof(header));
				stPACKET_CS_ATTACK atkPacket;

				if (m_dwActionCur == dfACTION_ATTACK_01_LEFT)
					SendProc_Atk_01(&header, &atkPacket, m_iDirCur, m_iCurX, m_iCurY);

				else if (m_dwActionCur == dfACTION_ATTACK_02_LEFT)
					SendProc_Atk_02(&header, &atkPacket, m_iDirCur, m_iCurX, m_iCurY);

				else if (m_dwActionCur == dfACTION_ATTACK_03_LEFT)
					SendProc_Atk_03(&header, &atkPacket, m_iDirCur, m_iCurX, m_iCurY);

				SendPacket(&header, (char*)&atkPacket);
			}

		}

		break;

		// IDLE 메시지 처리 ---------------------------------------
	case dfACTION_IDLE:
		// 공격 상태 아니고, 대기 자세도 아니면 대기상태 시작.
		if (m_bAttackState == FALSE && m_bIdleState == FALSE)
		{
			if (m_iDirCur == dfDIRECTION_LEFT)
				SetSprite(CSpritedib::ePLAYER_STAND_L01, 5, 5);
			else if (m_iDirCur == dfDIRECTION_RIGHT)
				SetSprite(CSpritedib::ePLAYER_STAND_R01, 5, 5);
			m_bIdleState = TRUE;	// 대기상태를 TRUE로 만들면, 다음 로직에 대기자세 메시지가 또 들어와도 애니메이션을 처음부터 재생 안함.		

			// 내 캐릭터라면, 대기 시작 데이터 Send
			if (m_bPlayerCharacter == TRUE && m_dwActionCur != m_dwActionOld)
			{
				// 만약 이전 행동이 공격이었으면, 패킷 보내지 않음. 공격이 끝나서 자연스럽게 IDLE로 돌아온 것!
				if (m_dwActionOld != dfACTION_ATTACK_01_LEFT && m_dwActionOld != dfACTION_ATTACK_02_LEFT && m_dwActionOld != dfACTION_ATTACK_03_LEFT)
				{
					st_NETWORK_PACKET_HEADER header;
					stPACKET_CS_MOVE_STOP packet;

					SendProc_MoveStop(&header, &packet, m_iDirCur, m_iCurX, m_iCurY);
					SendPacket(&header, (char*)&packet);
				}
			}
		}		
		break;

		// 이동 메시지 처리 ---------------------------------------
	default:
		m_bIdleState = FALSE;
		if (m_bAttackState == FALSE)
		{
			// 키 입력에 따른 좌표 이동 --------------------
			if (m_dwActionCur == dfACTION_MOVE_LL)			// LL
			{
				m_iDirCur = dfDIRECTION_LEFT;

				// 일정 이상 밖으로 가지 못하도록 제한
				m_iCurX -= PLAYER_XMOVE_PIXEL;				
				if (m_iCurX < dfRANGE_MOVE_LEFT)
					m_iCurX = dfRANGE_MOVE_LEFT;
				
			}

			else if (m_dwActionCur == dfACTION_MOVE_LU)		// LU
			{
				m_iDirCur = dfDIRECTION_LEFT;

				// 현재 Y좌표가 dfRANGE_MOVE_TOP(벽의 위 끝)이라면, x좌표 갱신하지 않음
				if (m_iCurY != dfRANGE_MOVE_TOP)
				{
					m_iCurX -= PLAYER_XMOVE_PIXEL;
					if (m_iCurX < dfRANGE_MOVE_LEFT)
						m_iCurX = dfRANGE_MOVE_LEFT;
				}

				// 현재, X좌표가 dfRANGE_MOVE_LEFT(벽의 왼쪽 끝)이라면, Y좌표는 갱신하지 않음. 
				if (m_iCurX != dfRANGE_MOVE_LEFT)
				{
					m_iCurY -= PLAYER_YMOVE_PIXEL;
					if (m_iCurY < dfRANGE_MOVE_TOP)
						m_iCurY = dfRANGE_MOVE_TOP;
				}
				
			}

			else if (m_dwActionCur == dfACTION_MOVE_UU)		// UU
			{
				m_iCurY -= PLAYER_YMOVE_PIXEL;
				if (m_iCurY < dfRANGE_MOVE_TOP)
					m_iCurY = dfRANGE_MOVE_TOP;
			}

			else if (m_dwActionCur == dfACTION_MOVE_RU)		// RU
			{
				m_iDirCur = dfDIRECTION_RIGHT;

				// 현재 Y좌표가 dfRANGE_MOVE_TOP(벽의 위 끝)이라면, x좌표 갱신하지 않음
				if (m_iCurY != dfRANGE_MOVE_TOP)
				{
					m_iCurX += PLAYER_XMOVE_PIXEL;
					if (m_iCurX > dfRANGE_MOVE_RIGHT)
						m_iCurX = dfRANGE_MOVE_RIGHT;
				}

				// 만약, X좌표가 dfRANGE_MOVE_RIGHT(벽의 오른쪽 끝)이라면, Y좌표는 갱신하지 않음. 
				if (m_iCurX != dfRANGE_MOVE_RIGHT)
				{
					m_iCurY -= PLAYER_YMOVE_PIXEL;
					if (m_iCurY < dfRANGE_MOVE_TOP)
						m_iCurY = dfRANGE_MOVE_TOP;
				}
				
			}

			else if (m_dwActionCur == dfACTION_MOVE_RR)		// RR
			{
				m_iDirCur = dfDIRECTION_RIGHT;

				m_iCurX += PLAYER_XMOVE_PIXEL;
				if (m_iCurX > dfRANGE_MOVE_RIGHT)
					m_iCurX = dfRANGE_MOVE_RIGHT;				
			}

			else if (m_dwActionCur == dfACTION_MOVE_RD)		// RD
			{
				m_iDirCur = dfDIRECTION_RIGHT;

				// 현재 Y좌표가 dfRANGE_MOVE_BOTTOM(벽의 바닥 끝)이라면, X좌표 갱신하지 않음
				if (m_iCurY != dfRANGE_MOVE_BOTTOM)
				{
					m_iCurX += PLAYER_XMOVE_PIXEL;
					if (m_iCurX > dfRANGE_MOVE_RIGHT)
						m_iCurX = dfRANGE_MOVE_RIGHT;
				}

				// 만약, X좌표가 dfRANGE_MOVE_RIGHT(벽의 오른쪽 끝)이라면, Y좌표는 갱신하지 않음. 
				if (m_iCurX != dfRANGE_MOVE_RIGHT)
				{
					m_iCurY += PLAYER_YMOVE_PIXEL;
					if (m_iCurY > dfRANGE_MOVE_BOTTOM)
						m_iCurY = dfRANGE_MOVE_BOTTOM;
				}
				
			}

			else if (m_dwActionCur == dfACTION_MOVE_DD)		// DD
			{
				m_iCurY += PLAYER_YMOVE_PIXEL;
				if (m_iCurY > dfRANGE_MOVE_BOTTOM)
					m_iCurY = dfRANGE_MOVE_BOTTOM;
			}

			else if (m_dwActionCur == dfACTION_MOVE_LD)		// LD
			{
				m_iDirCur = dfDIRECTION_LEFT;

				// 현재 Y좌표가 dfRANGE_MOVE_BOTTOM(벽의 바닥 끝)이라면, X좌표 갱신하지 않음
				if (m_iCurY != dfRANGE_MOVE_BOTTOM)
				{
					m_iCurX -= PLAYER_XMOVE_PIXEL;
					if (m_iCurX < dfRANGE_MOVE_LEFT)
						m_iCurX = dfRANGE_MOVE_LEFT;
				}

				// 만약, X좌표가 dfRANGE_MOVE_LEFT(벽의 왼쪽 끝)이라면, Y좌표는 갱신하지 않음.
				if (m_iCurX != dfRANGE_MOVE_LEFT)
				{
					m_iCurY += PLAYER_YMOVE_PIXEL;
					if (m_iCurY > dfRANGE_MOVE_BOTTOM)
						m_iCurY = dfRANGE_MOVE_BOTTOM;
				}
			}

			// 이전 상태와 현재 상태가 다르다면
			if (m_dwActionCur != m_dwActionOld)
			{
				// 무브 애니메이션 교체
				if (m_iDirCur != m_iDirOld ||	// 이전 방향과 현재 방향이 다르거나
					m_dwActionOld == dfACTION_IDLE ||	// 이전 상태가 대기였거나
					m_dwActionOld == dfACTION_ATTACK_01_LEFT || m_dwActionOld == dfACTION_ATTACK_02_LEFT || m_dwActionOld == dfACTION_ATTACK_03_LEFT) // 이전 상태가 공격1, 공격2, 공격3 중 하나였다면
				{
					if (m_iDirCur == dfDIRECTION_LEFT)
						SetSprite(CSpritedib::ePLAYER_MOVE_L01, 12, 4);

					else if (m_iDirCur == dfDIRECTION_RIGHT)
						SetSprite(CSpritedib::ePLAYER_MOVE_R01, 12, 4);
				}				
			}

			// 내 캐릭터이고, 이전과 상태가 다르다면, 이동 시작 데이터 Send
			if (m_bPlayerCharacter == TRUE && m_dwActionOld != m_dwActionCur)
			{
				st_NETWORK_PACKET_HEADER header;
				stPACKET_CS_MOVE_START packet;

				SendProc_MoveStart(&header, &packet, m_dwActionCur, m_iCurX, m_iCurY);
				SendPacket(&header, (char*)&packet);
			}
		}
		break;
	}

	m_iDirOld = m_iDirCur;			// 방향 교체
	m_dwActionOld = m_dwActionCur;	// 상태 교체
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
	iCheck = g_cSpriteDib->DrawSprite(CSpritedib::eGUAGE_HP, m_iCurX - 35, m_iCurY + 9, bypDest, iDestWidth, iDestHeight, iDestPitch, isPlayer(), GetHP());
	if (!iCheck)
		exit(-1);
}

//---------------------------
// 방향 전환 함수
//--------------------------
void PlayerObject::DirChange(int Dir)
{
	m_iDirCur = Dir;
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
// 감소된 만큼 m_iHP 갱신
//--------------------------
void PlayerObject::DamageFunc(int iHP, DWORD TargetID)
{
	if (iHP != -1)
		m_iHP = iHP;

	m_dwTargetID = TargetID;
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

//---------------------------
// m_iDirCur 게터
//--------------------------
int PlayerObject::GetCurDir()
{
	return m_iDirCur;
}

//---------------------------
// m_dwNowAtkType 게터
//--------------------------
DWORD PlayerObject::GetNowAtkType()
{
	return m_dwNowAtkType;
}

void PlayerObject::ChangeNowAtkType(DWORD dwAtktype)
{
	m_dwNowAtkType = dwAtktype;
}