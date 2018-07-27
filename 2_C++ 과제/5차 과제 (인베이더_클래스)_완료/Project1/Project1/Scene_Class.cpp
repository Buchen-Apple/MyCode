#include <iostream>
#include <cstring>
#include "Scene_Class.h"
#include "CList_Template.h"
#include "MoveQueue.h"

#define HEIGHT 30
#define WIDTH 81

#define UI_HEIGHT 27
#define UI_WIDTH 81

// 전역변수 모음
HANDLE hConsole;
CList<CObject*> list;
CSceneHandle hSceneHandle;
char cBackBuffer[HEIGHT][WIDTH];

// 씬 enum
enum SceneType
{
	None = 0, Title = 1, Select, Ingame, Lose, Victory
};

// Title과 Select에서 공용으로 사용하는 변수 모음. 이름공간
namespace TitleAndSelectText
{
	int m_iArrowShowPosy = 5;	// 타이틀 화면, 캐릭터 선택에서 화살표가 표시될 위치. 5면 Ingame / 6이면 Exit에 표시된다.
	bool m_bArrowShowFlag = true;	//타이틀 화면, 캐릭터 선택에서 화살표 보여줄지 결정하는 변수. ture면 보여줌 / flase면 안보여줌. 트루펄스 반복하면서 깜빡깜빡인다.

	char m_cTip[23] = "★ Z키를 눌러주세요 ★";
	char m_cTitle[6] = "TITLE";
	char m_cStart[6] = "START";
	char m_cExit[5] = "EXIT";

	char g_cCharSeletTip[23] = "캐릭터를 선택해 주세요";
	char g_cCharSeletChaTip_1[35] = "(체력: 1  / 추가미사일: 2초에 2개)";
	char g_cCharSeletChaTip_2[35] = "(체력: 2  / 추가미사일: 2초에 1개)";
	char g_cCharSeletChaTip_3[35] = "(체력: 5  / 추가미사일:   없음   )";;
}

// 콘솔 제어를 위한 준비 작업
void cs_Initial(void)
{
	CONSOLE_CURSOR_INFO stConsoleCursor;
	// 화면의 커서 안보이게 설정
	stConsoleCursor.bVisible = FALSE;
	stConsoleCursor.dwSize = 1; // 커서 크기

								// 콘솔화면 핸들을 구한다.
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorInfo(hConsole, &stConsoleCursor);
}
// 콘솔 화면의 커서를 x,y 좌표로 이동
void cs_MoveCursor(int iPosy, int iPosx)
{
	COORD stCoord;
	stCoord.X = iPosx;
	stCoord.Y = iPosy;
	// 원하는 위치로 커서 이동
	SetConsoleCursorPosition(hConsole, stCoord);
}


