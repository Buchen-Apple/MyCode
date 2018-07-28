#include "stdafx.h"
#include "RingBuff.h"
#include <string.h>

namespace Library_Jingyu
{
	// 사이즈 지정 안한 생성자 1
	CRingBuff::CRingBuff(void)
	{
		Initial(BUF_SIZE);
	}

	// 사이즈 지정한 생성자 2
	CRingBuff::CRingBuff(int iBufferSize)
	{
		Initial(iBufferSize);
	}

	// 사이즈 재지정
	void CRingBuff::Resize(int size)
	{
		delete[] m_Buff;
		Initial(size);
	}

	// 실제 초기화 (큐 준비)
	void CRingBuff::Initial(int iBufferSize)
	{
		m_BuffSize = iBufferSize;
		m_Buff = new char[m_BuffSize];
		m_Front = 0;
		m_Rear = 0;

		// SRW Lock 초기화
		InitializeSRWLock(&sl);

		//InitializeCriticalSection(&cs);
	}

	// 소멸자
	CRingBuff::~CRingBuff()
	{
		//DeleteCriticalSection(&cs);
		delete[] m_Buff;
	}

	// 버퍼의 사이즈 얻기.
	int	CRingBuff::GetBufferSize(void)
	{
		return m_BuffSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 용량.
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::GetUseSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 사용중 공간 계산
		// rear가 더 크다면
		if (TempRear > TempFront)
			return TempRear - TempFront;

		// 둘이 같다면
		else if (TempRear == TempFront)
			return 0;

		// front가 더 크다면
		else
			return m_BuffSize - (TempFront - TempRear);
	}

	/////////////////////////////////////////////////////////////////////////
	// 현재 버퍼에 남은 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)남은용량.
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::GetFreeSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 남은 공간 계산하기
		// rear가 더 크다면
		if (TempRear > TempFront)
			return (m_BuffSize - 1) - (TempRear - TempFront);

		// 둘이 같다면
		else if (TempRear == TempFront)
			return m_BuffSize - 1;

		// m_Front가 더 크다면
		else
			return (TempFront - 1) - TempRear;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	// (끊기지 않은 길이)
	//
	// Parameters: 없음.
	// Return: (int)사용가능 용량.
	////////////////////////////////////////////////////////////////////////
	int	CRingBuff::GetNotBrokenGetSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 한 번에 읽을 수 있는 크기
		// m_Rear와 m_Front가 같으면, 텅 빈상태이니 읽을게 없다.
		if (TempRear == TempFront)
			return 0;

		// rear가 더 크다면.
		if (TempRear > TempFront)
			return TempRear - TempFront;

