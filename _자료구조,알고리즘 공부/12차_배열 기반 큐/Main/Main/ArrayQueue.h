#ifndef __ARRAY_QUEUE_H__
#define __ARRAY_QUEUE_H__

#define ARRAY_SIZE	100

class arrayQueue
{
	typedef int Data;

	Data m_Array[ARRAY_SIZE];
	
	int m_iFront;
	int m_iRear;
	int m_iSize;

private:
	// 다음 위치 확인
	int NextPos(int Pos);

public:
	// 초기화
	void Init();

	// 인큐
	bool Enqueue(Data data);

	// 디큐
	bool Dequeue(Data *pData);

	// Peek
	bool Peek(Data *pData);

	// 사이즈
	int GetNodeSize();
};

#endif // !__ARRAY_QUEUE_H__
