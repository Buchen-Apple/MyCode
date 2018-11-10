#ifndef __NORMAL_QUEUE_TEMPLATE_H__
#define __NORMAL_QUEUE_TEMPLATE_H__

#include <windows.h>
#include "ObjectPool\Object_Pool_LockFreeVersion.h"
#include "CrashDump\CrashDump.h"

// --------------------------
// 일반 큐 (템플릿). 리스트 구조
// 스레드 안전하지 않음.
// --------------------------
namespace Library_Jingyu
{
	template <typename DATA>
	class CNormalQueue
	{
		// --------------------
		// 이너 클래스
		// --------------------

		// 노드 구조체
		struct stNode
		{
			DATA m_data;
			stNode* m_Next;		// 다음
		};


		// --------------------
		// 멤버 변수 
		// --------------------

		// 노드 메모리풀 (락프리 버전)
		CMemoryPool<stNode>* m_NodePool;

		// 헤더와 테일
		stNode* m_pHead;
		stNode* m_pTail;

		// 큐 안의 노드 수. (더미 제외)
		LONG m_iSize;		

		// 크래시용
		CCrashDump* m_dump;	
		

	public:
		// --------------------
		// 멤버 함수
		// --------------------

		// 생성자
		CNormalQueue();

		// 소멸자
		~CNormalQueue();

		// Enqueue
		// 
		// Parameters: (DATA) 넣을 데이터
		// Return: 없음. 내부에서 실패 시 Crash
		void Enqueue(DATA Data);

		// Dequeue
		//
		// Parameters: (DATA&) 뺄 데이터를 받을 포인터
		// return -1 : 큐에 할당된 데이터가 없음.
		// return 0 : 매개변수에 성공적으로 값을 채움.
		int Dequeue(DATA& Data);

		// 큐 사이즈 얻기(더미 노드 제외한 사이즈)
		//
		// Parameter : 없음
		// return : (int)큐 사이즈.
		int GetNodeSize()
		{	return m_iSize;	}

	};

	// 생성자
	template <typename DATA>
	CNormalQueue<DATA>::CNormalQueue()
	{
		// 노드 메모리풀 동적할당
		m_NodePool = new CMemoryPool<stNode>(0, false);

		// 덤프용
		m_dump = CCrashDump::GetInstance();

		// 헤더, 테일 셋팅. 
		// 최초는 더미노드.
		m_pHead = m_NodePool->Alloc();
		m_pHead->m_Next = nullptr;

		m_pTail = m_pHead;

		// 큐 사이즈 0으로 셋팅
		m_iSize = 0;		
	}

	// 소멸자
	template <typename DATA>
	CNormalQueue<DATA>::~CNormalQueue()
	{
		// 1. 큐의 노드 전부 다 노드 메모리풀로 delete
		stNode* NowNode = m_pHead;
		while (1)
		{
			// 일단 1개 반환. 생성자에서 1개 받기 때문에, 반환부터 해도 된다.
			m_NodePool->Free(NowNode);

			// 마지막 노드라면, break;
			if (NowNode->m_Next == nullptr)
				break;

			// 마지막 노드가 아니라면 다음으로 이동
			NowNode = NowNode->m_Next;
		}

		// 2. 노드 메모리풀 delete
		delete m_NodePool;
	}

	// Enqueue
	// 
	// Parameters: (DATA) 넣을 데이터
	// Return: 없음. 내부에서 실패 시 Crash
	template <typename DATA>
	void CNormalQueue<DATA>::Enqueue(DATA Data)
	{
		// 1. 테일 스냅샷
		stNode* LocalTail = m_pTail;

		// 2. 새로운 노드 만들고 셋팅
		stNode* NewNode = m_NodePool->Alloc();	

		NewNode->m_data = Data;	
		NewNode->m_Next = nullptr;		

		// 3. 테일의 Next가 nullptr이 아니면 Crash
		if (m_pTail->m_Next != nullptr)
			m_dump->Crash();	
		
		// 4. 테일을 -->로 1칸 이동		
		m_pTail = NewNode;

		// 5. 스냅샷으로 받아둔 테일을 이용해, 테일의 Next 연결
		LocalTail->m_Next = NewNode;	

		// 6. 큐 사이즈 1 증가
		InterlockedIncrement(&m_iSize);
	}


	// Dequeue
	//
	// Parameters: (DATA&) 뺄 데이터를 받을 포인터
	// return -1 : 큐에 할당된 데이터가 없음.
	// return 0 : 매개변수에 성공적으로 값을 채움.
	template <typename DATA>
	int CNormalQueue<DATA>::Dequeue(DATA& Data)
	{
		// 1. 헤더의 Next가 nullptr이면 데이터가 하나도 없는것.
		if (m_pHead->m_Next == nullptr)
			return -1;	

		// 2. 큐 사이즈 1 감소
		InterlockedDecrement(&m_iSize);

		// 3. 그게 아니라면 데이터가 있는것.
		// 헤더를 헤더->Next로 이동
		stNode* DeqNode = m_pHead;
		m_pHead = m_pHead->m_Next;	

		// 4. 인자에 헤더의 Next 데이터를 넣는다
		Data = m_pHead->m_data;		

		// 5. 반환할 노드가 실제 Tail과 같다면 Crash.
		// 지금, Enq에서 이 상황 발생 안하도록 처리했기 때문에 나면 안됨.
		if (DeqNode == m_pTail)
			m_dump->Crash();

		// 6. 메모리풀에 반환
		m_NodePool->Free(DeqNode);			

		return 0;
	}
	   	 
}





#endif // !__NORMAL_QUEUE_TEMPLATE_H__
