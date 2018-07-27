#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include "Protocol.h"
#include "ProtocolBuff\ProtocolBuff.h"
#include "RingBuff\RingBuff.h"

using namespace Library_Jingyu;

// 구조체 전방선언
struct stDummyClient;

// 네트워크 셋팅 함수 (윈속초기화, 커넥트 등..)
bool Network_Init(int* TryCount, int* FailCount, int* SuccessCount);

// 네트워크 프로세스
bool Network_Process();

// 인자로 받은 Socket 값을 기준으로 [더미 목록]에서 [더미를 골라낸다].(검색)
// 성공 시, 해당 더미의 정보 구조체의 주소를 리턴
// 실패 시 nullptr 리턴
stDummyClient* DummySearch(SOCKET sock);

// Disconnect 처리
void Disconnect(SOCKET sock);



///////////////////////////////
// 화면 출력용 값 반환 함수
///////////////////////////////
// 레이턴시 평균 반환 함수
DWORD GetAvgLaytency();


// TPS 반환 함수
int GetTPS();


///////////////////////////////
// 더미의 할 일.
///////////////////////////////
// 랜덤 문자열 생성 후 SendBuff에 넣는다.
void DummyWork_CreateString();



/////////////////////////
// Recv 처리 함수들
/////////////////////////
// Recv() 처리
bool RecvProc(stDummyClient* NowDummy);

// "에코 패킷" 처리
bool Network_Res_Acho(WORD Type, stDummyClient* NowDummy, CProtocolBuff* payload);



/////////////////////////
// Send 처리
/////////////////////////
// Send버퍼에 데이터 넣기
bool SendPacket(stDummyClient* NowDummy, CProtocolBuff* headerBuff, CProtocolBuff* payloadBuff);

// Send버퍼의 데이터를 Send하기
bool SendProc(CRingBuff* SendBuff, SOCKET sock);


#endif // !__NETWORK_FUNC_H__
