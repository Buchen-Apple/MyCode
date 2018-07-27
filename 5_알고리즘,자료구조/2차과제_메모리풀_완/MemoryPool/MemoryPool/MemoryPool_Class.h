#ifndef __MEMORYPOOL_CLASS_H__
#define __MEMORYPOOL_CLASS_H__
#include <new.h>
#include <stdlib.h>

#define MEMORYPOOL_ENDCODE	890226

template <typename DATA>
class CMemoryPool
{
private:
	/* **************************************************************** */
	// 각 블럭 앞에 사용될 노드 구조체.
	/* **************************************************************** */
	struct st_BLOCK_NODE
	{
		DATA		  stData;
		st_BLOCK_NODE *stpNextBlock;
		int			  stMyCode;
	};

private:
	int m_iBlockNum;			// 최대 블럭 개수
	bool m_bPlacementNew;		// 플레이스먼트 뉴 여부
	int m_iAllocCount;			// 확보된 블럭 개수. 새로운 블럭을 할당할 때 마다 1씩 증가. 해당 메모리풀이 할당한 메모리 블럭 수
	int m_iUseCount;			// 유저가 사용 중인 블럭 수. Alloc시 1 증가 / free시 1 감소
	st_BLOCK_NODE* m_pTop;		// Top 위치를 가리킬 변수

public:
	//////////////////////////////////////////////////////////////////////////
	// 생성자, 파괴자.
	//
	// Parameters:	(int) 최대 블럭 개수.
	//				(bool) 생성자 호출 여부.
	// Return:
	//////////////////////////////////////////////////////////////////////////
	CMemoryPool(int iBlockNum, bool bPlacementNew = false);
	virtual	~CMemoryPool();


	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다.
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	DATA	*Alloc(void);

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다.
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	bool	Free(DATA *pData);


	//////////////////////////////////////////////////////////////////////////
	// 현재 확보 된 블럭 개수를 얻는다. (메모리풀 내부의 전체 개수)
	//
	// Parameters: 없음.
	// Return: (int) 메모리 풀 내부 전체 개수
	//////////////////////////////////////////////////////////////////////////
	int		GetAllocCount(void) { return m_iAllocCount; }

	//////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 블럭 개수를 얻는다.
	//
	// Parameters: 없음.
	// Return: (int) 사용중인 블럭 개수.
	//////////////////////////////////////////////////////////////////////////
	int		GetUseCount(void) { return m_iUseCount; }
};


//////////////////////////////////////////////////////////////////////////
// 생성자, 파괴자.
//
// Parameters:	(int) 최대 블럭 개수.
//				(bool) 생성자 호출 여부.
// Return:
//////////////////////////////////////////////////////////////////////////
template <typename DATA>
CMemoryPool<DATA>::CMemoryPool(int iBlockNum, bool bPlacementNew)
{
	/////////////////////////////////////
	// bPlacementNew
	// 최초 신규 생성(공통) 1. 메모리 공간 할당(malloc, 생성자 호출 안함). 그 외 노드의 기본 정보 셋팅(next, 내코드)
	// 최초 신규 생성 2. true 시, '객체 생성자' 호출하지 않음.
	// 최초 신규 생성 2. false 시, '객체 생성자' 호출
	// 
	// 이후 Alloc, Free
	// true 1. 유저에게 전달 시, '객체 생성자' 호출 후 전달
	// true 2. 유저에게 받을 시, '객체 소멸자' 호출 후 메모리 풀에 추가
	// true 3. 프로그램 종료 시, 메모리풀 소멸자에서 모든 노드를 돌며 노드의 메모리를 'free()' 시킨다.
	//
	// false 1. 유저에게 전달 시, 그대로 전달
	// false 2. 유저에게 받을 시, 그대로 메모리풀에 추가
	// false 3. 프로그램 종료 시, 메모리풀 소멸자에서 모든 노드를 돌며 노드 DATA의 '객체 소멸자' 호출. 그리고 노드를 'free()' 시킨다.
	//
	// bPlacementNew가 TRUE : Alloc() 시, DATA의 Placement new로 생성자 호출 후 넘김, Free() 시, DATA의 Placement new로 소멸자 호출 후 내 메모리 풀에 넣는다. CMemoryPool소멸자에서, 해당 노드 free
	// bPlacementNew가 FALSE : Alloc() 시, DATA의 생성자 호출 안함. Free()시 소멸자 호출 안함. CMemoryPool소멸자에서, 해당 노드 free
	/////////////////////////////////////

	// 멤버 변수 셋팅	
	m_iBlockNum = iBlockNum;
	m_bPlacementNew = bPlacementNew;
	m_iAllocCount = m_iUseCount = 0;

	// iBlockNum > 0이라면,
	if (iBlockNum > 0)
	{
		// 오브젝트 고려, 총 사이즈 구함.
		int iMemSize = sizeof(st_BLOCK_NODE) * m_iBlockNum;

		// 사이즈 만큼 메모리 동적할당.
		char* pMemory = (char*)malloc(iMemSize);

		// 첫 위치를 Top으로 가리킨다. 
		m_pTop = (st_BLOCK_NODE*)pMemory;

		// 최초 1개의 노드 제작
		st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pMemory;
		if (bPlacementNew == false)
			new (&pNode->stData) DATA();
		pNode->stpNextBlock = NULL;
		pNode->stMyCode = MEMORYPOOL_ENDCODE;
		pMemory += sizeof(st_BLOCK_NODE);

		m_iAllocCount++;

		//  iBlockNum - 1 수 만큼 노드 초기화
		for (int i = 1; i < iBlockNum; ++i)
		{
			st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pMemory;

			if (bPlacementNew == false)
				new (&pNode->stData) DATA();

			if (i == 0)
				pNode->stpNextBlock = NULL;

			else
			{
				pNode->stpNextBlock = m_pTop;
				m_pTop = pNode;
			}

			pNode->stMyCode = MEMORYPOOL_ENDCODE;

			pMemory += sizeof(st_BLOCK_NODE);

			m_iAllocCount++;
		}
	}

	// iBlockNum이 0보다 작거나 같다면, 최초 1개만 만든다.
	else
	{
		st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
		if (bPlacementNew == false)
			new (&pNode->stData) DATA();
		pNode->stpNextBlock = NULL;
		pNode->stMyCode = MEMORYPOOL_ENDCODE;

		m_pTop = pNode;

		m_iAllocCount++;
	}

}

