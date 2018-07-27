#include "stdafx.h"
#include "ProtocolBuff.h"
#include "ExceptionClass.h"
#include <string.h>

#define BUFF_SIZE 1024

// 사이즈 지정한 생성자
CProtocolBuff::CProtocolBuff(int size)
{
	Init(size);
}

// 사이즈 지정 안한 생성자
CProtocolBuff::CProtocolBuff()
{
	Init(BUFF_SIZE);
}

// 소멸자
CProtocolBuff::~CProtocolBuff()
{
	delete m_pProtocolBuff;
}

// 초기화
void CProtocolBuff::Init(int size)
{
	m_Size = size;
	m_pProtocolBuff = new char[m_Size];
	m_Front = 0;
	m_Rear = 0;
}

// 버퍼 크기 재설정
int CProtocolBuff::ReAlloc(int size)
{
	// 기존 데이터를 temp에 보관시킨 후 m_pProtocolBuff 해제
	char* temp = new char[m_Size];
	memcpy(temp, &m_pProtocolBuff[m_Front], m_Size - m_Front);
	delete m_pProtocolBuff;

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
}

// 버퍼 클리어
void CProtocolBuff::Clear()
{
	m_Front = m_Rear = 0;
}

// 큐의 다음 인덱스를 체크하기 위한 함수
int CProtocolBuff::NextIndex(int iIndex, int size)
{
	if (iIndex + size > m_Size)
		return 0;

	else
		return iIndex += size;
}

// 데이터 넣기
int CProtocolBuff::PutData(const char* pSrc, int size)	throw (CException)
{
	// 큐 꽉찼는지 체크
	if (NextIndex(m_Rear, 1) == m_Front)
		throw CException(_T("ProtocalBuff(). PutData중 버퍼가 꽉참."));

	// 매개변수 size가 0이면 리턴
	if (size == 0)
		return 0;

	// 실제 인큐 사이즈를 저장할 변수
	int iRealCpySize;

	// rear가 큐의 끝에 도착하면 더 이상 쓰기 불가능. 그냥 종료시킨다.
	if (m_Rear >= m_Size)
		throw CException(_T("ProtocalBuff(). PutData중 Rear가 버퍼의 끝에 도착."));
	else
		iRealCpySize = size;

	// 메모리 복사
	memcpy(&m_pProtocolBuff[m_Rear], pSrc, iRealCpySize);

	// rear의 위치 이동
	m_Rear = NextIndex(m_Rear, iRealCpySize);

	return iRealCpySize;

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

	// 실제 디큐 사이즈를 저장할 변수
	int iRealCpySize;

	// front가 큐의 끝에 도착하면 더 이상 읽기 불가능. 그냥 종료시킨다.
	if (m_Front >= m_Size)
		throw CException(_T("ProtocalBuff(). GetData중 front가 버퍼의 끝에 도착."));
	else
		iRealCpySize = size;

	if (iRealCpySize == 0)
		iRealCpySize = 1;

	// 메모리 복사
	memcpy(pSrc, &m_pProtocolBuff[m_Front], iRealCpySize);

	// 디큐한 만큼 m_Front이동
	m_Front = NextIndex(m_Front, iRealCpySize);

	return iRealCpySize;
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
	if (NextIndex(m_Rear, 1) == m_Front)
		return -1;

	// 매개변수로 받은 iSize가 한 번에 이동할 수 있는 크기를 벗어나는지 체크
	int iRealMoveSize;

	if (m_Rear + size > m_Size)
		iRealMoveSize = m_Size - m_Rear;

	// 벗어나지 않는다면 그냥 그 값 사용
	else
		iRealMoveSize = size;

	if (iRealMoveSize == 0)
		iRealMoveSize = 1;

	// 실제 이동
	int iReturnSize = 0;

	while (size > 0)
	{
		// Rear의 위치가 버퍼의 끝이에 도착했으면 더 이상 이동 불가
		if (NextIndex(m_Rear, 1) == m_Front)
			break;

		// 포인터 1칸 이동
		m_Rear = NextIndex(m_Rear, 1);

		// 포인터 위치 이동 
		m_Rear = NextIndex(m_Rear, iRealMoveSize - 1);

		// 이동한 만큼 iSize에서 뺀다
		size -= iRealMoveSize;
		iReturnSize += iRealMoveSize;

		// 남은 이동 값 반영
		if (m_Rear + size >= m_Front - 1)
			iRealMoveSize = (m_Front - 1) - m_Rear;
		else
			iRealMoveSize = size;
	}

	return iReturnSize;
}

// Front 움직이기
int CProtocolBuff::MoveReadPos(int size)
{
	// 0사이즈를 삭제하려고 하면 리턴
	if (size == 0)
		return 0;

	// 삭제하려는 크기가, 데이터가 들어있는 크기보다 크거나 같다면, 버퍼 클리어.
	if (GetUseSize() <= size)
	{
		Clear();
		return -1;
	}

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

	// 실제 삭제
	int iReturnSize = 0;

	while (size > 0)
	{
		// 삭제하다가 버퍼가 다 비워졌으면 더 이상 삭제 불가
		if (m_Front == m_Rear)
			break;

		// 포인터 1칸 이동
		m_Front = NextIndex(m_Front, 1);

		// 삭제할 만큼 포인터 위치 이동 
		m_Front = NextIndex(m_Front, iRealRemoveSize - 1);

		// 이동한 만큼 iSize에서 뺀다
		size -= iRealRemoveSize;
		iReturnSize += iRealRemoveSize;
		iRealRemoveSize = size;

	}

	return iReturnSize;
}

// 현재 사용중인 용량 얻기.
int	CProtocolBuff::GetUseSize(void)
{
	return m_Size - GetFreeSize();
}

// 현재 버퍼에 남은 용량 얻기.
int	CProtocolBuff::GetFreeSize(void)
{
	int iSize;

	// 남은 공간 계산하기
	if (m_Rear >= m_Front)
		iSize = m_Size - (m_Rear - m_Front);
	else
		iSize = (m_Front - 1) - m_Rear;

	return iSize;
}