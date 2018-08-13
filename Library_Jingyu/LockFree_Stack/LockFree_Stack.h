#ifndef __LOCKFREE_STACK_H__
#define __LOCKFREE_STACK_H__

#include <Windows.h>
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"

namespace Library_Jingyu
{
	template <typename T>
	class CLF_Stack
	{
	private:

		LONG m_NodeCount;		// 리스트 내부의 노드 수

		struct st_LFS_NODE
		{
			T m_Data;
			st_LFS_NODE* m_stpNextBlock;
		};

		struct st_TOP
		{
			st_LFS_NODE* m_pTop = nullptr;
			LONG64 m_l64Count = 0;
		};

		alignas(16)	st_TOP m_stpTop;

		// 에러났을 때 크래시 내는 용도
		CCrashDump* m_CDump;

		// 메모리풀
		CMemoryPool<st_LFS_NODE>* m_MPool;


	public:
		// 생성자
		// 내부 메모리풀의 플레이스먼트 뉴 사용여부 인자로 받음. 
		// 디폴트는 false (플레이스먼트 뉴 사용 안함)
		CLF_Stack(bool bPlacementNew = false);

		// 소멸자
		~CLF_Stack();

		// 내부 노드 수 얻기
		LONG GetInNode();

		// 인덱스를 스택에 추가
		//
		// return : 없음 (void)
		void Push(T Data);


		// 인덱스 얻기
		//
		// 성공 시, T 리턴
		// 실패시 Crash 발생
		T Pop();
	};


	// 생성자
	// 내부 메모리풀의 플레이스먼트 뉴 사용여부 인자로 받음. 
	// 디폴트는 false (플레이스먼트 뉴 사용 안함)
	template <typename T>
	CLF_Stack<T>::CLF_Stack(bool bPlacementNew)
	{
		m_NodeCount = 0;
		m_CDump = CCrashDump::GetInstance();
		m_MPool = new CMemoryPool<st_LFS_NODE>(0, bPlacementNew);
	}

	// 소멸자
	template <typename T>
	CLF_Stack<T>::~CLF_Stack()
	{
		// null이 나올때까지 메모리 모두 반납
		while (m_stpTop.m_pTop != nullptr)
		{
			st_LFS_NODE* deleteNode = m_stpTop.m_pTop;
			m_stpTop.m_pTop = m_stpTop.m_pTop->m_stpNextBlock;

			m_MPool->Free(deleteNode);
		}

		delete m_MPool;
	}

	// 내부 노드 수 얻기
	template <typename T>
	LONG CLF_Stack<T>::GetInNode()
	{
		return m_NodeCount;
	}


	// 인덱스를 스택에 추가
	//
	// return : 없음 (void)
	template <typename T>
	void CLF_Stack<T>::Push(T Data)
	{
		// 새로운 노드 할당받은 후, 데이터 셋팅
		st_LFS_NODE* NewNode = m_MPool->Alloc();
		NewNode->m_Data = Data;

		// ---- 락프리 적용 ----
		alignas(16)  st_TOP localTop;
		do
		{
			// 로컬 Top 셋팅
			localTop.m_pTop = m_stpTop.m_pTop;
			localTop.m_l64Count = m_stpTop.m_l64Count;

			// 신 노드의 Next를 Top으로 설정
			NewNode->m_stpNextBlock = localTop.m_pTop;

			// Top이동 시도
		} while (!InterlockedCompareExchange128((LONG64*)&m_stpTop, localTop.m_l64Count + 1, (LONG64)NewNode, (LONG64*)&localTop));

		// 내부 노드 수 증가.
		InterlockedIncrement(&m_NodeCount);
	}


	// 인덱스 얻기
	//
	// 성공 시, T 리턴
	// 실패시 Crash 발생
	template <typename T>
	T CLF_Stack<T>::Pop()
	{
		// 더 이상 pop 할게 없으면 Crash 발생
		if (m_stpTop.m_pTop == nullptr)
			m_CDump->Crash();

		// 내부 노드 수 감소
		InterlockedDecrement(&m_NodeCount);

		// ---- 락프리 적용 ----
		alignas(16)  st_TOP localTop;
		do
		{
			localTop.m_pTop = m_stpTop.m_pTop;
			localTop.m_l64Count = m_stpTop.m_l64Count;

			// null체크
			if (localTop.m_pTop == nullptr)
			{
				m_CDump->Crash();
			}

		} while (!InterlockedCompareExchange128((LONG64*)&m_stpTop, localTop.m_l64Count + 1, (LONG64)localTop.m_pTop->m_stpNextBlock, (LONG64*)&localTop));	

		// 리턴할 데이터 받아두기
		T retval = localTop.m_pTop->m_Data;

		// 안쓰는것 Free하기
		m_MPool->Free(localTop.m_pTop);

		// 리턴
		return retval;
	}



}


#endif // !__LOCKFREE_STACK_H__