template <typename DATA>
CMemoryPool<DATA>::~CMemoryPool()
{
	// 내 풀에 있는 모든 노드를 동적해제
	// 플레이스먼트 뉴를 사용했기 때문에 delete는 절대 하면 안됨
	// Placement New로 할당한 공간은 절대 delete해서는 안된다.
	// 플레이스먼트 뉴는 그냥 초기화할 뿐, 힙에 공간을 할당하는 개념이 아님.

	// 내 메모리풀에 있는 노드를 모두 'free()' 한다.
	while (1)
	{
		if (m_pTop == NULL)
			break;

		st_BLOCK_NODE* deleteNode = (st_BLOCK_NODE*)m_pTop;
		m_pTop = m_pTop->stpNextBlock;

		// 플레이스먼트 뉴가 false라면, Free()시 '객체 소멸자'를 호출 안한 것이니 여기서 호출시켜줘야 한다.
		if (m_bPlacementNew == false)
			deleteNode->stData.~DATA();

		// malloc 했으니 free 한다.
		free(deleteNode);
	}
}

//////////////////////////////////////////////////////////////////////////
// 블럭 하나를 할당받는다.
//
// Parameters: 없음.
// Return: (DATA *) 데이타 블럭 포인터.
//////////////////////////////////////////////////////////////////////////
template <typename DATA>
DATA*	CMemoryPool<DATA>::Alloc(void)
{
	//////////////////////////////////
	// m_pTop가 NULL일때 처리
	//////////////////////////////////
	if (m_pTop == NULL)
	{
		if (m_iBlockNum > 0)
			return NULL;

		else
		{
			st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
			pNode->stpNextBlock = NULL;
			pNode->stMyCode = MEMORYPOOL_ENDCODE;

			// 플레이스먼트 뉴 사용 여부와 관계 없이, 여기까지 왔는데 m_iBlockNum < 0 이라면, 무조건 새로 생성하는 것.
			// 플레이스먼트 뉴 호출
			new (&pNode->stData) DATA();

			m_iAllocCount++;
			m_iUseCount++;

			return (DATA*)&pNode->stData;
		}
	}

	//////////////////////////////////
	// m_pTop가 NULL이 아닐 때 처리
	//////////////////////////////////
	st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)m_pTop;
	m_pTop = pNode->stpNextBlock;

	// 플레이스먼트 뉴를 사용한다면 사용자에게 주기전에 '객체 생성자' 호출
	if (m_bPlacementNew == true)
		new (&pNode->stData) DATA();

	m_iUseCount++;

	return (DATA*)&pNode->stData;
}

//////////////////////////////////////////////////////////////////////////
// 사용중이던 블럭을 해제한다.
//
// Parameters: (DATA *) 블럭 포인터.
// Return: (BOOL) TRUE, FALSE.
//////////////////////////////////////////////////////////////////////////
template <typename DATA>
bool	CMemoryPool<DATA>::Free(DATA *pData)
{
	// 이상한 포인터가오면 그냥 리턴
	if (pData == NULL)
		return false;

	// 내가 할당한 블럭이 맞는다 확인
	st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pData;

	if (pNode->stMyCode != MEMORYPOOL_ENDCODE)
		return false;

	//플레이스먼트 뉴를 사용한다면 메모리 풀에 추가하기 전에 '객체 소멸자' 호출
	if (m_bPlacementNew == true)
		pData->~DATA();

	// m_pTop에 연결한다.
	pNode->stpNextBlock = m_pTop;
	m_pTop = pNode;

	m_iUseCount--;

	return true;
}


#endif // !__MEMORYPOOL_CLASS_H__
