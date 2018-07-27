#pragma once
#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "BaseObject.h"
#include "ActionQueue.h"
#include "SpriteDIB.h"

class CPlayer	:public CBaseOBject
{
private:
	int m_PlayerPosX;
	int m_PlayerPosY;
	Queue m_PQueue;
	CSpritedib* g_cSpriteDib;

private:
	void KeyDownCheck();	// 키보드 눌렀는지 체크
	void ActionLogic();			// 이동, 공격 로직 처리

public:
	CPlayer();

	virtual void Action();			// 키다운 체크 후 이동, 공격 로직처리까지 하는 함수
	virtual void Draw(int SpriteIndex, BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch);			// 실제로 화면에 그리는 함수
	

	~CPlayer();

};

#endif // !__PLAYER_H__
