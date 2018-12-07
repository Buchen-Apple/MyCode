#include "pch.h"
#include "arrayQueue_Byte.h"
#include <Windows.h>

// 초기화
void arrayQueue_Byte::Init()
{
	m_iFront = 0;
	m_iRear = 0;
	m_iSize = 0;
}

// 인큐
bool arrayQueue_Byte::Enqueue(char* Data, int Size)
{
	// 큐에 공간이 충분한지 체크
	if (GetFreeSize() < Size)
		return false;		

	// 한 번에 인큐 가능한 사이즈 알아오기.
	int EnqSize = GetNotBrokenSize_Enqueue();

	// 모든 사이즈가 한 번에 인큐가 가능할 경우
	if (EnqSize >= Size)
	{
		// mem복사 한번만
		memcpy_s(&m_cBuff[m_iRear], Size, Data, Size);

		// Rear 이동
		m_iRear += Size;
	}

	// 한 번에 인큐 불가능한 경우
	else
	{
		int MoveSize = Size;

		// mem 복사 2회
		memcpy_s(&m_cBuff[m_iRear], EnqSize, Data, EnqSize);

		MoveSize = MoveSize - EnqSize;
		memcpy_s(&m_cBuff[0], MoveSize, &Data[EnqSize], MoveSize);

		// Rear 이동
		m_iRear = MoveSize;
	}

	return true;
}

// 디큐
bool arrayQueue_Byte::Dequeue(char* Data, int Size)
{
	// 큐가 비어있는지 체크
	if (m_iRear == m_iFront)
		return false;	

	// 한 번에 디큐 가능한 사이즈 알아오기.
	int DeqSize = GetNotBrokenSize_Dequeue();

	// 모든 사이즈를 한 번에 디큐 가능한 경우
	if(DeqSize >= Size)
	{
		// mem복사 한번만
		memcpy_s(Data, Size, &m_cBuff[m_iFront], Size);

		// Front 이동
		m_iFront += Size;
	}

	// 한 번에 디큐 불가능한 경우
	else
	{
		int MoveSize = Size;

		// mem 복사 2회
		memcpy_s(Data, DeqSize, &m_cBuff[m_iFront], DeqSize);

		MoveSize = MoveSize - DeqSize;
		memcpy_s(&Data[DeqSize], MoveSize, &m_cBuff[0], MoveSize);

		// Front 이동
		m_iFront = MoveSize;
	}

	return true;
}

// 픽
bool arrayQueue_Byte::Peek(char* Data, int Size)
{

}

// 사용 중인 사이즈
int arrayQueue_Byte::GetUseSize()
{
	// rear가 더 앞서있는 경우
	if (m_iFront < m_iRear)
		return m_iRear - m_iRear;

	// front가 더 앞서있는 경우	
	else if (m_iFront > m_iRear)
		return m_iRear + (BUF_SIZE - m_iFront);

	// 둘이 같다면 텅 빈것. 사용중인 사이즈가 없음
	else
		return 0;
}

// 사용 가능한 사이즈
int arrayQueue_Byte::GetFreeSize()
{
	// rear가 더 앞서있는 경우
	if (m_iFront < m_iRear)
		return m_iFront + ((BUF_SIZE - 1) - m_iRear);

	// front가 더 앞서있는 경우
	else if (m_iFront > m_iRear)
		return (m_iFront - 1) - m_iRear;

	// 둘이 같은 경우에는, 텅 빈것.
	else
		return BUF_SIZE - 1;
}

// 한 번에 인큐 가능한 길이(끊기지 않고)
int arrayQueue_Byte::GetNotBrokenSize_Enqueue()
{
	// Front가 0일경우
	if (m_iFront == 0)
		return (BUF_SIZE - 1) - m_iRear;

	// rear가 더 앞서있거나 같은 경우
	else if (m_iFront <= m_iRear)
		return BUF_SIZE - m_iRear;

	// front가 더 앞서있는 경우	
	else
		return (m_iFront - 1) - m_iRear;
}

// 한 번에 디큐 가능한 길이(끊기지 않고)
int arrayQueue_Byte::GetNotBrokenSize_Dequeue()
{
	// rear가 더 앞서있거나 같은 경우
	if (m_iFront <= m_iRear)
		return m_iRear - m_iFront;

	// front가 더 앞서있는 경우	
	else
		return BUF_SIZE - m_iFront;
}