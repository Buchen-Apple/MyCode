#include "stdafx.h"
#include "Player.h"

//---------------------------
// 생성자와 소멸자
//--------------------------
CPlayer::CPlayer()
{
	Init(&m_PQueue);
	g_cSpriteDib = CSpritedib::Getinstance(5, 0xffffffff, 32);	// 싱글톤으로 CSpritedib형 객체 하나 얻기
	m_PlayerPosX = 300;
	m_PlayerPosY = 300;

}

CPlayer::~CPlayer()
{
	delete g_cSpriteDib;
}

//---------------------------
// 키다운 체크 후 이동,로직처리까지 하는 함수
//--------------------------
void CPlayer::Action()
{	
	KeyDownCheck();
	ActionLogic();
}

//---------------------------
// 실제로 내 DC에 복사하는 함수
//--------------------------
void CPlayer::Draw(int SpriteIndex, BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch)
{
	BOOL iCheck = g_cSpriteDib->DrawSprite(SpriteIndex, m_PlayerPosX, m_PlayerPosY, bypDest, iDestWidth, iDestHeight, iDestPitch);
	if (!iCheck)
		exit(-1);
}						

//---------------------------
// 눌린 키 체크 후 큐에 값을 넣는다.
//--------------------------
void CPlayer::KeyDownCheck()
{
	//---------------------------
	// 키보드 입력 파트. 큐에 행동을 넣는다.
	// --------------------------
	if (GetAsyncKeyState(VK_LEFT) & 0x8000 && GetAsyncKeyState(VK_UP) & 0x8000)		// 좌상
		Enqueue(&m_PQueue, m_PlayerPosX -1, m_PlayerPosY -1);

	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && GetAsyncKeyState(VK_UP) & 0x8000)	// 우상
		Enqueue(&m_PQueue, m_PlayerPosX + 1, m_PlayerPosY - 1);

	else if (GetAsyncKeyState(VK_LEFT) & 0x8000 && GetAsyncKeyState(VK_DOWN) & 0x8000)	// 좌하
		Enqueue(&m_PQueue, m_PlayerPosX - 1, m_PlayerPosY + 1);

	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && GetAsyncKeyState(VK_DOWN) & 0x8000)	// 우하
		Enqueue(&m_PQueue, m_PlayerPosX + 1, m_PlayerPosY + 1);

	else if (GetAsyncKeyState(VK_UP) & 0x8000)						// 상
		Enqueue(&m_PQueue, m_PlayerPosX, m_PlayerPosY - 1);

	else if (GetAsyncKeyState(VK_DOWN) & 0x8000)					// 하	
		Enqueue(&m_PQueue, m_PlayerPosX, m_PlayerPosY + 1);

	else if (GetAsyncKeyState(VK_LEFT) & 0x8000)					// 좌
		Enqueue(&m_PQueue, m_PlayerPosX - 1, m_PlayerPosY);

	else if (GetAsyncKeyState(VK_RIGHT) & 0x8000)					// 우
		Enqueue(&m_PQueue, m_PlayerPosX + 1, m_PlayerPosY);
}

void CPlayer::ActionLogic()
{
	// 큐에 있는 값을 가져와서 이동처리.
	Dequeue(&m_PQueue, &m_PlayerPosX, &m_PlayerPosY);

}