#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__

#include <Windows.h>
#include <stdio.h>
#include <new.h>
#include <nmmintrin.h>


namespace Library_Jingyu
{

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

		// -------------
		// Top으로 사용할 구조체
		struct st_TOP
		{
			st_BLOCK_NODE* m_pTop;
			LONG64 m_l64Count = 0;
		};

	private:
		char* m_Memory;				// 나중에 소멸자에서 한 번에 free하기 위한 변수
		int m_iBlockNum;			// 최대 블럭 개수
		bool m_bPlacementNew;		// 플레이스먼트 뉴 여부
		LONG m_iAllocCount;			// 확보된 블럭 개수. 새로운 블럭을 할당할 때 마다 1씩 증가. 해당 메모리풀이 할당한 메모리 블럭 수
		LONG m_iUseCount;			// 유저가 사용 중인 블럭 수. Alloc시 1 증가 / free시 1 감소
		alignas(16)	st_TOP m_stpTop;

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
		// 할당된 메모리풀의 총 수. 
		//
		// Parameters: 없음.
		// Return: (int) 메모리 풀 내부 전체 개수
		//////////////////////////////////////////////////////////////////////////
		int		GetAllocCount(void) { return m_iAllocCount; }

		//////////////////////////////////////////////////////////////////////////
		// 사용자가 사용중인 블럭 개수를 얻는다.
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

			// pMemory 기억해둔다. 차후 소멸자에서 free하기 위해서
			m_Memory = pMemory;

			// 첫 위치를 Top으로 가리킨다. 			
			m_stpTop.m_pTop = (st_BLOCK_NODE*)pMemory;

			// 최초 1개의 노드 제작
			st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pMemory;
			if (bPlacementNew == false)
				new (&pNode->stData) DATA();

			pNode->stpNextBlock = nullptr;
			pNode->stMyCode = MEMORYPOOL_ENDCODE;
			pMemory += sizeof(st_BLOCK_NODE);

			m_iAllocCount++;

