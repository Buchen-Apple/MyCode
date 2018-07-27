//--------------------------------------------------
// 친구관리 프로토콜
//
//
//--------------------------------------------------

#define dfNETWORK_PORT		25000

#define dfNICK_MAX_LEN		20
#define dfPACKET_CODE		0x89


//------------------------------------------------------
//  패킷헤더
//
//	| PacketCode | MsgType | PayloadSize | * Payload * |
//		1Byte       2Byte      2Byte        Size Byte     
//
//------------------------------------------------------
#define dfHEADER_SIZE	5

#pragma pack(push, 1)   

struct st_PACKET_HEADER
{
	BYTE	byCode;

	WORD	wMsgType;
	WORD	wPayloadSize;
};

#pragma pack(pop)


//------------------------------------------------------------
// 회원가입 요청
//
// {
//		WCHAR[dfNICK_MAX_LEN]	닉네임
// }
//------------------------------------------------------------
#define df_REQ_ACCOUNT_ADD				1

//------------------------------------------------------------
// 회원가입 결과
//
// {
//		UINT64		AccountNo
// }
//------------------------------------------------------------
#define df_RES_ACCOUNT_ADD				2




//------------------------------------------------------------
// 회원로그인
//
// {
//		UINT64		AccountNo
// }
//------------------------------------------------------------
#define df_REQ_LOGIN					3

//------------------------------------------------------------
// 회원로그인 결과
//
// {
//		UINT64					AccountNo		// 0 이면 실패
//		WCHAR[dfNICK_MAX_LEN]	NickName
// }
//------------------------------------------------------------
#define df_RES_LOGIN					4



//------------------------------------------------------------
// 회원리스트 요청
//
// {
//		없음.
// }
//------------------------------------------------------------
#define df_REQ_ACCOUNT_LIST				10

//------------------------------------------------------------
// 회원리스트 결과
//
// {
//		UINT	Count		// 회원 수
//		{
//			UINT64					AccountNo
//			WCHAR[dfNICK_MAX_LEN]	NickName
//		}
// }
//------------------------------------------------------------
#define df_RES_ACCOUNT_LIST				11




//------------------------------------------------------------
// 친구목록 요청
//
// {
//		없음
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_LIST				12

//------------------------------------------------------------
// 친구목록 결과
//
// { 
//		UINT	FriendCount
//		{
//			UINT64					FriendAccountNo
//			WCHAR[dfNICK_MAX_LEN]	NickName
//		}	
// }
//------------------------------------------------------------
#define df_RES_FRIEND_LIST				13






//------------------------------------------------------------
// 친구요청 보낸 목록  요청
//
// {
//		없음
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_REQUEST_LIST		14

//------------------------------------------------------------
// 친구목록 결과
//
// {
//		UINT	FriendCount
//		{
//			UINT64					FriendAccountNo
//			WCHAR[dfNICK_MAX_LEN]	NickName
//		}	
// }
//------------------------------------------------------------
#define df_RES_FRIEND_REQUEST_LIST		15




//------------------------------------------------------------
// 친구요청 받은거 목록  요청
//
// {
//		없음
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_REPLY_LIST		16

//------------------------------------------------------------
// 친구목록 결과
//
// {
//		UINT	FriendCount
//		{
//			UINT64					FriendAccountNo
//			WCHAR[dfNICK_MAX_LEN]	NickName
//		}	
// }
//------------------------------------------------------------
#define df_RES_FRIEND_REPLY_LIST		17



//------------------------------------------------------------
// 친구관계 끊기
//
// {
//		UINT64	FriendAccountNo
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_REMOVE			20

//------------------------------------------------------------
// 친구관계 끊기 결과
//
// {
//		UINT64	FriendAccountNo
//
//		BYTE	Result
// }
//------------------------------------------------------------
#define df_RES_FRIEND_REMOVE			21

#define df_RESULT_FRIEND_REMOVE_OK			1
#define df_RESULT_FRIEND_REMOVE_NOTFRIEND	2
#define df_RESULT_FRIEND_REMOVE_FAIL		3



//------------------------------------------------------------
// 친구요청
//
// {
//		UINT64	FriendAccountNo
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_REQUEST			22

//------------------------------------------------------------
// 친구요청 결과
//
// {
//		UINT64	FriendAccountNo
//
//		BYTE	Result
// }
//------------------------------------------------------------
#define df_RES_FRIEND_REQUEST			23

#define df_RESULT_FRIEND_REQUEST_OK			1
#define df_RESULT_FRIEND_REQUEST_NOTFOUND	2
#define df_RESULT_FRIEND_REQUEST_AREADY		3




//------------------------------------------------------------
// 친구요청 취소
//
// {
//		UINT64	FriendAccountNo
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_CANCEL			24

//------------------------------------------------------------
// 친구요청취소 결과
//
// {
//		UINT64	FriendAccountNo
//
//		BYTE	Result
// }
//------------------------------------------------------------
#define df_RES_FRIEND_CANCEL			25

#define df_RESULT_FRIEND_CANCEL_OK			1
#define df_RESULT_FRIEND_CANCEL_NOTFRIEND	2
#define df_RESULT_FRIEND_CANCEL_FAIL		3




//------------------------------------------------------------
// 친구요청 거부
//
// {
//		UINT64	FriendAccountNo
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_DENY				26

//------------------------------------------------------------
// 친구요청 거부 결과
//
// {
//		UINT64	FriendAccountNo
//
//		BYTE	Result
// }
//------------------------------------------------------------
#define df_RES_FRIEND_DENY				27

#define df_RESULT_FRIEND_DENY_OK			1
#define df_RESULT_FRIEND_DENY_NOTFRIEND		2
#define df_RESULT_FRIEND_DENY_FAIL			3




//------------------------------------------------------------
// 친구요청 수락
//
// {
//		UINT64	FriendAccountNo
// }
//------------------------------------------------------------
#define df_REQ_FRIEND_AGREE				28

//------------------------------------------------------------
// 친구요청 수락 결과
//
// {
//		UINT64	FriendAccountNo
//
//		BYTE	Result
// }
//------------------------------------------------------------
#define df_RES_FRIEND_AGREE				29

#define df_RESULT_FRIEND_AGREE_OK			1
#define df_RESULT_FRIEND_AGREE_NOTFRIEND	2
#define df_RESULT_FRIEND_AGREE_FAIL			3









//------------------------------------------------------------
// 스트레스 테스트용 에코
//
// {
//			WORD		Size
//			Size		문자열 (WCHAR 유니코드)
// }
//------------------------------------------------------------
#define df_REQ_STRESS_ECHO				100

//------------------------------------------------------------
// 스트레스 테스트용 에코응답
//
// {
//			WORD		Size
//			Size		문자열 (WCHAR 유니코드)
// }
//------------------------------------------------------------
#define df_RES_STRESS_ECHO				101

