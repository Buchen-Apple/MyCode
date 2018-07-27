#ifndef __NETWORK_FUNC_H__
#define __NETWORK_FUNC_H__

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>
#include <map>

#include "protocol.h"
#include "RingBuff\RingBuff.h"
#include "ProtocolBuff\ProtocolBuff.h"

using namespace Library_Jingyu;



// ------------
// 리시브 관련 함수들
// ------------
// RecvPost함수. 실제 Recv 호출
int RecvPost();

// RecvProc 함수. 리시브 큐의 내용을 분석
bool RecvProc();

// 패킷 처리. RecvProc()에서 받은 패킷 처리.
bool PacketProc(BYTE PacketType, CProtocolBuff* Payload);




// ------------
// 패킷 만들기
// ------------
// 헤더 제작 함수
void CreateHeader(CProtocolBuff* header, BYTE PayloadSize, BYTE PacketType);

// 에코 패킷 만들기
bool Network_Send_Echo(CProtocolBuff *header, CProtocolBuff* payload);

// ------------
// 패킷 처리 함수들
// ------------
// 에코 패킷 처리 함수
bool Recv_Packet_Echo(CProtocolBuff* Payload);



// ------------
// 샌드 관련 함수들
// ------------
// 샌드 버퍼에 데이터 넣기
bool SendPacket(CProtocolBuff* header, CProtocolBuff* payload);

// 샌드 버퍼의 데이터 Send() 하기
int SendPost();




#endif // !__NETWORK_FUNC_H__

