#include "pch.h"
#include "ProtocolBuff(Net)_ObjectPool.h"

LONG g_lAllocNodeCount;

namespace Library_Jingyu
{

#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// 직렬화 버퍼 1개의 크기
	// 각 서버에 전역 변수로 존재해야 함.
	extern LONG g_lNET_BUFF_SIZE;

	// NetServer Packet 헤더 사이즈.
#define dfNETWORK_PACKET_HEADER_SIZE_NETSERVER 5

	// static 메모리풀
	CMemoryPoolTLS<CProtocolBuff_Net>* CProtocolBuff_Net::m_MPool = new CMemoryPoolTLS<CProtocolBuff_Net>(0, false);

	// 문제 생길 시 Crash 발생시킬 덤프.
	CCrashDump* CProtocolBuff_Net::m_Dump = CCrashDump::GetInstance();

	// 사이즈 지정한 생성자
	CProtocolBuff_Net::CProtocolBuff_Net(int size)
	{
		m_Size = size;
		m_pProtocolBuff = new char[size];
		m_Front = 0;
		m_Rear = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER; // 처음 앞에 5바이트는 헤더를 넣어야 하기 때문에 rear를 2로 설정해둔다.
		m_RefCount = 1;	// 레퍼런스 카운트 1으로 초기화 (생성되었으니 카운트 1이 되어야 한다.)

		m_bHeadCheck = false;
	}

	// 사이즈 지정 안한 생성자
	CProtocolBuff_Net::CProtocolBuff_Net()
	{
		m_Size = g_lNET_BUFF_SIZE;
		m_pProtocolBuff = new char[g_lNET_BUFF_SIZE];
		m_Front = 0;
		m_Rear = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER; // 처음 앞에 5바이트는 헤더를 넣어야 하기 때문에 rear를 2로 설정해둔다.
		m_RefCount = 1;	// 레퍼런스 카운트 1으로 초기화 (생성되었으니 카운트 1이 되어야 한다.)	

		m_bHeadCheck = false;
	}

	// 소멸자
	CProtocolBuff_Net::~CProtocolBuff_Net()
	{
		delete[] m_pProtocolBuff;
	}

	// 인코딩
	// 보내기 전에, 헤더를 넣는다. 이 때 암호화 후 넣는다.
	//
	// Parameter : 헤더 코드, XORCode1, XORCode2
	// return : 없음
	void CProtocolBuff_Net::Encode(BYTE bCode, BYTE bXORCode_1, BYTE bXORCode_2)
	{
		// 1. 들어가기전에 헤더가 채워졌는지 체크
		// 채워졌으면 그냥 return
		if (m_bHeadCheck == true)
			return;

		// 채워져 있지 않다면 true로 바꾼다.
		m_bHeadCheck = true;


		// --------------------------- 한번에 암호화
		// [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<여기까지 헤더   - Payload(Len byte)
		BYTE RandXORCode;
		WORD PayloadLen;
		BYTE CheckSum;

		// 1. Rand XOR Code 생성
		RandXORCode = rand();

		// 2. RandXORCode, 고정 XOR1, 고정 XOR2로 Payload를 바이트단위 xor
		// 동시에 Total도 구함.
		PayloadLen = GetUseSize() - dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		int Total = 0;

		int LoopCount = 0;
		int i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		while (LoopCount < PayloadLen)
		{
			Total += m_pProtocolBuff[i];
			m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ RandXORCode ^ bXORCode_1 ^ bXORCode_2;
			i++;
			LoopCount++;
		}

		// 3. Checksum 구한 후 RandXORCode와 Xor
		CheckSum = Total % 256;
		CheckSum = RandXORCode ^ CheckSum;

		// 4. 고정 XOR1, 2 로 RandXORCode와 CheckSum  XOR
		RandXORCode = RandXORCode ^ bXORCode_1 ^ bXORCode_2;
		CheckSum = CheckSum ^ bXORCode_1 ^ bXORCode_2;

		// 5. 헤더에 Copy(대입)
		*(char *)&m_pProtocolBuff[0] = bCode;
		*(short *)&m_pProtocolBuff[1] = PayloadLen;
		*(char *)&m_pProtocolBuff[3] = RandXORCode;
		*(char *)&m_pProtocolBuff[4] = CheckSum;
	}

