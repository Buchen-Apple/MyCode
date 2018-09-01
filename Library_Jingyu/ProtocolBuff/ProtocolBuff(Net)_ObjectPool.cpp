#include "pch.h"
#include "ProtocolBuff(Net)_ObjectPool.h"

namespace Library_Jingyu
{


#define _MyCountof(_array)		sizeof(_array) / (sizeof(_array[0]))

	// ����ȭ ���� 1���� ũ��
#define BUFF_SIZE 512

	// NetServer Packet ��� ������.
#define dfNETWORK_PACKET_HEADER_SIZE_NETSERVER 5

	// static �޸�Ǯ
	CMemoryPoolTLS<CProtocolBuff_Net>* CProtocolBuff_Net::m_MPool = new CMemoryPoolTLS<CProtocolBuff_Net>(100, false);

	// ���� ���� �� Crash �߻���ų ����.
	CCrashDump* CProtocolBuff_Net::m_Dump = CCrashDump::GetInstance();

	// ������ ������ ������
	CProtocolBuff_Net::CProtocolBuff_Net(int size)
	{
		m_Size = size;
		m_pProtocolBuff = new char[size];
		m_Front = 0;
		m_Rear = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER; // ó�� �տ� 5����Ʈ�� ����� �־�� �ϱ� ������ rear�� 2�� �����صд�.
		m_RefCount = 1;	// ���۷��� ī��Ʈ 1���� �ʱ�ȭ (�����Ǿ����� ī��Ʈ 1�� �Ǿ�� �Ѵ�.)

		m_bHeadCheck = false;
	}

	// ������ ���� ���� ������
	CProtocolBuff_Net::CProtocolBuff_Net()
	{
		m_Size = BUFF_SIZE;
		m_pProtocolBuff = new char[BUFF_SIZE];
		m_Front = 0;
		m_Rear = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER; // ó�� �տ� 5����Ʈ�� ����� �־�� �ϱ� ������ rear�� 2�� �����صд�.
		m_RefCount = 1;	// ���۷��� ī��Ʈ 1���� �ʱ�ȭ (�����Ǿ����� ī��Ʈ 1�� �Ǿ�� �Ѵ�.)	

		m_bHeadCheck = false;
	}

	// �Ҹ���
	CProtocolBuff_Net::~CProtocolBuff_Net()
	{
		delete[] m_pProtocolBuff;
	}

	// ���ڵ�
	// ������ ����, ����� �ִ´�. �� �� ��ȣȭ �� �ִ´�.
	//
	// Parameter : ��� �ڵ�, XORCode1, XORCode2
	// return : ����
	void CProtocolBuff_Net::Encode(BYTE bCode, BYTE bXORCode_1, BYTE bXORCode_2)
	{
		// 1. �������� ����� ä�������� üũ
		// ä�������� �׳� return
		if (m_bHeadCheck == true)
			return;

		// ä���� ���� �ʴٸ� true�� �ٲ۴�.
		m_bHeadCheck = true;


		// --------------------------- �ѹ��� ��ȣȭ
		// [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<������� ���   - Payload(Len byte)
		BYTE RandXORCode;
		WORD PayloadLen;
		BYTE CheckSum;

		// 1. Rand XOR Code ����
		RandXORCode = rand();

		// 2. RandXORCode, ���� XOR1, ���� XOR2�� Payload�� ����Ʈ���� xor
		// ���ÿ� Total�� ����.
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

		// 3. Checksum ���� �� RandXORCode�� Xor
		CheckSum = Total % 256;
		CheckSum = RandXORCode ^ CheckSum;

		// 4. ���� XOR1, 2 �� RandXORCode�� CheckSum  XOR
		RandXORCode = RandXORCode ^ bXORCode_1 ^ bXORCode_2;
		CheckSum = CheckSum ^ bXORCode_1 ^ bXORCode_2;

		// 5. ����� Copy(����)
		*(char *)&m_pProtocolBuff[0] = bCode;
		*(short *)&m_pProtocolBuff[1] = PayloadLen;
		*(char *)&m_pProtocolBuff[3] = RandXORCode;
		*(char *)&m_pProtocolBuff[4] = CheckSum;




		// --------------------------- �ϳ��ϳ� ��ȣȭ
		// [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<������� ���   - Payload(Len byte)

		//WORD PayloadLen = GetUseSize() - dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//BYTE RandXORCode;
		//BYTE CheckSum;

		//// ������ ��ȣȭ ����
		//// 1. Rand XOR Code ����
		//RandXORCode = rand();

		//// 2. Payload �� checksum ���
		//int Total = 0;
		//int LoopCount = 0;
		//int i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	Total += m_pProtocolBuff[i];
		//	i++;
		//	LoopCount++;
		//}

		//CheckSum = Total % 256;

		//// 3. Rand XOR Code ��[CheckSum, Payload] ����Ʈ ���� xor
		//CheckSum = CheckSum ^ RandXORCode;

		//LoopCount = 0;
		//i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ RandXORCode;
		//	i++;
		//	LoopCount++;
		//}		

		//// 4. ���� XOR Code 1 ��[Rand XOR Code, CheckSum, Payload] �� XOR
		//RandXORCode = RandXORCode ^ bXORCode_1;
		//CheckSum = CheckSum ^ bXORCode_1;

		//LoopCount = 0;
		//i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ bXORCode_1;
		//	i++;
		//	LoopCount++;
		//}	

		//// 5. ���� XOR Code 2 ��[Rand XOR Code, CheckSum, Payload] �� XOR
		//RandXORCode = RandXORCode ^ bXORCode_2;
		//CheckSum = CheckSum ^ bXORCode_2;

		//LoopCount = 0;
		//i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ bXORCode_2;
		//	i++;
		//	LoopCount++;
		//}		

		//// 6. ����� Copy(����)
		//*(char *)&m_pProtocolBuff[0] = bCode;
		//*(short *)&m_pProtocolBuff[1] = PayloadLen;
		//*(char *)&m_pProtocolBuff[3] = RandXORCode;
		//*(char *)&m_pProtocolBuff[4] = CheckSum;	
	}

