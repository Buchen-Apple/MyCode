#ifndef __MONITOR_PROTOCOL_H__
#define __MONITOR_PROTOCOL_H__


/*
모니터링 서버와 연동관련 프로토콜

본래 CommonProtocol.h 에 통합으로 들어있으나
CommonProtocol.h 의 채팅,로그인 등 패킷변동사항이 많아서
혼동이 올 수 있으므로 모니터링관련 패킷만 별도로 빼서 드립니다.

이를 따로 h 에 붙여서 사용 해주시고
차후에 채팅서버 변경사항 안내 후 최종 ComonProtocol 을 드립니다.


# 최종 프로젝트 채팅변경사항 - MO 서버 구성으로 방(채널) 개념의 채팅서버로 변경됩니다.
*/

enum en_PACKET_TYPE
{
	

};



// en_PACKET_SS_MONITOR_LOGIN
enum en_PACKET_CS_MONITOR_TOOL_SERVER_CONTROL
{
	dfMONITOR_SERVER_TYPE_LOGIN = 1,
	dfMONITOR_SERVER_TYPE_GAME = 2,
	dfMONITOR_SERVER_TYPE_CHAT = 3,
	dfMONITOR_SERVER_TYPE_AGENT = 4,

	dfMONITOR_SERVER_CONTROL_SHUTDOWN = 1,		// 서버 정상종료 (게임서버 전용)
	dfMONITOR_SERVER_CONTROL_TERMINATE = 2,		// 서버 프로세스 강제종료
	dfMONITOR_SERVER_CONTROL_RUN = 3,		// 서버 프로세스 생성 & 실행
};


enum en_PACKET_SS_MONITOR_DATA_UPDATE
{
	// 별도의 에이전트(서버 컨트롤용) 프로그램이 없으므로 하드웨어 자체의 모니터링 수집은
	// 매치메이킹 서버가 전담 하도록 합니다. 다른서버가 해도 됩니다. 누군가 하나만 하면 됩니다.

	// 2U 서버 1대에 매치메이킹 / 배틀 / 채팅  3대의 서버가 가동 되겠습니다.

	// 본래 서버 프로세스 ON 여부는 외부에서 판단하여 보내야 하지만
	// 에이전트 없이 진행 되므로 자기 자신이 보내는것으로 합니다.
	// 다만 꺼지면 해당 데이터 자체가 안가기 때문에 일정시간 후에 모니터링 툴은 OFF 로 인식하게 됩니다.
	// OFF 판단까지 5초~10초 정도 시간이 소요되지만 그렇게 합니다.


	dfMONITOR_DATA_TYPE_SERVER_CPU_TOTAL = 1,                    // 하드웨어 CPU 사용률 전체
	dfMONITOR_DATA_TYPE_SERVER_AVAILABLE_MEMORY,                 // 하드웨어 사용가능 메모리
	dfMONITOR_DATA_TYPE_SERVER_NETWORK_RECV,                     // 하드웨어 이더넷 수신 바이트
	dfMONITOR_DATA_TYPE_SERVER_NETWORK_SEND,                     // 하드웨어 이더넷 송신 바이트
	dfMONITOR_DATA_TYPE_SERVER_NONPAGED_MEMORY,                  // 하드웨어 논페이지 메모리 사용량

	dfMONITOR_DATA_TYPE_MATCH_SERVER_ON,                        // 매치메이킹 서버 ON
	dfMONITOR_DATA_TYPE_MATCH_CPU,                              // 매치메이킹 CPU 사용률 (커널 + 유저)
	dfMONITOR_DATA_TYPE_MATCH_MEMORY_COMMIT,                    // 매치메이킹 메모리 유저 커밋 사용량 (Private) MByte
	dfMONITOR_DATA_TYPE_MATCH_PACKET_POOL,                      // 매치메이킹 패킷풀 사용량
	dfMONITOR_DATA_TYPE_MATCH_SESSION,                          // 매치메이킹 접속 세션
	dfMONITOR_DATA_TYPE_MATCH_PLAYER,                           // 매치메이킹 접속 유저 (로그인 성공 후)
	dfMONITOR_DATA_TYPE_MATCH_MATCHSUCCESS,                     // 매치메이킹 방 배정 성공 수 (초당)