	// 인코딩2
	// 보내기 전에, 헤더를 넣는다. 이 때 암호화후 넣는다.
	//
	// Parameter : 헤더코드, XORCode(고정 XOR코드)
	void CProtocolBuff_Net::Encode2(BYTE bCode, BYTE bXORCode)
	{
		// 구조 : [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<여기까지 헤더   - Payload(Len byte)

		// 1. 들어가기전에 헤더가 채워졌는지 체크
		// 채워졌으면 그냥 return
		if (m_bHeadCheck == true)
			return;

		// 채워져 있지 않다면 true로 바꾼다.
		m_bHeadCheck = true;

		BYTE RandXORCode;
		WORD PayloadLen;
		BYTE CheckSum;

		// Rand XOR Code 생성
		RandXORCode = rand();

		// 1. 체크썸 제작.
		// 체크썸을 1번데이터로 보고 시작한다.
		PayloadLen = GetUseSize() - dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		int Total = 0;
		int LoopCount = 0;
		int i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		while (LoopCount < PayloadLen)
		{
			Total += m_pProtocolBuff[i];
			i++;
			LoopCount++;
		}
		CheckSum = Total % 256;


		// 2. 체크썸 XOR.		
		BYTE P;
		BYTE E;

		// 체크썸 먼저 XOR 한다.
		P = CheckSum ^ (RandXORCode + 1);
		E = CheckSum = P ^ (bXORCode + 1);


		// 3. 페이로드 바이트 단위 XOR
		i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;	
		LoopCount = 0;
		int Add = 2;
		while (LoopCount < PayloadLen)
		{
			P = m_pProtocolBuff[i] ^ (P + RandXORCode + Add);
			E = m_pProtocolBuff[i] = P ^ (E + bXORCode + Add);
			
			i++;
			LoopCount++;
			Add++;
		}		

		// 4. 헤더에 Copy(대입)
		*(char *)&m_pProtocolBuff[0] = bCode;
		*(short *)&m_pProtocolBuff[1] = PayloadLen;
		*(char *)&m_pProtocolBuff[3] = RandXORCode;
		*(char *)&m_pProtocolBuff[4] = CheckSum;
	}

	// 디코딩
	// 네트워크로 받은 패킷 중, 헤더를 해석한다.
	//
	// Parameter : 페이로드 길이, 랜덤xor코드, 체크썸, XORCode1, XORCode2
	// return : CheckSum이 다를 시 false
	bool CProtocolBuff_Net::Decode(WORD PayloadLen, BYTE RandXORCode, BYTE CheckSum, BYTE bXORCode_1, BYTE bXORCode_2)
	{	
		// -------------------------------------- 한번에 풀기
		// [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<여기까지 헤더   - Payload(Len byte)		

		// 1. 고정 XOR Code2, 1로 [Rand XOR Code] XOR (RandXORCode 복호화 완료)
		RandXORCode = RandXORCode ^ bXORCode_1 ^ bXORCode_2;

		// 2. 고정 XOR Code2, 1, RandXORCode로 [CheckSum] XOR (CheckSum 복호화 완료)
		CheckSum = CheckSum ^ bXORCode_1 ^ bXORCode_2 ^ RandXORCode;

		// 3. 고정 XOR Code2, 1, RandXORCode로 [Payload]를 XOR (페이로드 복호화 완료 + 4번절차를 위한 Total 구하기)
		int RecvTotal = 0;
		BYTE CompareChecksum = 0;		

		int LoopCount = 0;
		while (LoopCount < PayloadLen)
		{
			RecvTotal += m_pProtocolBuff[LoopCount] = m_pProtocolBuff[LoopCount] ^ bXORCode_1 ^ bXORCode_2 ^ RandXORCode;
			LoopCount++;
		}

		// 4. Payload 를 checksum 공식으로 계산 후 패킷의 checksum 과 비교
		CompareChecksum = RecvTotal % 256;
		if (CompareChecksum != CheckSum)
			return false;

		return true;		
	}

	// 디코딩2
	// 네트워크로 받은 패킷 중, 헤더를 해석한다.
	//
	// Parameter : 페이로드 길이, 랜덤xor코드, 체크썸, XORCode
	// return : CheckSum이 다를 시 false
	bool CProtocolBuff_Net::Decode2(WORD PayloadLen, BYTE RandXORCode, BYTE CheckSum, BYTE bXORCode)
	{
		// [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<여기까지 헤더   - Payload(Len byte)	

		// 1. 체크섬 복호화.		
		BYTE E = CheckSum;
		BYTE P = E ^ (bXORCode + 1);
		CheckSum = P ^ (RandXORCode + 1);

		// 2. 페이로드 복호화
		int LoopCount = 0;
		int Add = 2;
		int RecvTotal = 0;
		BYTE P2;
		while (LoopCount < PayloadLen)
		{
			P2 = m_pProtocolBuff[LoopCount] ^ (E + bXORCode + Add);
			E = m_pProtocolBuff[LoopCount];

			RecvTotal += m_pProtocolBuff[LoopCount] = P2 ^ (P + RandXORCode + Add);
			P = P2;

			LoopCount++;
			Add++;
		}

		// 3. Payload 를 checksum 공식으로 계산 후 패킷의 checksum 과 비교
		BYTE CompareChecksum = RecvTotal % 256;
		if (CompareChecksum != CheckSum)
			return false;

		return true;
	}