	// ���ڵ�
	// ��Ʈ��ũ�� ���� ��Ŷ ��, ����� �ؼ��Ѵ�.
	//
	// Parameter : ��� �迭, XORCode1, XORCode2
	// return : CheckSum�� �ٸ� �� false
	bool CProtocolBuff_Net::Decode(BYTE* Header, BYTE bXORCode_1, BYTE bXORCode_2)
	{		

		// -------------------------------------- �ѹ��� Ǯ��
		// [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<������� ���   - Payload(Len byte)

		/*
		Head[0] : Code
		Head[1~2] : Len
		Head[3] : Rand XOR Code
		Head[4] = CheckSum
		*/

		WORD PayloadLen = (WORD)Header[1];
		BYTE RandXORCode = Header[3];
		BYTE CheckSum = Header[4];

		// 1. ���� XOR Code2, 1�� [Rand XOR Code] XOR (RandXORCode ��ȣȭ �Ϸ�)
		RandXORCode = RandXORCode ^ bXORCode_1 ^ bXORCode_2;

		// 2. ���� XOR Code2, 1, RandXORCode�� [CheckSum] XOR (CheckSum ��ȣȭ �Ϸ�)
		CheckSum = CheckSum ^ bXORCode_1 ^ bXORCode_2 ^ RandXORCode;

		// 3. ���� XOR Code2, 1, RandXORCode�� [Payload]�� XOR (���̷ε� ��ȣȭ �Ϸ� + 4�������� ���� Total ���ϱ�)
		int RecvTotal = 0;
		BYTE CompareChecksum = 0;

		/*int LoopCount = 0;
		int i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		while (LoopCount < PayloadLen)
		{
			RecvTotal += m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ bXORCode_1 ^ bXORCode_2 ^ RandXORCode;
			i++;
			LoopCount++;
		}*/

		int LoopCount = 0;
		while (LoopCount < PayloadLen)
		{
			RecvTotal += m_pProtocolBuff[LoopCount] = m_pProtocolBuff[LoopCount] ^ bXORCode_1 ^ bXORCode_2 ^ RandXORCode;
			LoopCount++;
		}

		// 4. Payload �� checksum �������� ��� �� ��Ŷ�� checksum �� ��
		CompareChecksum = RecvTotal % 256;
		if (CompareChecksum != CheckSum)
			return false;

		return true;


			   
		// -------------------------------------- �ϳ��ϳ� Ǯ��
		
		// [Code(1byte) - Len(2byte) - Rand XOR Code(1byte) - CheckSum(1byte)] <<������� ���   - Payload(Len byte)

		/*
		Head[0] : Code
		Head[1~2] : Len
		Head[3] : Rand XOR Code
		Head[4] = CheckSum
		*/

		//
		//WORD PayloadLen = (WORD)Header[1];
		//BYTE RandXORCode = Header[3];
		//BYTE CheckSum = Header[4];

		//// ���� �� ��ȣȭ ����
		//// 1. ���� XOR Code 2 �� [ Rand XOR Code , CheckSum , Payload ] �� XOR
		//RandXORCode = RandXORCode ^ bXORCode_2;
		//CheckSum = CheckSum ^ bXORCode_2;

		//int LoopCount = 0;
		//int i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ bXORCode_2;
		//	i++;
		//	LoopCount++;
		//}

		//// 2. ���� XOR Code 1 ��[Rand XOR Code, CheckSum, Payload] �� XOR
		//RandXORCode = RandXORCode ^ bXORCode_1;
		//CheckSum = CheckSum ^ bXORCode_1;

		//LoopCount = 0;
		//i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ bXORCode_1;
		//	i++;
		//	LoopCount++;
		//}

		//// 3. Rand XOR Code �� �ľ�.
		//// �̰� ���ϴ°���?

		//// 4. Rand XOR Code ��[CheckSum, Payload] ����Ʈ ���� xor
		//CheckSum = CheckSum ^ RandXORCode;

		//LoopCount = 0;
		//i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	m_pProtocolBuff[i] = m_pProtocolBuff[i] ^ RandXORCode;
		//	i++;
		//	LoopCount++;
		//}
	

		//// 5. Payload �� checksum �������� ��� �� ��Ŷ�� checksum �� ��
		//int RecvTotal = 0;
		//BYTE CompareChecksum = 0;

		//LoopCount = 0;
		//i = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;
		//while (LoopCount < PayloadLen)
		//{
		//	RecvTotal += m_pProtocolBuff[i];
		//	i++;
		//	LoopCount++;
		//}	

		//CompareChecksum = RecvTotal % 256;

		//if (CompareChecksum != CheckSum)
		//	return false;

		//return true;
		
	}

