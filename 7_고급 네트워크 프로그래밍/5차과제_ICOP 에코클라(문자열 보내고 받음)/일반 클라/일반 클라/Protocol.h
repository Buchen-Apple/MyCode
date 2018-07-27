#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define SERVER_PORT	9000

// -------------------------
// 패킷 헤더 구조
//
// 총 6바이트
// WORD - Type
// DWORD - PayloadSize
// -------------------------

// 헤더 사이즈
#define dfNETWORK_PACKET_HEADER_SIZE	4

#pragma pack(push, 1)
struct stNETWORK_PACKET_HEADE
{
	BYTE	byCode;
	BYTE	bySize;
	BYTE	byType;
	BYTE	byTemp;
};
#pragma pack(pop)


//---------------------------------------------------------------
// 패킷의 가장 앞에 들어갈 패킷코드.
//---------------------------------------------------------------
#define dfNETWORK_PACKET_CODE	((BYTE)0x89)


//---------------------------------------------------------------
// 패킷의 가장 뒤에 들어갈 패킷코드.
// 패킷의 끝 부분에는 1Byte 의 EndCode 가 포함된다.  0x79
//---------------------------------------------------------------
#define dfNETWORK_PACKET_END	((BYTE)0x79)



#define	dfPACKET_CS_ECHO						252
//---------------------------------------------------------------
// Echo 용 패킷					Client -> Server
//
//	4	-	Time
//
//---------------------------------------------------------------

#define	dfPACKET_SC_ECHO						253
//---------------------------------------------------------------
// Echo 응답 패킷				Server -> Client
//
//	4	-	Time
//
//---------------------------------------------------------------




#endif // !__PROTOCOL_H__