// --------------------------
// CPlayer 클래스 멤버함수
// --------------------------
// 생성자
CPlayer::CPlayer()
{
	Type = 1;
	m_iPosX = 0;
	m_iPosY = 0;
	m_bAriveCheck = 0;
	m_MoveQueue = new Queue;
	m_AtkQueue = new Queue;
	m_cLook = 0;
	m_cMissaleLook = 0;
	m_cHP[0] = 0;
	m_cHP[1] = 0;
	m_iCharSpeed = 0;
	m_iBonusMIssaleCount = 0;
	m_BonusMIssaleTImeSave = 0;
	iPlayerMissaleCount = 0;
}
// 플레이어 정보 세팅. CSceneSelect에서 기본 정보 세팅
void CPlayer::InfoSetFunc_1(int m_iArrowShowPosy)
{
	if (m_iArrowShowPosy == 5)
	{
		m_cLook = 'A';
		m_cMissaleLook = '1';
		m_cHP[0] = '0';
		m_cHP[1] = '1';
		m_iCharSpeed = 1;
		m_iBonusMIssaleCount = 2;
	}
	else if (m_iArrowShowPosy == 7)
	{
		m_cLook = 'B';
		m_cMissaleLook = '2';
		m_cHP[0] = '0';
		m_cHP[1] = '2';
		m_iCharSpeed = 1;
		m_iBonusMIssaleCount = 1;
	}
	else if (m_iArrowShowPosy == 9)
	{
		m_cLook = 'V';
		m_cMissaleLook = '3';
		m_cHP[0] = '0';
		m_cHP[1] = '5';
		m_iCharSpeed = 1;
		m_iBonusMIssaleCount = 0;
	}
}
// 플레이어 정보 세팅. CSceneGame에서 좀 더 자세히 정보 추가세팅
void CPlayer::InfoSetFunc_2()
{
	// 인 게임을 시작하면, 현재 시간 저장. 아군 캐릭터의 추가 미사일 발사를 위해.
	m_BonusMIssaleTImeSave = GetTickCount64();

	m_iPosX = 40;	// 플레이어 시작 위치 세팅
	m_iPosY = 20;
	m_bAriveCheck = true;	// 플레이어 생존 여부를 true로 만듦

	// 플레이어 행동 로직 큐, 공격 로직 큐 초기화
	Init(m_MoveQueue);
	Init(m_AtkQueue);
}
// 플레이어 이동/ 공격 여부 체크
void CPlayer::KeyDownCheck()
{
	// 유저가 누른 키를 체크해, 행동 로직을 큐로 저장. 저장한 행동 로직은 '로직'에서 작동
	if (GetAsyncKeyState(VK_UP) & 0x8001)
	{
		if (m_iPosY - 1 > (UI_HEIGHT / 2))
			Enqueue(m_MoveQueue, m_iPosX, m_iPosY - m_iCharSpeed);
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8001)
	{
		if (m_iPosY + 1 < UI_HEIGHT - 1)
			Enqueue(m_MoveQueue, m_iPosX, m_iPosY + m_iCharSpeed);
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8001)
	{
		if (m_iPosX + 1 < 69)
			Enqueue(m_MoveQueue, m_iPosX + m_iCharSpeed, m_iPosY);
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8001)
	{
		if (m_iPosX - 1 > 11)
			Enqueue(m_MoveQueue, m_iPosX - m_iCharSpeed, m_iPosY);
	}

	// 유저가 공격 버튼을 눌렀으면,공격 로직을 큐에 저장. 저장한 공격 로직은 '로직'에서 작동
	if (GetAsyncKeyState(0x5A) & 0x8001)
	{
		if (iPlayerMissaleCount < PLAYER_MISSALE_COUNT)
		{
			iPlayerMissaleCount++;
			Enqueue(m_AtkQueue, m_iPosX, m_iPosY - 1);
		}
	}
}
// 플레이어의 HP를 얻는 함수
char* CPlayer::GetHP()
{
	return m_cHP;
}
// 플레이어의 생존여부를 얻는 함수
bool CPlayer::GetArive()
{
	return m_bAriveCheck;
}
// 플레이어의 이동 로직처리 함수
void CPlayer::PlayerMoveFunc()
{
	////유저 이동 로직 (유저가 생존했을 때만 로직 실행) ////
	int iTempPosx;
	int iTempPosy;

	// 현재 유저의 위치를 버퍼에 저장한다.
	cBackBuffer[m_iPosY][m_iPosX] = m_cLook;

	// 현재 유저의 x,y 위치를 저장한다.
	iTempPosx = m_iPosX;
	iTempPosy = m_iPosY;

	// 행동 로직을 받아서(큐), 이동할 위치가 있으면 그 위치로 이동시킨다.
	while (Dequeue(m_MoveQueue, &m_iPosX, &m_iPosY))
	{
		cBackBuffer[iTempPosy][iTempPosx] = ' ';	//디큐가 성공하면, 유저가 기존에 있던 위치는 스페이스로 변경한다.
		cBackBuffer[m_iPosY][m_iPosX] = m_cLook;

		iTempPosx = m_iPosX;	// 다음 while문때도 디큐가 성공할 수 있으니, 디큐 후의 위치도 저장한다.
		iTempPosy = m_iPosY;
	}
	
}
// 미사일 이동 및 소멸 처리 함수
void CPlayer::MissaleMoveFunc(CMissale* missale, CEnemy* enemy, int* iEnemyAriveCount)
{	
	//// 생성되어 있는 유저의 미사일 이동 및 소멸 로직 (유저가 생존했을 때만 로직 실행)////		

	// 미사일 이동 및 소멸 로직// 
	// 살아있는 미사일 중, 아군 미사일을 -y쪽으로 1칸 이동한다.
	for (int i = PLAYER_MISSALE_COUNT; i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT; i++)
	{
		if (missale[i].m_bAriveCheck == true && missale[i].m_PlayerOrEnemy == true)	//살아있는 미사일이며 아군 미사일인 경우
		{
			missale[i].m_iPosY--;	// -y쪽으로 1칸 이동시킨다.
									
			if (missale[i].m_iPosY == 3)	// -y쪽으로 1칸 이동했는데, 그게 벽이면 false로 만듦.
			{
				missale[i].m_bAriveCheck = false;
				iPlayerMissaleCount--;
			}
			// -y쪽으로 1칸 이동했는데, 그게 몬스터면 false로 만들고 해당 위치의 몬스터를 false로 만듦(사망 상태)
			// flase가 된 몬스터는 자동으로 버퍼에서 제외된다.
			else if (cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] == 'M')
			{
				for (int j = 0; j < ENEMY_COUNT; j++)
				{
					if (enemy[j].m_iPosY == missale[i].m_iPosY && enemy[j].m_iPosX == missale[i].m_iPosX)
					{
						// 해당 몬스터가 살아있는 몬스터라면, (혹시 모르니 여기서 생존여부 다시한번 체크. 자꾸 이상한게 1~2마리 남았는데 게임이 종료됨..)
						if (enemy[j].m_bAriveCheck == true)
						{
							enemy[j].m_bAriveCheck = false;
							(*iEnemyAriveCount)--;	// 생존 중인 Enemy 수 1개체 줄임
							if (*iEnemyAriveCount == 0)
								cBackBuffer[enemy[j].m_iPosY][enemy[j].m_iPosX] = ' ';	// 적이 0명이 되면, 마지막 적을 스페이스로 만든다. 그래야 화면에는 사라진 것 처럼 나옴.

							missale[i].m_bAriveCheck = false;
							iPlayerMissaleCount--;	// 생존 중인 미사일 수 1개 줄임		
							break;
						}
					}
				}

			}
			// 소멸되지 않는 미사일이면 (벽도 아니고, 몬스터와 충돌하지도 않음), 미사일을 그린다.
			else
			{
				if (missale[i].m_BonusOrNot == false)
					cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = m_cMissaleLook;
				else
					cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'b';
			}
		}
	}

	
}
// 미사일 생성 처리 함수
void CPlayer::MissaleCreateFunc(CMissale* missale)
{
	//// 미사일 생성 로직 ////
	int iTempPosx;
	int iTempPosy;
	int i = 0;
	// 공격 로직을 받아서(큐), 해당 좌표의 미사일을 활성화시킨다.
	while (Dequeue(m_AtkQueue, &iTempPosx, &iTempPosy))
	{
		// m_bAriveCheck 값이 false인(생존중이지 않은) 아군 미사일을 찾는다.
		for (i = PLAYER_MISSALE_COUNT; i < PLAYER_MISSALE_COUNT + ENEMY_MISSALE_COUNT; i++)
		{
			if (missale[i].m_bAriveCheck == false && missale[i].m_PlayerOrEnemy == true)
				break;
		}

		// i의 값이, PLAYER_MISSALE_COUNT + ENEMY_MISSALE_COUNT보다 작으면 죽은 상태의(false) 미사일을 찾았다는 것이니 아래 로직 진행
		if (i < PLAYER_MISSALE_COUNT + ENEMY_MISSALE_COUNT)
		{
			// 찾은 위치의 미사일을 생존(true)으로 만들고, 찾은 위치의 x,y값을 설정한다.
			missale[i].m_bAriveCheck = true;
			missale[i].m_BonusOrNot = false;	//보너스 외형을 체크하기 위한것. 보너스면 true / 보너스 미사일이 아니면 false
			missale[i].m_iPosX = iTempPosx;
			missale[i].m_iPosY = iTempPosy;

			// 그리고, 생성된 미사일을 버퍼에 저장한다.
			cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = m_cMissaleLook;
		}
	}

}
// 보너스 미사일 생성 처리. 이동은 MissaleMoveFunc에서 같이 처리
void CPlayer::BonusMissaleFunc(CMissale* missale, ULONGLONG TimeCheck)
{
	// 유저의 공격 입력과는 상관 없이, 일정 시간마다 보너스 미사일이 발사된다.
	// 만약, 공격할 때가 됐다면,
	if (m_BonusMIssaleTImeSave + PLAYER_AI_ATACK_TIME <= TimeCheck)
	{
		// 현재 플레이어가 가지고 있는 추가 미사일 발사 카운트만큼 반복하며 미사일 생성
		// 추가 미사일 발사 카운트가 0이라면, 반복하지 않는다.
		for (int h = 1; h <= m_iBonusMIssaleCount; ++h)
		{
			int i = 0;

			// m_bAriveCheck 값이 false인(생존중이지 않은) 아군 미사일을 찾는다.
			for (i = PLAYER_MISSALE_COUNT; i < PLAYER_MISSALE_COUNT + ENEMY_MISSALE_COUNT; i++)
			{
				if (missale[i].m_bAriveCheck == false && missale[i].m_PlayerOrEnemy == true)
					break;
			}

			// i의 값이, ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT보다 작으면 false인 배열을 찾았다는 것이니 아래 로직 진행
			if (i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT)
			{
				// 찾은 위치의 미사일을 생존(true)로 만들고, 보너스 미사일(true)를 설정하고 미사일을 생성한다.
				missale[i].m_bAriveCheck = true;
				missale[i].m_BonusOrNot = true;
				if (h == 1)
				{
					missale[i].m_iPosX = m_iPosX - 2;
					missale[i].m_iPosY = m_iPosY - 1;
					cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'b';
				}
				else if (h == 2)
				{
					missale[i].m_iPosX = m_iPosX + 2;
					missale[i].m_iPosY = m_iPosY - 1;
					cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'b';
				}
			}
		}
		// 미사일을 모두 생성했으면, 현재 시간을 저장한다. 그래야 다음에 또 추가 미사일을 발사하니까..
		m_BonusMIssaleTImeSave = TimeCheck;
	}

}
// 소멸자
CPlayer::~CPlayer()
{
	delete m_MoveQueue;
	delete m_AtkQueue;
}


