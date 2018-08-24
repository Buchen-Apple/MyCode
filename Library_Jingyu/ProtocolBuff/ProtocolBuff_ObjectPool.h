#ifndef __PROTOCOL_BUFF_H__
#define __PROTOCOL_BUFF_H__

#include <Windows.h>
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"

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

	class CProtocolBuff
	{
	private:
		// 직렬화 버퍼
		char* m_pProtocolBuff;

		// 버퍼 사이즈
		int m_Size;

		// Front
		int m_Front;

		// Rear
		int m_Rear;

		// 레퍼런스 카운트. 소멸 체크하는 카운트
		LONG m_RefCount;

		// CProtocolBuff를 다루는 메모리풀 (락프리)
		static CMemoryPoolTLS< CProtocolBuff>* m_MPool;

		// 문제 생길 시 Crash 발생시킬 덤프.
		static CCrashDump* m_Dump;


	private:
		// 초기화
		void Init();

	public:
		// 생성자, 소멸자
		CProtocolBuff(int size);
		CProtocolBuff();
		~CProtocolBuff();

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

		// Rear를 인자로 받은 값으로 강제로 변경시키기
		int CompulsionMoveWritePos(int size);

		// Front 움직이기
		int MoveReadPos(int size);

		// Front를 인자로 받은 값으로 강제로 변경시키기
		int CompulsionMoveReadPos(int size);

		// 현재 사용중인 용량 얻기.
		int GetUseSize(void);

		// 현재 버퍼에 남은 용량 얻기.
		int	GetFreeSize(void);

		// operator >>... 데이터 빼기
		template<typename T>
		CProtocolBuff& operator >> (T& value)
		{
			GetData(reinterpret_cast<char*>(&value), sizeof(T));
			return *this;
		}

		// operator <<... 데이터 넣기.
		template<typename T>
		CProtocolBuff& operator << (T value)
		{
			PutData(reinterpret_cast<char*>(&value), sizeof(T));
			return *this;
		}

	public:
		// ------
		// 레퍼런스 카운트 관련 함수
		// ------

		// static 함수 -----------
		// Alloc. 현재는 이 안에서 new 후 레퍼런스 카운트 1 증가
		static CProtocolBuff* Alloc();

		// Free. 현재는 이 안에서 레퍼런스 카운트 1 감소.
		// 만약, 레퍼런스 카운트가 0이라면 삭제함. delete 함
		static void Free(CProtocolBuff* PBuff);

		// 일반 함수 -----------
		// 레퍼런스 카운트 1 Add하는 함수
		void Add();

		static CMemoryPoolTLS< CProtocolBuff>* Test();
	};


}

#endif // !__PROTOCOL_BUFF_H__