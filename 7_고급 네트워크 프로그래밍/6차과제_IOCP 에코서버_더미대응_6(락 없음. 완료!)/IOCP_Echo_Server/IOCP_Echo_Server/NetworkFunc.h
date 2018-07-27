#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>
#include <map>

#include "protocol.h"
#include "RingBuff\RingBuff.h"
#include "ProtocolBuff\ProtocolBuff.h"

using namespace Library_Jingyu;
using namespace std;


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


// 세션 구조체
struct stSession
{
	SOCKET m_Client_sock;	

	TCHAR m_IP[30];
	USHORT m_prot;

	OVERLAPPED m_RecvOverlapped;
	OVERLAPPED m_SendOverlapped;

	CRingBuff m_RecvBuff;
	CRingBuff m_SendBuff;

	LONG	m_IOCount = 0;

	// Send가능 상태인지 체크. 1이면 Send중, 0이면 Send중 아님
	LONG	m_SendFlag = 0;	
};


extern SRWLOCK g_Session_map_srwl;

// 세션 저장 map.
// Key : SOCKET, Value : 세션구조체
extern map<SOCKET, stSession*> map_Session;

#define	LockSession()	AcquireSRWLockExclusive(&g_Session_map_srwl)
#define UnlockSession()	ReleaseSRWLockExclusive(&g_Session_map_srwl)


// 접속 종료
void Disconnect(stSession* DeleteSession);

// 로그 찍기 함수
void Log(TCHAR *szString, int LogLevel);




// ------------
// 리시브 관련 함수들
// ------------
// RecvProc 함수
bool RecvProc(stSession* NowSession);

// RecvPost 함수
bool RecvPost(stSession* NowSession);

// Accept용 RecvPost함수
bool RecvPost_Accept(stSession* NowSession);

// 받은 패킷 처리 분기문 함수
bool PacketProc(stSession* NowSession, CProtocolBuff* Payload);



// ------------
// 패킷 처리 함수들
// ------------
bool Recv_Packet_Echo(stSession* NowSession, CProtocolBuff* Payload);



// ------------
// 샌드할 패킷 만들기 함수들
// ------------
// 에코패킷 만들기
void Send_Packet_Echo(CProtocolBuff* Buff, WORD PayloadSize, char* RetrunText, int RetrunTextSize);




// ------------
// 샌드 관련 함수들
// ------------
// 샌드 버퍼에 데이터 넣기
bool SendPacket(stSession* NowSession, CProtocolBuff* Buff);

// 샌드 버퍼의 데이터 WSASend() 하기
bool SendPost(stSession* NowSession);





#endif // !__NETWORK_FUNC_H__

