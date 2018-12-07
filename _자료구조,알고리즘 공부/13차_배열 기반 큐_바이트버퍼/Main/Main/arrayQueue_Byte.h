#ifndef __ARRAY_QUEUE_BYTE_H__
#define __ARRAY_QUEUE_BYTE_H__

#define BUF_SIZE	5

class arrayQueue_Byte
{
	char m_cBuff[BUF_SIZE];
	int m_iFront;
	int m_iRear;
	int m_iSize;


public:
	// 초기화
	void Init();

	// 인큐
	bool Enqueue(char* Data, int Size);

	// 디큐
	bool Dequeue(char* Data, int Size);

	// 픽
	bool Peek(char* Data, int Size);

	// 사용 중인 사이즈
	int GetUseSize();

	// 사용 가능한 사이즈
	int GetFreeSize();

	// 한 번에 인큐 가능한 길이(끊기지 않고)
	int GetNotBrokenSize_Enqueue();

	// 한 번에 디큐 가능한 길이(끊기지 않고)
	int GetNotBrokenSize_Dequeue();
};

#endif // !__ARRAY_QUEUE_BYTE_H__
