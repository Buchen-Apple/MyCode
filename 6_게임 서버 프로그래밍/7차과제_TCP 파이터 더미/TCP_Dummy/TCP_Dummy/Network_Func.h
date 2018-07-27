#ifndef __NETWORK_FUNC_H__

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>
#include "Protocol.h"
#include "ProtocolBuff\ProtocolBuff.h"
#include "RingBuff\RingBuff.h"

using namespace Library_Jingyu;

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




// 전방선언들
struct stSession;
struct stDummyClient;


// ---------------------------------
// 기타 함수
// ---------------------------------
// 네트워크 프로세스
// 여기서 false가 반환되면, 프로그램이 종료된다.
bool Network_Process();

// 네트워크 셋팅 함수 (윈속초기화, 커넥트 등..)
bool Network_Init(int* TryCount, int* FailCount, int* SuccessCount);

// Disconnect 처리
void Disconnect(stSession* Account);

// 로그 찍기 함수
void Log(TCHAR *szString, int LogLevel);

// 데드레커닝 함수
// 인자로 (현재 액션, 액션 시작 시간, 액션 시작 위치, (OUT)계산 후 좌표)를 받는다.
void DeadReckoning(BYTE NowAction, ULONGLONG ActionTick, int ActionStartX, int ActionStartY, int* ResultX, int* ResultY);

// 초당 RTT 계산.
ULONGLONG RTTAvr();





// ---------------------------------
// 검색용 함수
// --------------------------------
// 인자로 받은 Socket 값을 기준으로 [회원 목록]에서 [유저를 골라낸다].(검색)
// 성공 시, 해당 유저의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stSession* ClientSearch_AcceptList(SOCKET sock);

// 인자로 받은 유저 ID로 [플레이어 목록]에서 [플레이어를 골라낸다.] (검색)
// 성공 시, 해당 플레이어의 구조체 주소를 리턴
// 실패 시 nullptr 리턴
stDummyClient* ClientSearch_ClientList(DWORD ID);





// ---------------------------------
// AI 함수
// ---------------------------------
void DummyAI();




// ---------------------------------
// Update()에서 유저 액션 처리 함수
// ---------------------------------
// 유저 이동 액션 처리
void Action_Move(stDummyClient* NowPlayer);









// ---------------------------------
// Recv 처리 함수들
// ---------------------------------
// Recv() 처리
bool RecvProc(stSession* Account);

// 패킷 처리
bool PacketProc(WORD PacketType, stDummyClient* Player, CProtocolBuff* Payload);

// 내 캐릭터 생성
bool Network_Recv_CreateMyCharacter(stDummyClient* Player, CProtocolBuff* Payload);

// 다른 사람 캐릭터 생성
bool Network_Recv_CreateOtherCharacter(stDummyClient* Player, CProtocolBuff* Payload);

// 캐릭터 삭제
bool Network_Recv_DeleteCharacter(stDummyClient* Player, CProtocolBuff* Payload);

// 다른 유저 이동 시작
bool Network_Recv_OtherMoveStart(stDummyClient* Player, CProtocolBuff* Payload);

// 다른 유저 이동 정지
bool Network_Recv_OtherMoveStop(stDummyClient* Player, CProtocolBuff* Payload);

// 다른 유저 공격 1번 시작
bool Network_Recv_OtherAttack_01(stDummyClient* Player, CProtocolBuff* Payload);

// 다른 유저 공격 2번 시작
bool Network_Recv_OtherAttack_02(stDummyClient* Player, CProtocolBuff* Payload);

// 다른 유저 공격 3번 시작
bool Network_Recv_OtherAttack_03(stDummyClient* Player, CProtocolBuff* Payload);

// 데미지 패킷
bool Network_Recv_Damage(stDummyClient* Player, CProtocolBuff* Payload);

// 싱크 패킷
bool Network_Recv_Sync(stDummyClient* Player, CProtocolBuff* Payload);

// 에코 패킷
bool Network_Recv_Echo(stDummyClient* Player, CProtocolBuff* Payload);




// ---------------------------------
// 패킷 생성 함수
// ---------------------------------
// 헤더 제작 함수
void CreateHeader(CProtocolBuff* header, BYTE PayloadSize, BYTE PacketType);

// 이동 시작 패킷 만들기
bool Network_Send_MoveStart(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload);

// 이동 정지 패킷 만들기
bool Network_Send_MoveStop(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload);

// 공격 1번 시작 패킷 만들기
bool Network_Send_Attack_01(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload);

// 공격 2번 시작 패킷 만들기
bool Network_Send_Attack_02(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload);

// 공격 3번 시작 패킷 만들기
bool Network_Send_Attack_03(BYTE Dir, WORD X, WORD Y, CProtocolBuff *header, CProtocolBuff* payload);

// 에코 패킷 만들기
bool Network_Send_Echo(CProtocolBuff *header, CProtocolBuff* payload);



// ---------------------------------
// Send
// ---------------------------------
// SendBuff에 데이터 넣기
bool SendPacket(stSession* Account, CProtocolBuff* header, CProtocolBuff* payload);

// SendBuff의 데이터를 Send하기
bool SendProc(stSession* Account);





#endif // !__NETWORK_FUNC_H__