// --------------------------
// CEnemy 클래스 멤버함수
// --------------------------
// 생성자
CEnemy::CEnemy()
{
	Type = 2;
	EnemyAtkQueue = new Queue;
}
// 대입 연산자
CEnemy& CEnemy::operator=(const CEnemy* ref)
{
	m_iPosX = ref->m_iPosX;
	m_iPosY = ref->m_iPosY;
	m_bAriveCheck = ref->m_bAriveCheck;
	m_iAttackTick = ref->m_iAttackTick;
	m_MoveTimeSave = ref->m_MoveTimeSave;
	m_AttackTimeSave = ref->m_AttackTimeSave;
	EnemyAtkQueue = ref->EnemyAtkQueue;

	return *this;
}
// 몬스터 정보 세팅
void CEnemy::InfoSetFunc(int* iEnemyPosx, int* iEnemyPosy, int* iAttackTick1, int* iAttackTick2, int* iAtkPatten, ULONGLONG AttackTimeSave)
{
	// -----------------------
	//		적군 정보 세팅
	// -----------------------

	Init(EnemyAtkQueue);

	//  현재 시간 저장. 
	m_AttackTimeSave = AttackTimeSave;	// 몬스터의 공격 AI 작동을 위한 시간 체크.이 시간부터 m_iAttackTick시간 이후에 AI 작동
	m_MoveTimeSave = AttackTimeSave;	// 몬스터의 이동 AI 작동을 위한 시간 체크.이 시간부터 ENEMY_AI_TIME시간 이후에 AI 작동

	m_iPosX = *iEnemyPosx;
	m_iPosY = *iEnemyPosy;
	m_iAttackTick = *iAttackTick1;
	m_bAriveCheck = true;

	// 아래 if문은 적군의 공격 AI 틱 지정
	if (*iAtkPatten == 4)
	{
		*iAttackTick1 = *iAttackTick2;
		*iAtkPatten = 0;
		*iAttackTick2 += 600;
	}
	else
	{
		*iAttackTick1 += 1500;
		(*iAtkPatten)++;
	}

	// 아래 if문은 적군의 최초 스폰 위치 지정
	if (*iEnemyPosx > 60)
	{
		*iEnemyPosx = 17;
		*iEnemyPosy += 1;
	}
	else
		*iEnemyPosx += 5;
}
// 몬스터 AI 이동 처리
void CEnemy::EnemyMoveFunc(int* iEnemyAIState, ULONGLONG ulEnemyAiTimeCur, bool* bAiActiveCheck)
{
	//// Enemy 이동 AI 로직 (현재 생존한 Enemy를 기준으로 로직 실행)	////
	// 이전 AI가 작동한지 1.4초(ENEMY_AI_TIME)가 된 경우
	if (m_MoveTimeSave + ENEMY_AI_TIME <= ulEnemyAiTimeCur)
	{
		if (*iEnemyAIState == 0)
			m_iPosX -= 2;
		else if (*iEnemyAIState == 1)
			m_iPosY--;	
		else if (*iEnemyAIState == 2)
			m_iPosX += 2;
		else if (*iEnemyAIState == 3)
			m_iPosY++;

		*bAiActiveCheck = true;					// 한 번이라도 작동하면 true로 만듦
		m_MoveTimeSave = ulEnemyAiTimeCur;	// 시간을 새로 저장. 그래야 다음 AI를 작동시킬 기준이 되기 때문에.
	}	
	cBackBuffer[m_iPosY][m_iPosX] = 'M';
}
// 몬스터 생존여부 리턴
bool CEnemy::GetArive()
{
	return m_bAriveCheck;
}
// 몬스터 공격 인큐 처리
void CEnemy::EnemyEnqueueFunc(ULONGLONG ulEnemyAiTimeCur)
{
	//// Enemy 공격 AI 로직 (현재 생존한 Enemy를 기준으로 로직 실행). 적 미사일을 생성한다.	////

	// Enemy 공격 큐 넣기 로직
	// 해당 적군이 공격할 때가 됐으면, 공격 명령을 큐로 보냄
	if (ulEnemyAiTimeCur >= m_AttackTimeSave + m_iAttackTick)
	{
		Enqueue(EnemyAtkQueue, m_iPosX, m_iPosY + 1);
		m_AttackTimeSave = ulEnemyAiTimeCur;	// 적군의 구조체 변수에 현재 시간 저장.
	}
}
// 몬스터 미사일 이동 처리
void CEnemy::EnemyMissaleMoveFunc(CMissale* missale, CPlayer* player)
{
	//// Enemy 미사일 이동 및 소멸 로직 (몬스터 생존과 상관 없이 로직 실행)	////
	// 살아있는 모든 미사일을 -y쪽으로 1칸 이동 후 버퍼에 저장.		
	for (int i = 0; i < ENEMY_MISSALE_COUNT; i++)
	{
		if (missale[i].m_bAriveCheck == true && missale[i].m_PlayerOrEnemy == false)	// 살아있고, 몬스터의 미사일이면. Y++로 1칸 이동
		{
			missale[i].m_iPosY++;

			// +y쪽으로 1칸 이동했는데, 그게 벽이면 false로 만듦.
			if (missale[i].m_iPosY == UI_HEIGHT - 1)
				missale[i].m_bAriveCheck = false;

			// +y쪽으로 1칸 이동했는데, 그게 유저면.  
			else if (cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] == player->m_cLook)
			{
				// 유저의 hp를 1 깍는다. 1깍을 때, HP 수치를 보여주는 문자열을 체크해 체력을 감소시킨다.
				if (player->m_cHP[0] != '0')
				{
					player->m_cHP[0]--;
					if (player->m_cHP[0] == '0')
						player->m_cHP[1] = '9';
				}
				else if (player->m_cHP[1] != '0')
					player->m_cHP[1]--;

				// 유저의 hp가 0이라면 유저의 해당 위치의 유저를 false로 만듦(사망 상태)
				if (player->m_cHP[1] == '0')
					player->m_bAriveCheck = false;

				// 사망과는 상관 없이, 유저가 공격받으면 유저 위치를 스페이스바로 바꾼다. 이는, 공격 받을 때 깜빡이기 or 사망하면 안보이게 하기의 역활이다.
				cBackBuffer[player->m_iPosY][player->m_iPosX] = ' ';
			}

			else
				cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'v';
		}
	}
}
// 몬스터 공격 디큐 처리
void CEnemy::EnemyDequeueFunc(CMissale* missale)
{
	//// Enemy 공격 큐 빼오기 로직 ////
	// 공격 로직을 받아서(큐), 해당 좌표의 미사일을 활성화시킨다. 
	int iTempPosx;
	int iTempPosy;

	while (Dequeue(EnemyAtkQueue, &iTempPosx, &iTempPosy))
	{
		int i = 0;
		// 사망 상태의(false) 적의 미사일을 찾는다.
		for (; i < ENEMY_MISSALE_COUNT; i++)
		{
			if (missale[i].m_bAriveCheck == false && missale[i].m_PlayerOrEnemy == false)
				break;
		}

		// i의 값이, ENEMY_MISSALE_COUNT보다 작으면 false인 배열을 찾았다는 것이니 아래 로직 진행
		if (i < ENEMY_MISSALE_COUNT)
		{
			// 찾은 위치의 미사일을 생존(true)으로 만들고, 찾은 위치의 x,y값을 설정한다.
			missale[i].m_bAriveCheck = true;
			missale[i].m_iPosX = iTempPosx;
			missale[i].m_iPosY = iTempPosy;

			// 그리고, 생성된 미사일을 버퍼에 저장한다.
			cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'v';
		}
	}
}
// 소멸자
CEnemy::~CEnemy()
{
	delete EnemyAtkQueue;
}