	// 버퍼 크기 재설정 (만들어는 뒀지만 현재 쓰는곳 없음)
	int CProtocolBuff_Net::ReAlloc(int size)
	{
		// 기존 데이터를 temp에 보관시킨 후 m_pProtocolBuff 해제
		char* temp = new char[m_Size];
		memcpy(temp, &m_pProtocolBuff[m_Front], m_Size - m_Front);
		delete[] m_pProtocolBuff;

		// 새로운 크기로 메모리 할당
		m_pProtocolBuff = new char[size];

		// temp에 보관시킨 정보 복사
		memcpy(m_pProtocolBuff, temp, m_Size - m_Front);

		// 버퍼 크기 갱신
		m_Size = size;

		// m_Rear와 m_Front의 위치 변경.
		// m_Rear가 10이었고 m_Front가 3이었으면, m_Rear는 7이되고 m_Front는 0이된다. 즉, 리어와 프론트의 차이는 동일하다.
		m_Rear -= m_Front;
		m_Front = 0;

		return size;
	}

	// 버퍼 클리어
	void CProtocolBuff_Net::Clear()
	{
		m_Front = m_Rear = 0;
	}

	// 데이터 넣기
	int CProtocolBuff_Net::PutData(const char* pSrc, int size)
	{
		// 큐 꽉찼거나 rear가 Size를 앞질렀는지 체크
		if (m_Rear >= m_Size)
			throw CException(_T("ProtocalBuff(). PutData --> Buff Full!!"));

		// 메모리 복사
		// 1~8바이트 까지는 memcpy보다 대입연산으로 처리한다.
		// 커널모드 전환을 줄여, 속도 증가 기대.
		switch (size)
		{
		case 1:
			*(char *)&m_pProtocolBuff[m_Rear] = *(char *)pSrc;
			break;
		case 2:
			*(short *)&m_pProtocolBuff[m_Rear] = *(short *)pSrc;
			break;
		case 3:
			*(short *)&m_pProtocolBuff[m_Rear] = *(short *)pSrc;
			*(char *)&m_pProtocolBuff[m_Rear + 2] = *(char *)&pSrc[2];
			break;
		case 4:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			break;
		case 5:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(char *)&m_pProtocolBuff[m_Rear + 4] = *(char *)&pSrc[4];
			break;
		case 6:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(short *)&m_pProtocolBuff[m_Rear + 4] = *(short *)&pSrc[4];
			break;
		case 7:
			*(int *)&m_pProtocolBuff[m_Rear] = *(int *)pSrc;
			*(short *)&m_pProtocolBuff[m_Rear + 4] = *(short *)&pSrc[4];
			*(char *)&m_pProtocolBuff[m_Rear + 6] = *(char *)&pSrc[6];
			break;
		case 8:
			*((__int64 *)&m_pProtocolBuff[m_Rear]) = *((__int64 *)pSrc);
			break;

		default:
			memcpy_s(&m_pProtocolBuff[m_Rear], size, pSrc, size);
			break;
		}

		// rear의 위치 이동
		m_Rear += size;

		return size;
	}

	// 데이터 빼기
	int CProtocolBuff_Net::GetData(char* pSrc, int size)
	{
		// 큐 비었나 체크. 비었다면 예외 리턴
		if (m_Front == m_Rear)
			throw CException(_T("ProtocalBuff. GetData -> Queue Empty."));

		// front가 큐의 끝에 도착하면 더 이상 읽기 불가능. 예외 리턴
		else if (m_Front >= m_Size)
			throw CException(_T("ProtocalBuff. GetData -> Not Data."));

		// front + Size가 Rear를 앞지른다면 예외 리턴
		else if (m_Front + size > m_Rear)
			throw CException(_T("ProtocalBuff. GetData -> SizeError"));

		// 메모리 복사
		// 1~8바이트 까지는 memcpy보다 대입연산으로 처리한다.
		// 커널모드 전환을 줄여, 속도 증가 기대.
		switch (size)
		{
		case 1:
			*(char *)pSrc = *(char *)&m_pProtocolBuff[m_Front];
			break;
		case 2:
			*(short *)pSrc = *(short *)&m_pProtocolBuff[m_Front];
			break;
		case 3:
			*(short *)pSrc = *(short *)&m_pProtocolBuff[m_Front];
			*(char *)&pSrc[2] = *(char *)&m_pProtocolBuff[m_Front + 2];
			break;
		case 4:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			break;
		case 5:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(char *)&pSrc[4] = *(char *)&m_pProtocolBuff[m_Front + 4];
			break;
		case 6:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(short *)&pSrc[4] = *(short *)&m_pProtocolBuff[m_Front + 4];
			break;
		case 7:
			*(int *)pSrc = *(int *)&m_pProtocolBuff[m_Front];
			*(short *)&pSrc[4] = *(short *)&m_pProtocolBuff[m_Front + 4];
			*(char *)&pSrc[6] = *(char *)&m_pProtocolBuff[m_Front + 6];
			break;
		case 8:
			*((__int64 *)pSrc) = *((__int64 *)&m_pProtocolBuff[m_Front]);
			break;

		default:
			memcpy_s(pSrc, size, &m_pProtocolBuff[m_Front], size);
			break;
		}

		// 디큐한 만큼 m_Front이동
		m_Front += size;

		return size;
	}

