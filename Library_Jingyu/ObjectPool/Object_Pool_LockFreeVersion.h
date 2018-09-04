#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__

#include <Windows.h>
#include <stdio.h>
#include <new.h>
#include "CrashDump\CrashDump.h"


extern LONG g_lAllocNodeCount;
extern LONG g_lFreeNodeCount;

namespace Library_Jingyu
{

#define MEMORYPOOL_ENDCODE	890226

	/////////////////////////////////////////////////////
	// 메모리 풀(오브젝트 풀)
	////////////////////////////////////////////////////
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

		/* **************************************************************** */
		// Top으로 사용할 구조체
		/* **************************************************************** */
		struct st_TOP
		{
			st_BLOCK_NODE* m_pTop;
			LONG64 m_l64Count = 0;
		};

	private:
		// ----------- 멤버변수 위치를 잡을 때, '캐시 친화 코드(Cache Friendly Code)' 최대한 적용 고려
		// 이 class에서 핵심 함수는 Alloc, Free. 해당 함수의 코드에 맞춰서 멤버변수 배치

		char* m_Memory;					// 나중에 소멸자에서 한 번에 free하기 위한 변수		
		alignas(16)	st_TOP m_stpTop;	// Top. 메모리풀은 스택 구조이다.
		int m_iBlockNum;				// 최대 블럭 개수		
		bool m_bPlacementNew;			// 플레이스먼트 뉴 여부
		LONG m_iAllocCount;				// 확보된 블럭 개수. 새로운 블럭을 할당할 때 마다 1씩 증가. 해당 메모리풀이 할당한 메모리 블럭 수
		LONG m_iUseCount;				// 유저가 사용 중인 블럭 수. Alloc시 1 증가 / free시 1 감소

	public:
		//////////////////////////////////////////////////////////////////////////
		// 생성자
		//
		// Parameters:	(int) 최대 블럭 개수.
		//				(bool) 생성자 호출 여부.
		//////////////////////////////////////////////////////////////////////////
		CMemoryPool(int iBlockNum, bool bPlacementNew = false);

