#pragma once
#ifndef __SCENE_CLASS_H__
#define __SCENE_CLASS_H__

#include <Windows.h>
#define ENEMY_COUNT	50
#define ENEMY_MISSALE_COUNT 50
#define PLAYER_MISSALE_COUNT 50

// 콘솔 제어를 위한 준비 작업
void cs_Initial(void);
// 콘솔 화면의 커서를 x,y 좌표로 이동
void cs_MoveCursor(int iPosy, int iPosx);

// 전방선언
enum SceneType;	// 씬 enum
class Queue;	// MoveQueue.h의 Queue 클래스
class CSceneHandle;	// CSceneHandle 클래스
class CPlayer;		// CPlayer 전방 선언


// CObject 클래스 선언
class CObject
{	
public:
	int Type;	// 0 : 아직 타입 부여 안됨 1 : 플레이어 / 2 : 몬스터 / 3 : 미사일
	CObject() : Type(0) {}	// 생성자
	
	virtual void InterfaceFunc() {};

};

class CMissale :public CObject
{
	int m_iPosX, m_iPosY;
	bool m_PlayerOrEnemy;
	bool m_bAriveCheck;
	bool m_BonusOrNot;
	friend class CPlayer;
	friend class CEnemy;
public:
	CMissale(); // 생성자
	CMissale& operator=(const CMissale*);	// 대입 연산자
	void InfoSetFunc(bool);	// 미사일 정보 세팅
};


// CEnemy 클래스 선언
class CEnemy :public CObject
{
	friend class CPlayer;
	int m_iPosX, m_iPosY;
	bool m_bAriveCheck;
	int m_iAttackTick;
	ULONGLONG m_MoveTimeSave;
	ULONGLONG m_AttackTimeSave;
	Queue* EnemyAtkQueue;
	
	const int ENEMY_AI_TIME = 1400;	// 몬스터 AI작동 틱. 1400(1.4초)마다 1회씩 AI작동
public:
	CEnemy();	// 생성자
	CEnemy& operator=(const CEnemy*);								// 대입 연산자
	void InfoSetFunc(int*, int*, int*, int*, int*, ULONGLONG);		// 몬스터 정보 세팅
	void EnemyMoveFunc(int*, ULONGLONG, bool*);						// 몬스터 AI 이동 처리
	bool GetArive();												// 해당 몬스터의 생존여부 리턴
	void EnemyEnqueueFunc(ULONGLONG);								// 해당 몬스터의 공격 인큐
	void EnemyMissaleMoveFunc(CMissale*, CPlayer*);				// 몬스터 미사일 이동 및 소멸 처리 함수
	void EnemyDequeueFunc(CMissale*);										// 해당 몬스터의 공격 디큐
	~CEnemy();														// 소멸자
};

// CPlayer 클래스 선언
class CPlayer	:public CObject
{
	friend class CEnemy;
private:
	int m_iPosX, m_iPosY;
	bool m_bAriveCheck;
	Queue* m_MoveQueue;
	Queue* m_AtkQueue;
	char m_cLook;							// 캐릭터 모양
	char m_cMissaleLook;					// 미사일 모양
	char m_cHP[2];
	int m_iCharSpeed;
	int m_iBonusMIssaleCount;				// 한 번에 발사되는 보너스 미사일 갯수. 캐릭터에 따라 다르다.
	ULONGLONG m_BonusMIssaleTImeSave;		// 미사일 발사 시작 시간 저장. 
	int iPlayerMissaleCount;				// 현재 보유한 미사일 수

