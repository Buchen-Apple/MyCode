#ifndef __LOCKFREE_QUEUE_H__
#define __LOCKFREE_QUEUE_H__

#include <Windows.h>
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"
#include <list>

using namespace std;

#define LOG_COUNT 160000
#define LOG_CLEAR_TIME	100


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

		// 메모리 로깅용
		struct st_Trace
		{
			// flag 
			// 1: 인큐에서 라인 연결 시도, 2 : 성공
			// 3: 인큐에서 tail이동 시도, 4 : 성공
			// 5: 디큐에서 tail 이동 시도, 6 : 성공
			// 7: 디큐에서 헤더 이동 시도, 8: 성공
			// 9: 인큐에서 tail이동 시도2, 10 : 성공2
			BOOL m_bFlag;

			ULONGLONG m_TempCount = 0;

			// 헤더, 로컬헤더 주소
			st_LFQ_NODE* m_head;
			st_LFQ_NODE* m_head_Next;
			st_LFQ_NODE* m_Localhead;

			// 테일, 로컬테일 주소
			st_LFQ_NODE* m_tail;
			st_LFQ_NODE* m_tail_Next;
			st_LFQ_NODE* m_Localtail;

			// 이번에 Tail과 연결할 newNode
			st_LFQ_NODE* m_newNode;
		};

		// 추적용
		list<st_Trace> TraceArray[3];
		ULONGLONG TempCount = 0;
		ULONGLONG Time[3];

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
		void Enqueue(T data, int ID);

		// Dequeue
		// out 매개변수. 값을 채워준다.
		//
		// return -1 : 큐에 할당된 데이터가 없음.
		// return 0 : 매개변수에 성공적으로 값을 채움.
		int Dequeue(T& OutData, int ID);

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
		m_stpHead.m_pPoint->m_stpNextBlock = nullptr;

		m_stpTail.m_pPoint = m_stpHead.m_pPoint;

		// 추적용 !!
		Time[1] = Time[0] = Time[2] = GetTickCount64();
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
	void CLF_Queue<T>::Enqueue(T data, int ID)
	{
		ULONGLONG TempTime = GetTickCount64();
		if (Time[ID] + LOG_CLEAR_TIME <= TempTime)
		{
			TraceArray[ID].clear();
			Time[ID] = TempTime;
		}

		st_LFQ_NODE* NewNode = m_MPool->Alloc();

		NewNode->m_Data = data;
		NewNode->m_stpNextBlock = nullptr;

		// 락프리 구조 (카운터 사용) ------------
		alignas(16) st_NODE_POINT LocalTail;

		while (true)
		{
			// tail 스냅샷
			LocalTail.m_pPoint = m_stpTail.m_pPoint;
			LocalTail.m_l64Count = m_stpTail.m_l64Count;

			// 정말 m_stpTail이 LocalTail와 같다면 로직 진행
			if (LocalTail.m_pPoint == m_stpTail.m_pPoint)
			{
				// Next가 Null이면 로직 진행
				if (LocalTail.m_pPoint->m_stpNextBlock == nullptr)
				{
					// 연결 시도 로그
					st_Trace Trace;
					Trace.m_bFlag = 1;

					Trace.m_head = m_stpHead.m_pPoint;
					Trace.m_tail = m_stpTail.m_pPoint;

					Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
					Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

					Trace.m_Localhead = nullptr;
					Trace.m_Localtail = LocalTail.m_pPoint;

					Trace.m_newNode = NewNode;

					Trace.m_TempCount = InterlockedIncrement(&TempCount);

					TraceArray[ID].push_back(Trace);

					if (InterlockedCompareExchange64((LONG64*)&m_stpTail.m_pPoint->m_stpNextBlock, (LONG64)NewNode, (LONG64)nullptr) == (LONG64)nullptr)
					{
						// 연결 성공 로그
						st_Trace Trace;
						Trace.m_bFlag = 2;

						Trace.m_head = m_stpHead.m_pPoint;
						Trace.m_tail = m_stpTail.m_pPoint;

						Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
						Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

						Trace.m_Localhead = nullptr;
						Trace.m_Localtail = LocalTail.m_pPoint;

						Trace.m_newNode = NewNode;

						Trace.m_TempCount = InterlockedIncrement(&TempCount);

						TraceArray[ID].push_back(Trace);



						// 성공했으면 이제 Tail 이동작업
						// 얘는 그냥 시도한다. 누가 바꿨으면 실패하고 끝.		

						// 이동 시도 로그
						Trace.m_bFlag = 3;

						Trace.m_head = m_stpHead.m_pPoint;
						Trace.m_tail = m_stpTail.m_pPoint;

						Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
						Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

						Trace.m_Localhead = nullptr;
						Trace.m_Localtail = LocalTail.m_pPoint;

						Trace.m_newNode = NewNode;

						Trace.m_TempCount = InterlockedIncrement(&TempCount);

						TraceArray[ID].push_back(Trace);

						if (InterlockedCompareExchange128((LONG64*)&m_stpTail, LocalTail.m_l64Count + 1, (LONG64)NewNode, (LONG64*)&LocalTail) == TRUE)
						{
							// 이동 성공 로그
							st_Trace Trace;
							Trace.m_bFlag = 4;

							Trace.m_head = m_stpHead.m_pPoint;
							Trace.m_tail = m_stpTail.m_pPoint;

							Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
							Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

							Trace.m_Localhead = nullptr;
							Trace.m_Localtail = LocalTail.m_pPoint;

							Trace.m_newNode = NewNode;

							Trace.m_TempCount = InterlockedIncrement(&TempCount);

							TraceArray[ID].push_back(Trace);
						}

						break;
					}
				}

				else
				{
					// null이 아니라면 Tail이동작업 시도

					// 이동 시도 로그
					st_Trace Trace;
					Trace.m_bFlag = 9;

					Trace.m_head = m_stpHead.m_pPoint;
					Trace.m_tail = m_stpTail.m_pPoint;

					Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
					Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

					Trace.m_Localhead = nullptr;
					Trace.m_Localtail = LocalTail.m_pPoint;

					Trace.m_newNode = NewNode;

					Trace.m_TempCount = InterlockedIncrement(&TempCount);

					TraceArray[ID].push_back(Trace);

					if (InterlockedCompareExchange128((LONG64*)&m_stpTail, LocalTail.m_l64Count + 1, (LONG64)LocalTail.m_pPoint->m_stpNextBlock, (LONG64*)&LocalTail) == TRUE)
					{
						// 이동 성공 로그
						st_Trace Trace;
						Trace.m_bFlag = 10;

						Trace.m_head = m_stpHead.m_pPoint;
						Trace.m_tail = m_stpTail.m_pPoint;

						Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
						Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

						Trace.m_Localhead = nullptr;
						Trace.m_Localtail = LocalTail.m_pPoint;

						Trace.m_newNode = NewNode;

						Trace.m_TempCount = InterlockedIncrement(&TempCount);

						TraceArray[ID].push_back(Trace);
					}
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
	int CLF_Queue<T>::Dequeue(T& OutData, int ID)
	{
		ULONGLONG TempTime = GetTickCount64();
		if (Time[ID] + LOG_CLEAR_TIME <= TempTime)
		{
			TraceArray[ID].clear();
			Time[ID] = TempTime;
		}

		// 큐 내부의 사이즈 1 감소
		// 노드가 없으면 -1 리턴
		if (InterlockedDecrement(&m_NodeCount) < 0)
			return -1;

		// 락프리 구조 --------------------------
		// 디큐는, 기본적으로 현재 헤드가 가리키고 있는 노드의, 다음 노드의 값을 리턴한다.
		// 헤드는 무조건 더미를 가리키는 것이다.
		alignas(16) st_NODE_POINT LocalHead, LocalTail;
		st_LFQ_NODE *pDeleteHead;

		while (true)
		{
			// head 스냅샷
			pDeleteHead = LocalHead.m_pPoint = m_stpHead.m_pPoint;
			LocalHead.m_l64Count = m_stpHead.m_l64Count;

			// tail 스냅샷
			LocalTail.m_pPoint = m_stpTail.m_pPoint;
			LocalTail.m_l64Count = m_stpTail.m_l64Count;

			// 정말 m_stpHead이 LocalHead와 같다면 로직 진행
			if (m_stpHead.m_pPoint == pDeleteHead)
			{
				// 헤더와 테일이 같으면서
				if (pDeleteHead == LocalTail.m_pPoint)
				{
					// pLocalNext가 null인지 체크
					if (pDeleteHead->m_stpNextBlock == nullptr)
					{
						continue;
						//m_CDump->Crash();
						//return - 1;
					}

					else
					{
						// tail 이동 시도	

						// 이동 시도 로그
						st_Trace Trace;
						Trace.m_bFlag = 5;

						Trace.m_head = m_stpHead.m_pPoint;
						Trace.m_tail = m_stpTail.m_pPoint;

						Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
						Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

						Trace.m_Localhead = LocalHead.m_pPoint;
						Trace.m_Localtail = LocalTail.m_pPoint;

						Trace.m_newNode = nullptr;

						Trace.m_TempCount = InterlockedIncrement(&TempCount);

						TraceArray[ID].push_back(Trace);

						if (InterlockedCompareExchange128((LONG64*)&m_stpTail, LocalTail.m_l64Count + 1, (LONG64)LocalTail.m_pPoint->m_stpNextBlock, (LONG64*)&LocalTail) == TRUE)
						{
							// 이동 성공 로그
							st_Trace Trace;
							Trace.m_bFlag = 6;

							Trace.m_head = m_stpHead.m_pPoint;
							Trace.m_tail = m_stpTail.m_pPoint;

							Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
							Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

							Trace.m_Localhead = LocalHead.m_pPoint;
							Trace.m_Localtail = LocalTail.m_pPoint;

							Trace.m_newNode = nullptr;

							Trace.m_TempCount = InterlockedIncrement(&TempCount);

							TraceArray[ID].push_back(Trace);
						}
					}
				}

				// 헤더와 테일이 다르면 디큐작업 진행
				else
				{
					// 헤더 이동 시도

					// 이동 시도 로그
					st_Trace Trace;
					Trace.m_bFlag = 7;

					Trace.m_head = m_stpHead.m_pPoint;
					Trace.m_tail = m_stpTail.m_pPoint;

					Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
					Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

					Trace.m_Localhead = LocalHead.m_pPoint;
					Trace.m_Localtail = LocalTail.m_pPoint;

					Trace.m_newNode = nullptr;

					Trace.m_TempCount = InterlockedIncrement(&TempCount);

					TraceArray[ID].push_back(Trace);

					if (InterlockedCompareExchange128((LONG64*)&m_stpHead, LocalHead.m_l64Count + 1, (LONG64)LocalHead.m_pPoint->m_stpNextBlock, (LONG64*)&LocalHead))
					{
						// 이동 성공 로그
						st_Trace Trace;
						Trace.m_bFlag = 8;

						Trace.m_head = m_stpHead.m_pPoint;
						Trace.m_tail = m_stpTail.m_pPoint;

						Trace.m_head_Next = Trace.m_head->m_stpNextBlock;
						Trace.m_tail_Next = Trace.m_tail->m_stpNextBlock;

						Trace.m_Localhead = LocalHead.m_pPoint;
						Trace.m_Localtail = LocalTail.m_pPoint;

						Trace.m_newNode = nullptr;

						Trace.m_TempCount = InterlockedIncrement(&TempCount);

						TraceArray[ID].push_back(Trace);

						// out 매개변수에 데이터 셋팅
						OutData = pDeleteHead->m_stpNextBlock->m_Data;

						// 이동 전의 헤더를 메모리풀에 반환
						m_MPool->Free(pDeleteHead);
						break;
					}
				}
			}
		}

		// 성공했으니 0 리턴
		return 0;
	}

}



#endif // !__LOCKFREE_QUEUE_H__
