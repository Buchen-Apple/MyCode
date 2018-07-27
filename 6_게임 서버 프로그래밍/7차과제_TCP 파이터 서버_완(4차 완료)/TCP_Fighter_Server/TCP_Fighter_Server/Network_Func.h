#pragma once

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>
#include "Protocol.h"
#include "ProtocolBuff\ProtocolBuff.h"
#include "RingBuff\RingBuff.h"
//#include "profiling\Profiling_Class.h"

// Profiling_Class.h가 선언되어 있지 않다면, 아래 매크로들을 공백으로 만듦. 
#ifndef __PROFILING_CLASS_H__
#define BEGIN(STR) 
#define END(STR)
#define FREQUENCY_SET()
#define PROFILING_SHOW()
#define PROFILING_FILE_SAVE()
#define RESET()
#else
#define BEGIN(STR)				BEGIN(STR)
#define END(STR)				END(STR)
#define FREQUENCY_SET()			FREQUENCY_SET()
#define PROFILING_SHOW()		PROFILING_SHOW()
#define PROFILING_FILE_SAVE()	PROFILING_FILE_SAVE()
#define RESET()					RESET()

#endif // !__PROFILING_CLASS_H__

using namespace Library_Jingyu;

// 구조체 전방선언
struct stAccount;
struct stPlayer;
struct stSectorCheck;

// 로그 레벨
#define dfLOG_LEVEL_DEBUG		0
#define dfLOG_LEVEL_WARNING		1
#define dfLOG_LEVEL_ERROR		2