// --------------------------
// CMissale 클래스 멤버함수
// --------------------------
// 생성자
CMissale::CMissale()
{
	Type = 3;
}
// 대입 연산자
CMissale& CMissale::operator=(const CMissale* ref)
{
	m_iPosX = ref->m_iPosX;
	m_iPosY = ref->m_iPosY;
	m_PlayerOrEnemy = ref->m_PlayerOrEnemy;
	m_bAriveCheck = ref->m_bAriveCheck;
	m_BonusOrNot = ref->m_BonusOrNot;

	return *this;

}
// 미사일 정보 세팅
void CMissale::InfoSetFunc(bool bIdentity)
{
	// -----------------------
	//	 미사일 세팅
	// -----------------------
	//배열 0 ~ 49는 적군 것. 배열 50 ~ 99번은 플레이어의 것
	m_PlayerOrEnemy = bIdentity;	// 미사일의 정보를 아군/적군것으로 세팅. 트루면 아군 / 펄스면 적군
	m_bAriveCheck = false;			// 미사일의 생존 여부 설정. 처음 시작때는 모두 비활성화상태.
}


// --------------------------
// CSceneBase 클래스 멤버함수
// --------------------------
// CSceneBase::버퍼 초기화 함수
void CSceneBase::BufferClear()
{
	memset(cBackBuffer, 0X20, HEIGHT*WIDTH);
	for (int i = 0; i < HEIGHT; ++i)
		cBackBuffer[i][WIDTH - 1] = '\0';
}
// CSceneBase::버퍼 플립 함수
void CSceneBase::BufferFlip(int iPosy, int iPosx)
{
	cs_MoveCursor(iPosy, iPosx);
	printf("%s", cBackBuffer[iPosy]);
}


