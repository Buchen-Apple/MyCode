#include "stdafx.h"
#include "PlayerObject.h"
#include "Network_Func.h"
#include "EffectObject.h"
#include "ProtocolBuff\ProtocolBuff.h"

extern CList<BaseObject*> list;					// Main에서 할당한 객체 저장 리스트	

using namespace Library_Jingyu;

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
	// 공격 중일 경우
	if (m_bAttackState == TRUE)
	{
		// 마지막 프레임이 아니라면 
		if (Get_bGetEndFrame() == FALSE)
		{
			// 내 캐릭터일 경우, 메시지를 강제로 이전 메시지로 변환.즉, 내 캐릭터가 공격 중에는 다른 액션 모두 무시
			if(m_bPlayerCharacter == TRUE)			
				m_dwActionCur = m_dwActionOld;
		}

		// 마지막 프레임이라면
		else
		{		
			// 현재 상태를 IDLE로 변경
			m_dwActionCur = dfACTION_IDLE;			

			// 내 캐릭터라면 
			if (m_bPlayerCharacter == TRUE)
			{
				// 상태 교체. 이걸로, switch/case문의 IDLE로 갔을 때 패킷을 안보내게 된다.
				m_dwActionOld = m_dwActionCur;	
			}

			// 공격 모드 해제
			m_bAttackState = FALSE;
			m_dwNowAtkType = 0;
						
		}			
	}

	// 공격 메시지 처리 ---------------------------------------
	switch (m_dwActionCur)
	{
	case dfACTION_ATTACK_01_LEFT:
	case dfACTION_ATTACK_02_LEFT:
	case dfACTION_ATTACK_03_LEFT:

		// 내 캐릭터의 경우, 공격 중이 아닐때만 아래 로직 처리
		// 내 캐릭터가 아닐 경우, 새로 공격 패킷이 들어왔으면 무조건 처리
		if ((m_bPlayerCharacter == TRUE && m_bAttackState == FALSE) || (m_bPlayerCharacter == FALSE && m_bPacketProcCheck == TRUE))		
		{
			m_bAttackState = TRUE;
			m_bIdleState = FALSE;
			m_bPacketProcCheck = FALSE;

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

			// 공격 이펙트 셋팅
			// 1. 내 공격 타입에 따라 이펙트 생성
			// 공격자의 공격 타입(공격 1,2,3번)에 따라 이펙트 생성
			BaseObject* Effect = new EffectObject(NULL, 2, m_iCurX, m_iCurY, m_dwNowAtkType);

			// 생성한 이펙트의 최초 스프라이트 세팅
			Effect->SetSprite(CSpritedib::eEFFECT_SPARK_01, 4, 3);

			// 리스트에 등록
			list.push_back(Effect);


			// 네트워크. 
			// 내 캐릭터일 경우 패킷 보냄
			if (m_bPlayerCharacter == TRUE)
			{
				CProtocolBuff headerBuff(dfNETWORK_PACKET_HEADER_SIZE);

				// 이전 행동이 대기, 공격이었으면 STOP패킷 보내지 않음. 이미 멈춰있는 상태이기 때문에!
				if (m_dwActionOld != dfACTION_IDLE && m_dwActionOld != dfACTION_ATTACK_01_LEFT && m_dwActionOld != dfACTION_ATTACK_02_LEFT && m_dwActionOld != dfACTION_ATTACK_03_LEFT)
				{
					// 이전 행동이 이동 중이었으면, Stop 패킷을 Send()해서 일단 멈춤상태로 만든다.	
					try
					{
						CProtocolBuff packet(dfPACKET_CS_MOVE_STOP_SIZE);

						SendProc_MoveStop((char*)&headerBuff, (char*)&packet, m_iDirCur, m_iCurX, m_iCurY);
						SendPacket((char*)&headerBuff, (char*)&packet);

					}
					catch (CException exc)
					{
						TCHAR* text = (TCHAR*)exc.GetExceptionText();
						ErrorTextOut(text);
						exit(-1);
					}
					
				}

				// 공격 패킷 Send
				headerBuff.Clear();
				CProtocolBuff atkPacket(dfPACKET_CS_ATTACK1_SIZE);
				try
				{
					if (m_dwActionCur == dfACTION_ATTACK_01_LEFT)
						SendProc_Atk_01((char*)&headerBuff, (char*)&atkPacket, m_iDirCur, m_iCurX, m_iCurY);

					else if (m_dwActionCur == dfACTION_ATTACK_02_LEFT)
						SendProc_Atk_02((char*)&headerBuff, (char*)&atkPacket, m_iDirCur, m_iCurX, m_iCurY);

					else if (m_dwActionCur == dfACTION_ATTACK_03_LEFT)
						SendProc_Atk_03((char*)&headerBuff, (char*)&atkPacket, m_iDirCur, m_iCurX, m_iCurY);

					SendPacket((char*)&headerBuff, (char*)&atkPacket);

				}
				catch (CException exc)
				{
					TCHAR* text = (TCHAR*)exc.GetExceptionText();
					ErrorTextOut(text);
					exit(-1);
				}								
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

			// 내 캐릭터이고, 이동 액션이 아이들이 아니라면 대기 시작 패킷 Send
			if (m_bPlayerCharacter == TRUE && m_dwActionCur != m_dwActionOld)
			{
				try
				{
					CProtocolBuff headerBuff(dfNETWORK_PACKET_HEADER_SIZE);
					CProtocolBuff packet(dfPACKET_CS_MOVE_STOP_SIZE);

					SendProc_MoveStop((char*)&headerBuff, (char*)&packet, m_iDirCur, m_iCurX, m_iCurY);
					SendPacket((char*)&headerBuff, (char*)&packet);

				}
				catch (CException exc)
				{
					TCHAR* text = (TCHAR*)exc.GetExceptionText();
					ErrorTextOut(text);
					exit(-1);
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
				try
				{
					CProtocolBuff headerBuff(dfNETWORK_PACKET_HEADER_SIZE);
					CProtocolBuff packet(dfPACKET_CS_MOVE_START_SIZE);

					SendProc_MoveStart((char*)&headerBuff, (char*)&packet, m_dwActionCur, m_iCurX, m_iCurY);
					SendPacket((char*)&headerBuff, (char*)&packet);

				}
				catch (CException exc)
				{
					TCHAR* text = (TCHAR*)exc.GetExceptionText();
					ErrorTextOut(text);
					exit(-1);
				}				
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
	// 카메라 좌표 기준, 내가 출력될 위치 구한다.
	CMap* SIngletonMap = CMap::Getinstance(0, 0, 0, 0, 0);
	int ShowX = SIngletonMap->GetShowPosX(m_iCurX);
	int ShowY = SIngletonMap->GetShowPosY(m_iCurY);

	/*
	if (ShowX < 0)
		ShowX = 0;

	if(ShowY < 0)
		ShowY = 0;
	*/

	// 내 그림자 
	BOOL iCheck = g_cSpriteDib->DrawSprite(CSpritedib::eSHADOW, ShowX, ShowY, bypDest, iDestWidth, iDestHeight, iDestPitch, isPlayer());
	if (!iCheck)
		exit(-1);

	// 내 캐릭터
	// 캐릭터의 m_iCurX, m_iCurY는 중점을 기준으로 계산한 캐릭터의 위치이다. 즉, 비트맵의 좌상단이 아니다!	
	iCheck = g_cSpriteDib->DrawSprite(m_iSpriteNow, ShowX, ShowY, bypDest, iDestWidth, iDestHeight, iDestPitch, isPlayer());
	if (!iCheck)
		exit(-1);

	// 내 HP 게이지
	iCheck = g_cSpriteDib->DrawSprite(CSpritedib::eGUAGE_HP, ShowX - 35, ShowY + 9, bypDest, iDestWidth, iDestHeight, iDestPitch, isPlayer(), GetHP());
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

void PlayerObject::NonMyAtackCheck(BOOL Check)
{
	m_bPacketProcCheck = Check;
}