// 로그 매크로 포멧
#define _LOG(LogLevel, fmt, ...)						\
do{														\
	if(g_LogLevel <= LogLevel)							\
	{													\
		swprintf_s(g_szLogBuff, fmt, ## __VA_ARGS__);	\
		Log(g_szLogBuff, LogLevel);						\
	}													\
} while (0)												\


// ---------------------------------
// 기타 함수
// ---------------------------------
// 네트워크 프로세스
// 여기서 false가 반환되면, 프로그램이 종료된다.
bool Network_Process(SOCKET* listen_sock);

// Accept 처리.
bool Accept(SOCKET* client_sock, SOCKADDR_IN clinetaddr);

// Accept 시, 내 기준 9방향 섹터의 유저들 정보를 NewAccount의 SendBuff에 넣기
bool Accept_Surport(stPlayer* Player, stSectorCheck* Sector);

// Disconnect 처리
void Disconnect(stAccount* Account);

// 데드레커닝 함수
// 인자로 (현재 액션, 액션 시작 시간, 액션 시작 위치, (OUT)계산 후 좌표)를 받는다.
void DeadReckoning(BYTE NowAction, ULONGLONG ActionTick, int ActionStartX, int ActionStartY, int* ResultX, int* ResultY);

// 로그 찍기 함수
void Log(TCHAR *szString, int LogLevel);





// ---------------------------------
// 섹터처리용 함수
// ---------------------------------
// 섹터 체인지 체크
void SectorChange(stAccount* Account);

// 인자로 받은 Sector의 유저들의 SendBuff에 인자로 받은 패킷 넣기
bool SendPacket_Sector(stPlayer* Player, CProtocolBuff* header, CProtocolBuff* payload, stSectorCheck* Sector);

// 인자로 받은 섹터 1개의 유저들의 SendBuff에 인자로 받은 패킷 넣기.
bool SendPacket_SpecialSector(stPlayer* Player, CProtocolBuff* header, CProtocolBuff* payload, int SectorX, int SectorY);

// 모든 섹터의 Index, 인원 수를 찍는 함수. 디버깅용
void Print_Sector(stPlayer* NowPlayer);

// 인자로 받은 X,Y를 기준으로, 인자로 받은 구조체에 9방향을 넣어준다.
void GetSector(int SectorX, int SectorY, stSectorCheck* Sector);

// 인자로 받은 올드 섹터좌표, 신 섹터좌표를 기준으로 캐릭터를 삭제해야하는 섹터, 캐릭터를 추가해야하는 섹터를 구한다.
void GetUpdateSectorArount(int OldSectorX, int OldSectorY, int NewSectorX, int NewSectorY, stSectorCheck* RemoveSector, stSectorCheck* AddSector);

// 섹터가 이동되었을 때 호출되는 함수. 나를 기준으로 캐릭터 삭제/추가를 체크한다.
void CharacterSectorUpdate(stPlayer* NowPlayer, int NewSectorX, int NowSectorY);







// ---------------------------------
// Update()에서 유저 액션 처리 함수
// ---------------------------------
// 각 플레이어 액션 체크
void ActionProc();

// 유저 이동 액션 처리
void Action_Move(stPlayer* NowPlayer);







// ---------------------------------
// 검색용 함수
// ---------------------------------
// 인자로 받은 Socket 값을 기준으로 [회원 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stAccount* ClientSearch_AcceptList(SOCKET sock);








// ---------------------------------
// Recv 처리 함수들
// ---------------------------------
// Recv() 처리
bool RecvProc(stAccount* Account);

// 패킷 처리
bool PacketProc(WORD PacketType, stAccount* Account, CProtocolBuff* Payload);

// "캐릭터 이동 시작" 패킷 처리
bool Network_Req_MoveStart(stAccount* Account, CProtocolBuff* payload);

// "캐릭터 이동 정지" 패킷 처리
bool Network_Req_MoveStop(stAccount* Account, CProtocolBuff* payload);

// "공격 1번" 패킷 처리
bool Network_Req_Attack_1(stAccount* Account, CProtocolBuff* payload);

// "공격 2번" 패킷 처리
bool Network_Req_Attack_2(stAccount* Account, CProtocolBuff* payload);

// "공격 3번" 패킷 처리
bool Network_Req_Attack_3(stAccount* Account, CProtocolBuff* payload);

// "공격 1번" 패킷의 데미지 처리. Network_Req_Attack_1()에서 사용.
bool Network_Requ_Attck_1_Damage(DWORD AttackID, int AttackX, int AttackY, BYTE AttackDir);

// "공격 2번" 패킷의 데미지 처리. Network_Req_Attack_2()에서 사용.
bool Network_Requ_Attck_2_Damage(DWORD AttackID, int AttackX, int AttackY, BYTE AttackDir);

// "공격 3번" 패킷의 데미지 처리. Network_Req_Attack_3()에서 사용.
bool Network_Requ_Attck_3_Damage(DWORD AttackID, int AttackX, int AttackY, BYTE AttackDir);

// 에코 패킷 처리
bool Network_Req_StressTest(stAccount* Account, CProtocolBuff* Packet);




// ---------------------------------
// 패킷 생성 함수
// ---------------------------------
// 헤더 제작 함수
void CreateHeader(CProtocolBuff* header, BYTE PayloadSize, BYTE PacketType);

// "내 캐릭터 생성" 패킷 제작
void Network_Send_CreateMyCharacter(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y, BYTE HP);

// "다른 사람 캐릭터 생성" 패킷 제작
void Network_Send_CreateOtherCharacter(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y, BYTE HP);

// "다른 사람 삭제" 패킷 제작
void Network_Send_RemoveOtherCharacteor(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID);

// "다른 사람 이동 시작" 패킷 제작
void Network_Send_MoveStart(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y);

// "다른 사람 정지" 패킷 제작
void Network_Send_MoveStop(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y);

// "싱크 맞추기" 패킷 제작
void Network_Send_Sync(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, WORD X, WORD Y);

// "1번 공격 시작" 패킷 만들기
void Network_Send_Attack_1(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y);

// "2번 공격 시작" 패킷 만들기
void Network_Send_Attack_2(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y);

// "3번 공격 시작" 패킷 만들기
void Network_Send_Attack_3(CProtocolBuff* header, CProtocolBuff* payload, DWORD ID, BYTE Dir, WORD X, WORD Y);

// "데미지" 패킷 만들기
void Network_Send_Damage(CProtocolBuff *header, CProtocolBuff* payload, DWORD AttackID, DWORD DamageID, BYTE HP);




// ---------------------------------
// Send
// ---------------------------------
// SendBuff에 데이터 넣기
BOOL SendPacket(stAccount* Account, CProtocolBuff* header, CProtocolBuff* payload);

// SendBuff의 데이터를 Send하기
bool SendProc(stAccount* Account);



