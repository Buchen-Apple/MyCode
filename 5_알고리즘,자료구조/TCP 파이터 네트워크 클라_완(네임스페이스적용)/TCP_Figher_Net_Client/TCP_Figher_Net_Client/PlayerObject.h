#pragma once
#ifndef __PLAYER_OBJECT_H__
#define __PLAYER_OBJECT_H__

#include "BaseObject.h"

#define PLAYER_XMOVE_PIXEL 3
#define PLAYER_YMOVE_PIXEL 2

#define dfACTION_MOVE_LL	0
#define dfACTION_MOVE_LU	1
#define dfACTION_MOVE_UU	2
#define dfACTION_MOVE_RU	3
#define dfACTION_MOVE_RR	4
#define dfACTION_MOVE_RD	5
#define dfACTION_MOVE_DD	6
#define dfACTION_MOVE_LD	7
#define dfACTION_ATTACK_01_LEFT		20
#define dfACTION_ATTACK_02_LEFT		21
#define dfACTION_ATTACK_03_LEFT		22
#define dfACTION_ATTACK_01_RIGHT	23
#define dfACTION_ATTACK_02_RIGHT	24
#define dfACTION_ATTACK_03_RIGHT	25
#define dfACTION_IDLE		100

#define dfDIRECTION_LEFT	dfACTION_MOVE_LL
#define dfDIRECTION_RIGHT	dfACTION_MOVE_RR

//-----------------------------------------------------------------
// 화면 이동 범위.
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	50 + 1
#define dfRANGE_MOVE_LEFT	10 + 2
#define dfRANGE_MOVE_RIGHT	630 - 3
#define dfRANGE_MOVE_BOTTOM	470 - 1


class PlayerObject :public BaseObject
{
private:
	BOOL m_bPlayerCharacter;	// 내 캐릭터인지 체크. true면 내 캐릭터 / false면 다른사람 캐릭터
	BOOL m_bPacketProcCheck;	// 내 캐릭터가 아닌 경우, 현재 새로운 공격 패킷이 왔는지 체크. 이게 TRUE면 공격 로직을 무조건 처리한다.
	int m_iHP;					// 내 HP
	DWORD m_dwActionCur;		// 현재 액션이 무엇인지 저장.
	DWORD m_dwActionOld;		// 현재의 바로 이전 액션이 무엇인지 저장. 현재 액션 / 과거 액션을 비교해 다르다면 메시지를 보낸다.
	int m_iDirCur;				// 현재 바라보는 방향 (0 : 좌 / 4 : 우)
	int m_iDirOld;				// 이전에 바라보던 방향		
	bool m_bAttackState;		// 공격 중인지 체크하는 변수. FLASE면 공격 중 아님 / TRUE면 공격 중
	bool m_bIdleState;			// 대기 자세인지 체크하는 변수. FALSE면 대기중 아님 / TRUE면 대기중.
	DWORD m_dwTargetID;			// 내가 데미지를 입힐 대상의 ID
	DWORD m_dwNowAtkType;		// 현재 공격중인 타입. 0이면 공격중 아님.

public:
	PlayerObject(int iObjectID, int iObjectType, int iCurX = 50, int iCurY = 300, int iDirCur = dfDIRECTION_RIGHT);

	virtual ~PlayerObject();
	virtual void Action();
	virtual void Draw(BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch);

	void NonMyAtackCheck(BOOL Check);
	void DirChange(int Dir);
	void MemberSetFunc(BOOL bPlayerChar, int iHP);
	void DamageFunc(int iHP, DWORD TargetID);
	void ActionProc();
	BOOL isPlayer();
	int GetHP();
	int GetCurDir();
	DWORD GetNowAtkType();
	void ChangeNowAtkType(DWORD dwAtktype);

};

#endif // !__PLAYER_OBJECT_H__