	const int UI_HEIGHT = 27;
	const int UI_WIDTH = 81;
	const int PLAYER_AI_ATACK_TIME = 2000;	// 2000(2초)마다 1회씩 보너스 미사일 발사. 보너스 미사일이 있는 경우!

public:	
	CPlayer();
	void InfoSetFunc_1(int);				// 플레이어 정보 세팅. CSceneSelect에서 기본 정보 세팅
	void InfoSetFunc_2();					// 플레이어 정보 세팅. CSceneGame에서 좀 더 자세히 정보 추가세팅
	void KeyDownCheck();					// 플레이어 이동 / 공격 여부 체크
	char* GetHP();							// 플레이어의 현재 Hp를 얻어내는 함수
	bool GetArive();						// 플레이어의 생존여부를 얻어내는 함수
	void PlayerMoveFunc();					// 플레이어 이동 로직처리 함수
	void MissaleMoveFunc(CMissale*, CEnemy*, int*);					// 미사일 이동 및 소멸 처리 함수
	void MissaleCreateFunc(CMissale*);							// 미사일 생성 처리 함수
	void BonusMissaleFunc(CMissale*, ULONGLONG);				// 보너스 미사일 생성 처리. 이동은 MissaleMoveFunc에서 같이 처리
	~CPlayer();													// 소멸자
};



// CSceneBase 클래스 선언
class CSceneBase
{	
protected:
	// 버퍼 초기화 함수
	void BufferClear();
	// 버퍼 플립 함수
	void BufferFlip(int iPosy, int iPosx);

public:
	virtual void run() = 0;
	virtual ~CSceneBase() {};
	
};

// CSceneTitle 클래스 선언
class CSceneTitle :public CSceneBase
{
public:	
	void Title();			// Title 함수	
	virtual void run();		// run 함수	

};

// CSceneSelect 클래스 선언
class CSceneSelect :public CSceneBase
{	
private:
	// 키보드 체크 함수
	void KeyDown();
	// 캐릭터 선택 함수
	void CharSeleet();

public:	
	virtual void run();				// run 함수
	
};

// CSceneGame 클래스 선언
class CSceneGame :public CSceneBase
{
private:
	const char g_Hp[6] = "HP : ";
	const char g_Time[8] = "Time : ";

private:
	CPlayer* player;
	CEnemy* enemy;
	CMissale* missale;
	ULONGLONG m_ulStartTimeSave;									// 게임 시작 시점의 시간 저장. 여기저기 잘 사용됨.
	ULONGLONG ulUITimeSave;											// UI에 표시되는 시간을 체크하기 위한 타이머.
	ULONGLONG ulUITimeNow;											// UI에 표시되는 시간을 체크하기 위한 타이머2. cur 시간 저장
	bool bAiActiveCheck;											// 몬스터의 AI작동 여부 체크.
	int iEnemyAIState;												// 이동 AI의 순서 지정. 1일때는 좌로 이동 등등..
	int iEnemyAriveCount;											// 생존 중인 몬스터의 수. 기본은 50(모두 생존)

	char Time[6];		// UI 시간표시를 위한 변수
	char a, b, c, d;	// UI 시간표시를 위한 변수들

public:
	CSceneGame();				// 생성자
	virtual void run();			// run 함수
	void Moverun();	// run 함수 안에서 호출된다. 이동 로직을 담당하는 함수
	void Atkrun();				// run 함수 안에서 호출된다. 이동 로직을 담당하는 함수
	void UIRendering();		// run 함수 안에서 호출된다. 타임 ui를 랜더링하는 함수
	void GetListData();			// run 함수 안에서 호출된다. 각종 데이터를 리스트에서 꺼내오는 절차
	void UploadListData();		// run 함수 안에서 호출된다. 각종 데이터를 리스트에 넣는 절차
	~CSceneGame();				// 소멸자
};

class CSceneLose :public CSceneBase
{
	const char LOSE_TEXT[13] = "Game Over...";
public:
	virtual void run();		//run 함수
	
};

class CSceneVictory :public CSceneBase
{
	const char VICTORY_TEXT[13] = "Victory!!...";
public:
	virtual void run();		//run 함수

};

// CSceneHandle 클래스 선언
class CSceneHandle
{
private:
	CSceneBase * m_NowScene;
	SceneType m_eNextScene;
	SceneType m_NowSceneText;

public:	
	CSceneHandle();	// 생성자	
	void run();		// run 함수						
	void LoadScene(SceneType temp); // CSceneHandle::씬 변경 함수
	~CSceneHandle();	// 소멸자
};


extern CSceneHandle hSceneHandle;

//#undef ENEMY_COUNT

#endif // !__SCENE_CLASS_H__