		// front가 더 크다면
		else
			return m_BuffSize - (TempFront + 1);	
	}

	int	CRingBuff::GetNotBrokenPutSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		//한 번에 쓸 수 있는 크기
		// rear가 더 크거나 같다면
		if (TempRear >= TempFront)
			return m_BuffSize - (TempRear + 1);

		// front가 더 크다면
		else
			return (TempFront - 1) - TempRear;
	}

	// 큐의 다음 인덱스를 체크하기 위한 함수
	int CRingBuff::NextIndex(int iIndex, int iSize)
	{
		if (iIndex + iSize >= m_BuffSize)
			return 0;

		else
			return iIndex += iSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// WritePos 에 데이타 넣음.
	//
	// Parameters: (char *)데이타 포인터. (int)크기. 
	// Return: (int)넣은 크기. 큐가 꽉 찼으면 -1
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::Enqueue(char *chpData, int iSize)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 매개변수 size가 0이면 리턴
		if (iSize == 0)
			return 0;

		// 실제 인큐 사이즈를 저장할 변수
		int iRealEnqueueSize = 0;

		// 인큐 한 사이즈를 저장할 변수 iRealCpySize.
		int iRealCpySize = 0;

		while (iSize > 0)
		{
			// Rear 1칸 이동
			TempRear = NextIndex(TempRear, 1);		

			// 큐 꽉찼는지 체크
			if (TempRear == TempFront)
				return -1;

			// 실제 넣을 수 있는 사이즈를 찾는다.
			if (TempRear >= TempFront)
				iRealEnqueueSize = m_BuffSize - TempRear;
			else
				iRealEnqueueSize = TempFront - TempRear;

			if (iRealEnqueueSize > iSize)
				iRealEnqueueSize = iSize;

			// 메모리 복사
			memcpy(&m_Buff[TempRear], chpData + iRealCpySize, iRealEnqueueSize);

			// rear의 위치 이동
			TempRear = NextIndex(TempRear, iRealEnqueueSize - 1);

			iRealCpySize += iRealEnqueueSize;
			iSize -= iRealEnqueueSize;			
		}

		m_Rear = TempRear;

		return iRealCpySize;
	}

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 가져옴. ReadPos 이동.
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기. 큐가 비었으면 -1
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::Dequeue(char *chpDest, int iSize)
	{
		int TempFront = m_Front;
		int TempRear = m_Rear;

		// 매개변수 size가 0이면 리턴
		if (iSize == 0)
			return 0;

		// 실제 Dequeue 할 사이즈
		int iRealDequeueSIze = 0;

		// Dequeue 한 사이즈를 저장할 변수 iRealCpySize.
		int iRealCpySize = 0;

		while (iSize > 0)
		{
			// 큐 비었나 체크
			if (TempFront == TempRear)
				return -1;

			// 이번에 Dequeue할 수 있는 사이즈를 찾는다.
			// rear가 더 크다면.
			if (TempRear > TempFront)
				iRealDequeueSIze = TempRear - TempFront;

			// front가 더 크다면
			else
				iRealDequeueSIze = m_BuffSize - (TempFront + 1);

			if (iRealDequeueSIze > iSize)
				iRealDequeueSIze = iSize;

			// Front 1칸 앞으로 이동
			TempFront = NextIndex(TempFront, 1);

			// 메모리 복사
			if (iRealDequeueSIze == 0)
				iRealDequeueSIze = 1;

			memcpy_s(chpDest + iRealCpySize, GetFreeSize(), &m_Buff[TempFront], iRealDequeueSIze);
			//memcpy(chpDest + iRealCpySize, &m_Buff[TempFront], iRealDequeueSIze);

			// 디큐한 만큼 m_Front이동
			TempFront = NextIndex(TempFront, iRealDequeueSIze - 1);

			iRealCpySize += iRealDequeueSIze;
			iSize -= iRealDequeueSIze;
		}

		m_Front = TempFront;

		return iRealCpySize;
	}

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 읽어옴. ReadPos 고정. (Front)
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기. 큐가 비었으면 -1
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::Peek(char *chpDest, int iSize)
	{
		// Peek 한 사이즈를 저장할 변수 iRealCpySize.
		int TempFront = m_Front;
		int TempRear = m_Rear;
		int iRealCpySize = 0;

		// 매개변수 size가 0이면 리턴
		if (iSize == 0)
			return 0;

		// 실제 Peek 할 사이즈
		int iRealPeekSIze = 0;		

		while (iSize > 0)
		{
			// 큐 비었나 체크
			if (TempFront == TempRear)
				return -1;

			// 이번에 Peek할 수 있는 사이즈를 찾는다.
			// rear가 더 크다면.
			if (TempRear > TempFront)
				iRealPeekSIze = TempRear - TempFront;

			// front가 더 크다면
			else
				iRealPeekSIze = m_BuffSize - (TempFront + 1);

			if (iRealPeekSIze > iSize)
				iRealPeekSIze = iSize;

			// 임시 Front 1칸 앞으로 이동
			TempFront = NextIndex(TempFront, 1);
			
			// 메모리 복사
			if (iRealPeekSIze == 0)
				iRealPeekSIze = 1;

			memcpy(chpDest + iRealCpySize, &m_Buff[TempFront], iRealPeekSIze);

			// 디큐한 만큼 임시 m_Front이동
			TempFront = NextIndex(TempFront, iRealPeekSIze - 1);
			
			iRealCpySize += iRealPeekSIze;
			iSize -= iRealPeekSIze;
		}

		return iRealCpySize;
	}


	/////////////////////////////////////////////////////////////////////////
	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	//
	// Parameters: 없음.
	// Return: 실제 삭제 및 이동한 사이즈.
	/////////////////////////////////////////////////////////////////////////
	int CRingBuff::RemoveData(int iSize)
	{
		int tempFront = m_Front;

		// 0사이즈를 이동하려고 하면 리턴
		if (iSize == 0)
			return 0;

		// 매개변수로 받은 iSize가 한 번에 이동할 수 있는 크기를 벗어나는지 체크
		int iRealMoveSize;

		if (tempFront + iSize >= m_BuffSize)
			iRealMoveSize = (tempFront + iSize) - m_BuffSize;

		else
			iRealMoveSize = tempFront + iSize;

		m_Front = iRealMoveSize;

		return iRealMoveSize;
	}

	// rear 이동
	int	CRingBuff::MoveWritePos(int iSize)
	{
		int tempRear = m_Rear;

		// 0사이즈를 이동하려고 하면 리턴
		if (iSize == 0)
			return 0;

		if (iSize < 0)
			int abc = 10;

		// 매개변수로 받은 iSize가 한 번에 이동할 수 있는 크기를 벗어나는지 체크
		int iRealMoveSize;

		if (tempRear + iSize >= m_BuffSize)
			iRealMoveSize = (tempRear + iSize) - m_BuffSize;

		else
			iRealMoveSize = tempRear + iSize;

		m_Rear = iRealMoveSize;

		return iRealMoveSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 모든 데이타 삭제.
	//
	// Parameters: 없음.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////
	void CRingBuff::ClearBuffer(void)
	{
		m_Front = m_Rear = 0;
	}

	/////////////////////////////////////////////////////////////////////////
	// 링버퍼 뒤에가 끊겼을때, 0번인덱스부터 front까지의 길이
	//
	// Parameters: 없음.
	// Return: 0~front까지의 사이즈 리턴
	/////////////////////////////////////////////////////////////////////////
	int CRingBuff::GetFrontSize(void)
	{
		return m_Front;
	}


	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetBufferPtr(void)
	{
		return m_Buff;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 ReadPos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetReadBufferPtr(void)
	{
		return (char*)&m_Front;
	}

	/////////////////////////////////////////////////////////////////////////
	// Fornt 1칸 앞 위치의 Buff 주소 반환
	//
	// Parameters: 없음.
	// Return: (char*)&Buff[m_Front 한칸앞]
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetFrontBufferPtr(void)
	{
		return m_Buff + NextIndex(m_Front, 1);	
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 WritePos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetWriteBufferPtr(void)
	{
		return (char*)&m_Rear;
	}

	/////////////////////////////////////////////////////////////////////////
	// Rear 1칸 앞 위치의 Buff 주소 반환
	//
	// Parameters: 없음.
	// Return: (char*)&Buff[m_Rear 한칸앞]
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetRearBufferPtr(void)
	{
		return m_Buff + NextIndex(m_Rear, 1);
	}	

	/////////////////////////////////////////////////////////////////////////
	// 크리티컬 섹션 입장
	//
	// Parameters: 없음.
	// Return: 없음
	/////////////////////////////////////////////////////////////////////////
	void CRingBuff::EnterLOCK()
	{
		//EnterCriticalSection(&cs);
		AcquireSRWLockExclusive(&sl);
	}


	/////////////////////////////////////////////////////////////////////////
	// 크리티컬 섹션 퇴장
	//
	// Parameters: 없음.
	// Return: 없음
	/////////////////////////////////////////////////////////////////////////
	void CRingBuff::LeaveLOCK()
	{
		//LeaveCriticalSection(&cs);
		ReleaseSRWLockExclusive(&sl);
	}

}