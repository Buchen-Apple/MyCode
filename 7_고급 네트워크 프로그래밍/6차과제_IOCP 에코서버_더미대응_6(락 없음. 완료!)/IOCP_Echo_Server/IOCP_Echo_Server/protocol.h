#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define SERVER_PORT	6000

// -------------------------
// 패킷 헤더 구조
//
// 총 6바이트
// WORD - Type
// DWORD - PayloadSize
// -------------------------

// 헤더 사이즈
#define dfNETWORK_PACKET_HEADER_SIZE	2


#endif // !__PROTOCOL_H__


