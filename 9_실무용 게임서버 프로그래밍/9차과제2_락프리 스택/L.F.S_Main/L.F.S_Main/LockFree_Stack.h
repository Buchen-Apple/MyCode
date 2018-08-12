#ifndef __LOCKFREE_STACK_H__
#define __LOCKFREE_STACK_H__

#include <Windows.h>
#include "CrashDump\CrashDump.h"

namespace Library_Jingyu
{
	template <typename T>
	class CLF_Stack
	{
	private:
		struct st_LFS_NODE
		{
			T m_Data;
			st_LFS_NODE* m_stpNextBlock;
		};

		// 스택은 Last-In-First_Out
		st_LFS_NODE* m_stpTop = nullptr;

		// 에러났을 때 크래시 내는 용도
		CCrashDump* m_CDump = CCrashDump::GetInstance();

	public:
		// 생성자

		// 소멸자
		~CLF_Stack()
		{
			// null이 나올때까지 돌면서 삭제.
			while (1)
			{

			}


		}


		// 인덱스를 스택에 추가
		//
		// return : 없음 (void)
		void Push(T Data)
		{
			st_LFS_NODE* NewNode = new st_LFS_NODE;
			NewNode.m_Data = Data;
			NewNode.m_stpNextBlock = m_stpTop;

			m_stpTop = NewNode;
		}


		// 인덱스 얻기
		//
		// 성공 시, T 리턴
		// 실패시 Crash 발생
		T Pop()
		{
			// 더 이상 pop 할게 없으면 Crash 발생
			if (m_stpTop == nullptr)
				m_CDump->Crash();

			T retval = m_stpTop.m_Data;
			st_LFS_NODE* TempNode = m_stpTop;

			m_stpTop = m_stpTop.m_stpNextBlock;

			delete TempNode;

			return retval;
		}


		// 인덱스 얻기
		//
		// 반환값 ULONGLONG ---> 이 반환값으로 시프트 연산 할 수도 있어서, 혹시 모르니 unsigned로 리턴한다.
		// 0이상(0포함) : 인덱스 정상 반환
		// 10000000(천만) : 빈 인덱스 없음.
		ULONGLONG Pop()
		{
			// 인덱스 비었나 체크 ----------
			// 보통 Max로 설정한 유저가 모두 들어왔을 경우, 빈 인덱스가 없다.
			if (m_iTop == 0)
				return 10000000;

			// 빈 인덱스가 있으면, m_iTop을 감소 후 인덱스 리턴.
			m_iTop--;

			return m_iArray[m_iTop];
		}

		// 인덱스 넣기
		//
		// 반환값 bool
		// true : 인덱스 정상으로 들어감
		// false : 인덱스 들어가지 않음 (이미 Max만큼 꽉 참)
		bool Push(ULONGLONG PushIndex)
		{
			// 인덱스가 꽉찼나 체크 ---------
			// 인덱스가 꽉찼으면 false 리턴
			if (m_iTop == m_iMax)
				return false;

			// 인덱스가 꽉차지 않았으면, 추가
			m_iArray[m_iTop] = PushIndex;
			m_iTop++;

			return true;
		}

		CLF_Stack(int Max)
		{
			m_iTop = Max;
			m_iMax = Max;

			m_iArray = new ULONGLONG[Max];

			// 최초 생성 시, 모두 빈 인덱스
			for (int i = 0; i < Max; ++i)
				m_iArray[i] = i;
		}

		~CLF_Stack()
		{
			delete[] m_iArray;
		}

	};
}


#endif // !__LOCKFREE_STACK_H__