	// ���� ũ�� �缳�� (������ ������ ���� ���°� ����)
	int CProtocolBuff_Net::ReAlloc(int size)
	{
		// ���� �����͸� temp�� ������Ų �� m_pProtocolBuff ����
		char* temp = new char[m_Size];
		memcpy(temp, &m_pProtocolBuff[m_Front], m_Size - m_Front);
		delete[] m_pProtocolBuff;

		// ���ο� ũ��� �޸� �Ҵ�
		m_pProtocolBuff = new char[size];

		// temp�� ������Ų ���� ����
		memcpy(m_pProtocolBuff, temp, m_Size - m_Front);

		// ���� ũ�� ����
		m_Size = size;

		// m_Rear�� m_Front�� ��ġ ����.
		// m_Rear�� 10�̾��� m_Front�� 3�̾�����, m_Rear�� 7�̵ǰ� m_Front�� 0�̵ȴ�. ��, ����� ����Ʈ�� ���̴� �����ϴ�.
		m_Rear -= m_Front;
		m_Front = 0;

		return size;
	}

	// ���� Ŭ����
	void CProtocolBuff_Net::Clear()
	{
		m_Front = m_Rear = 0;
	}

	// ������ �ֱ�
	int CProtocolBuff_Net::PutData(const char* pSrc, int size)
	{
		// ť ��á�ų� rear�� Size�� ���������� üũ
		if (m_Rear >= m_Size)
			throw CException(_T("ProtocalBuff(). PutData�� ���۰� ����."));

		// �޸� ����
		// 1~8����Ʈ ������ memcpy���� ���Կ������� ó���Ѵ�.
		// Ŀ�θ�� ��ȯ�� �ٿ�, �ӵ� ���� ���.
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

		// rear�� ��ġ �̵�
		m_Rear += size;

		return size;
	}

