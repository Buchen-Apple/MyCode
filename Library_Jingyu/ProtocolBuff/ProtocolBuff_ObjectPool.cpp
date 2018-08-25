#include "pch.h"
#include "ProtocolBuff_ObjectPool.h"

namespace Library_Jingyu
{


#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

#define BUFF_SIZE 256

	// static 메모리풀
	CMemoryPoolTLS<CProtocolBuff>* CProtocolBuff::m_MPool = new CMemoryPoolTLS<CProtocolBuff>(100, false);

	// 문제 생길 시 Crash 발생시킬 덤프.
	CCrashDump* CProtocolBuff::m_Dump = CCrashDump::GetInstance();

	// 사이즈 지정한 생성자
	CProtocolBuff::CProtocolBuff(int size)
	{		
		m_Size = size;
		m_pProtocolBuff = new char[size];
		m_Front = 0;
		m_Rear = 2; // 처음 앞에 2바이트는 헤더를 넣어야 하기 때문에 rear를 2로 설정해둔다.
		m_RefCount = 1;	// 레퍼런스 카운트 1으로 초기화 (생성되었으니 카운트 1이 되어야 한다.)
	}

	// 사이즈 지정 안한 생성자
	CProtocolBuff::CProtocolBuff()
	{		
		m_Size = BUFF_SIZE;
		m_pProtocolBuff = new char[BUFF_SIZE];
		m_Front = 0;
		m_Rear = 2; // 처음 앞에 2바이트는 헤더를 넣어야 하기 때문에 rear를 2로 설정해둔다.
		m_RefCount = 1;	// 레퍼런스 카운트 1으로 초기화 (생성되었으니 카운트 1이 되어야 한다.)	
	}

	// 소멸자
	CProtocolBuff::~CProtocolBuff()
	{
		delete[] m_pProtocolBuff;
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
		// 큐 꽉찼거나 rear가 Size를 앞질렀는지 체크
		if (m_Rear >= m_Size)
			throw CException(_T("ProtocalBuff(). PutData중 버퍼가 꽉참."));

		// 메모리 복사
		// 1~8바이트 까지는 memcpy보다 대입연산으로 처리한다.
		// 커널모드 전환을 줄여, 속도 증가 기대.
		switch (size)
		{
		case 1:
			*(char *)&m_pProtocolBuff[m_Rear] = *(char *)pSrc;
			break;
		case 2:
			*(short *)&m_pProtocolBuff[m_Rear] = *(short *)pSrc;
			break;
		case 3:
			*(short *)&m_pProtocolBuff[m_Rear] = *(short *)pSrc;
			*(char *)&m_pProtocolBuff[m_Rear + sizeof(short)] = *(char *)&pSrc[sizeof(short)];
			break;
		case 4:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			break;
		case 5:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(char *)&m_pProtocolBuff[m_Rear + sizeof(int)] = *(char *)&pSrc[sizeof(int)];
			break;
		case 6:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(short *)&m_pProtocolBuff[m_Rear + sizeof(int)] = *(short *)&pSrc[sizeof(int)];
			break;
		case 7:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(short *)&m_pProtocolBuff[m_Rear + sizeof(int)] = *(short *)&pSrc[sizeof(int)];
			*(char *)&m_pProtocolBuff[m_Rear + sizeof(int) + sizeof(short)] = *(char *)&pSrc[sizeof(int) + sizeof(short)];
			break;
		case 8:
			*((__int64 *)&m_pProtocolBuff[m_Rear]) = *((__int64 *)pSrc);
			break;

		default:
			memcpy_s(&m_pProtocolBuff[m_Rear], size, pSrc, size);
			break;
		}

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

		// front가 큐의 끝에 도착하면 더 이상 읽기 불가능. 그냥 종료시킨다.
		if (m_Front >= m_Size)
			throw CException(_T("ProtocalBuff(). GetData중 front가 버퍼의 끝에 도착."));
		
		// 메모리 복사
		// 1~8바이트 까지는 memcpy보다 대입연산으로 처리한다.
		// 커널모드 전환을 줄여, 속도 증가 기대.
		switch (size)
		{
		case 1:
			*(char *)pSrc = *(char *)&m_pProtocolBuff[m_Front];
			break;
		case 2:
			*(short *)pSrc = *(short *)&m_pProtocolBuff[m_Front];
			break;
		case 3:
			*(short *)pSrc = *(short *)&m_pProtocolBuff[m_Front];
			*(char *)&pSrc[sizeof(short)] = *(char *)&m_pProtocolBuff[m_Front + sizeof(short)];
			break;
		case 4:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			break;
		case 5:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(char *)&pSrc[sizeof(int)] = *(char *)&m_pProtocolBuff[m_Front + sizeof(int)];
			break;
		case 6:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(short *)&pSrc[sizeof(int)] = *(short *)&m_pProtocolBuff[m_Front + sizeof(int)];
			break;
		case 7:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(short *)&pSrc[sizeof(int)] = *(short *)&m_pProtocolBuff[m_Front + sizeof(int)];
			*(char *)&pSrc[sizeof(int) + sizeof(short)] = *(char *)&m_pProtocolBuff[m_Front + sizeof(int) + sizeof(short)];
			break;
		case 8:
			*((__int64 *)pSrc) = *((__int64 *)&m_pProtocolBuff[m_Front]);
			break;

		default:
			memcpy_s(pSrc, size, &m_pProtocolBuff[m_Front], size);
			break;
		}

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

	// Front 움직이기
	int CProtocolBuff::MoveReadPos(int size)
	{
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

	// 현재 사용중인 용량 얻기.
	int	CProtocolBuff::GetUseSize(void)
	{
		return m_Rear - m_Front;
	}

	// 현재 버퍼에 남은 용량 얻기.
	int	CProtocolBuff::GetFreeSize(void)
	{
		return m_Size - m_Rear;
	}


	// 메모리풀에서 직렬화버퍼 1개 Alloc
	CProtocolBuff* CProtocolBuff::Alloc()
	{
		return  m_MPool->Alloc();;
	}

	// Free. 레퍼런스 카운트 1 감소.
	// 만약, 레퍼런스 카운트가 0이라면 메모리풀에 Free함
	void CProtocolBuff::Free(CProtocolBuff* pBuff)
	{
		// 인터락으로 안전하게 감소.
		// 만약 감소 후 0이됐다면 delete
		if (InterlockedDecrement(&pBuff->m_RefCount) == 0)
		{
			pBuff->m_Rear = 2;		// rear 값 2로 초기화
			pBuff->m_RefCount = 1;	// ref값 1로 초기화
			m_MPool->Free(pBuff);
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
		_tcscpy_s(ExceptionText, _MyCountof(ExceptionText), str);
	}

	// 예외 텍스트의 주소 반환
	char* CException::GetExceptionText()
	{
		return (char*)&ExceptionText;
	}
}