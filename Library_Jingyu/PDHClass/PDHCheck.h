#ifndef __PDH_CHECK_H__
#define __PDH_CHECK_H__

#include <Windows.h>

#include <Pdh.h>
#include <pdhmsg.h>

#pragma comment(lib, "Pdh.lib")

// ---------------------
// 성능 모니터에서 정보 빼오는 클래스
// PDH 이용
// ---------------------
namespace Library_Jingyu
{

	// 쿼리 1개마다 쿼리날리기, 결과얻기를 반복해야 한다.
	class CPDH
	{
		// -----------------
		// 카운터와 쿼리문
		// -----------------

		// 사용가능 메모리 카운트
		PDH_HCOUNTER	m_pdh_AVA_MEM_Counter;		

		// 논 페이지드 메모리 사용량 카운트
		PDH_HCOUNTER	m_pdh_NonPagedPool_Counter;

		// 네트워크 리시브용 카운트
		PDH_HCOUNTER	m_pdh_Recv_Counter;

		// 네트워크 샌드용 카운트
		PDH_HCOUNTER	m_pdh_Send_Counter;

		// 유저 커밋 메모리 사용량(Private Byte)
		PDH_HCOUNTER	m_pdh_UserCommit;
		


		// -----------------
		// 멤버 변수
		// -----------------	

		// 현재 프로세스의 이름
		TCHAR m_ProcessName[50];

		// PDH에 전송할 쿼리.
		PDH_HQUERY	m_pdh_Query;	

		// 사용가능 메모리 저장
		double m_pdh_AVA_Mem_Value;		

		// 논 페이지드 메모리 저장
		double m_pdh_NonPagedPool_Value;

		// 네트워크 리시브용 저장
		double m_pdh_Recv_Value;

		// 네트워크 샌드용 저장
		double m_pdh_Send_Value;

		// 유저 커밋 메모리 사용량(Private Byte)
		double m_pdh_UserCommit_Value;
		
		// 네트워크 샌드,리시브용 버퍼
		// 해당 정보는 PdhGetFormattedCounterArray로 구해오기 때문에 배열 필요.
		// 길이를 미리 생성자에서 구해둔다.
		PDH_FMT_COUNTERVALUE_ITEM* m_pitems_Recv;
		DWORD m_dwBufferSize_Recv, m_dwItemCount_Recv;

		PDH_FMT_COUNTERVALUE_ITEM *m_pitems_Send;
		DWORD m_dwBufferSize_Send, m_dwItemCount_Send;


	private:
		// -----------------
		// 기능 함수
		// -----------------	

		// 현재 프로세스 이름 셋팅
		// 멤버변수에 저장한다.
		//
		// Parameter : 없음
		// return : 없음
		void SetProcessName();

	public:
		// 생성자
		CPDH();

		// -----------------
		// 게터 함수
		// -----------------	

		// 정보 갱신하기
		// 카운터의 정보 모두 갱신 후, 각 변수에 넣어둔다.
		// 
		// Parameter : 없음
		// return : 없음
		void SetInfo();

		// 사용가능 메모리 얻기 (MByte단위)
		//
		// parameter : 없음
		// return : 하드웨어의 사용가능 메모리
		double Get_AVA_Mem();

		// 논 페이지드 메모리 사용량 얻기 (Byte단위)
		//
		// parameter : 없음
		// return : 하드웨어의 논 페이지드 메모리 사용량
		double Get_NonPaged_Mem();

		// 네트워크 이더넷 Recv 얻기 (Byte)
		//
		// parameter : 없음
		// return : 네트워크 이더넷 Recv
		double Get_Net_Recv();


		// 네트워크 이더넷 Send 얻기 (Byte)
		//
		// parameter : 없음
		// return : 네트워크 이더넷 Send
		double Get_Net_Send();	

		// 유저 커밋 크기 (Byte)
		//
		// parameter : 없음
		// return : 유저 커밋 크기
		double Get_UserCommit();

	};
}

#endif // !__PDH_CHECK_H__