	// ������ ����
	int CProtocolBuff_Net::GetData(char* pSrc, int size)
	{
		// ť ����� üũ
		if (m_Front == m_Rear)
			throw CException(_T("ProtocalBuff(). GetData�� ť�� �������."));

		// front�� ť�� ���� �����ϸ� �� �̻� �б� �Ұ���. �׳� �����Ų��.
		if (m_Front >= m_Size)
			throw CException(_T("ProtocalBuff(). GetData�� front�� ������ ���� ����."));

		// �޸� ����
		// 1~8����Ʈ ������ memcpy���� ���Կ������� ó���Ѵ�.
		// Ŀ�θ�� ��ȯ�� �ٿ�, �ӵ� ���� ���.
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

		// ��ť�� ��ŭ m_Front�̵�
		m_Front += size;

		return size;
	}

	// ������ ������ ����.
	char* CProtocolBuff_Net::GetBufferPtr(void)
	{
		return m_pProtocolBuff;
	}

	// Rear �����̱�
	int CProtocolBuff_Net::MoveWritePos(int size)
	{
		// ���� Rear�� ��ġ�� ������ ���̸� �� �̻� �̵� �Ұ�
		if (m_Rear == m_Size)
			return -1;

		// �Ű������� ���� iSize�� �� ���� �̵��� �� �ִ� ũ�⸦ ������� üũ
		int iRealMoveSize;

		if (m_Rear + size > m_Size)
			iRealMoveSize = m_Size - m_Rear;

		// ����� �ʴ´ٸ� �׳� �� �� ���
		else
			iRealMoveSize = size;

		m_Rear += iRealMoveSize;

		return iRealMoveSize;
	}

	// Front �����̱�
	int CProtocolBuff_Net::MoveReadPos(int size)
	{
		// ���۰� �� ���¸� 0 ����
		if (m_Front == m_Rear)
			return 0;

		// �ѹ��� ������ �� �ִ� ũ�⸦ ������� üũ
		int iRealRemoveSize;

		if (m_Front + size > m_Size)
			iRealRemoveSize = m_Size - m_Front;

		// ����� �ʴ´ٸ� �׳� �� ũ�� ���
		else
			iRealRemoveSize = size;

		m_Front += iRealRemoveSize;

		return iRealRemoveSize;
	}

	// ���� ������� �뷮 ���.
	int CProtocolBuff_Net::GetUseSize(void)
	{
		return m_Rear - m_Front;
	}

	// ���� ���ۿ� ���� �뷮 ���.
	int	CProtocolBuff_Net::GetFreeSize(void)
	{
		return m_Size - m_Rear;
	}

	// static �Լ� -----------
		// Alloc. ����� �� �ȿ��� Alloc ��, ���۷��� ī��Ʈ 1 ����
	CProtocolBuff_Net* CProtocolBuff_Net::Alloc()
	{
		return  m_MPool->Alloc();
	}

	// Free. ����� �� �ȿ��� ���۷��� ī��Ʈ 1 ����.
	// ����, ���۷��� ī��Ʈ�� 0�̶�� ������. TLSPool�� Free ��
	void CProtocolBuff_Net::Free(CProtocolBuff_Net* pBuff)
	{
		// ���Ͷ����� �����ϰ� ����.
		// ���� ���� �� 0�̵ƴٸ� delete
		if (InterlockedDecrement(&pBuff->m_RefCount) == 0)
		{
			pBuff->m_Rear = dfNETWORK_PACKET_HEADER_SIZE_NETSERVER;		// rear �� 2�� �ʱ�ȭ. ��� ���� Ȯ��
			pBuff->m_RefCount = 1;	// ref�� 1�� �ʱ�ȭ
			pBuff->m_bHeadCheck = false;	// ��� ä�� ���� false�� ����
			m_MPool->Free(pBuff);
		}
	}

	// �Ϲ� �Լ� -----------
	// ���۷��� ī��Ʈ 1 Add�ϴ� �Լ�
	void CProtocolBuff_Net::Add()
	{
		// ���Ͷ����� �����ϰ� ����
		InterlockedIncrement(&m_RefCount);
	}




	// ------------------------------------
	// ------------------------------------
	// ------------------------------------
	// ------------------------------------
	// ���ܿ�

	// ������
	CException::CException(const wchar_t* str)
	{
		_tcscpy_s(ExceptionText, _MyCountof(ExceptionText), str);
	}

	// ���� �ؽ�Ʈ�� �ּ� ��ȯ
	char* CException::GetExceptionText()
	{
		return (char*)&ExceptionText;
	}
}