// --------------------------
// CSceneTitle 클래스 멤버함수
// --------------------------
// CSceneTitle:생성자
// CSceneTitle::Title 함수
void CSceneTitle::Title()
{
	memcpy(cBackBuffer[3] + 38, TitleAndSelectText::m_cTitle, sizeof(TitleAndSelectText::m_cTitle) - 1);
	memcpy(cBackBuffer[5] + 38, TitleAndSelectText::m_cStart, sizeof(TitleAndSelectText::m_cStart) - 1);
	memcpy(cBackBuffer[6] + 38, TitleAndSelectText::m_cExit, sizeof(TitleAndSelectText::m_cExit) - 1);

	memcpy(cBackBuffer[8] + 30, TitleAndSelectText::m_cTip, strlen(TitleAndSelectText::m_cTip) + 1 - 1);

	// iArrowShowPosy의 값에 따라 기존 화살표는 지워줌
	if (TitleAndSelectText::m_iArrowShowPosy == 5)
	{
		cBackBuffer[6][35] = ' ';
		cBackBuffer[6][36] = ' ';
	}
	else if (TitleAndSelectText::m_iArrowShowPosy == 6)
	{
		cBackBuffer[5][35] = ' ';
		cBackBuffer[5][36] = ' ';
	}

	// 화살표 깜빡깜빡 절차
	if (TitleAndSelectText::m_bArrowShowFlag == true)
	{
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][35] = '-';
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][36] = '>';
	}
	else
	{
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][35] = ' ';
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][36] = ' ';
	}


	for (int i = 0; i<HEIGHT; ++i)
		BufferFlip(i, 0);
}
// CSceneTitle::run 함수
void CSceneTitle::run()
{	
	// 백버퍼 초기화
	BufferClear();
	// 타이틀 창 표시 함수 호출
	Title();
	Sleep(200);

	// 깜빡깜빡 하기 위한 절차
	if (TitleAndSelectText::m_bArrowShowFlag == true)
		TitleAndSelectText::m_bArrowShowFlag = false;
	else
		TitleAndSelectText::m_bArrowShowFlag = true;

	// 유저가 Z키를 눌렀을 때, 현재 화살표가 어딜 가리키고 있는지에 따라 다음 씬이 인게임인지 EXIT 인지 결정
	if (GetAsyncKeyState(0x5A) & 0x8001)
	{
		if (TitleAndSelectText::m_iArrowShowPosy == 5)
			hSceneHandle.LoadScene(Select);

		else if (TitleAndSelectText::m_iArrowShowPosy == 6)
			hSceneHandle.LoadScene(Lose);
	}

	// 유저가 키보드 위/아래를 선택하면, 선택한 버튼에 따라 iArrowShowPosy의 값을 바꿔줌. 즉, 화살표가 위 아래중 어디를 가리킬지 결정
	if (GetAsyncKeyState(VK_DOWN) & 0x8001)
		TitleAndSelectText::m_iArrowShowPosy = 6;
	if (GetAsyncKeyState(VK_UP) & 0x8001)
		TitleAndSelectText::m_iArrowShowPosy = 5;
}