	// 버퍼의 포인터 얻음.
	char* CProtocolBuff_Net::GetBufferPtr(void)
	{
		return (char*)m_pProtocolBuff;
	}

	// Rear 움직이기
	int CProtocolBuff_Net::MoveWritePos(int size)
	{
		// 현재 Rear의 위치가 버퍼의 끝이면 더 이상 이동 불가
		if (m_Rear == m_Size)
			return -1;

		// 매개변수로 받은 iSize가 한 번에 이동할 수 있는 크기를 벗어나는지 체크
		int iRealMoveSize;

		if (m_Rear + size > m_Size)
			iRealMoveSize = m_Size - m_Rear;

		// 벗어나지 않는다면 그냥 그 값 사용
		else
			iRealMoveSize = size;

		m_Rear += iRealMoveSize;

		return iRealMoveSize;
	}

	// Front 움직이기
	int CProtocolBuff_Net::MoveReadPos(int size)
	{
		// 버퍼가 빈 상태면 0 리턴
		if (m_Front == m_Rear)
			return 0;

		// 한번에 삭제할 수 있는 크기를 벗어나는지 체크
		int iRealRemoveSize;

		if (m_Front + size > m_Size)
			iRealRemoveSize = m_Size - m_Front;

		// 벗어나지 않는다면 그냥 그 크기 사용
		else
			iRealRemoveSize = size;

		m_Front += iRealRemoveSize;

		return iRealRemoveSize;
	}

	// 현재 사용중인 용량 얻기.
	int CProtocolBuff_Net::GetUseSize(void)
	{
		return m_Rear - m_Front;
	}

	// 현재 버퍼에 남은 용량 얻기.
	int	CProtocolBuff_Net::GetFreeSize(void)
	{
		return m_Size - m_Rear;
	}


	// ----------------------
	// static 함수 
	// ----------------------

	// Alloc. 
	CProtocolBuff_Net* CProtocolBuff_Net::Alloc()
	{
		InterlockedIncrement(&g_lAllocNodeCount);
		return  m_MPool->Alloc();
	}

	// Free. 현재는 이 안에서 레퍼런스 카운트 1 감소.
	// 만약, 레퍼런스 카운트가 0이라면 삭제함. TLSPool에 Free 함
	void CProtocolBuff_Net::Free(CProtocolBuff_Net* pBuff)
	{	

		// 인터락으로 안전하게 감소.
		// 만약 감소 후 0이됐다면 delete
		if (InterlockedDecrement(&pBuff->m_RefCount) == 0)
		{
			InterlockedDecrement(&g_lAllocNodeCount);

			pBuff->m_Rear = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;		// rear 값 5로 초기화. 헤더 영역 확보
			pBuff->m_Front = 0;		// Front 초기화
			pBuff->m_RefCount = 1;	// ref값 1로 초기화
			pBuff->m_bHeadCheck = false;	// 헤더 채움 여부 false로 변경
			m_MPool->Free(pBuff);
		}
	}

	// 노드 카운트 얻기
	LONG CProtocolBuff_Net::GetNodeCount()
	{
		return g_lAllocNodeCount;
	}


	// 일반 함수 -----------
	// 레퍼런스 카운트 1 Add하는 함수
	void CProtocolBuff_Net::Add()
	{
		// 인터락으로 안전하게 증가
		InterlockedIncrement(&m_RefCount);
	}

	// 레퍼런스 카운트를 인자로 받아서 Add하는 함수
	// 위의 Add함수 오버로딩
	void CProtocolBuff_Net::Add(int Count)
	{
		// 인터락으로 안전하게 증가
		InterlockedAdd(&m_RefCount, Count);
	}
}