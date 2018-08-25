#include "pch.h"
#include "RingBuff\RingBuff.h"

namespace Library_Jingyu
{
#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 사이즈 지정 안한 생성자 1
	CRingBuff::CRingBuff(void)
	{
		Initial(BUF_SIZE);
	}

	// 사이즈 지정한 생성자 2
	CRingBuff::CRingBuff(int iBufferSize)
	{
		Initial(iBufferSize);
	}

	// 사이즈 재지정
	void CRingBuff::Resize(int size)
	{
		delete[] m_Buff;
		Initial(size);
	}

	// 실제 초기화 (큐 준비)
	void CRingBuff::Initial(int iBufferSize)
	{
		m_BuffSize = iBufferSize;
		m_Buff = new char[m_BuffSize];
		m_Front = 0;
		m_Rear = 0;

		// SRW Lock 초기화
		InitializeSRWLock(&sl);

		//InitializeCriticalSection(&cs);
	}

	// 소멸자
	CRingBuff::~CRingBuff()
	{
		//DeleteCriticalSection(&cs);
		delete[] m_Buff;
	}

	// 버퍼의 사이즈 얻기.
	int	CRingBuff::GetBufferSize(void)
	{
		return m_BuffSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 용량.
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::GetUseSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 사용중 공간 계산
		// rear가 더 크거나 같다면
		if (TempRear >= TempFront)
			return TempRear - TempFront;

		// front가 더 크다면
		else
			return m_BuffSize - (TempFront - TempRear);
	}

	/////////////////////////////////////////////////////////////////////////
	// 현재 버퍼에 남은 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)남은용량.
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::GetFreeSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 남은 공간 계산하기
		// rear가 더 크거나 같다면
		if (TempRear >= TempFront)
			return (m_BuffSize - 1) - (TempRear - TempFront);

		// m_Front가 더 크다면
		else
			return (TempFront - 1) - TempRear;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	// (끊기지 않은 길이)
	//
	// Parameters: 없음.
	// Return: (int)사용가능 용량.
	////////////////////////////////////////////////////////////////////////
	// 한 번에 읽을 수 있는 크기
	int	CRingBuff::GetNotBrokenGetSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 한 번에 읽을 수 있는 크기
		// rear가 크거나 같다면
		if (TempRear >= TempFront)
			return TempRear - TempFront;