// --------------------------
// CSceneSelect 클래스 멤버함수
// --------------------------
// run 함수
void CSceneSelect::run()
{
	// 백버퍼 초기화
	BufferClear();

	// 캐릭터 선택창 표시 함수 호출
	CharSeleet();
	Sleep(200);

	// 깜빡깜빡 하기 위한 절차
	if (TitleAndSelectText::m_bArrowShowFlag == true)
		TitleAndSelectText::m_bArrowShowFlag = false;
	else
		TitleAndSelectText::m_bArrowShowFlag = true;
	
	// Z버튼 키 다운 체크
	KeyDown();

	// 유저가 키보드 위/아래를 선택하면, 선택한 버튼에 따라 iArrowShowPosy의 값을 바꿔줌. 즉, 화살표가 위, 중간, 아래 중 어디를 가리킬지 결정
	if (GetAsyncKeyState(VK_DOWN) & 0x8001)
	{
		if (TitleAndSelectText::m_iArrowShowPosy != 9)
			TitleAndSelectText::m_iArrowShowPosy += 2;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8001)
	{
		if (TitleAndSelectText::m_iArrowShowPosy != 5)
			TitleAndSelectText::m_iArrowShowPosy -= 2;
	}

}
// 캐릭터 선택 함수
void CSceneSelect::CharSeleet()
{
	memcpy(cBackBuffer[3] + 30, TitleAndSelectText::g_cCharSeletTip, sizeof(TitleAndSelectText::g_cCharSeletTip) - 1);

	cBackBuffer[5][23] = 'A';
	memcpy(cBackBuffer[5] + 25, TitleAndSelectText::g_cCharSeletChaTip_1, sizeof(TitleAndSelectText::g_cCharSeletChaTip_1) - 1);

	cBackBuffer[7][23] = 'B';
	memcpy(cBackBuffer[7] + 25, TitleAndSelectText::g_cCharSeletChaTip_2, sizeof(TitleAndSelectText::g_cCharSeletChaTip_2) - 1);

	cBackBuffer[9][23] = 'V';
	memcpy(cBackBuffer[9] + 25, TitleAndSelectText::g_cCharSeletChaTip_3, sizeof(TitleAndSelectText::g_cCharSeletChaTip_3) - 1);

	memcpy(cBackBuffer[11] + 30, TitleAndSelectText::m_cTip, sizeof(TitleAndSelectText::m_cTip) - 1);

	// iArrowShowPosy의 값에 따라 기존 화살표는 지워줌
	if ((TitleAndSelectText::m_iArrowShowPosy) == 5)
	{
		cBackBuffer[7][20] = ' ';
		cBackBuffer[7][21] = ' ';

		cBackBuffer[9][20] = ' ';
		cBackBuffer[9][21] = ' ';
	}
	else if (TitleAndSelectText::m_iArrowShowPosy == 7)
	{
		cBackBuffer[5][20] = ' ';
		cBackBuffer[5][21] = ' ';

		cBackBuffer[9][20] = ' ';
		cBackBuffer[9][21] = ' ';
	}
	else if (TitleAndSelectText::m_iArrowShowPosy == 9)
	{
		cBackBuffer[5][20] = ' ';
		cBackBuffer[5][21] = ' ';

		cBackBuffer[7][20] = ' ';
		cBackBuffer[7][21] = ' ';
	}

	// 화살표 깜빡깜빡 절차
	if (TitleAndSelectText::m_bArrowShowFlag == true)
	{
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][20] = '-';
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][21] = '>';
	}
	else
	{
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][20] = ' ';
		cBackBuffer[TitleAndSelectText::m_iArrowShowPosy][21] = ' ';
	}

	for (int i = 0; i<HEIGHT; ++i)
		BufferFlip(i, 0);
}
// 키보드 체크
void CSceneSelect::KeyDown()
{
	// 유저가 Z키를 눌렀을 때, 현재 화살표가 어딜 가리키고 있는지에 따라 캐릭터 정보 세팅
	if (GetAsyncKeyState(0x5A) & 0x8001)
	{
		/*CList<CObject*>::iterator itor;
		itor = list.begin();*/
		CPlayer* player = new CPlayer;

		player->InfoSetFunc_1(TitleAndSelectText::m_iArrowShowPosy);
		list.push_back(player);
		hSceneHandle.LoadScene(Ingame);
	}

}


