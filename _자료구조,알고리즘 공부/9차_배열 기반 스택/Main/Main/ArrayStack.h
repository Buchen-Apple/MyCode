#ifndef __ARRAY_STACK_H__
#define __ARRAY_STACK_H__


#define SIZE 100

class arrayStack
{
	typedef int Data;

	Data m_stackArr[SIZE];
	int m_iTop;
	int m_iSize;


public:
	// 초기화
	void Init();

	// 삽입
	bool Push(Data data);

	// 삭제
	bool Pop(Data *pData);

	// Peek
	bool  Peek(Data *pData);
	
	// 비었는지 체크
	bool IsEmpty();

	// 내부의 데이터 수
	int GetNodeSize();
};

#endif // !__ARRAY_STACK_H__