			//  iBlockNum - 1 수 만큼 노드 초기화
			for (int i = 1; i < iBlockNum; ++i)
			{
				st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pMemory;

				if (bPlacementNew == false)
					new (&pNode->stData) DATA();

				pNode->stpNextBlock = m_stpTop.m_pTop;
				m_stpTop.m_pTop = pNode;				

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
			pNode->stpNextBlock = nullptr;
			pNode->stMyCode = MEMORYPOOL_ENDCODE;

			m_stpTop.m_pTop = pNode;

			m_iAllocCount++;
		}

	}

	template <typename DATA>
	CMemoryPool<DATA>::~CMemoryPool()
	{
		// 내 풀에 있는 모든 노드를 동적해제
		// 플레이스먼트 뉴를 사용 했든 안했든 delete는 안됨. 소멸자가 무조건 호출되니까!! free로 메모리 해제해야함. 
		// 플레이스먼트 뉴는 그냥 초기화할 뿐, 힙에 공간을 할당하는 개념이 아님.
		//
		// 플레이스먼트 뉴를 사용한 경우 : 이미, Free() 함수에서 소멸자를 호출했으니 소멸자 호출 안함. 
		// 플레이스먼트 뉴를 사용안한 경우 : 이번에 소멸자를 호출한다. 
		//
		// m_iBlockNum이 0이라면, 낱개로 malloc 한 것이니 낱개로 free 한다. (Alloc시 마다 생성했음. Free()에서는 플레이스먼트 뉴에 따라 소멸자만 호출했었음. 메모리 해제 안했음)
		// m_iBlockNum이 0보다 크다면, 마지막에 한 번에 메모리 전체 free 한다. 


		// 내 메모리풀에 있는 노드를 모두 'free()' 한다.
		while (1)
		{
			if (m_stpTop.m_pTop == nullptr)
				break;

			st_BLOCK_NODE* deleteNode = m_stpTop.m_pTop;
			m_stpTop.m_pTop = m_stpTop.m_pTop->stpNextBlock;

			// 플레이스먼트 뉴가 false라면, Free()시 '객체 소멸자'를 호출 안한 것이니 여기서 호출시켜줘야 한다.
			if (m_bPlacementNew == false)
				deleteNode->stData.~DATA();

			// m_iBlockNum가 0이라면, 낱개로 Malloc했으니(Alloc시 마다 생성했음. Free()에서는 소멸자만 호출하고, 메모리 해제는 안함.) free한다.
			if (m_iBlockNum == 0)
				free(deleteNode);
		}

		//  m_iBlockNum가 0보다 크다면, 한 번에 malloc 한 것이니 한번에 free한다.
		if (m_iBlockNum > 0)
			free(m_Memory);
	}

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다. (Pop)
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	DATA*	CMemoryPool<DATA>::Alloc(void)
	{
		bool bContinueFlag;		

		while (1)
		{
			bContinueFlag = false;

			//////////////////////////////////
			// m_pTop가 NULL일때 처리
			//////////////////////////////////
			if (m_stpTop.m_pTop == nullptr)
			{
				if (m_iBlockNum > 0)
					return nullptr;

				// m_iBlockNum <= 0 라면, 유저가 갯수 제한을 두지 않은것. 
				// 새로 생성한다.
				else
				{
					st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
					pNode->stpNextBlock = NULL;
					pNode->stMyCode = MEMORYPOOL_ENDCODE;

					// 플레이스먼트 뉴 호출
					new (&pNode->stData) DATA();

					// alloc카운트, 유저 사용중 카운트 증가
					InterlockedIncrement(&m_iAllocCount);
					InterlockedIncrement(&m_iUseCount);

					return &pNode->stData;
				}
			}

			//////////////////////////////////
			// m_pTop가 NULL이 아닐 때 처리
			//////////////////////////////////
			alignas(16)  st_TOP localTop;
			// ---- 락프리 적용 ----
			do
			{
				localTop.m_l64Count = m_stpTop.m_l64Count;
				localTop.m_pTop = m_stpTop.m_pTop;				

				// null체크
				if (localTop.m_pTop == nullptr)
				{
					bContinueFlag = true;
					break;
				}						

			} while (!InterlockedCompareExchange128((LONG64*)&m_stpTop, localTop.m_l64Count + 1, (LONG64)localTop.m_pTop->stpNextBlock, (LONG64*)&localTop));


			if (bContinueFlag == true)
				continue;

			// 이 아래에서는 모두 스냅샷으로 찍은 localTop을 사용. 이걸 하는 중에 m_pTop이 바뀔 수 있기 때문에!
			// 플레이스먼트 뉴를 사용한다면 사용자에게 주기전에 '객체 생성자' 호출
			if (m_bPlacementNew == true)
				new (&localTop.m_pTop->stData) DATA();

			// 유저 사용중 카운트 증가. 새로 할당한게 아니기 때문에 Alloc카운트는 변동 없음.
			InterlockedIncrement(&m_iUseCount);

			return &localTop.m_pTop->stData;

		}
		
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다. (Push)
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

		// 내가 할당한 블럭이 맞는지 확인
		st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pData;

		if (pNode->stMyCode != MEMORYPOOL_ENDCODE)
			return false;	

		// 유저 사용중 카운트 감소
		// free(Push) 시에는, 일단 카운트를 먼저 감소시킨다. 그래도 된다! 어차피 락프리는 100%성공 보장.
		InterlockedDecrement(&m_iUseCount);

		//플레이스먼트 뉴를 사용한다면 메모리 풀에 추가하기 전에 '객체 소멸자' 호출
		if (m_bPlacementNew == true)
			pData->~DATA();
		
		// ---- 락프리 적용 ----
		st_BLOCK_NODE* localTop;

		do
		{
			// 로컬 Top 셋팅 
			localTop = m_stpTop.m_pTop;

			// 새로 들어온 노드의 Next를 Top으로 찌름
			pNode->stpNextBlock = localTop;

			// Top이동 시도			
		} while (InterlockedCompareExchange64((LONG64*)&m_stpTop.m_pTop, (LONG64)pNode, (LONG64)localTop) != (LONG64)localTop);

		return true;
	}
}

#endif // !__OBJECT_POOL_H__