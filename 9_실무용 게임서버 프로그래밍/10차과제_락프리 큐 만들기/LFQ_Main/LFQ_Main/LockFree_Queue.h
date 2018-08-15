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
		};

		// tail용 구조체
		struct st_NODE_POINT
		{
			st_LFQ_NODE* m_pPoint = nullptr;
			LONG64 m_l64Count = 0;
		};


		// 내부의 노드 수
		LONG m_NodeCount;

		// 메모리풀
		CMemoryPool<st_LFQ_NODE>* m_MPool;

		// 에러났을 때 크래시 내는 용도
		CCrashDump* m_CDump;

		// 시작 노드 가리키는 포인터
		alignas(16) st_NODE_POINT m_stpHead;

		// 마지막 노드 가리키는 포인터
		alignas(16) st_NODE_POINT m_stpTail;



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
		m_stpHead.m_pPoint = m_MPool->Alloc();
		m_stpTail.m_pPoint = m_stpHead.m_pPoint;

		// 테일의 Next는 null이다.			
		m_stpTail.m_pPoint->m_stpNextBlock = nullptr;
	}

	// 소멸자
	template <typename T>
	CLF_Queue<T>::~CLF_Queue()
	{
		// null이 나올때까지 메모리 모두 반납. 헤더를 움직이면서!
		while (m_stpHead.m_pPoint != nullptr)
		{
			st_LFQ_NODE* deleteNode = m_stpHead.m_pPoint;
			m_stpHead.m_pPoint = m_stpHead.m_pPoint->m_stpNextBlock;

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

		// 락프리 구조 (카운터 사용) ------------
		alignas(16) st_NODE_POINT LocalTail;
		st_LFQ_NODE* pLocalNext;

		while (true)
		{
			// tail 스냅샷
			LocalTail.m_pPoint = m_stpTail.m_pPoint;
			LocalTail.m_l64Count = m_stpTail.m_l64Count;

			// tail->Next 스냅샷
			pLocalNext = LocalTail.m_pPoint->m_stpNextBlock;

			// 정말 m_stpTail이 LocalTail와 같다면 로직 진행
			if (LocalTail.m_pPoint == m_stpTail.m_pPoint &&
				LocalTail.m_l64Count == m_stpTail.m_l64Count)
			{

				// 스냅샷으로 찍은 next가 null일때만 로직 진행.
				if (pLocalNext == nullptr)
				{
					// 인큐 작업(노드와 노드 연결 작업)
					if (InterlockedCompareExchangePointer((PVOID*)&LocalTail.m_pPoint->m_stpNextBlock, NewNode, pLocalNext) == pLocalNext)
					{
						// 성공했으면 이제 Tail 이동작업
						// 얘는 그냥 시도한다. 누가 바꿨으면 실패하고 끝.
						InterlockedCompareExchange128((LONG64*)&m_stpTail, LocalTail.m_l64Count + 1, (LONG64)NewNode, (LONG64*)&LocalTail);

						break;
					}
				}

				else
				{
					// null이 아니라면 Tail이동작업 시도
					InterlockedCompareExchange128((LONG64*)&m_stpTail, LocalTail.m_l64Count + 1, (LONG64)pLocalNext, (LONG64*)&LocalTail);
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

		// 락프리 구조 --------------------------
		// 디큐는, 기본적으로 현재 헤드가 가리키고 있는 노드의, 다음 노드의 값을 리턴한다.
		// 헤드는 무조건 더미를 가리키는 것이다.
		alignas(16) st_NODE_POINT LocalHead;
		alignas(16) st_NODE_POINT TempLocalHead;
		alignas(16) st_NODE_POINT LocalTail;
		st_LFQ_NODE* pLocalNext;

		while (true)
		{
			// head 스냅샷
			TempLocalHead.m_pPoint = LocalHead.m_pPoint = m_stpHead.m_pPoint;
			TempLocalHead.m_l64Count = LocalHead.m_l64Count = m_stpHead.m_l64Count;			

			// tail 스냅샷
			LocalTail.m_pPoint = m_stpTail.m_pPoint;
			LocalTail.m_l64Count = m_stpTail.m_l64Count;

			// head->Next 스냅샷
			pLocalNext = LocalHead.m_pPoint->m_stpNextBlock;

			// 헤더가 같을 경우
			if (LocalHead.m_pPoint == m_stpHead.m_pPoint &&
				LocalHead.m_pPoint->m_stpNextBlock == m_stpHead.m_pPoint->m_stpNextBlock)
			{
				// 헤더와 테일이 같은지 체크
				if (LocalHead.m_pPoint == LocalTail.m_pPoint &&
					LocalHead.m_pPoint->m_stpNextBlock == LocalTail.m_pPoint->m_stpNextBlock &&
					pLocalNext == nullptr)
				{
					m_CDump->Crash();
					//return - 1;
				}

				// 헤더와 테일이 다르면 디큐작업 진행
				else if (InterlockedCompareExchange128((LONG64*)&m_stpHead, LocalHead.m_l64Count + 1, (LONG64)pLocalNext, (LONG64*)&LocalHead))
				{
					// tail 이동 시도
					LocalHead.m_pPoint = TempLocalHead.m_pPoint;
					LocalHead.m_l64Count = m_stpTail.m_l64Count;
					InterlockedCompareExchange128((LONG64*)&m_stpTail, LocalHead.m_l64Count + 1, (LONG64)pLocalNext, (LONG64*)&LocalHead);

					// out 매개변수에 데이터 셋팅
					OutData = TempLocalHead.m_pPoint->m_Data;

					// 이동 전, 헤더를 메모리풀에 반환
					m_MPool->Free(TempLocalHead.m_pPoint);
					break;
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
