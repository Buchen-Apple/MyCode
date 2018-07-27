// 슈팅게임 과제
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <mmsystem.h>
#include "MoveQueue.h"
#include <conio.h>

HANDLE hConsole;

#define HEIGHT 30
#define WIDTH 81
#define UI_HEIGHT 27
#define UI_WIDTH 81
#define ENEMY_COUNT 50
#define ENEMY_MISSALE_COUNT 50
#define PLAYER_MISSALE_COUNT 50
#define ENEMY_AI_TIME 1400
#define PLAYER_AI_ATACK_TIME 2000

// 전역변수 모음
int g_iState = 0;
char cBackBuffer[HEIGHT][WIDTH];
char g_cTitle[] = "TITLE";
char g_cStart[] = "START";
char g_cExit[] = "EXIT";
char g_cTip[] = "★ Z키를 눌러주세요 ★";
char g_cCharSeletTip[] = "캐릭터를 선택해 주세요";
char g_cCharSeletChaTip_1[] = "(체력: 1  / 추가미사일: 2초에 2개)";
char g_cCharSeletChaTip_2[] = "(체력: 2  / 추가미사일: 2초에 1개)";
char g_cCharSeletChaTip_3[] = "(체력: 5  / 추가미사일:   없음   )";
char g_Hp[] = "HP : ";
char g_Time[] = "Time : ";

// 플레이어 구조체
typedef struct
{
	int m_iPosX, m_iPosY;
	bool m_bAriveCheck;
	Queue queue;
	Queue Atkqueue;
	char m_cLook;
	char m_cMissaleLook;
	char m_cHP[2];
	int m_iCharSpeed;
	int m_iBonusMIssaleCount;
	ULONGLONG m_BonusMIssaleTImeSave;

}Player;

// 적군 구조체
typedef struct
{
	int m_iPosX, m_iPosY;
	bool m_bAriveCheck;
	int m_iAttackTick;
	ULONGLONG m_AttackTimeSave;

}Enemy;

// 미사일 구조체
typedef struct
{
	int m_iPosX, m_iPosY;
	bool m_PlayerOrEnemy;
	bool m_bAriveCheck;
	bool m_BonusOrNot;
}Missale;


// 씬 넘버
enum
{
	e_Title = 0, e_ChaSelect, e_Ingame, e_Exit
};

// 콘솔 제어를 위한 준비 작업
void cs_Initial(void);

// 콘솔 화면의 커서를 x,y 좌표로 이동
void cs_MoveCursor(int iPosy, int iPosx);

// 버퍼 플립 함수
void BufferFlip(int iPosy, int iPosx);

// Title 함수
void Title(int* iArrowShowPosy, bool* bArrowShowFlag);

// 캐릭터 선택 함수
void CharSeleet(int* iArrowShowPosy, bool* bArrowShowFlag);

// 버퍼 초기화 함수
void BufferClear();

