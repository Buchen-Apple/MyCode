#include "RingBuff.h"
#include <string.h>

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
	delete m_Buff;
	Initial(size);
}

// 실제 초기화 (큐 준비)
void CRingBuff::Initial(int iBufferSize)
{
	m_BuffSize = iBufferSize;
	m_Buff = new char[m_BuffSize];
	m_Front = 0;
	m_Rear = 0;
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
	return m_BuffSize - GetFreeSize();
}

/////////////////////////////////////////////////////////////////////////
// 현재 버퍼에 남은 용량 얻기.
//
// Parameters: 없음.
// Return: (int)남은용량.
/////////////////////////////////////////////////////////////////////////
int	CRingBuff::GetFreeSize(void)
{
	int iSize;

	// 남은 공간 계산하기
	if (m_Rear >= m_Front)
		iSize = m_BuffSize - (m_Rear - m_Front);
	else
		iSize = m_Front - m_Rear;

	return iSize;
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
	// 한 번에 읽을 수 있는 크기

	// m_Rear와 m_Front가 같으면, 텅 빈상태이니 읽을게 없다.
	if (m_Rear == m_Front)
		return 0;

	// 계산을 위해 m_Rear의 위치가 어딘지 구한다.
	// m_Rear가 m_Front보다 크다면 m_Rear가 m_Front기준 >> 이쪽에 있다는 것
	// 이 때는, Front부터 Rear까지 한 번에 읽을 수 있다.
	else if (m_Rear > m_Front)
		return m_Rear - m_Front;

	// 반대로 m_Front가 m_Rear보다 크다면 m_Front가 m_Rear기준 >> 이쪽에 있다는 것
	// 이 때는, Front부터 배열의 끝까지만 한 번에 읽을 수 있다.
	else
		return m_BuffSize - m_Front;
}

int	CRingBuff::GetNotBrokenPutSize(void)
{
	// 한 번에 쓸 수 있는 크기

	// m_Rear + 1이 m_Front와 같다면 꽉 찬 상태이니 더 이상 쓸 수 없다.
	if (NextIndex(m_Rear, 1) == m_Front)
		return 0;

	// 계산을 위해 m_Rear의 위치가 어딘지 구한다.
	// m_Front가 m_Rear보다 크다면 m_Front가 m_Rear기준 >> 이쪽에 있다는 것
	// 이 때는, m_Rear부터 m_Front-1까지 쓸 수 있다. (m_Rear와 m_Front가 같아지면 텅 빈 상태가 되니까!)
	else if (m_Front > m_Rear)
		return (m_Front - 1) - m_Rear;

	// m_Rear가 m_Front보다 크다면 m_Rear가 m_Front기준 >> 이쪽에 있다는 것
	// 이 때는, 배열의 끝 까지만 쓸 수 있다.
	else
		return m_BuffSize - m_Rear;
}

// 큐의 다음 인덱스를 체크하기 위한 함수
int CRingBuff::NextIndex(int iIndex, int iSize)
{
	if (iIndex + iSize == m_BuffSize)
		return 0;

	else
		return iIndex += iSize;
}

/////////////////////////////////////////////////////////////////////////
// WritePos 에 데이타 넣음.
//
// Parameters: (char *)데이타 포인터. (int)크기. 
// Return: (int)넣은 크기.
/////////////////////////////////////////////////////////////////////////
int	CRingBuff::Enqueue(char *chpData, int iSize)
{
	// 큐 꽉찼는지 체크
	if (NextIndex(m_Rear, 1) == m_Front)
		return -1;

	// 매개변수 size가 0이면 리턴
	if (iSize == 0)
		return 0;

	// 실제 인큐 사이즈를 저장할 변수
	int iRealCpySize;

	// 일단 Rear 1칸 이동
	m_Rear = NextIndex(m_Rear, 1);

	// 큐의 끝 인덱스를 체크해, 실제 큐에 넣을 수 있는 사이즈를 찾는다.
	if (m_Rear + iSize > m_BuffSize)
		iRealCpySize = m_BuffSize - m_Rear;
	else
		iRealCpySize = iSize;

	// 메모리 복사
	memcpy(&m_Buff[m_Rear], chpData, iRealCpySize);

	// rear의 위치 이동
	m_Rear = NextIndex(m_Rear, iRealCpySize - 1);

	return iRealCpySize;
}

/////////////////////////////////////////////////////////////////////////
// ReadPos 에서 데이타 가져옴. ReadPos 이동.
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int	CRingBuff::Dequeue(char *chpDest, int iSize)
{
	// 큐 비었나 체크
	if (m_Front == m_Rear)
		return -1;

	// 매개변수 size가 0이면 리턴
	if (iSize == 0)
		return 0;

	// 실제 디큐 사이즈를 저장할 변수
	int iRealCpySize;

	// 일단 Front 1칸 이동
	m_Front = NextIndex(m_Front, 1);

	// 큐의 끝을 체크해, 실제 디큐할 수 있는 사이즈를 찾는다.
	if (m_Front + iSize > m_BuffSize)
		iRealCpySize = m_BuffSize - m_Front;
	else
		iRealCpySize = iSize;

	// 메모리 복사
	memcpy(chpDest, &m_Buff[m_Front], iRealCpySize);

	// 디큐한 만큼 m_Front이동
	m_Front = NextIndex(m_Front, iRealCpySize - 1);

	return iRealCpySize;
}

