#ifndef __PROTOCOL_BUFF_H__
#define __PROTOCOL_BUFF_H__

/*
LanServer 전용 직렬화 버퍼.
NetServer 용 직렬화 버퍼는 따로 제작
*/

#include <Windows.h>
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"

#include <time.h>

namespace Library_Jingyu
{   


	// Recv()패킷 처리 중, 예외 발생 시 던지는 예외클래스이다.
	class CException
	{
	private:
		wchar_t ExceptionText[100];

	public:
		// 생성자
		CException(const wchar_t* str);

		// 예외 텍스트 포인터 반환
		char* GetExceptionText();
	};

	class CProtocolBuff_Lan
	{
		friend class CLanServer;	

	private:
		// ----------- 멤버변수 위치를 잡을 때 '캐시 친화 코드(Cache Friendly Code)' 최대한 적용 고려
		// 이 class에서 핵심 함수는 PutData, GetData, MoveWritePos, MoveReadPos. 
		// 해당 함수의 코드에 맞춰서 멤버변수 배치
		
		// 버퍼 사이즈
		int m_Size;

		// Front
		int m_Front;

		// 레퍼런스 카운트. 소멸 체크하는 카운트
		LONG m_RefCount;

		// Rear
		int m_Rear;
	
		// 직렬화 버퍼
		char* m_pProtocolBuff;		

		// CProtocolBuff를 다루는 메모리풀 (락프리)
		static CMemoryPoolTLS< CProtocolBuff_Lan>* m_MPool;

		// 문제 생길 시 Crash 발생시킬 덤프.
		static CCrashDump* m_Dump;

	private:
		// 헤더를 채우는 함수.
		void SetProtocolBuff_HeaderSet();

	public:
		// 생성자, 소멸자
		CProtocolBuff_Lan(int size);
		CProtocolBuff_Lan();
		~CProtocolBuff_Lan();

		// 버퍼 크기 재설정 (만들어는 뒀지만 현재 쓰는곳 없음)
		int ReAlloc(int size);

		// 버퍼 클리어
		void Clear();

		// 데이터 넣기
		int PutData(const char* pSrc, int size);

		// 데이터 빼기
		int GetData(char* pSrc, int size);

		// 버퍼의 포인터 얻음.
		char* GetBufferPtr(void);

		// Rear 움직이기
		int MoveWritePos(int size);

		// Front 움직이기
		int MoveReadPos(int size);

		// 현재 사용중인 용량 얻기.
		int GetUseSize(void);

		// 현재 버퍼에 남은 용량 얻기.
		int	GetFreeSize(void);

		// operator >>... 데이터 빼기
		template<typename T>
		CProtocolBuff_Lan& operator >> (T& value)
		{
			GetData(reinterpret_cast<char*>(&value), sizeof(T));
			return *this;
		}

		// operator <<... 데이터 넣기.
		template<typename T>
		CProtocolBuff_Lan& operator << (T value)
		{
			PutData(reinterpret_cast<char*>(&value), sizeof(T));
			return *this;
		}

	public:
		// ------
		// 레퍼런스 카운트 관련 함수
		// ------

		// static 함수 -----------
		// Alloc. 현재는 이 안에서 Alloc 후, 레퍼런스 카운트 1 증가
		static CProtocolBuff_Lan* Alloc();

		// Free. 현재는 이 안에서 레퍼런스 카운트 1 감소.
		// 만약, 레퍼런스 카운트가 0이라면 삭제함. TLSPool에 Free 함
		static void Free(CProtocolBuff_Lan* PBuff);

		// 일반 함수 -----------
		// 레퍼런스 카운트 1 Add하는 함수
		void Add();
	};
}

#endif // !__PROTOCOL_BUFF_H__