int main()
{
	system("mode con cols=83 lines=33");

	bool bArrowShowFlag = true;	//타이틀 화면에서 화살표 보여줄지 결정하는 변수. ture면 보여줌 / flase면 안보여줌. 트루펄스 반복하면서 깜빡깜빡인다.
	int iArrowShowPosy = 5;	// 타이틀 화면에서 화살표가 표시될 위치. 5면 Ingame / 6이면 Exit에 표시된다.
	int iTempPosx;	// 임시 x위치 저장
	int iTempPosy;	// 임시 y위치 저장
	int iEnemyAIState = 0; // 몬스터의 이동 패턴.
	ULONGLONG ulEnemyAiTimeSave = 0;	// Enemy의 패턴 실행 시간 체크.
	ULONGLONG ulEnemyAiTimeCur = 0;	// Enemy의 패턴 실행 시간 체크. 현재 시간 체크
	bool bAiActiveCheck = false;	// Enemy의 Ai활성화 체크.
	int iPlayerMissaleCount = 0;	// 현재 살아있는(화면에 보여지는) 플레이어의 미사일 개수 카운트
	int iEnemyAriveCount = ENEMY_COUNT;	// 살아있는 Enemy 수 체크. 몬스터가 모두 죽으면 게임 오버
	Queue MonsterAtkQueue;	// 몬스터의 공격 명령을 저장하는 큐

	Init(&MonsterAtkQueue);

	// ---------------------------
	//	 시간 UI를 위한 정보 세팅
	// ---------------------------
	char Time[6];
	Time[5] = '\0';
	char a = '0';
	char b = '0';
	char c = '0';
	char d = '0';
	ULONGLONG ulUITimeSave = 0;	// UI에 표시되는 시간을 체크하기 위한 타이머.

	// -----------------------
	//	  플레이어 정보 세팅
	// -----------------------
	// 플레이어 구조체 세팅
	Player player;	
	player.m_iPosX = 40;
	player.m_iPosY = 20;
	player.m_bAriveCheck = true;

	// 플레이어 행동 로직 큐, 공격 로직 큐 초기화
	Init(&player.queue);
	Init(&player.Atkqueue);

	// -----------------------
	//		적군 정보 세팅
	// -----------------------
	// 적군 구조체 배열 세팅. 적은 50마리이다! (ENEMY_COUNT가 50)
	Enemy enemy[ENEMY_COUNT]; 
	int iEnemyPosx = 17;
	int iEnemyPosy = 5;
	iTempPosx = 1400;
	iTempPosy = 1000;
	iArrowShowPosy = 0;
	for (int i = 0; i < ENEMY_COUNT; ++i)
	{
		enemy[i].m_iPosX = iEnemyPosx;
		enemy[i].m_iPosY = iEnemyPosy;
		enemy[i].m_iAttackTick = iTempPosx;
		enemy[i].m_bAriveCheck = true;
		
		// 아래 if문은 적군의 공격 AI 틱 지정
		if (iArrowShowPosy == 4)
		{
			iTempPosx = iTempPosy;
			iArrowShowPosy = 0;
			iTempPosy += 600;
		}
		else
		{
			iTempPosx += 1500;
			iArrowShowPosy++;
		}

		// 아래 if문은 적군의 최초 스폰 위치 지정
		if (iEnemyPosx > 60)
		{
			iEnemyPosx = 17;
			iEnemyPosy += 1;
		}
		else
			iEnemyPosx += 5;
	}

	iArrowShowPosy = 5;	//잠깐 썻던 변수 다시 원복..

	// -----------------------
	//	  미사일 구조체 세팅
	// -----------------------
	//배열 0 ~ 49는 적군 것. 배열 50 ~ 99번은 플레이어의 것
	Missale missale[ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT];

	// 미사일의 정보를 아군/적군것으로 세팅. 트루면 아군 / 펄스면 적군
	for (int i = 0; i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT; ++i)	
	{
		if (i < ENEMY_MISSALE_COUNT)
			missale[i].m_PlayerOrEnemy = false;	// 펄스면 적군
		else
			missale[i].m_PlayerOrEnemy = true;	// 트루면 아군
	}

	// 미사일의 생존 여부 설정. 처음 시작때는 모두 비활성화상태.
	for (int i = 0; i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT; ++i)
		missale[i].m_bAriveCheck = false;


	// 화면의 커서 안보이게 처리 및 핸들 선언
	cs_Initial();
		
	timeBeginPeriod(1);
	while (1)
	{
		switch (g_iState)
		{
		case e_Title:
			// 백버퍼 초기화
			BufferClear();

			// 타이틀 창 표시 함수 호출
			Title(&iArrowShowPosy, &bArrowShowFlag);
			Sleep(200);

			// 깜빡깜빡 하기 위한 절차
			if (bArrowShowFlag == true)
				bArrowShowFlag = false;
			else
				bArrowShowFlag = true;
			

			// 유저가 Z키를 눌렀을 때, 현재 화살표가 어딜 가리키고 있는지에 따라 다음 씬이 인게임인지 EXIT 인지 결정
			if (GetAsyncKeyState(0x5A) & 0x8001)
				if (iArrowShowPosy == 5)
				{
					g_iState = e_ChaSelect;
					break;
				}
				else if(iArrowShowPosy == 6)
				{
					g_iState = e_Exit;
					break;
				}
			// 유저가 키보드 위/아래를 선택하면, 선택한 버튼에 따라 iArrowShowPosy의 값을 바꿔줌. 즉, 화살표가 위 아래중 어디를 가리킬지 결정
			if (GetAsyncKeyState(VK_DOWN) & 0x8001)
				iArrowShowPosy = 6;
			if(GetAsyncKeyState(VK_UP) & 0x8001)
				iArrowShowPosy = 5;

			break;
		case e_ChaSelect:
			// 백버퍼 초기화
			BufferClear();

			// 캐릭터 선택창 표시 함수 호출
			CharSeleet(&iArrowShowPosy, &bArrowShowFlag);
			Sleep(200);

			// 깜빡깜빡 하기 위한 절차
			if (bArrowShowFlag == true)
				bArrowShowFlag = false;
			else
				bArrowShowFlag = true;

			// 유저가 Z키를 눌렀을 때, 현재 화살표가 어딜 가리키고 있는지에 따라 캐릭터 정보 세팅
			if (GetAsyncKeyState(0x5A) & 0x8001)
			{
				if (iArrowShowPosy == 5)
				{
					player.m_cLook = 'A';
					player.m_cMissaleLook = '1';
					player.m_cHP[0] = '0';
					player.m_cHP[1] = '1';
					player.m_iCharSpeed = 1;
					player.m_iBonusMIssaleCount = 2;
				}
				else if (iArrowShowPosy == 7)
				{
					player.m_cLook = 'B';
					player.m_cMissaleLook = '2';
					player.m_cHP[0] = '0';
					player.m_cHP[1] = '2';
					player.m_iCharSpeed = 1;
					player.m_iBonusMIssaleCount = 1;
				}
				else if (iArrowShowPosy == 9)
				{
					player.m_cLook = 'V';
					player.m_cMissaleLook = '3';
					player.m_cHP[0] = '0';
					player.m_cHP[1] = '5';
					player.m_iCharSpeed = 1;
					player.m_iBonusMIssaleCount = 0;
				}

				g_iState = e_Ingame;
				break;
			}

			// 유저가 키보드 위/아래를 선택하면, 선택한 버튼에 따라 iArrowShowPosy의 값을 바꿔줌. 즉, 화살표가 위, 중간, 아래 중 어디를 가리킬지 결정
			if (GetAsyncKeyState(VK_DOWN) & 0x8001)
			{
				if(iArrowShowPosy != 9)
					iArrowShowPosy += 2;
			}
			if (GetAsyncKeyState(VK_UP) & 0x8001)
			{
				if (iArrowShowPosy != 5)
					iArrowShowPosy -= 2;
			}				

			break;
		case e_Ingame:	
			// 화면 입장 시, 혹시 모르니, player를 생존상태로 만든다.
			player.m_bAriveCheck = true;

			// 혹시 모르니 모든 적을 생존상태로 만든다.
			for (int i = 0; i < ENEMY_COUNT; ++i)
				enemy[i].m_bAriveCheck = true;

			// 인 게임 시작 부터 시간 체크. 몬스터 AI 작동을 위한 시간 체크
			ulEnemyAiTimeSave = GetTickCount64();

			// 인 게임을 시작하면, 현재 시간 저장. 몬스터의 공격 AI 작동을 위한 시간 체크
			for (int i = 0; i < ENEMY_COUNT; ++i)
				enemy[i].m_AttackTimeSave = ulEnemyAiTimeSave;

			// 인 게임을 시작하면, 현재 시간 저장. 아군 캐릭터의 추가 미사일 발사를 위해.
			player.m_BonusMIssaleTImeSave = ulEnemyAiTimeSave;

			// 인 게임을 시작하면, 현재 시간 저장. UI에 시간표시를 위해
			ulUITimeSave = ulEnemyAiTimeSave;

			while (1)
			{
				// 백버퍼 초기화
				BufferClear();

				// -----------------------
				//		키보드 체크
				// -----------------------
				//유저가 생존했을 때만 키보드 체크
				if (player.m_bAriveCheck)	
				{
					// 유저가 누른 키를 체크해, 행동 로직을 큐로 저장. 저장한 행동 로직은 '로직'에서 작동
					if (GetAsyncKeyState(VK_UP) & 0x8001)
					{
						if(player.m_iPosY -1 > (UI_HEIGHT /2))
							Enqueue(&player.queue, player.m_iPosX, player.m_iPosY - player.m_iCharSpeed);
					}
					if (GetAsyncKeyState(VK_DOWN) & 0x8001)
					{
						if(player.m_iPosY +1 < UI_HEIGHT -1)
							Enqueue(&player.queue, player.m_iPosX, player.m_iPosY + player.m_iCharSpeed);
					}
					if (GetAsyncKeyState(VK_RIGHT) & 0x8001)
					{
						if(player.m_iPosX +1 < 69)
							Enqueue(&player.queue, player.m_iPosX + player.m_iCharSpeed, player.m_iPosY);
					}
					if (GetAsyncKeyState(VK_LEFT) & 0x8001)
					{
						if(player.m_iPosX -1 > 11)
							Enqueue(&player.queue, player.m_iPosX - player.m_iCharSpeed, player.m_iPosY);
					}

					// 유저가 공격 버튼을 눌렀으면,공격 로직을 큐에 저장. 저장한 공격 로직은 '로직'에서 작동
					if (GetAsyncKeyState(0x5A) & 0x8001)
					{
						if (iPlayerMissaleCount < PLAYER_MISSALE_COUNT)
						{
							iPlayerMissaleCount++;
							Enqueue(&player.Atkqueue, player.m_iPosX, player.m_iPosY - 1);							
						}
					}
				}

				// -----------------------
				//		    로직
				// -----------------------				

				////유저 이동 로직 (유저가 생존했을 때만 로직 실행) ////
				if (player.m_bAriveCheck)
				{
					// 현재 유저의 위치를 버퍼에 저장한다.
					cBackBuffer[player.m_iPosY][player.m_iPosX] = player.m_cLook;

					// 현재 유저의 x,y 위치를 저장한다.
					iTempPosx = player.m_iPosX;
					iTempPosy = player.m_iPosY;

					// 행동 로직을 받아서(큐), 이동할 위치가 있으면 그 위치로 이동시킨다.
					while (Dequeue(&player.queue, &player.m_iPosX, &player.m_iPosY))
					{
						cBackBuffer[iTempPosy][iTempPosx] = ' ';	//디큐가 성공하면, 유저가 기존에 있던 위치는 스페이스로 변경한다.
						cBackBuffer[player.m_iPosY][player.m_iPosX] = player.m_cLook;

						iTempPosx = player.m_iPosX;	// 다음 while문때도 디큐가 성공할 수 있으니, 디큐 후의 위치도 저장한다.
						iTempPosy = player.m_iPosY;
					}
				}

				//// Enemy 이동 AI 로직 (현재 생존한 Enemy를 기준으로 로직 실행)	////
				// 로직 실행 전, 현재 시간을 받아온다. 1.4초(ENEMY_AI_TIME)마다 살아있는 적은 AI 패턴에 따라 움직인다.
				ulEnemyAiTimeCur = GetTickCount64();

				for (int i = 0; i < ENEMY_COUNT; ++i)
				{
					//  살아있는 적군에 한해 아래 로직 실행
					if (enemy[i].m_bAriveCheck == true)
					{
						// 이전 AI가 작동한지 1.4초(ENEMY_AI_TIME)가 된 경우
						if (ulEnemyAiTimeSave + ENEMY_AI_TIME <= ulEnemyAiTimeCur)
						{
							if (iEnemyAIState == 0)
								enemy[i].m_iPosX -= 2;
							else if (iEnemyAIState == 1)
								enemy[i].m_iPosY--;
							else if (iEnemyAIState == 2)
								enemy[i].m_iPosX += 2;
							else if (iEnemyAIState == 3)
								enemy[i].m_iPosY++;

							// AI가 작동했다는 플래그 on. 마지막 적군 or 첫 적군 등 지정해서 체크하면, 해당 적이 죽었을 때 에러가 발생하니 그냥 발생할 때 마다 플레그 온
							bAiActiveCheck = true;
						}
						cBackBuffer[enemy[i].m_iPosY][enemy[i].m_iPosX] = 'M';
					}
				}

				// Enemy 이동 AI가 작동했다면
				if (bAiActiveCheck)
				{
					// 시간을 새로 저장한다. 그래야 다음 AI를 작동시킬 기준이 되기 때문에.
					ulEnemyAiTimeSave = GetTickCount64();

					// AI의 마지막 패턴까지 했으면 다시 처음 패턴을 해야하니 처음으로 돌려준다.
					if (iEnemyAIState == 3)
						iEnemyAIState = 0;
					else
						iEnemyAIState++;

					// AI 작동 여부를 본래상태(False)로 변경
					bAiActiveCheck = false;
				}				

				//// 유저의 미사일 이동 및 소멸 로직 (유저가 생존했을 때만 로직 실행)////		
				if (player.m_bAriveCheck)
				{
					// 살아있는 모든 미사일을 -y쪽으로 1칸 이동 후 버퍼에 저장.		
					for (int i = ENEMY_MISSALE_COUNT; i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT; ++i)
					{
						if (missale[i].m_bAriveCheck == true)
						{
							missale[i].m_iPosY--;

							// -y쪽으로 1칸 이동했는데, 그게 벽이면 false로 만듦.
							if (missale[i].m_iPosY == 3)
							{
								missale[i].m_bAriveCheck = false;
								iPlayerMissaleCount--;
							}

							// -y쪽으로 1칸 이동했는데, 그게 몬스터면 false로 만들고 해당 위치의 몬스터를 false로 만듦(사망 상태)
							// flase가 된 몬스터는 자동으로 버퍼에서 제외된다.
							else if (cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] == 'M')
							{
								int j = 0;
								for (; j < ENEMY_COUNT; ++j)
								{
									if (enemy[j].m_iPosY == missale[i].m_iPosY && enemy[j].m_iPosX == missale[i].m_iPosX)
									{
										// 해당 몬스터가 살아있는 몬스터라면, (혹시 모르니 여기서 생존여부 다시한번 체크. 자꾸 이상한게 1~2마리 남았는데 게임이 종료됨..)
										if (enemy[j].m_bAriveCheck == true)
										{

											enemy[j].m_bAriveCheck = false;
											iEnemyAriveCount--;	// 생존 중인 Enemy 수 1개체 줄임
											if (iEnemyAriveCount == 0)
												cBackBuffer[enemy[j].m_iPosY][enemy[j].m_iPosX] = ' ';	// 적이 0명이 되면, 마지막 적을 스페이스로 만든다. 그래야 화면에는 사라진 것 처럼 나옴.

											missale[i].m_bAriveCheck = false;
											iPlayerMissaleCount--;	// 생존 중인 미사일 수 1개 줄임		
											break;
										}
									}
								}								

							}
							else
							{
								if(missale[i].m_BonusOrNot == false)
									cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = player.m_cMissaleLook;
								else
									cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'b';
							}
						}
					}
				}

				//// 유저 공격 로직 ////
				// 로직 실행 전, 시간을 받아온다.
				ulEnemyAiTimeCur = GetTickCount64();

				if (player.m_bAriveCheck)
				{
					int i = PLAYER_MISSALE_COUNT;
					// 공격 로직을 받아서(큐), 해당 좌표의 미사일을 활성화시킨다.
					while (Dequeue(&player.Atkqueue, &iTempPosx, &iTempPosy))
					{						
						// 값이 false인 아군 미사일을 찾는다.
						while (missale[i].m_bAriveCheck)
						{
							// 만약, i가 미사일의 범위를 벗어났으면 break;
							if (i == ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT)
								break;

							// 그게 아니라면 다음 배열 검사.
							i++;
						}

						// i의 값이, ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT보다 작으면 false인 배열을 찾았다는 것이니 아래 로직 진행
						if (i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT)
						{
							// 찾은 위치의 미사일을 true로 만들고, 찾은 위치의 x,y값을 설정한다.
							missale[i].m_bAriveCheck = true;
							missale[i].m_BonusOrNot = false;	//보너스 외형을 체크하기 위한것. 보너스면 true / 보너스 미사일이 아니면 false
							missale[i].m_iPosX = iTempPosx;
							missale[i].m_iPosY = iTempPosy;

							// 그리고, 생성된 미사일을 버퍼에 저장한다.
							cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = player.m_cMissaleLook;
						}
					}

					
					// 유저의 공격 입력과는 상관 없이, 일정 시간마다 보너스 미사일이 발사된다.
					// 만약, 공격할 때가 됐다면,
					if (player.m_BonusMIssaleTImeSave + PLAYER_AI_ATACK_TIME <= ulEnemyAiTimeCur)
					{
						i = PLAYER_MISSALE_COUNT;
						
						// 현재 플레이어가 가지고 있는 추가 미사일 발사 카운트만큼 반복하며 미사일 생성
						// 추가 미사일 발사 카운트가 0이라면, 반복하지 않는다.
						for (int h = 1; h <= player.m_iBonusMIssaleCount; ++h)
						{
							// 값이 false인 아군 미사일을 찾는다.
							while (missale[i].m_bAriveCheck)
							{
								// 만약, i가 미사일의 범위를 벗어났으면 break;
								if (i == ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT)
									break;

								// 그게 아니라면 다음 배열 검사.
								i++;
							}
							// i의 값이, ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT보다 작으면 false인 배열을 찾았다는 것이니 아래 로직 진행
							if (i < ENEMY_MISSALE_COUNT + PLAYER_MISSALE_COUNT)
							{
								// 찾은 위치의 미사일을 true로 만들고, 미사일을 생성한다.
								missale[i].m_bAriveCheck = true;
								missale[i].m_BonusOrNot = true;
								if (h == 1)
								{
									missale[i].m_iPosX = player.m_iPosX - 2;
									missale[i].m_iPosY = player.m_iPosY - 1;
									cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'b';
								}
								else if (h == 2)
								{
									missale[i].m_iPosX = player.m_iPosX + 2;
									missale[i].m_iPosY = player.m_iPosY - 1;
									cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'b';
								}								
							}
						}
						// 미사일을 모두 생성했으면, 현재 시간을 저장한다. 그래야 다음에 또 추가 미사일을 발사하니까..
						player.m_BonusMIssaleTImeSave = ulEnemyAiTimeCur;
					}					
				}				

				//// Enemy 공격 AI 로직 (현재 생존한 Enemy를 기준으로 로직 실행). 적 미사일을 생성한다.	////
				// 로직 실행 전, 시간을 받아온다.
				ulEnemyAiTimeCur = GetTickCount64();
				
				// Enemy 공격 큐 넣기 로직
				for (int i = 0; i < ENEMY_COUNT; ++i)
				{
					// 살아있는 적군에 한해 아래 로직 실행
					if (enemy[i].m_bAriveCheck == true)
					{
						// 해당 적군이 공격할 때가 됐으면, 공격 명령을 큐로 보냄
						if (ulEnemyAiTimeCur >= enemy[i].m_AttackTimeSave + enemy[i].m_iAttackTick)
						{
							Enqueue(&MonsterAtkQueue, enemy[i].m_iPosX, enemy[i].m_iPosY + 1);
							enemy[i].m_AttackTimeSave = ulEnemyAiTimeCur;	// 적군의 구조체 변수에 현재 시간 저장.
						}
					}
				}


				//// Enemy 미사일 이동 및 소멸 로직 (몬스터 생존과 상관 없이 로직 실행)	////
				// 살아있는 모든 미사일을 -y쪽으로 1칸 이동 후 버퍼에 저장.		
				for (int i = 0; i < ENEMY_MISSALE_COUNT; ++i)
				{
					if (missale[i].m_bAriveCheck == true)
					{
						missale[i].m_iPosY++;

						// +y쪽으로 1칸 이동했는데, 그게 벽이면 false로 만듦.
						if (missale[i].m_iPosY == UI_HEIGHT - 1)
							missale[i].m_bAriveCheck = false;

						// +y쪽으로 1칸 이동했는데, 그게 유저면.  
						else if (cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] == player.m_cLook)
						{
							// 유저의 hp를 1 깍는다. 1깍을 때, HP 수치를 보여주는 문자열을 체크해 체력을 감소시킨다.
							if (player.m_cHP[0] != '0')
							{
								player.m_cHP[0]--;
								if (player.m_cHP[0] == '0')
									player.m_cHP[1] = '9';
							}
							else if (player.m_cHP[1] != '0')
								player.m_cHP[1]--;
							
							// 유저의 hp가 0이라면 유저의 해당 위치의 유저를 false로 만듦(사망 상태)
							if (player.m_cHP[1] == '0')
								player.m_bAriveCheck = false;

							// 사망과는 상관 없이, 유저가 공격받으면 유저 위치를 스페이스바로 바꾼다. 이는, 공격 받을 때 깜빡이기 or 사망하면 안보이게 하기의 역활이다.
							cBackBuffer[player.m_iPosY][player.m_iPosX] = ' ';
						}

						else
							cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'v';
					}
				}		


				//// Enemy 공격 큐 빼오기 로직 ////
				// 공격 로직을 받아서(큐), 해당 좌표의 미사일을 활성화시킨다. 
				// 유저가 생존했을 때만 사용
				if (player.m_bAriveCheck)
				{
					while (Dequeue(&MonsterAtkQueue, &iTempPosx, &iTempPosy))
					{
						int i = 0;
						// 값이 false인 적군 미사일을 찾는다.
						while (missale[i].m_bAriveCheck)
						{
							// 만약, i가 미사일의 범위를 벗어났으면 break;
							if (i == ENEMY_MISSALE_COUNT)
								break;

							// 그게 아니라면 다음 배열 검사.
							i++;
						}

						// i의 값이, ENEMY_MISSALE_COUNT보다 작으면 false인 배열을 찾았다는 것이니 아래 로직 진행
						if (i < ENEMY_MISSALE_COUNT)
						{
							// 찾은 위치의 미사일을 true로 만들고, 찾은 위치의 x,y값을 설정한다.
							missale[i].m_bAriveCheck = true;
							missale[i].m_iPosX = iTempPosx;
							missale[i].m_iPosY = iTempPosy;

							// 그리고, 생성된 미사일을 버퍼에 저장한다.
							cBackBuffer[missale[i].m_iPosY][missale[i].m_iPosX] = 'v';
						}
					}
				}

				// -----------------------
				//		   랜더링
				// -----------------------	
				//// 맵 테두리를 세팅한다. ////
				for (int i = 3; i < UI_HEIGHT; ++i)
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

				//// UI 세팅 ////
				// HP UI 세팅 //
				memcpy(cBackBuffer[28] + 38, g_Hp, sizeof(g_Hp) - 1);
				memcpy(cBackBuffer[28] + 43, player.m_cHP, sizeof(player.m_cHP));

				// Time UI 세팅 //
				// Time : 라는 문자열 복사.
				memcpy(cBackBuffer[1] + 35, g_Time, sizeof(g_Time) - 1);

				// 현재 시간을 받아온다. UITime 표시 체크를 위해.
				ulEnemyAiTimeCur = GetTickCount64();
				
				// 이전에 시간 체크한지 1초가 됐으면, 표시되는 시간 증가.
				if (ulUITimeSave + 1000 <= ulEnemyAiTimeCur)
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
					ulUITimeSave = ulEnemyAiTimeCur;
				}

				Time[0] = a;
				Time[1] = b;
				Time[2] = ':';
				Time[3] = c;
				Time[4] = d;

				// Time문자열 복사. 변경됐으면 변경된 값이 복사될테고 변경된거 없으면 그냥 이전 값이 복사될것임. 버퍼 계속 비워주니 새로 복사해줘야함.
				memcpy(cBackBuffer[1] + 42, Time, sizeof(Time) - 1);

				//// 화면 그리기 ////
				for (int i = 0; i< HEIGHT; ++i)
					BufferFlip(i, 0);

				Sleep(50);

				// -----------------------
				//	  게임 종료 체크
				// -----------------------	
				// 적이 0명이거나 아군이 사망하면 게임 종료 게임 종료
				if (iEnemyAriveCount == 0 || player.m_bAriveCheck == false)
				{
					fputs("\n게임 끝!.", stdout);
					Sleep(1500);					

					g_iState = e_Exit;
					break;
				}
			}
			break;
		case e_Exit:
			fputs("\n", stdout);
			exit(1);
			break;
		}
	}
	timeEndPeriod(1);
	return 0;
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

