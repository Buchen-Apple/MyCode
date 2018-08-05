#ifndef __MY_SMART_PTR_H__
#define __MY_SMART_PTR_H__

#include <Windows.h>
#include <map>


namespace Library_Jingyu
{

	// ------------------------
	// 내가 만든 스마트포인터
	// ------------------------
	template <typename T>
	class  My_Smart_PTR
	{
	private:
		T * m_pPtr;		// 가리키고있는 포인터
		long* m_lCount;	// 현재 가리키고 있는 포인터의 카운터. 동적할당해서 보관.

	public:
		// ------------- 생성자와 소멸자 ---------------
		// 생성자
		My_Smart_PTR(T* P);

		// 복사 생성자
		My_Smart_PTR(const My_Smart_PTR<T>& Copy);

		// 소멸자
		~My_Smart_PTR();

	public:
		// ------------- 각종 연산자 오버로딩 ---------------
		// -> 연산자
		T* operator->() const;

		// * 연산자
		T& operator*() const;

		// 대입 연산자
		My_Smart_PTR<T>& operator=(const My_Smart_PTR<T>& ref);



	public:
		// ------------- 게터 ---------------
		// g_My_Smart_PTR_Count 값 얻기
		long GetRefCount() const;


	};

	// 아래는 Cpp 부분
	//--------------------------------------------------------------------
	//--------------------------------------------------------------------
	// ******************************
	// 생성자와 소멸자
	// ******************************
	// 생성자
	template <typename T>
	My_Smart_PTR<T>::My_Smart_PTR(T* P)
	{
		m_pPtr = P;
		m_lCount = new long;
		*m_lCount = 1;
	}

	// 복사 생성자
	template <typename T>
	My_Smart_PTR<T>::My_Smart_PTR(const My_Smart_PTR<T>& Copy)
	{
		m_pPtr = Copy.m_pPtr;
		InterlockedIncrement(Copy.m_lCount);
		m_lCount = Copy.m_lCount;
		
	}

	// 소멸자
	template <typename T>
	My_Smart_PTR<T>::~My_Smart_PTR()
	{
		if (InterlockedDecrement(m_lCount) == 0)
		{
			delete m_pPtr;
			delete m_lCount;
		}		
	}


	// ******************************
	// 각종 연산자 오버로딩
	// ******************************
	// -> 연산자
	template <typename T>
	T* My_Smart_PTR<T>::operator->() const
	{
		return m_pPtr;
	}

	// * 연산자
	template <typename T>
	T& My_Smart_PTR<T>::operator*() const
	{
		return *m_pPtr;
	}

	// 대입 연산자
	template <typename T>
	My_Smart_PTR<T>& My_Smart_PTR<T>::operator=(const My_Smart_PTR<T>& ref)
	{
		// 기존에 가리키던 포인터의 레퍼런스 카운트를 1 감소시키고 0이라면 삭제
		if (InterlockedDecrement(m_lCount) == 0)
		{
			delete m_pPtr;
			delete m_lCount;
		}

		// ptr 갱신
		m_pPtr = ref.m_pPtr;

		// refcount 갱신
		InterlockedIncrement(Copy.m_lCount);
		m_lCount = Copy.m_lCount; 
		
	}



	// ******************************
	// 게터
	// ******************************
	// g_My_Smart_PTR_Count 값 얻기
	template <typename T>
	long My_Smart_PTR<T>::GetRefCount() const
	{
		return *m_lCount;
	}


}



#endif // !__MY_SMART_PTR_H__
