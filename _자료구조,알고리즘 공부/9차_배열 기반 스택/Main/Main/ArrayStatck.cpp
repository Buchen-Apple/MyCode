#include "pch.h"
#include "ArrayStack.h"

// 초기화
void arrayStack::Init()
{
	m_iTop = -1;	// 삽입 시, Top 이동 후에 데이터를 넣기 때문에 -1부터 시작.
	m_iSize = 0;
}

// 삽입
bool arrayStack::Push(Data data)
{
	// 배열에 더 이상 공간이 없으면 return false
	if (m_iTop == SIZE - 1)
		return false;

	// 공간이 있으면 데이터 넣기
	m_iTop++;
	m_stackArr[m_iTop] = data;

	m_iSize++;

	return true;
}

// 삭제
bool arrayStack::Pop(Data *pData)
{
	// 배열에 데이터가 하나도 없으면 return false
	if (m_iTop == -1)
		return false;

	// 데이터가 있으면 데이터 리턴
	*pData = m_stackArr[m_iTop];
	m_iTop--;

	m_iSize--;

	return true;
}

// Peek
bool  arrayStack::Peek(Data *pData)
{
	// 배열에 데이터가 하나도 없으면 return false
	if (m_iTop = -1)
		return false;

	// 데이터가 있으면 데이터 리턴
	*pData = m_stackArr[m_iTop];

	return true;
}

// 비었는지 체크
bool arrayStack::IsEmpty()
{
	if (m_iTop == -1)
		return true;

	return false;
}

// 내부의 데이터 수
int arrayStack::GetNodeSize()
{
	return m_iSize;
}