		// front가 더 크다면
		else
			return m_BuffSize - (TempFront + 1);	
	}

	//한 번에 쓸 수 있는 크기
	int	CRingBuff::GetNotBrokenPutSize(void)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		//한 번에 쓸 수 있는 크기
		// rear가 더 크거나 같다면
		if (TempRear >= TempFront)
			return m_BuffSize - (TempRear + 1);

		// front가 더 크다면
		else
			return (TempFront - 1) - TempRear;
	}

	// 큐의 다음 인덱스를 체크하기 위한 함수
	int CRingBuff::NextIndex(int iIndex, int iSize)
	{
		if (iIndex + iSize >= m_BuffSize)
			return 0;

		else
			return iIndex += iSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// WritePos 에 데이타 넣음.
	//
	// Parameters: (char *)데이타 포인터. (int)크기. 
	// Return: (int)넣은 크기. 큐가 꽉 찼으면 -1
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::Enqueue(char *chpData, int iSize)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 매개변수 size가 0이면 리턴
		if (iSize == 0)
			return 0;

		// --------------------
		// 1. 한번에 넣을 수 있는 사이즈, free 사이즈를 얻는다.
		// --------------------
		int EnqueueSize;
		int FreeSize;

		// Rear가 더 크거나 같다면
		if (TempRear >= TempFront)
		{
			// 한 번에 넣을 수 있는 사이즈
			EnqueueSize = m_BuffSize - (TempRear + 1);

			// free 사이즈 구하기
			FreeSize = (m_BuffSize - 1) - (TempRear - TempFront);
		}

		// front가 더 크다면
		else
		{
			// 한 번에 넣을 수 있는 사이즈
			EnqueueSize = (TempFront - 1) - TempRear;

			// free 사이즈 구하기
			FreeSize = EnqueueSize;
		}

		// --------------------
		// 2. 만약 여유공간이 0이라면, 버퍼 꽉찬 것.
		// --------------------
		if (FreeSize == 0)
			return -1;

		// --------------------
		// 3. TempRear 1칸 이동. 현재는 카피 전 무조건 하는 행동..
		// --------------------
		TempRear = NextIndex(TempRear, 1);


		// --------------------
		// 4. 한 번에 인큐할 수 있을경우
		// --------------------
		if (EnqueueSize >= iSize)
		{	
			// 1 ~ 8바이트까지는 캐스팅해서 강제로 넣는다.
			if (iSize <= 8)
			{
				switch (iSize)
				{
				case 1:
					*(char *)&m_Buff[TempRear] = *(char *)chpData;
					break;
				case 2:
					*(short *)&m_Buff[TempRear] = *(short *)chpData;
					break;
				case 3:
					*(short *)&m_Buff[TempRear] = *(short *)chpData;
					*(char *)&m_Buff[TempRear + sizeof(short)] = *(char *)&chpData[sizeof(short)];
					break;
				case 4:
					*(int *)&m_Buff[TempRear] = *(int *)chpData;
					break;
				case 5:
					*(int *)&m_Buff[TempRear] = *(int *)chpData;
					*(char *)&m_Buff[TempRear + sizeof(int)] = *(char *)&chpData[sizeof(int)];
					break;
				case 6:
					*(int *)&m_Buff[TempRear] = *(int *)chpData;
					*(short *)&m_Buff[TempRear + sizeof(int)] = *(short *)&chpData[sizeof(int)];
					break;
				case 7:
					*(int *)&m_Buff[TempRear] = *(int *)chpData;
					*(short *)&m_Buff[TempRear + sizeof(int)] = *(short *)&chpData[sizeof(int)];
					*(char *)&m_Buff[TempRear + sizeof(int) + sizeof(short)] = *(char *)&chpData[sizeof(int) + sizeof(short)];
					break;
				case 8:
					*((__int64 *)&m_Buff[TempRear]) = *((__int64 *)chpData);
					break;
				}
			}

			// 8바이트 이상이면 memcpy한다
			else
			{
				memcpy_s(&m_Buff[TempRear], iSize, chpData, iSize);
			}

			// 카피 끝났으면 TempRear움직인다.
			TempRear = NextIndex(TempRear, iSize - 1);
		}

		// --------------------
		// 4. 두 번에 인큐할 수 있을 경우
		// --------------------
		else
		{
			// memcpy_s 2회
			// 1) 먼저, 현재 위치부터 버퍼 끝까지 1회 넣는다. 
			memcpy_s(&m_Buff[TempRear], EnqueueSize, chpData, EnqueueSize);

			// 2) TempRear 이동시킨 후, 그 위치부터 나머지 넣는다.
			TempRear = NextIndex(TempRear, EnqueueSize);			
			memcpy_s(&m_Buff[TempRear], iSize - EnqueueSize, &chpData[EnqueueSize], iSize - EnqueueSize);

			// 3) TempRear 위치 다시 이동
			TempRear = NextIndex(TempRear, iSize - EnqueueSize - 1);

		}

		// --------------------
		// 5. m_Rear갱신 후 리턴.
		// --------------------
		m_Rear = TempRear;

		return iSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 가져옴. ReadPos 이동.
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기. 큐가 비었으면 -1
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::Dequeue(char *chpDest, int iSize)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 매개변수 size가 0이면 리턴
		if (iSize == 0)
			return 0;

		// --------------------
		// 1. 한번에 뺄 수 있는 사이즈, Use 사이즈를 얻는다.
		// --------------------
		int DequeueSize;
		int UseSize;


		// Rear가 더 크거나 같다면
		if (TempRear >= TempFront)
		{
			// 한 번에 뺄 수 있는 사이즈
			DequeueSize = TempRear - TempFront;

			// Use 사이즈 구하기
			UseSize = TempRear - TempFront;
		}

		// front가 더 크다면
		else
		{
			// 한 번에 뺄 수 있는 사이즈
			DequeueSize = m_BuffSize - (TempFront + 1);

			// free 사이즈 구하기
			UseSize = m_BuffSize - (TempFront - TempRear);
		}

		// --------------------
		// 2. 만약 사용중 공간이 0이라면, 디큐할 데이터가 없는것.
		// --------------------
		if (UseSize == 0)
			return -1;

		// --------------------
		// 3. TempFont 1칸 이동. 현재는 카피 전 무조건 하는 행동..
		// --------------------
		TempFront = NextIndex(TempFront, 1);


		// --------------------
		// 4. 한 번에 디큐할 수 있을경우
		// --------------------
		if (DequeueSize >= iSize)
		{
			// 1 ~ 8바이트까지는 캐스팅해서 강제로 넣는다.
			if (iSize <= 8)
			{
				switch (iSize)
				{
				case 1:
					*(char *)chpDest = *(char *)&m_Buff[TempFront];
					break;
				case 2:
					*(short *)chpDest = *(short *)&m_Buff[TempFront];
					break;
				case 3:
					*(short *)chpDest = *(short *)&m_Buff[TempFront];
					*(char *)&chpDest[sizeof(short)] = *(char *)&m_Buff[TempFront + sizeof(short)];
					break;
				case 4:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					break;
				case 5:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					*(char *)&chpDest[sizeof(int)] = *(char *)&m_Buff[TempFront + sizeof(int)];
					break;
				case 6:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					*(short *)&chpDest[sizeof(int)] = *(short *)&m_Buff[TempFront + sizeof(int)];
					break;
				case 7:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					*(short *)&chpDest[sizeof(int)] = *(short *)&m_Buff[TempFront + sizeof(int)];
					*(char *)&chpDest[sizeof(int) + sizeof(short)] = *(char *)&m_Buff[TempFront + sizeof(int) + sizeof(short)];
					break;
				case 8:
					*((__int64 *)chpDest) = *((__int64 *)&m_Buff[TempFront]);
					break;
				}
			}

			// 8바이트 이상이면 memcpy한다
			else
			{
				memcpy_s(chpDest, iSize, &m_Buff[TempFront], iSize);
			}

			// 카피 끝났으면 TempFront움직인다.
			TempFront = NextIndex(TempFront, iSize - 1);
		}

		// --------------------
		// 4. 두 번에 인큐할 수 있을 경우
		// --------------------
		else
		{
			// memcpy_s 2회
			// 1) 먼저, 현재 위치부터 버퍼 끝까지 1회 디큐
			memcpy_s(chpDest, DequeueSize, &m_Buff[TempFront], DequeueSize);

			// 2) TempFront 이동시킨 후, 그 위치부터 나머지 넣는다.
			TempFront = NextIndex(TempFront, DequeueSize);
			memcpy_s(&chpDest[DequeueSize], iSize - DequeueSize, &m_Buff[TempFront], iSize - DequeueSize);

			// 3) TempFront 위치 다시 이동
			TempFront = NextIndex(TempFront, iSize - DequeueSize - 1);

		}

		// --------------------
		// 5. m_Front갱신 후 리턴.
		// --------------------
		m_Front = TempFront;

		return iSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// ReadPos 에서 데이타 읽어옴. ReadPos 고정. (Front)
	//
	// Parameters: (char *)데이타 포인터. (int)크기.
	// Return: (int)가져온 크기. 큐가 비었으면 -1
	/////////////////////////////////////////////////////////////////////////
	int	CRingBuff::Peek(char *chpDest, int iSize)
	{
		int TempRear = m_Rear;
		int TempFront = m_Front;

		// 매개변수 size가 0이면 리턴
		if (iSize == 0)
			return 0;

		// --------------------
		// 1. 한번에 뺄 수 있는 사이즈, Use 사이즈를 얻는다.
		// --------------------
		int DequeueSize;
		int UseSize;


		// Rear가 더 크거나 같다면
		if (TempRear >= TempFront)
		{
			// 한 번에 뺄 수 있는 사이즈
			DequeueSize = TempRear - TempFront;

			// Use 사이즈 구하기
			UseSize = TempRear - TempFront;
		}

		// front가 더 크다면
		else
		{
			// 한 번에 뺄 수 있는 사이즈
			DequeueSize = m_BuffSize - (TempFront + 1);

			// free 사이즈 구하기
			UseSize = m_BuffSize - (TempFront - TempRear);
		}

		// --------------------
		// 2. 만약 사용중 공간이 0이라면, 디큐할 데이터가 없는것.
		// --------------------
		if (UseSize == 0)
			return -1;

		// --------------------
		// 3. TempFont 1칸 이동. 현재는 카피 전 무조건 하는 행동..
		// --------------------
		TempFront = NextIndex(TempFront, 1);


		// --------------------
		// 4. 한 번에 픽할 수 있을경우
		// --------------------
		if (DequeueSize >= iSize)
		{
			// 1 ~ 8바이트까지는 캐스팅해서 강제로 넣는다.
			if (iSize <= 8)
			{
				switch (iSize)
				{
				case 1:
					*(char *)chpDest = *(char *)&m_Buff[TempFront];
					break;
				case 2:
					*(short *)chpDest = *(short *)&m_Buff[TempFront];
					break;
				case 3:
					*(short *)chpDest = *(short *)&m_Buff[TempFront];
					*(char *)&chpDest[sizeof(short)] = *(char *)&m_Buff[TempFront + sizeof(short)];
					break;
				case 4:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					break;
				case 5:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					*(char *)&chpDest[sizeof(int)] = *(char *)&m_Buff[TempFront + sizeof(int)];
					break;
				case 6:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					*(short *)&chpDest[sizeof(int)] = *(short *)&m_Buff[TempFront + sizeof(int)];
					break;
				case 7:
					*(int *)chpDest = *(int *)&m_Buff[TempFront];
					*(short *)&chpDest[sizeof(int)] = *(short *)&m_Buff[TempFront + sizeof(int)];
					*(char *)&chpDest[sizeof(int) + sizeof(short)] = *(char *)&m_Buff[TempFront + sizeof(int) + sizeof(short)];
					break;
				case 8:
					*((__int64 *)chpDest) = *((__int64 *)&m_Buff[TempFront]);
					break;
				}
			}

			// 8바이트 이상이면 memcpy한다
			else
			{
				memcpy_s(chpDest, iSize, &m_Buff[TempFront], iSize);
			}

			// 카피 끝났으면 TempFront움직인다.
			TempFront = NextIndex(TempFront, iSize - 1);
		}

		// --------------------
		// 4. 두 번에 픽할 수 있을 경우
		// --------------------
		else
		{
			// memcpy_s 2회
			// 1) 먼저, 현재 위치부터 버퍼 끝까지 1회 디큐
			memcpy_s(chpDest, DequeueSize, &m_Buff[TempFront], DequeueSize);

			// 2) TempFront 이동시킨 후, 그 위치부터 나머지 넣는다.
			TempFront = NextIndex(TempFront, DequeueSize);
			memcpy_s(&chpDest[DequeueSize], iSize - DequeueSize, &m_Buff[TempFront], iSize - DequeueSize);

			// 3) TempFront 위치 다시 이동
			TempFront = NextIndex(TempFront, iSize - DequeueSize - 1);

		}

		// --------------------
		// 5. m_Front갱신 '안하고!!' 그냥 리턴
		// --------------------

		return iSize;
	}


	/////////////////////////////////////////////////////////////////////////
	// 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	//
	// Parameters: 없음.
	// Return: 실제 삭제 및 이동한 사이즈.
	/////////////////////////////////////////////////////////////////////////
	int CRingBuff::RemoveData(int iSize)
	{
		int tempFront = m_Front;

		// 0사이즈를 이동하려고 하면 리턴
		if (iSize == 0)
			return 0;

		// 매개변수로 받은 iSize가 한 번에 이동할 수 있는 크기를 벗어나는지 체크
		int iRealMoveSize;

		if (tempFront + iSize >= m_BuffSize)
			iRealMoveSize = (tempFront + iSize) - m_BuffSize;

		else
			iRealMoveSize = tempFront + iSize;

		m_Front = iRealMoveSize;

		return iRealMoveSize;
	}

	// rear 이동
	int	CRingBuff::MoveWritePos(int iSize)
	{
		int tempRear = m_Rear;

		// 0사이즈를 이동하려고 하면 리턴
		if (iSize == 0)
			return 0;

		// 매개변수로 받은 iSize가 한 번에 이동할 수 있는 크기를 벗어나는지 체크
		int iRealMoveSize;

		if (tempRear + iSize >= m_BuffSize)
			iRealMoveSize = (tempRear + iSize) - m_BuffSize;

		else
			iRealMoveSize = tempRear + iSize;

		m_Rear = iRealMoveSize;

		return iRealMoveSize;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 모든 데이타 삭제.
	//
	// Parameters: 없음.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////
	void CRingBuff::ClearBuffer(void)
	{
		m_Front = m_Rear = 0;
	}

	/////////////////////////////////////////////////////////////////////////
	// 링버퍼 뒤에가 끊겼을때, 0번인덱스부터 front까지의 길이
	//
	// Parameters: 없음.
	// Return: 0~front까지의 사이즈 리턴
	/////////////////////////////////////////////////////////////////////////
	int CRingBuff::GetFrontSize(void)
	{
		return m_Front;
	}


	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetBufferPtr(void)
	{
		return m_Buff;
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 ReadPos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetReadBufferPtr(void)
	{
		return (char*)&m_Front;
	}

	/////////////////////////////////////////////////////////////////////////
	// Fornt 1칸 앞 위치의 Buff 주소 반환
	//
	// Parameters: 없음.
	// Return: (char*)&Buff[m_Front 한칸앞]
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetFrontBufferPtr(void)
	{
		return m_Buff + NextIndex(m_Front, 1);	
	}

	/////////////////////////////////////////////////////////////////////////
	// 버퍼의 WritePos 포인터 얻음.
	//
	// Parameters: 없음.
	// Return: (char *) 버퍼 포인터.
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetWriteBufferPtr(void)
	{
		return (char*)&m_Rear;
	}

	/////////////////////////////////////////////////////////////////////////
	// Rear 1칸 앞 위치의 Buff 주소 반환
	//
	// Parameters: 없음.
	// Return: (char*)&Buff[m_Rear 한칸앞]
	/////////////////////////////////////////////////////////////////////////
	char* CRingBuff::GetRearBufferPtr(void)
	{
		return m_Buff + NextIndex(m_Rear, 1);
	}	

	/////////////////////////////////////////////////////////////////////////
	// 크리티컬 섹션 입장
	//
	// Parameters: 없음.
	// Return: 없음
	/////////////////////////////////////////////////////////////////////////
	void CRingBuff::EnterLOCK()
	{
		//EnterCriticalSection(&cs);
		AcquireSRWLockExclusive(&sl);
	}


	/////////////////////////////////////////////////////////////////////////
	// 크리티컬 섹션 퇴장
	//
	// Parameters: 없음.
	// Return: 없음
	/////////////////////////////////////////////////////////////////////////
	void CRingBuff::LeaveLOCK()
	{
		//LeaveCriticalSection(&cs);
		ReleaseSRWLockExclusive(&sl);
	}

}