// --------------------------
// CSceneGame 클래스 멤버함수
// --------------------------
// 생성자
CSceneGame::CSceneGame()
{
	bAiActiveCheck = false;
	iEnemyAIState = 0;	
	// ---------------------------
	//	각종 변수 동적할당
	// ---------------------------
	player = new CPlayer;
	enemy = new CEnemy[ENEMY_COUNT];
	missale = new CMissale[ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT];

	// ---------------------------
	//	 시간 UI를 위한 정보 세팅
	// ---------------------------						
	Time[5] = '\0';
	a = '0';
	b = '0';
	c = '0';
	d = '0';

	timeBeginPeriod(1);
	m_ulStartTimeSave = GetTickCount64();
	ulUITimeSave = m_ulStartTimeSave;
	timeEndPeriod(1);

	// ---------------------------
	//	 플레이어 정보 추가 세팅
	// ---------------------------
	// Select에서 세팅한 플레이어 정보를 가져옴. 
	CList<CObject*>::iterator itor;
	for (itor = list.begin(); itor != list.end(); itor++)
	{
		player = (CPlayer*)*itor;
		if (player->Type == 1)
			break;
	}	

	// 나머지 플레이어 정보 세팅
	player->InfoSetFunc_2();

	// 기존 플레이어 정보 삭제 후, 새로 세팅된 정보로 List에 넣는다.
	list.erase(itor);
	list.push_back(player);

	// ---------------------------
	//	 몬스터 정보 세팅
	// ---------------------------

	// 몬스터 정보 세팅
	// -> 몬스터 정보는, 객체마다 다른 정보를 갖기 때문에 여기서 세팅
	int iEnemyPosx = 17;
	int iEnemyPosy = 5;
	int m_iAttackTick1 = 1400;
	int m_iAttackTick2 = 1000;
	int iAtkPatten = 0;

	// 정의한 몬스터 정보들 세팅
	iEnemyAriveCount = 0;;	// 생존 중인 몬스터의 수. 새로 생성될 때 마다 하나씩 증가.
	for (int i = 0; i < ENEMY_COUNT; i++)
	{
		enemy[i].InfoSetFunc(&iEnemyPosx, &iEnemyPosy, &m_iAttackTick1, &m_iAttackTick2, &iAtkPatten, m_ulStartTimeSave);
		iEnemyAriveCount++;
		list.push_back(&enemy[i]);	// 몬스터 정보 세팅 후 List에 넣음. 이걸 50번 반복.
	}

	// ---------------------------
	//	미사일 정보 세팅
	// ---------------------------
	for (int i = 0; i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT; i++)
	{
		if (i < ENEMY_MISSALE_COUNT)
			missale[i].InfoSetFunc(false);	// false면 몬스터 미사일
		else
			missale[i].InfoSetFunc(true);	// true면 플레이어 미사일

		list.push_back(&missale[i]);		// 세팅이 다 됐으면 해당 미사일을 리스트에 넣음. 이걸 100번 반복.
	}

}
// run 함수
void CSceneGame::run()
{
	if (player->GetArive())	// 유저가 살아있다면.
	{
		timeBeginPeriod(1);
		m_ulStartTimeSave = GetTickCount64();	// 공용으로 사용하기 위해, 현재 시간 저장		
		GetListData();	// 플레이어, 몬스터, 미사일을 리스트에서 가져옴		
		BufferClear();	// 백버퍼 초기화

		// -----------------------
		//		키보드 체크
		// -----------------------
		player->KeyDownCheck();

		// -----------------------
		//		    로직
		// -----------------------	
		Moverun();					// 유저 및 몬스터 이동 로직
		Atkrun();					// 유저 및 몬스터 공격 로직

		// -----------------------
		//		  랜더링
		// -----------------------	
		UIRendering();	// UI 세팅(맵 테두리, HP, 시간)
		//// 화면 그리기. 이때 UI를 제외한 모든 객체(현재는 플레이어와 몬스터)가 그려진다. ////
		for (int i = 0; i < HEIGHT; i++)
			BufferFlip(i, 0);

		UploadListData();		// 플레이어, 몬스터, 미사일을 리스트에 올림
		Sleep(50);
		timeEndPeriod(1);

		if (iEnemyAriveCount == 0)
			hSceneHandle.LoadScene(Victory);
	}
	else
		hSceneHandle.LoadScene(Lose);
}
// run 함수 안에서 호출된다. 이동 로직을 담당하는 함수
void CSceneGame::Moverun()
{
	//유저 이동 로직 (유저가 생존했을 때만 로직 실행)
	player->PlayerMoveFunc();

	// 몬스터 이동 로직
	for (int i = 0; i < ENEMY_COUNT; i++)
	{
		if (enemy[i].GetArive())	// 살아있는 몬스터는
			enemy[i].EnemyMoveFunc(&iEnemyAIState, m_ulStartTimeSave, &bAiActiveCheck);	// AI에 따라 이동
	}

	//// 몬스터 이동 AI가 작동했다면
	if (bAiActiveCheck)
	{
		// AI의 마지막 패턴까지 했으면 다시 처음 패턴을 해야하니 처음으로 돌려준다.
		if (iEnemyAIState == 3)
			iEnemyAIState = 0;
		else
			iEnemyAIState++;	// 그게 아니라면 그냥 1 증가.

								// AI 작동 여부를 본래상태(False)로 변경
		bAiActiveCheck = false;
	}

}
// run 함수 안에서 호출된다. 공격 로직을 담당하는 함수
void CSceneGame::Atkrun()
{
	//유저 공격 로직 //
	player->MissaleMoveFunc(missale, enemy, &iEnemyAriveCount);			// 미사일 이동 및 소멸처리.
	player->MissaleCreateFunc(missale);									// 새로운 미사일 생성할게 있으면 생성 처리.
	player->BonusMissaleFunc(missale, m_ulStartTimeSave);				// 보너스 미사일 생성 및 이동처리

	// 몬스터 공격 로직//
	// Enemy 공격 인큐 로직
	for (int i = 0; i < ENEMY_COUNT; i++)
	{
		// 살아있는 적군에 한해 아래 로직 실행
		if (enemy[i].GetArive())
		{
			enemy[i].EnemyEnqueueFunc(m_ulStartTimeSave);
		}
	}

	// 몬스터 모든 미사일 이동 및 소멸 처리
	enemy[0].EnemyMissaleMoveFunc(missale, player);	// 해당 로직은 CMissale에 있는게 맞는것같은데... 이미 여기에 들어가버림. 그냥 0번 하나만 호출

	// Enemy 공격 디큐 로직
	for (int i = 0; i < ENEMY_COUNT; i++)
	{
		// 살아있는 적군에 한해 아래 로직 실행
		if (enemy[i].GetArive())
		{
			enemy[i].EnemyDequeueFunc(missale);
		}
	}

}
// run 함수 안에서 호출된다. UI 세팅
void CSceneGame::UIRendering()
{
	//// 맵 테두리를 세팅한다. ////
	for (int i = 3; i < UI_HEIGHT; i++)
	{
		if (i == 3 || i == UI_HEIGHT - 1)
			for (int j = 10; j < 71; ++j)
				cBackBuffer[i][j] = '=';
		else
		{
			cBackBuffer[i][10] = 'i';
			cBackBuffer[i][70] = 'i';
		}
	}

	//// HP UI 세팅 ////
	memcpy(cBackBuffer[28] + 38, g_Hp, sizeof(g_Hp) - 1);
	memcpy(cBackBuffer[28] + 43, player->GetHP(), 2);

	//// Time UI 세팅 ////
	// 현재 시간을 받아온다. UITime 표시 체크를 위해.
	ulUITimeNow = m_ulStartTimeSave;

	// Time : 라는 문자열 복사.
	memcpy(cBackBuffer[1] + 35, g_Time, sizeof(g_Time) - 1);

	// 이전에 시간 체크한지 1초가 됐으면, 표시되는 시간 증가.
	if (ulUITimeSave + 1000 <= ulUITimeNow)
	{
		// 현재 9초라면(이제 10초가 될 차례라면)
		if (d == '9')
		{
			// 3번째 자리 체크. 
			// 3번쨰 자리가 5가 아니라면 아직 59초가 안됐다는 거니까 3번째 자리의 초를 1 증가. 그리고 4번째 자리 초를 0으로 만듦.
			if (c != '5')
			{
				c++;
				d = '0';
			}
			// 3번째 자리가 5라면 
			else
			{
				// 2분째 자리 체크
				// 3번째 자리가 5인데, 2번째 자리가 9가 아니라면, 02:59(2분 59초) 의 형태이니, 2번째 자리를 하나 증가시키고 초를 모두 0으로 만듦
				if (b != '9')
				{
					b++;
					c = '0';
					d = '0';
				}
				// 3번째 자리가 5인데, 2번째 자리가 9라면 09:59(9분 59초) 이니 첫 번째 자리를 하나 증가시키고 모두를 0으로 만듦.
				else
				{
					a++;
					b = '0';
					c = '0';
					d = '0';
				}
			}
		}
		// 자리수를 바꿀 필요가 없으면 그냥 초만 증가.
		else
			d++;

		// 다음 시간 체크타임을 알아야하니 현재 시간 저장.
		ulUITimeSave = ulUITimeNow;
	}
	Time[0] = a;
	Time[1] = b;
	Time[2] = ':';
	Time[3] = c;
	Time[4] = d;

	// Time문자열 복사. 변경됐으면 변경된 값이 복사될테고 변경된거 없으면 그냥 이전 값이 복사될것임. 버퍼 계속 비워주니 새로 복사해줘야함.
	memcpy(cBackBuffer[1] + 42, Time, sizeof(Time) - 1);

}
// run 함수 안에서 호출된다. 각종 데이터를 리스트에서 꺼내오는 절차
void CSceneGame::GetListData()
{
	// 플레이어, 몬스터, 미사일을 모두 리스트에서 가져옴
	// 플레이어를 가져옴. 가져온 후, 해당 플레이어는 지운다. 왜냐하면, 수정 후 Run 함수 마지막에 다시 등록. 
	CList<CObject*>::iterator itor1;
	for (itor1 = list.begin(); itor1 != list.end(); itor1++)
	{
		player = (CPlayer*)*itor1;
		if (player->Type == 1)
			break;
	}
	list.erase(itor1);

	// 몬스터를 가져옴
	CList<CObject*>::iterator itor2;
	int i = 0;

	for (itor2 = list.begin(); itor2 != list.end();)
	{
		if (i == ENEMY_COUNT)
			break;
		enemy[i] = (CEnemy*)(*itor2);
		if (enemy[i].Type == 2)	// 몬스터를 골라낸다.
		{
			i++;					// 몬스터라면, 다음 배열을 준비한다.
			list.erase(itor2);		// 그리고, 리스트에서 삭제한다. 왜냐하면, 수정 후 다시 등록하기 위해서.
		}
		else                    // 몬스터를 못찾았다면, itor를 ++한 후 다음 리스트를 찾아온다.
			itor2++;
	}

	// 미사일을 가져옴
	// 아군, 적군 구분 없이 모든 미사일을 꺼내온다.
	CList<CObject*>::iterator itor3;
	i = 0;

	for (itor3 = list.begin(); itor3 != list.end();)
	{
		if (i == PLAYER_MISSALE_COUNT + ENEMY_MISSALE_COUNT)
			break;
		missale[i] = (CMissale*)*itor3;
		if (missale[i].Type == 3)	// 미사일을 골라낸다.
		{
			i++;					// 미사일이라면 다음 배열을 준비한다.
			list.erase(itor3);		// 그리고, 리스트에서 삭제한다. 왜냐하면, 수정 후 다시 등록하기 위해서.
		}
		else						// 미사일을 못찾았다면, itor를 ++한 후 다음 리스트를 찾아온다.
			itor3++;
	}

}
// run 함수 안에서 호출된다. 각종 데이터를 리스트에 넣는 절차
void CSceneGame::UploadListData()
{
	// -----------------------------------------
	//	플레이어, 몬스터, 미사일을 리스트에 추가
	// -----------------------------------------
	list.push_front(player);	// 플레이어

	for (int i = 0; i < ENEMY_COUNT; i++)	// 몬스터
	{
		list.push_back(&enemy[i]);
	}

	for (int i = 0; i < PLAYER_MISSALE_COUNT + ENEMY_MISSALE_COUNT; i++)	// 미사일
	{
		list.push_back(&missale[i]);
	}

}
// 소멸자
CSceneGame::~CSceneGame()
{
	delete player;
	delete[] enemy;
	delete[] missale;
}


