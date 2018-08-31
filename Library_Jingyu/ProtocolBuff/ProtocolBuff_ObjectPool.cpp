#include "pch.h"
#include "ProtocolBuff_ObjectPool.h"

namespace Library_Jingyu
{


#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 직렬화 버퍼 1개의 크기
#define BUFF_SIZE 512

	// LanServer Packet 헤더 사이즈.
#define dfNETWORK_PACKET_HEADER_SIZE 2

	// static 메모리풀
	CMemoryPoolTLS<CProtocolBuff_Lan>* CProtocolBuff_Lan::m_MPool = new CMemoryPoolTLS<CProtocolBuff_Lan>(100, false);

	// 문제 생길 시 Crash 발생시킬 덤프.
	CCrashDump* CProtocolBuff_Lan::m_Dump = CCrashDump::GetInstance();
	   	 
	// 사이즈 지정한 생성자
	CProtocolBuff_Lan::CProtocolBuff_Lan(int size)
	{		
		m_Size = size;
		m_pProtocolBuff = new char[size];
		m_Front = 0;
		m_Rear = dfNETWORK_PACKET_HEADER_SIZE; // 처음 앞에 2바이트는 헤더를 넣어야 하기 때문에 rear를 2로 설정해둔다.
		m_RefCount = 1;	// 레퍼런스 카운트 1으로 초기화 (생성되었으니 카운트 1이 되어야 한다.)
	}

	// 사이즈 지정 안한 생성자
	CProtocolBuff_Lan::CProtocolBuff_Lan()
	{		
		m_Size = BUFF_SIZE;
		m_pProtocolBuff = new char[BUFF_SIZE];
		m_Front = 0;
		m_Rear = dfNETWORK_PACKET_HEADER_SIZE; // 처음 앞에 2바이트는 헤더를 넣어야 하기 때문에 rear를 2로 설정해둔다.
		m_RefCount = 1;	// 레퍼런스 카운트 1으로 초기화 (생성되었으니 카운트 1이 되어야 한다.)	
	}

	// 소멸자
	CProtocolBuff_Lan::~CProtocolBuff_Lan()
	{
		delete[] m_pProtocolBuff;
	}

	// 헤더를 채우는 함수. Lan서버 전용
	void CProtocolBuff_Lan::SetProtocolBuff_HeaderSet()
	{
		// 현재, 헤더는 무조건 페이로드 사이즈. 즉, 8이 들어간다.
		WORD wHeader = GetUseSize() - dfNETWORK_PACKET_HEADER_SIZE;

		*(short *)m_pProtocolBuff = *(short *)&wHeader;	
	}


	// 버퍼 크기 재설정
	// 만들어뒀지만 현재 안쓰는중.. 사용하려면 손 좀 더 봐야함.
	int CProtocolBuff_Lan::ReAlloc(int size)
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
	void CProtocolBuff_Lan::Clear()
	{
		m_Front = m_Rear = 0;
	}

	// 데이터 넣기
	int CProtocolBuff_Lan::PutData(const char* pSrc, int size)
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
			*(char *)&m_pProtocolBuff[m_Rear + 2] = *(char *)&pSrc[2];
			break;
		case 4:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			break;
		case 5:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(char *)&m_pProtocolBuff[m_Rear + 4] = *(char *)&pSrc[4];
			break;
		case 6:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(short *)&m_pProtocolBuff[m_Rear + 4] = *(short *)&pSrc[4];
			break;
		case 7:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(short *)&m_pProtocolBuff[m_Rear + 4] = *(short *)&pSrc[4];
			*(char *)&m_pProtocolBuff[m_Rear + 6] = *(char *)&pSrc[6];
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
	int CProtocolBuff_Lan::GetData(char* pSrc, int size)
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
			*(char *)&pSrc[2] = *(char *)&m_pProtocolBuff[m_Front + 2];
			break;
		case 4:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			break;
		case 5:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(char *)&pSrc[4] = *(char *)&m_pProtocolBuff[m_Front + 4];
			break;
		case 6:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(short *)&pSrc[4] = *(short *)&m_pProtocolBuff[m_Front + 4];
			break;
		case 7:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(short *)&pSrc[4] = *(short *)&m_pProtocolBuff[m_Front + 4];
			*(char *)&pSrc[6] = *(char *)&m_pProtocolBuff[m_Front + 6];
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
	char* CProtocolBuff_Lan::GetBufferPtr(void)
	{
		return m_pProtocolBuff;
	}

	// Rear 움직이기
	int CProtocolBuff_Lan::MoveWritePos(int size)
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
	int CProtocolBuff_Lan::MoveReadPos(int size)
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
	int	CProtocolBuff_Lan::GetUseSize(void)
	{
		return m_Rear - m_Front;
	}

	// 현재 버퍼에 남은 용량 얻기.
	int	CProtocolBuff_Lan::GetFreeSize(void)
	{
		return m_Size - m_Rear;
	}


	// 메모리풀에서 직렬화버퍼 1개 Alloc
	CProtocolBuff_Lan* CProtocolBuff_Lan::Alloc()
	{
		return  m_MPool->Alloc();
	}

	// Free. 레퍼런스 카운트 1 감소.
	// 만약, 레퍼런스 카운트가 0이라면 메모리풀에 Free함
	void CProtocolBuff_Lan::Free(CProtocolBuff_Lan* pBuff)
	{
		// 인터락으로 안전하게 감소.
		// 만약 감소 후 0이됐다면 delete
		if (InterlockedDecrement(&pBuff->m_RefCount) == 0)
		{
			pBuff->m_Rear = dfNETWORK_PACKET_HEADER_SIZE;		// rear 값 2로 초기화. 헤더 영역 확보
			pBuff->m_RefCount = 1;	// ref값 1로 초기화
			m_MPool->Free(pBuff);
		}

	}

	// 레퍼런스 카운트 1 Add하는 함수
	void CProtocolBuff_Lan::Add()
	{
		// 인터락으로 안전하게 증가
		InterlockedIncrement(&m_RefCount);
	}

	// ------------------------------------
	// ------------------------------------
	// ------------------------------------
	// ------------------------------------
	// 예외용

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