		//////////////////////////////////////////////////////////////////////////
		// 파괴자
		//
		// 내부에 있는 모든 노드를 동적해제
		//////////////////////////////////////////////////////////////////////////
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
	// 생성자
	//
	// Parameters:	(int) 최대 블럭 개수.
	//				(bool) 생성자 호출 여부.
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	CMemoryPool<DATA>::CMemoryPool(int iBlockNum, bool bPlacementNew)
	{		
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

	//////////////////////////////////////////////////////////////////////////
	// 파괴자
	//
	// 내부에 있는 모든 노드를 동적해제
	//////////////////////////////////////////////////////////////////////////
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
			// 현재 탑이 nullptr이 될때까지 반복.
			if (m_stpTop.m_pTop == nullptr)
				break;

			// 삭제할 노드를 받아두고, Top을 한칸 이동.
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
				// 락프리에서 유니크함을 보장하는 값은 Count값.
				// 때문에, 루프 시작 전에 무조건 Count를 먼저 받아온다. 
				// top을 먼저 받아올 경우, 컨텍스트 스위칭으로 인해 다른 스레드에서 Count값 변조 후, 변조된 값을 받아올 수도 있다.
				localTop.m_l64Count = m_stpTop.m_l64Count;
				localTop.m_pTop = m_stpTop.m_pTop;	

				// null체크
				if (localTop.m_pTop == nullptr)
				{
					// null이라면, 처음부터 다시 로직을 돌린다.
					// flag를 true로 바꾸면, do while문 밖에서 체크 후 continue 실행
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
	// Return: (bool) true, false.
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

		//플레이스먼트 뉴를 사용한다면 메모리 풀에 추가하기 전에 '객체 소멸자' 호출
		if (m_bPlacementNew == true)
			pData->~DATA();

		// 유저 사용중 카운트 감소
		// free(Push) 시에는, 일단 카운트를 먼저 감소시킨다. 그래도 된다! 어차피 락프리는 100%성공 보장.
		InterlockedDecrement(&m_iUseCount);		
		
		// ---- 락프리 적용 ----
		st_BLOCK_NODE* localTop;

		do
		{
			// 로컬 Top 셋팅 
			// Push는 딱히 카운트가 필요 없다.
			localTop = m_stpTop.m_pTop;

			// 새로 들어온 노드의 Next를 Top으로 찌름
			pNode->stpNextBlock = localTop;

			// Top이동 시도			
		} while (InterlockedCompareExchange64((LONG64*)&m_stpTop.m_pTop, (LONG64)pNode, (LONG64)localTop) != (LONG64)localTop);

		return true;
	}



	// -------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------

	/////////////////////////////////////////////////////
	// 메모리 풀 TLS 버전
	//
	// 내부에서 청크를 다룸
	////////////////////////////////////////////////////
	template <typename DATA>
	class CMemoryPoolTLS
	{	
		struct stChunk;

		// 실제 청크 내에서 관리되는 노드.
		struct Node
		{
			DATA  m_Data;
			stChunk* m_pMyChunk;
			int	stMyCode;
		};

	private:	
		// ----------------------------
		// 청크 구조체
		// ----------------------------
		struct stChunk
		{	

#define NODE_COUNT 200	// 1개의 청크가 다루는 노드의 수			

			// 청크 멤버변수

			// ----------- 멤버변수 위치를 잡을 때, '캐시 친화 코드(Cache Friendly Code)' 최대한 적용 고려
			// 이 struct에서는 상위 클래스인  CMemoryPoolTLS의 Alloc, Free가 핵심 함수. 
			// 해당 함수의 코드에 맞춰서 멤버변수 배치

			LONG m_iTop;					// top 겸 Alloc카운트. 0부터 시작
			LONG m_iFreeRef;				// Free 카운트. 0부터 시작
			Node m_arrayNode[NODE_COUNT];	// 실제 다루는 청크 내부 노드. 정적 배열			
			CCrashDump* m_ChunkDump;		// 에러 발생 시 덤프를 남길 덤프변수.

			// 청크 생성자
			stChunk();

			// 청크 소멸자
			~stChunk();			

			// Alloc카운트 얻기
			int GetAllocCount()
			{	return m_iTop;	}

			// Free 카운트 얻기
			LONG GetFreeCount()
			{	return m_iFreeRef;	}
		};	


	private:
		// ------------------
		// 멤버 변수
		// ------------------
		// ----------- 멤버변수 위치를 잡을 때, '캐시 친화 코드(Cache Friendly Code)' 최대한 적용 고려
		// 이 class에서 핵심 함수는 Alloc, Free. 해당 함수의 코드에 맞춰서 멤버변수 배치

		DWORD m_dwIndex;					// 각 스레드가 사용하는 TLS의 Index
		CMemoryPool<stChunk>* m_ChunkPool;	// TLSPool 내부에서 청크를 다루는 메모리풀 (락프리 구조)
		static bool m_bPlacementNew;		// 청크 내부에서 다루는 노드 안의 데이터의 플레이스먼트 뉴 호출 여부(청크 아님. 청크 내부안의 노드의 데이터에 대해 플레이스먼트 뉴 호출 여부)		
		CCrashDump* m_TLSDump;				// 에러 발생시 덤프를 남길 덤프 변수



	public:
		//////////////////////////////////////////////////////////////////////////
		// 생성자
		//
		// Parameters:	(int) 최대 블럭 개수. (청크의 수)
		//				(bool) 생성자 호출 여부. (청크 내부에서 관리되는 DATA들의 생성자 호출 여부. 청크 생성자 호출여부 아님)
		//////////////////////////////////////////////////////////////////////////
		CMemoryPoolTLS(int iBlockNum, bool bPlacementNew);

		//////////////////////////////////////////////////////////////////////////
		// 파괴자
		// 
		// 청크 메모리풀 해제, TLSIndex 반환.
		//////////////////////////////////////////////////////////////////////////
		virtual ~CMemoryPoolTLS();

		//////////////////////////////////////////////////////////////////////////
		// 블럭 하나를 할당받는다. (Pop)
		//
		// Parameters: 없음.
		// Return: (DATA *) 데이타 블럭 포인터.
		//////////////////////////////////////////////////////////////////////////
		DATA* Alloc();

		//////////////////////////////////////////////////////////////////////////
		// 사용중이던 블럭을 해제한다. (Push)
		//
		// Parameters: (DATA *) 블럭 포인터.
		// Return: 없음
		//////////////////////////////////////////////////////////////////////////
		void Free(DATA* pData);

		// 내부에 있는 청크 수 얻기
		int GetAllocChunkCount()
		{	
			return m_ChunkPool->GetAllocCount(); 
		}

		// 외부에서 사용 중인 청크 수 얻기
		int GetOutChunkCount()
		{	
			return m_ChunkPool->GetUseCount();	
		}



	};	

	template <typename DATA>
	bool CMemoryPoolTLS<DATA>::m_bPlacementNew;

	// ----------------------
	// 
	// CMemoryPoolTLS 내부의 이너 구조체, stChunk.
	// 
	// ----------------------

	// 청크 생성자
	template <typename DATA>
	CMemoryPoolTLS<DATA>::stChunk::stChunk()
	{
		m_ChunkDump = CCrashDump::GetInstance();

		for (int i = 0; i < NODE_COUNT; ++i)
		{
			m_arrayNode[i].m_pMyChunk = this;
			m_arrayNode[i].stMyCode = MEMORYPOOL_ENDCODE;
		}

		m_iTop = 0;
		m_iFreeRef = 0;
	}

	// 청크 소멸자
	template <typename DATA>
	CMemoryPoolTLS<DATA>::stChunk::~stChunk()
	{
		// 플레이스먼트 뉴 여부가 false라면, 청크Free 시, 소멸자가 호출 안된것이니 여기서 소멸자 호출시켜줘야 함
		if (m_bPlacementNew == false)
		{
			for (int i = 0; i < NODE_COUNT; ++i)
			{
				m_arrayNode[i].m_Data.~DATA();
			}
		}
	}

	// ----------------------
	// 
	// CMemoryPoolTLS 부분
	// 
	// ----------------------

	//////////////////////////////////////////////////////////////////////////
	// 생성자
	//
	// Parameters:	(int) 최대 블럭 개수. (청크의 수)
	//				(bool) 생성자 호출 여부. (청크 내부에서 관리되는 DATA들의 생성자 호출 여부. 청크 생성자 호출여부 아님)
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	CMemoryPoolTLS<DATA>::CMemoryPoolTLS(int iBlockNum, bool bPlacementNew)
	{
		// 청크 메모리풀 설정 (청크는 무조건 플레이스먼트 뉴 사용 안함)
		m_ChunkPool = new CMemoryPool<stChunk>(iBlockNum, false);

		// 덤프 셋팅
		m_TLSDump = CCrashDump::GetInstance();

		// 데이터의 플레이스먼트 뉴 여부 저장(청크 아님. 청크 내부에서 관리되는 데이터의 플레이스먼트 뉴 호출 여부)
		m_bPlacementNew = bPlacementNew;

		// TLSIndex 알아오기
		// 앞으로 모든 스레드는 이 인덱스를 사용해 청크 관리.
		// 리턴값이 인덱스 아웃이면 Crash
		m_dwIndex = TlsAlloc();
		if (m_dwIndex == TLS_OUT_OF_INDEXES)
			m_TLSDump->Crash();
	}
	
	//////////////////////////////////////////////////////////////////////////
	// 파괴자
	// 
	// 청크 메모리풀 해제, TLSIndex 반환.
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	CMemoryPoolTLS<DATA>::~CMemoryPoolTLS()
	{
		// 청크 관리 메모리풀 해제
		delete m_ChunkPool;

		// TLS 인덱스 반환
		if (TlsFree(m_dwIndex) == FALSE)
		{
			DWORD Error = GetLastError();
			m_TLSDump->Crash();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 블럭 하나를 할당받는다. (Pop)
	//
	// Parameters: 없음.
	// Return: (DATA *) 데이타 블럭 포인터.
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	DATA* CMemoryPoolTLS<DATA>::Alloc()
	{

		int abc = 0;
		int abcdef = 0;

		// 현재 이 함수를 호출한 스레드의 TLS 인덱스에 청크가 있는지 확인
		stChunk* pChunk = (stChunk*)TlsGetValue(m_dwIndex);

		// TLS에 청크가 없으면 청크 새로 할당함.
		if (pChunk == nullptr)
		{
			pChunk = m_ChunkPool->Alloc();			
			if (TlsSetValue(m_dwIndex, pChunk) == FALSE)
			{
				DWORD Error = GetLastError();
				m_TLSDump->Crash();
			}
			abc = 10;

			if (pChunk->m_iTop != 0 || pChunk->m_iFreeRef != 0)
				abcdef = 10;
		}	

		InterlockedIncrement(&g_lAllocNodeCount);
			
		// 만약, Top이 NODE_COUNT보다 크거나 같다면 뭔가 잘못된것.
		// 이 전에 이미 캐치되었어야 함
		if (pChunk->m_iTop >= NODE_COUNT)
			pChunk->m_ChunkDump->Crash();	

		// 현재 Top의 데이터를 가져온다. 그리고 Top을 1 증가
		DATA* retData = &pChunk->m_arrayNode[pChunk->m_iTop].m_Data;
		pChunk->m_iTop++;

		// 만약, 청크의 데이터를 모두 Alloc했으면, TLS 청크를 NULL로 만든다.
		if (pChunk->m_iTop == NODE_COUNT)
		{
			if (TlsSetValue(m_dwIndex, nullptr) == FALSE)
			{
				DWORD Error = GetLastError();
				pChunk->m_ChunkDump->Crash();
			}
		}

		// 플레이스먼트 뉴 여부에 따라 생성자 호출
		if (m_bPlacementNew == true)
			new (retData) DATA();

		return retData;		
	}

	//////////////////////////////////////////////////////////////////////////
	// 사용중이던 블럭을 해제한다. (Push)
	//
	// Parameters: (DATA *) 블럭 포인터.
	// Return: 없음
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	void CMemoryPoolTLS<DATA>::Free(DATA* pData)
	{	
		// 청크 알아옴
		stChunk* pChunk = ((Node*)pData)->m_pMyChunk;

		// 안전성 검사 -------
		// 이상한 포인터가 오면 그냥 리턴
		if (pData == NULL)
			m_TLSDump->Crash();

		// 내가 할당한 블럭이 맞는지 확인
		if (((Node*)pData)->stMyCode != MEMORYPOOL_ENDCODE)
			m_TLSDump->Crash();

		InterlockedDecrement(&g_lAllocNodeCount);
		//InterlockedIncrement(&g_lFreeNodeCount);

		// FreeRefCount 1 증가
		// 만약 NODE_COUNT가 되면 청크 내부 내용 초기화 후 청크 관리 메모리풀로 Free
		if (InterlockedIncrement(&pChunk->m_iFreeRef) == NODE_COUNT)
		{
			// Free하기전에, 플레이스먼트 뉴를 사용한다면 모든 DATA의 소멸자 호출
			if (m_bPlacementNew == true)
			{
				for (int i = 0; i < NODE_COUNT; ++i)
				{
					pChunk->m_arrayNode[i].m_Data.~DATA();
				}
			}

			// Top과 RefCount 초기화
			pChunk->m_iTop = 0;
			pChunk->m_iFreeRef = 0;

			// 청크 관리 메모리풀로 청크 Free
			if (m_ChunkPool->Free(pChunk) == false)
				m_TLSDump->Crash();
		}
	}




}

#endif // !__OBJECT_POOL_H__