// 버퍼 플립 함수
void BufferFlip(int iPosy, int iPosx)
{
	cs_MoveCursor(iPosy, iPosx);
	printf("%s", cBackBuffer[iPosy]);
}
// Title 함수
void Title(int* iArrowShowPosy, bool* bArrowShowFlag)
{

	memcpy(cBackBuffer[3] + 38, g_cTitle, sizeof(g_cTitle) - 1);
	memcpy(cBackBuffer[5] + 38, g_cStart, sizeof(g_cStart) - 1);
	memcpy(cBackBuffer[6] + 38, g_cExit, sizeof(g_cExit) - 1);

	memcpy(cBackBuffer[8] + 30, g_cTip, sizeof(g_cTip) - 1);

	// iArrowShowPosy의 값에 따라 기존 화살표는 지워줌
	if ((*iArrowShowPosy) == 5)
	{
		cBackBuffer[6][35] = ' ';
		cBackBuffer[6][36] = ' ';
	}
	else if ((*iArrowShowPosy) == 6)
	{
		cBackBuffer[5][35] = ' ';
		cBackBuffer[5][36] = ' ';
	}

	// 화살표 깜빡깜빡 절차
	if ((*bArrowShowFlag) == true)
	{
		cBackBuffer[(*iArrowShowPosy)][35] = '-';
		cBackBuffer[(*iArrowShowPosy)][36] = '>';
	}
	else
	{
		cBackBuffer[(*iArrowShowPosy)][35] = ' ';
		cBackBuffer[(*iArrowShowPosy)][36] = ' ';
	}


	for (int i = 0; i<HEIGHT; ++i)
		BufferFlip(i, 0);
}
// 캐릭터 선택 함수
void CharSeleet(int* iArrowShowPosy, bool* bArrowShowFlag)
{
	memcpy(cBackBuffer[3] + 30, g_cCharSeletTip, sizeof(g_cCharSeletTip) - 1);

	cBackBuffer[5][23] = 'A';
	memcpy(cBackBuffer[5] + 25, g_cCharSeletChaTip_1, sizeof(g_cCharSeletChaTip_1) - 1);

	cBackBuffer[7][23] = 'B';
	memcpy(cBackBuffer[7] + 25, g_cCharSeletChaTip_2, sizeof(g_cCharSeletChaTip_2) - 1);

	cBackBuffer[9][23] = 'V';
	memcpy(cBackBuffer[9] + 25, g_cCharSeletChaTip_3, sizeof(g_cCharSeletChaTip_3) - 1);

	memcpy(cBackBuffer[11] + 30, g_cTip, sizeof(g_cTip) - 1);

	// iArrowShowPosy의 값에 따라 기존 화살표는 지워줌
	if ((*iArrowShowPosy) == 5)
	{
		cBackBuffer[7][20] = ' ';
		cBackBuffer[7][21] = ' ';

		cBackBuffer[9][20] = ' ';
		cBackBuffer[9][21] = ' ';
	}
	else if ((*iArrowShowPosy) == 7)
	{
		cBackBuffer[5][20] = ' ';
		cBackBuffer[5][21] = ' ';

		cBackBuffer[9][20] = ' ';
		cBackBuffer[9][21] = ' ';
	}
	else if ((*iArrowShowPosy) == 9)
	{
		cBackBuffer[5][20] = ' ';
		cBackBuffer[5][21] = ' ';

		cBackBuffer[7][20] = ' ';
		cBackBuffer[7][21] = ' ';
	}

	// 화살표 깜빡깜빡 절차
	if ((*bArrowShowFlag) == true)
	{
		cBackBuffer[(*iArrowShowPosy)][20] = '-';
		cBackBuffer[(*iArrowShowPosy)][21] = '>';
	}
	else
	{
		cBackBuffer[(*iArrowShowPosy)][20] = ' ';
		cBackBuffer[(*iArrowShowPosy)][21] = ' ';
	}

	for (int i = 0; i<HEIGHT; ++i)
		BufferFlip(i, 0);
}
// 버퍼 초기화 함수
void BufferClear()
{
	memset(cBackBuffer, 0X20, HEIGHT*WIDTH);
	for (int i = 0; i < HEIGHT; ++i)
		cBackBuffer[i][WIDTH - 1] = '\0';
}