/////////////////////////////////////////////////////////////////////////
// ReadPos 에서 데이타 읽어옴. ReadPos 고정. (Front)
//
// Parameters: (char *)데이타 포인터. (int)크기.
// Return: (int)가져온 크기.
/////////////////////////////////////////////////////////////////////////
int	CRingBuff::Peek(char *chpDest, int iSize)
{
	// 큐 비었나 체크
	if (m_Front == m_Rear)
		return -1;

	// 매개변수 size가 0이면 리턴
	if (iSize == 0)
		return 0;

	// 실제 디큐 사이즈를 저장할 변수
	int iRealCpySize;

	// Peek에 사용할 임시 Front 1칸 이동
	int TempFront = NextIndex(m_Front, 1);

	// 큐의 끝을 체크해, 실제 디큐할 수 있는 사이즈를 찾는다.
	if (TempFront + iSize > m_BuffSize)
		iRealCpySize = m_BuffSize - TempFront;
	else
		iRealCpySize = iSize;

	// 메모리 복사
	memcpy(chpDest, &m_Buff[TempFront], iRealCpySize);

	return iRealCpySize;
}


/////////////////////////////////////////////////////////////////////////
// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
//
// Parameters: 없음.
// Return: 없음.
/////////////////////////////////////////////////////////////////////////
int CRingBuff::RemoveData(int iSize)
{
	// 0사이즈를 삭제하려고 하면 리턴
	if (iSize == 0)
		return 0;

	// 삭제하려는 크기가, 데이터가 들어있는 크기보다 크거나 같다면, 버퍼 클리어.
	if (GetUseSize() <= iSize)
	{
		ClearBuffer();
		return -1;
	}

	// 버퍼가 빈 상태면 0 리턴
	if (m_Front == m_Rear)
		return 0;

	// 한번에 삭제할 수 있는 크기를 벗어나는지 체크
	int iRealRemoveSize;

	if (m_Front + iSize > m_BuffSize)
		iRealRemoveSize = m_BuffSize - m_Front;

	// 벗어나지 않는다면 그냥 그 크기 사용
	else
		iRealRemoveSize = iSize;

	// 실제 삭제
	int iReturnSize = 0;

	while (iSize > 0)
	{
		// 삭제하다가 버퍼가 다 비워졌으면 더 이상 삭제 불가
		if (m_Front == m_Rear)
			break;

		// 포인터 1칸 이동
		m_Front = NextIndex(m_Front, 1);

		// 포인터 위치 이동 
		m_Front = NextIndex(m_Front, iRealRemoveSize - 1);

		// 이동한 만큼 iSize에서 뺀다
		iSize -= iRealRemoveSize;
		iReturnSize += iRealRemoveSize;

		// 남은 이동 값 반영
		if (m_Front + iSize >= m_Rear)
			iRealRemoveSize = m_Rear - m_Front;
		else
			iRealRemoveSize = iSize;

	}

	return iReturnSize;
}

int	CRingBuff::MoveWritePos(int iSize)
{
	// 0사이즈를 이동하려고 하면 리턴
	if (iSize == 0)
		return 0;

	// 버퍼의 끝에 도착했으면 더 이상 이동 불가
	if (NextIndex(m_Rear, 1) == m_Front)
		return -1;

	// 한 번에 이동할 수 있는 크기를 벗어나는지 체크
	int iRealMoveSize;

	if (m_Rear + iSize > m_BuffSize)
		iRealMoveSize = m_BuffSize - m_Rear;

	// 벗어나지 않는다면 그냥 그 값 사용
	else
		iRealMoveSize = iSize;

	// 실제 이동
	int iReturnSize = 0;

	while (iSize > 0)
	{
		// 버퍼의 끝에 도착했으면 더 이상 이동 불가
		if (NextIndex(m_Rear, 1) == m_Front)
			break;

		// 포인터 1칸 이동
		m_Rear = NextIndex(m_Rear, 1);

		// 포인터 위치 이동 
		m_Rear = NextIndex(m_Rear, iRealMoveSize - 1);

		// 이동한 만큼 iSize에서 뺀다
		iSize -= iRealMoveSize;
		iReturnSize += iRealMoveSize;

		// 남은 이동 값 반영
		if (m_Rear + iSize >= m_Front - 1)
			iRealMoveSize = (m_Front - 1) - m_Rear;
		else
			iRealMoveSize = iSize;
	}

	return iReturnSize;

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
// 버퍼의 WritePos 포인터 얻음.
//
// Parameters: 없음.
// Return: (char *) 버퍼 포인터.
/////////////////////////////////////////////////////////////////////////
char* CRingBuff::GetWriteBufferPtr(void)
{
	return (char*)&m_Rear;
}