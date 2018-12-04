#include "pch.h"
#include "ArrayList.h"

using namespace std;

// 리스트 초기화.
void ArrayList::ListInit(int MaxCount)
{
	m_iMaxNodeCount = MaxCount;

	m_Array = new LData[MaxCount];

	m_iNodeCount = 0;
	m_iNowIndex = -1;
}

// 리스트에 데이터 넣기
//
// Parameter : 넣을 데이터.
// return : 없음
void ArrayList::ListInsert(LData data)
{
	// Max체크
	if (m_iNodeCount == m_iMaxNodeCount)
	{
		cout << "count max" << endl;
		return;
	}

	// Max가 아니면 넣는다.	
	m_Array[m_iNodeCount] = data;
	m_iNodeCount++;
}

// 리스트의 첫 데이터 가져오기
//
// Parameter : 데이터를 받아올 포인터
// return :  데이터가 없을 경우 false.
//			 데이터가 있을 경우 true 리턴
bool ArrayList::ListFirst(LData* data)
{
	// 노드가 하나도 없을 경우
	if (m_iNodeCount == 0)
		return false;

	// 노드가 있을 경우
	m_iNowIndex = 0;
	*data = m_Array[m_iNowIndex];

	return true;
}

// 리스트의 두번째 이후 데이터 가져오기
//
// Parameter : 데이터를 받아올 포인터
// return :  데이터가 없을 경우 false.
//			 데이터가 있을 경우 true 리턴
bool ArrayList::ListNext(LData* data)
{
	// 끝에 도착했을 경우
	if (m_iNowIndex == m_iNodeCount-1)
		return false;

	// 노드가 있을 경우
	m_iNowIndex++;
	*data = m_Array[m_iNowIndex];

	return true;
}

// 리스트의 현재 데이터 삭제. (가장 최근에 읽은 데이터)
//
// Parameter : 삭제된 데이터를 받아올 포인터
// return :  데이터가 없을 경우 false.
//			 데이터가 있을 경우 true 리턴
bool ArrayList::ListRemove(LData* data)
{
	// 노드가 하나도 없을 경우
	if (m_iNodeCount == 0)
		return false;

	// 리턴할 데이터 받아두기
	*data = m_Array[m_iNowIndex];

	// 한칸씩 당겨서 빈공간 채우기
	int TempPos = m_iNowIndex;
	while (TempPos < m_iNodeCount)
	{
		m_Array[TempPos] = m_Array[TempPos + 1];
		TempPos++;
	}

	// 변수들 감소
	m_iNowIndex--;
	m_iNodeCount--;

	return true;
}

// 리스트 내부의 카운트 수
int ArrayList::ListCount()
{
	return m_iNodeCount;
}