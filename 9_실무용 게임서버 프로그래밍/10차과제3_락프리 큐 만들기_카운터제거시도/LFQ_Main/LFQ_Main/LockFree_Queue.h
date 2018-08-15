#ifndef __LOCKFREE_QUEUE_H__
#define __LOCKFREE_QUEUE_H__

#include <Windows.h>
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"
#include <list>

using namespace std;

namespace Library_Jingyu
{
	template <typename T>
	class CLF_Queue
	{
		// 노드
		struct st_LFQ_NODE
		{
			T m_Data;
			st_LFQ_NODE* m_stpNextBlock;

			st_LFQ_NODE& operator=(const st_LFQ_NODE& TempNode)
			{
				m_Data = TempNode.m_Data;
				m_stpNextBlock = TempNode.m_stpNextBlock;
				return *this;
			}

			bool operator==(const st_LFQ_NODE& TempNode)
			{
				return(m_Data == TempNode.m_Data && m_stpNextBlock == TempNode.m_stpNextBlock);
			}
		};

		// 내부의 노드 수
		LONG m_NodeCount;

		// 메모리풀
		CMemoryPool<st_LFQ_NODE>* m_MPool;

		// 에러났을 때 크래시 내는 용도
		CCrashDump* m_CDump;

		// 시작 노드 가리키는 포인터
		st_LFQ_NODE* m_stpHead;

		// 마지막 노드 가리키는 포인터
		st_LFQ_NODE* m_stpTail;



	public:
		// 생성자
		// 내부 메모리풀의 플레이스먼트 뉴 사용여부 인자로 받음. 
		// 디폴트는 false (플레이스먼트 뉴 사용 안함)
		CLF_Queue(bool bPlacementNew = false);

		// 소멸자
		~CLF_Queue();

		// 내부 노드 수 얻기
		LONG GetInNode();

		// Enqueue
		void Enqueue(T data);

		// Dequeue
		// out 매개변수. 값을 채워준다.
		//
		// return -1 : 큐에 할당된 데이터가 없음.
		// return 0 : 매개변수에 성공적으로 값을 채움.
		int Dequeue(T& OutData);

	};

	// 생성자
	// 내부 메모리풀의 플레이스먼트 뉴 사용여부 인자로 받음. 
	// 디폴트는 false (플레이스먼트 뉴 사용 안함)
	template <typename T>
	CLF_Queue<T>::CLF_Queue(bool bPlacementNew)
	{
		// 내부 노드 셋팅. 최초는 0개
		m_NodeCount = 0;

		// 메모리풀 생성자 호출
		m_MPool = new CMemoryPool<st_LFQ_NODE>(0, bPlacementNew);

		// 크래시 내는 용도
		m_CDump = CCrashDump::GetInstance();

		// 최초 생성 시, 더미를 하나 만든다.
		// 만든 더미는 헤더와 테일이 가리킨다.
		// 이 때 만들어진 더미는, 데이터가 없는 더미이다.
		m_stpHead = m_MPool->Alloc();
		m_stpTail = m_stpHead;

		// 테일의 Next는 null이다.			
		m_stpTail->m_stpNextBlock = nullptr;
	}

	// 소멸자
	template <typename T>
	CLF_Queue<T>::~CLF_Queue()
	{
		// null이 나올때까지 메모리 모두 반납. 헤더를 움직이면서!
		while (m_stpHead != nullptr)
		{
			st_LFQ_NODE* deleteNode = m_stpHead;
			m_stpHead = m_stpHead->m_stpNextBlock;

			m_MPool->Free(deleteNode);
		}

		delete m_MPool;
	}

	// 내부 노드 수 얻기
	template <typename T>
	LONG CLF_Queue<T>::GetInNode()
	{
		return m_NodeCount;
	}


	// Enqueue
	template <typename T>
	void CLF_Queue<T>::Enqueue(T data)
	{
		st_LFQ_NODE* NewNode = m_MPool->Alloc();

		NewNode->m_Data = data;
		NewNode->m_stpNextBlock = nullptr;

		st_LFQ_NODE* pLocalTail;
		st_LFQ_NODE* pLocalNext;

		while (true)
		{
			// tail 스냅샷			
			pLocalTail = m_stpTail;

			// Next 스냅샷		
			pLocalNext = pLocalTail->m_stpNextBlock;

			// 락프리 구조 ----------------------------
			// tail이 localtail과 같은지
			if (pLocalTail == m_stpTail && 
				pLocalTail->m_stpNextBlock == m_stpTail->m_stpNextBlock)
			{
				// 이 때 Next가 Null이라면 로직 진행
				if (pLocalNext == nullptr)
				{
					if (InterlockedCompareExchangePointer((PVOID*)&pLocalTail->m_stpNextBlock, NewNode, pLocalNext) == pLocalNext)
					{
						// tail 이동 시도
						InterlockedCompareExchangePointer((PVOID*)&m_stpTail, NewNode, pLocalTail);
						break;
					}
				}

				// Null이 아니라면 tail 이동만 시도
				else
				{
					InterlockedCompareExchangePointer((PVOID*)&m_stpTail, pLocalNext, pLocalTail);
				}
			}

		}

		// 큐 내부의 사이즈 1 증가
		InterlockedIncrement(&m_NodeCount);
	}

	// Dequeue
	// out 매개변수. 값을 채워준다.
	//
	// return -1 : 큐에 할당된 데이터가 없음.
	// return 0 : 매개변수에 성공적으로 값을 채움.
	template <typename T>
	int CLF_Queue<T>::Dequeue(T& OutData)
	{

		// 노드가 없으면 -1 리턴
		if (m_NodeCount == 0)
			return -1;

		// 락프리 구조 ----------------------------
		st_LFQ_NODE* pLocalHead;
		st_LFQ_NODE* pLocalHead_Next;
		st_LFQ_NODE* pLocalTail;
		while (true)
		{
			// 헤더, 헤더->Next, 테일 셋팅
			pLocalHead = m_stpHead;
			pLocalHead_Next = pLocalHead->m_stpNextBlock;
			pLocalTail = m_stpTail;

			// 만약, 헤더가 아직도 진짜 헤더라면
			if (pLocalHead == m_stpHead && 
				pLocalHead->m_stpNextBlock == m_stpHead->m_stpNextBlock)
			{
				// 헤더의 Next가 널인지 체크해야한다.
				// 이 때, 정말 null인지는 헤더와 테일이 같고, next가 null이면 그땐 정말 없는것.
				if (pLocalHead == pLocalTail && 
					pLocalHead->m_stpNextBlock == pLocalTail->m_stpNextBlock && 
					pLocalHead_Next == nullptr)
				{
					m_CDump->Crash();
					//return - 1;
				}

				// 헤더가 null이 아니라면 로직 진행
				else
				{
					// 매개변수에 정보 저장.
					OutData = pLocalHead_Next->m_Data;

					// 헤더 이동 시도
					if (InterlockedCompareExchangePointer((PVOID*)&m_stpHead, pLocalHead_Next, pLocalHead) == pLocalHead)
					{
						// 헤더가 성공적으로 이동되었다면, tail이동 시도
						//InterlockedCompareExchangePointer((PVOID*)&m_stpTail, pLocalHead_Next, pLocalHead);

						// 이전 헤더 반환.
						m_MPool->Free(pLocalHead);
						break;
					}
				}

			}

		}

		// 큐 내부의 사이즈 1 감소
		InterlockedDecrement(&m_NodeCount);

		// 성공했으니 0 리턴
		return 0;
	}

}



#endif // !__LOCKFREE_QUEUE_H__
