#pragma once
#ifndef CRINGBUFFER_H__
#define CRINGBUFFER_H__

class CRingBuffer
{
private:
	const static int		dfBUFFERSIZE		= 10000;
	//
	int						m_iBufferSize;
	int						m_iBufferSize_Decrement;
	int						m_iFront;
	int						m_iRear;
	char					*m_pBuffer;
	//
	CRITICAL_SECTION		m_Cri;
public:
	explicit CRingBuffer(int _iSize = dfBUFFERSIZE);
	~CRingBuffer();
public:
	bool Resize(int _iSize);
	int GetBufferSize();
	int GetUseSize();
	int GetFreeSize();
	int GetUsePieceSize();
	int GetFreePieceSize();
	int Enqueue(char *pInBuff, int _iSize);
	int Dequeue(char *pOutBuff, int _iSize);
	int Peek(char *pOutBuff, int _iSize);
	void RemoveData(int _iSize);
	void MoveWritePos(int _iSize);
	void ClearBuffer();
	char *GetBufferPtr() { return m_pBuffer; }
	char *GetUsePtr() { return &m_pBuffer[m_iFront]; }
	char *GetFreePtr() { return &m_pBuffer[m_iRear]; }
	//
	int GetFront() { return m_iFront; }
	int GetRear() { return m_iRear; }
	//
	void EnterThisCri() { EnterCriticalSection(&m_Cri); }
	void LeaveThisCri() { LeaveCriticalSection(&m_Cri); }
};

#endif
