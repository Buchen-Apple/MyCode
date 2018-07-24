#ifndef __PROTOCOL_BUFF_H__
#define __PROTOCOL_BUFF_H__

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

	private:
		// 초기화
		void Init(int size);

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
		int PutData(const char* pSrc, int size) throw (CException);

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
	};


}

#endif // !__PROTOCOL_BUFF_H__