	dfMONITOR_DATA_TYPE_MASTER_SERVER_ON,                        // 마스터 서버 ON
	dfMONITOR_DATA_TYPE_MASTER_CPU,                              // 마스터 CPU 사용률 (프로세스)
	dfMONITOR_DATA_TYPE_MASTER_CPU_SERVER,                       // 마스터 CPU 사용률 (서버 컴퓨터 전체)
	dfMONITOR_DATA_TYPE_MASTER_MEMORY_COMMIT,                    // 마스터 메모리 유저 커밋 사용량 (Private) MByte
	dfMONITOR_DATA_TYPE_MASTER_PACKET_POOL,                      // 마스터 패킷풀 사용량
	dfMONITOR_DATA_TYPE_MASTER_MATCH_CONNECT,                    // 마스터 매치메이킹 서버 연결 수
	dfMONITOR_DATA_TYPE_MASTER_MATCH_LOGIN,                      // 마스터 매치메이킹 서버 로그인 수
	dfMONITOR_DATA_TYPE_MASTER_STAY_CLIENT,                      // 마스터 대기자 클라이언트
	dfMONITOR_DATA_TYPE_MASTER_BATTLE_CONNECT,                   // 마스터 배틀 서버 연결 수
	dfMONITOR_DATA_TYPE_MASTER_BATTLE_LOGIN,                     // 마스터 배틀 서버 로그인 후
	dfMONITOR_DATA_TYPE_MASTER_BATTLE_STANDBY_ROOM,              // 마스터 배틀 서버 로그인 후



	dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,                       // 배틀서버 ON
	dfMONITOR_DATA_TYPE_BATTLE_CPU,                             // 배틀서버 CPU 사용률 (커널 + 유저)
	dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT,                   // 배틀서버 메모리 유저 커밋 사용량 (Private) MByte
	dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL,                     // 배틀서버 패킷풀 사용량
	dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS,                        // 배틀서버 Auth 스레드 초당 루프 수
	dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS,                        // 배틀서버 Game 스레드 초당 루프 수
	dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL,                     // 배틀서버 접속 세션전체
	dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH,                    // 배틀서버 Auth 스레드 모드 인원
	dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME,                    // 배틀서버 Game 스레드 모드 인원
	dfMONITOR_DATA_TYPE_BATTLE_,		                        // 배틀서버 로그인을 성공한 전체 인원		<- 삭제
	dfMONITOR_DATA_TYPE_BATTLE_ROOM_WAIT,                       // 배틀서버 대기방 수
	dfMONITOR_DATA_TYPE_BATTLE_ROOM_PLAY,                       // 배틀서버 플레이방 수

	dfMONITOR_DATA_TYPE_CHAT_SERVER_ON,                         // 채팅서버 ON
	dfMONITOR_DATA_TYPE_CHAT_CPU,                               // 채팅서버 CPU 사용률 (커널 + 유저)
	dfMONITOR_DATA_TYPE_CHAT_MEMORY_COMMIT,                     // 채팅서버 메모리 유저 커밋 사용량 (Private) MByte
	dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL,                       // 채팅서버 패킷풀 사용량
	dfMONITOR_DATA_TYPE_CHAT_SESSION,                           // 채팅서버 접속 세션전체
	dfMONITOR_DATA_TYPE_CHAT_PLAYER,                            // 채팅서버 로그인을 성공한 전체 인원
	dfMONITOR_DATA_TYPE_CHAT_ROOM                               // 배틀서버 방 수

};





#endif // !__MONITOR_PROTOCOL_H__

