#ifndef __ARRAY_LIST_H__
#define __ARRAY_LIST_H__

#include "NameCard.h"

// 배열 기반 리스트 클래스
// 윤성우 책 버전 (전역을 클래스로만 바꿈)
class ArrayList
{
	typedef NameCard* LData;

	LData* m_Array;;
	int m_iNowIndex;		// 현재 가리키는 노드
	int m_iNodeCount;		// 내부의 노드 수
	int m_iMaxNodeCount;	// 최대 보유 가능한 노드

public:
	// 리스트 초기화.
	void ListInit(int MaxCount);

	// 리스트에 데이터 넣기
	//
	// Parameter : 넣을 데이터.
	// return : 없음
	void ListInsert(LData data);

	// 리스트의 첫 데이터 가져오기
	//
	// Parameter : 데이터를 받아올 포인터
	// return :  데이터가 없을 경우 false.
	//			 데이터가 있을 경우 true 리턴
	bool ListFirst(LData* data);

	// 리스트의 두번째 이후 데이터 가져오기
	//
	// Parameter : 데이터를 받아올 포인터
	// return :  데이터가 없을 경우 false.
	//			 데이터가 있을 경우 true 리턴
	bool ListNext(LData* data);

	// 리스트의 현재 데이터 삭제. (가장 최근에 읽은 데이터)
	//
	// Parameter : 삭제된 데이터를 받아올 포인터
	// return :  데이터가 없을 경우 false.
	//			 데이터가 있을 경우 true 리턴
	bool ListRemove(LData* data);

	// 리스트 내부의 카운트 수
	int ListCount();
};




#endif // !__ARRAY_LIST_H__