// --------------------------
// CSceneLose 클래스 멤버함수
// --------------------------
void CSceneLose::run()
{
	BufferClear();	// 백버퍼 초기화
	memcpy(cBackBuffer[10] + 38, LOSE_TEXT, sizeof(LOSE_TEXT) - 1);
	for (int i = 0; i < HEIGHT; i++)
		BufferFlip(i, 0);

	Sleep(1500);
	fputs("\n", stdout);
	exit(-1);	
}

// --------------------------
// CSceneVictory 클래스 멤버함수
// --------------------------

void CSceneVictory::run()
{
	BufferClear();	// 백버퍼 초기화
	memcpy(cBackBuffer[10] + 38, VICTORY_TEXT, sizeof(VICTORY_TEXT) - 1);
	for (int i = 0; i < HEIGHT; i++)
		BufferFlip(i, 0);

	Sleep(1500);
	fputs("\n", stdout);
	exit(-1);

}


// --------------------------
// CSceneHandle 클래스 멤버함수
// --------------------------
// 생성자
CSceneHandle::CSceneHandle()
{
	m_NowScene = new CSceneTitle;
	m_NowSceneText = Title;
}
// CSceneHandle::run 함수
void CSceneHandle::run()
{
	if (m_eNextScene != None)
	{
		delete m_NowScene;
		if (m_NowSceneText != Select)
		{
			CList<CObject*>::iterator itor;
			for (itor = list.begin(); itor != list.end();)
			{
				itor = list.erase(itor);
			}
		}
		
		switch (m_eNextScene)
		{
		case Title:
			m_NowScene = new CSceneTitle;
			m_NowSceneText = m_eNextScene;
			break;
		case Select:
			m_NowScene = new CSceneSelect;
			m_NowSceneText = m_eNextScene;
			break;
		case Ingame:
			m_NowScene = new CSceneGame;
			m_NowSceneText = m_eNextScene;
			break;
		case Lose:
			m_NowScene = new CSceneLose;
			m_NowSceneText = m_eNextScene;
			break;
		case Victory:
			m_NowScene = new CSceneVictory;
			m_NowSceneText = m_eNextScene;
			break;
		}
	}
	m_eNextScene = None;	
	m_NowScene->run();
}
// CSceneHandle::씬 변경 함수
void CSceneHandle::LoadScene(SceneType temp)
{
	m_eNextScene = temp;
}
// CSceneHandle::소멸자
CSceneHandle::~CSceneHandle()
{
	delete m_NowScene;
}