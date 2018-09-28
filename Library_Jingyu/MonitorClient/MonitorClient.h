#ifndef __MONITOR_CLIENT_H__
#define __MONITOR_CLIENT_H__

#include "NetworkLib\NetworkLib_NetClient.h"
#include "LockFree_Queue\LockFree_Queue.h"
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "MonitorView.h"

// ----------------------
// 모니터링 클라이언트
// ----------------------
namespace Library_Jingyu
{
	class  CMonitorClient :public CNetClient
	{
		// 데이터 전송 시 서버 No
		enum en_Monitor_Type
		{
			dfMONITOR_ETC = 2,		// 채팅서버 외 모든 정보.
			dfMONITOR_CHATSERVER = 3		// 채팅 서버
		};

		// SetMonitorClass 에서 사용.
		// 파일에서 읽어온 정보를 저장해둔다.
		struct stSetMonitorInfo
		{
			bool m_bUseCheck; // 사용여부

			int m_iDataTypeCount;
			int m_iDataType[4];
			int m_iGraphType;
			int m_iPosX;
			int m_iPosY;
			int m_iWidth;
			int m_iHeight;

			TCHAR m_tcCaptionText[30];
			TCHAR m_tcUnit[20];

			int m_iRedColor;
			int m_iGreenColor;
			int m_iBlueColor;

			int m_iGraphMaxValue;			
			int m_iAlarmValue;	
			int m_iMinAlarmValue;
		
			int m_iServerNo[4];			
			TCHAR m_tcColumText[4][20];
		};


		// 화면에 출력할 데이터 보관 구조체
		struct stLastData
		{
			bool m_bFirstCheck;		// 최초로 데이터를 한 번이라도 받은적 있는지 체크. 받은 적 있으면 true
			int m_ZeroCount = 0;	// 데이터를 못받으면 0을 찍는데, 이걸 몇번했는지 체크.
			BYTE m_ServerNo = 0;
			int m_Value = -1;			
		};

		// 일감 구조체
		struct st_WorkNode
		{
			WORD Type;	
			ULONGLONG SessionID;
			CProtocolBuff_Net* m_pPacket;
		};

		struct stInfo
		{
			TCHAR m_tcConnectIP[30];
			int m_iPort;
			int m_iCreateWorker;
			int m_iActiveWorker;
			int m_bHeadCode;
			int m_bXORCode1;
			int m_bXORCode2;
			int m_iNodelay;
			int m_iLogLevel;
		};

		// 모니터링 서버로 들고갈 키
		char m_cLoginKey[32];

		// 일감 TLS
		CMemoryPoolTLS<st_WorkNode>* m_WorkPool;

		// 일감 관리 큐
		CLF_Queue<st_WorkNode*> *m_LFQueue;

		// 현재 세션 ID
		ULONGLONG m_ullSessionID;

		// 로그인 패킷까지 받아서 로그인 처리된 상태인지
		// true면 로그인 중.
		bool m_bLoginCheck;

		// 모니터링 뷰어 배열
		// 타입 수 만큼 가지고 있음.
		// 최초에는, 모두 nullptr
		CMonitorGraphUnit* m_pMonitor[dfMONITOR_DATA_TYPE_END-1] = { nullptr, };

		// 뷰어에 출력하기 전, 마지막으로 받은 데이터를 보관하는 장소.
		stLastData m_LastData[dfMONITOR_DATA_TYPE_END - 1];

		// Config 데이터
		stInfo m_stConfig;		

	public:
		// -----------------------
		// 생성자와 소멸자
		// -----------------------
		CMonitorClient();
		virtual ~CMonitorClient();


	public:
		// -----------------------
		// 외부에서 사용 가능한 기능 함수
		// -----------------------

		// 모니터 클래스 셋팅.
		// 외부의, WM_CREATE에서 호출
		//
		// Parameter : 인스턴스, 윈도우 핸들
		// retrun : 없음
		void SetMonitorClass(HINSTANCE hInst, HWND hWnd);

		// 1초에 1회 호출되는 업데이트 정보
		//
		// Parameter : 없음
		// return : 없음
		void Update();

		// 모니터링 클라이언트 스타트
		// 내부에서는 NetClient의 Start 함수 호출
		//
		// Parameter : 없음
		// return : 실패 시 false
		bool ClientStart();

		// 모니터링 클라이언트 스탑
		// 내부에서는 NetClient의 Stop 함수 호출
		//
		// Parameter : 없음
		// return : 없음
		void ClientStop();



	private:
		// -----------------------
		// 내부에서만 사용 가능한 기능 함수
		// -----------------------

		// 로그인 요청에 대한 응답 처리
		//
		// Parameter : SessionID, Packet(Net)
		// return : 없음
		void Login_Packet(ULONGLONG SessionID, CProtocolBuff_Net* Packet);

		// 데이터 전송 패킷 받았을 시 하는 일
		//
		// Parameter : SessionID, Packet(Net), stLastData*
		// return : 없음
		void Data_Packet(ULONGLONG SessionID, CProtocolBuff_Net* Packet, stLastData* LastData);

		// 파싱 함수
		// 
		// Parameter : config 구조체
		// return : 실패 시 false
		bool SetFile(stInfo* Config);		

		// 모니터링 정보를 읽어오는 함수
		// 
		// Parameter : stSetMonitorInfo 구조체, 구역 이름
		// return : 실패 시 false
		bool SetMonitorInfo(stSetMonitorInfo* pConfig, TCHAR* AreaName);



	private:
		// -----------------------
		// 가상함수
		// -----------------------

		// 목표 서버에 연결 성공 후, 호출되는 함수 (ConnectFunc에서 연결 성공 후 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnConnect(ULONGLONG ClinetID);

		// 목표 서버에 연결 종료 후 호출되는 함수 (InDIsconnect 안에서 호출)
		//
		// parameter : 세션키
		// return : 없음
		virtual void OnDisconnect(ULONGLONG ClinetID);


		// 패킷 수신 완료 후 호출되는 함수.
		//
		// parameter : 유저 세션키, 받은 패킷
		// return : 없음
		virtual void OnRecv(ULONGLONG SessionID, CProtocolBuff_Net* Payload);

		// 패킷 송신 완료 후 호출되는 함수
		//
		// parameter : 유저 세션키, Send 한 사이즈
		// return : 없음
		virtual void OnSend(ULONGLONG SessionID, DWORD SendSize);

		// 워커 스레드가 깨어날 시 호출되는 함수.
		// GQCS 바로 하단에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadBegin();

		// 워커 스레드가 잠들기 전 호출되는 함수
		// GQCS 바로 위에서 호출
		// 
		// parameter : 없음
		// return : 없음
		virtual void OnWorkerThreadEnd();

		// 에러 발생 시 호출되는 함수.
		//
		// parameter : 에러 코드(실제 윈도우 에러코드는 WinGetLastError() 함수로 얻기 가능. 없을 경우 0이 리턴됨)
		//			 : 에러 코드에 대한 스트링
		// return : 없음
		virtual void OnError(int error, const TCHAR* errorStr);

	};
}

#endif // !__MONITOR_CLIENT_H__

