#include "pch.h"
#include "ProtocolBuff_ObjectPool.h"

namespace Library_Jingyu
{


#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

#define BUFF_SIZE 1024

	// static 메모리풀
	CMemoryPool<CProtocolBuff>* CProtocolBuff::m_MPool = new CMemoryPool<CProtocolBuff>(0, true);

	// 문제 생길 시 Crash 발생시킬 덤프.
	CCrashDump* CProtocolBuff::m_Dump = CCrashDump::GetInstance();

	// 사이즈 지정한 생성자
	CProtocolBuff::CProtocolBuff(int size, bool bPlacementNew)
	{
		Init(size, bPlacementNew);
	}

	// 사이즈 지정 안한 생성자
	CProtocolBuff::CProtocolBuff(bool bPlacementNew)
	{
		Init(BUFF_SIZE, bPlacementNew);
	}

	// 소멸자
	CProtocolBuff::~CProtocolBuff()
	{
		delete[] m_pProtocolBuff;
	}

	// 초기화
	void CProtocolBuff::Init(int size, bool bPlacementNew)
	{
		m_Size = size;		
		m_pProtocolBuff = new char[size];
		m_Front = 0;
		m_Rear = 2; // 처음 앞에 2바이트는 헤더를 넣어야 하기 때문에 rear를 2로 설정해둔다.
		m_RefCount = 0;	// 레퍼런스 카운트 0으로 초기화
	}

	// 버퍼 크기 재설정
	int CProtocolBuff::ReAlloc(int size)
	{
		// 기존 데이터를 temp에 보관시킨 후 m_pProtocolBuff 해제
		char* temp = new char[m_Size];
		memcpy(temp, &m_pProtocolBuff[m_Front], m_Size - m_Front);
		delete[] m_pProtocolBuff;

		// 새로운 크기로 메모리 할당
		m_pProtocolBuff = new char[size];

		// temp에 보관시킨 정보 복사
		memcpy(m_pProtocolBuff, temp, m_Size - m_Front);

		// 버퍼 크기 갱신
		m_Size = size;

		// m_Rear와 m_Front의 위치 변경.
		// m_Rear가 10이었고 m_Front가 3이었으면, m_Rear는 7이되고 m_Front는 0이된다. 즉, 리어와 프론트의 차이는 동일하다.
		m_Rear -= m_Front;
		m_Front = 0;

		return size;
	}

	// 버퍼 클리어
	void CProtocolBuff::Clear()
	{
		m_Front = m_Rear = 0;
	}

	// 데이터 넣기
	int CProtocolBuff::PutData(const char* pSrc, int size)
	{
		// 큐 꽉찼는지 체크
		if (m_Rear == m_Size)
			throw CException(_T("ProtocalBuff(). PutData중 버퍼가 꽉참."));

		// 매개변수 size가 0이면 리턴
		if (size == 0)
			return 0;

		// 메모리 복사
		memcpy(&m_pProtocolBuff[m_Rear], pSrc, size);

		// rear의 위치 이동
		m_Rear += size;

		return size;
	}

	// 데이터 빼기
	int CProtocolBuff::GetData(char* pSrc, int size)
	{
		// 큐 비었나 체크
		if (m_Front == m_Rear)
			throw CException(_T("ProtocalBuff(). GetData중 큐가 비어있음."));

		// 매개변수 size가 0이면 리턴
		if (size == 0)
			return 0;

		// front가 큐의 끝에 도착하면 더 이상 읽기 불가능. 그냥 종료시킨다.
		if (m_Front >= m_Size)
			throw CException(_T("ProtocalBuff(). GetData중 front가 버퍼의 끝에 도착."));
		
		// 메모리 복사
		memcpy(pSrc, &m_pProtocolBuff[m_Front], size);

		// 디큐한 만큼 m_Front이동
		m_Front += size;

		return size;
	}

	// 버퍼의 포인터 얻음.
	char* CProtocolBuff::GetBufferPtr(void)
	{
		return m_pProtocolBuff;
	}

	// Rear 움직이기
	int CProtocolBuff::MoveWritePos(int size)
	{
		// 0사이즈를 이동하려고 하면 리턴
		if (size == 0)
			return 0;

		// 현재 Rear의 위치가 버퍼의 끝이면 더 이상 이동 불가
		if (m_Rear == m_Size)
			return -1;

		// 매개변수로 받은 iSize가 한 번에 이동할 수 있는 크기를 벗어나는지 체크
		int iRealMoveSize;

		if (m_Rear + size > m_Size)
			iRealMoveSize = m_Size - m_Rear;

		// 벗어나지 않는다면 그냥 그 값 사용
		else
			iRealMoveSize = size;

		m_Rear += iRealMoveSize;

		return iRealMoveSize;
		
	}
	
	// Rear를 인자로 받은 값으로 강제로 변경시키기
	int CProtocolBuff::CompulsionMoveWritePos(int size)
	{
		return m_Rear = size;
	}

	// Front 움직이기
	int CProtocolBuff::MoveReadPos(int size)
	{
		// 0사이즈를 삭제하려고 하면 리턴
		if (size == 0)
			return 0;

		// 버퍼가 빈 상태면 0 리턴
		if (m_Front == m_Rear)
			return 0;

		// 한번에 삭제할 수 있는 크기를 벗어나는지 체크
		int iRealRemoveSize;

		if (m_Front + size > m_Size)
			iRealRemoveSize = m_Size - m_Front;

		// 벗어나지 않는다면 그냥 그 크기 사용
		else
			iRealRemoveSize = size;


		m_Front += iRealRemoveSize;

		return iRealRemoveSize;		
	}
	
	// Front를 인자로 받은 값으로 강제로 변경시키기
	int CProtocolBuff::CompulsionMoveReadPos(int size)
	{
		m_Front = size;
	}

	// 현재 사용중인 용량 얻기.
	int	CProtocolBuff::GetUseSize(void)
	{
		//return m_Size - GetFreeSize();

		return m_Rear - m_Front;
	}

	// 현재 버퍼에 남은 용량 얻기.
	int	CProtocolBuff::GetFreeSize(void)
	{
		return m_Size - m_Rear;
	}


	// Alloc. 현재는 이 안에서 new 후 레퍼런스 카운트 1 증가
	CProtocolBuff* CProtocolBuff::Alloc()
	{
		CProtocolBuff* NewAlloc = m_MPool->Alloc();

		//CProtocolBuff* NewAlloc = new CProtocolBuff;
		NewAlloc->m_RefCount++; // 이땐 최초 할당이기때문에 인터락 필요 없음.

		return NewAlloc;
	}

	// Free. 현재는 이 안에서 레퍼런스 카운트 1 감소.
	// 만약, 레퍼런스 카운트가 0이라면 삭제함. delete 함
	void CProtocolBuff::Free(CProtocolBuff* pBuff)
	{
		// 인터락으로 안전하게 감소.
		// 만약 감소 후 0이됐다면 delete
		if (InterlockedDecrement(&pBuff->m_RefCount) == 0)
		{
			if (m_MPool->Free(pBuff) == false)
				m_Dump->Crash();

			//delete pBuff;
		}

	}

	// 레퍼런스 카운트 1 Add하는 함수
	void CProtocolBuff::Add()
	{
		// 인터락으로 안전하게 증가
		InterlockedIncrement(&m_RefCount);
	}








	// 생성자
	CException::CException(const wchar_t* str)
	{
		_tcscpy_s(ExceptionText, _countof(ExceptionText), str);
	}

	// 예외 텍스트의 주소 반환
	char* CException::GetExceptionText()
	{
		return (char*)&ExceptionText;
	}
}