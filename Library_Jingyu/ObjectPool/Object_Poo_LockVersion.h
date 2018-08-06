#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__

#include <new.h>
#include <stdlib.h>

namespace Library_Jingyu
{

#define MEMORYPOOL_ENDCODE	890226

	template <typename DATA>
	class CMemoryPool
	{
	private:
		/* **************************************************************** */
		// �� ���� �տ� ���� ��� ����ü.
		/* **************************************************************** */
		struct st_BLOCK_NODE
		{
			DATA		  stData;
			st_BLOCK_NODE *stpNextBlock;
			int			  stMyCode;
		};

	private:
		char* m_Memory;				// ���߿� �Ҹ��ڿ��� �� ���� free�ϱ� ���� ����
		int m_iBlockNum;			// �ִ� ���� ����
		bool m_bPlacementNew;		// �÷��̽���Ʈ �� ����
		int m_iAllocCount;			// Ȯ���� ���� ����. ���ο� ������ �Ҵ��� �� ���� 1�� ����. �ش� �޸�Ǯ�� �Ҵ��� �޸� ���� ��
		int m_iUseCount;			// ������ ��� ���� ���� ��. Alloc�� 1 ���� / free�� 1 ����
		st_BLOCK_NODE* m_pTop;		// Top ��ġ�� ����ų ����

		SRWLOCK sl;

	public:
		//////////////////////////////////////////////////////////////////////////
		// ������, �ı���.
		//
		// Parameters:	(int) �ִ� ���� ����.
		//				(bool) ������ ȣ�� ����.
		// Return:
		//////////////////////////////////////////////////////////////////////////
		CMemoryPool(int iBlockNum, bool bPlacementNew = false);
		virtual	~CMemoryPool();


		//////////////////////////////////////////////////////////////////////////
		// ���� �ϳ��� �Ҵ�޴´�.
		//
		// Parameters: ����.
		// Return: (DATA *) ����Ÿ ���� ������.
		//////////////////////////////////////////////////////////////////////////
		DATA	*Alloc(void);

		//////////////////////////////////////////////////////////////////////////
		// ������̴� ������ �����Ѵ�.
		//
		// Parameters: (DATA *) ���� ������.
		// Return: (BOOL) TRUE, FALSE.
		//////////////////////////////////////////////////////////////////////////
		bool	Free(DATA *pData);


		//////////////////////////////////////////////////////////////////////////
		// ���� Ȯ�� �� ���� ������ ��´�. (�޸�Ǯ ������ ��ü ����)
		//
		// Parameters: ����.
		// Return: (int) �޸� Ǯ ���� ��ü ����
		//////////////////////////////////////////////////////////////////////////
		int		GetAllocCount(void) { return m_iAllocCount; }

		//////////////////////////////////////////////////////////////////////////
		// ���� ������� ���� ������ ��´�.
		//
		// Parameters: ����.
		// Return: (int) ������� ���� ����.
		//////////////////////////////////////////////////////////////////////////
		int		GetUseCount(void) { return m_iUseCount; }


		// SRWLock �ɱ�
		void EnterLOCK()
		{	AcquireSRWLockExclusive(&sl);}

		// SRWLock Ǯ��
		void LeaveLOCK()
		{	ReleaseSRWLockExclusive(&sl);	}

	};


	//////////////////////////////////////////////////////////////////////////
	// ������, �ı���.
	//
	// Parameters:	(int) �ִ� ���� ����.
	//				(bool) ������ ȣ�� ����.
	// Return:
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	CMemoryPool<DATA>::CMemoryPool(int iBlockNum, bool bPlacementNew)
	{

		// SRW Lock �ʱ�ȭ
		InitializeSRWLock(&sl);

		/////////////////////////////////////
		// bPlacementNew
		// ���� �ű� ����(����) 1. �޸� ���� �Ҵ�(malloc, ������ ȣ�� ����). �� �� ����� �⺻ ���� ����(next, ���ڵ�)
		// ���� �ű� ���� 2. true ��, '��ü ������' ȣ������ ����.
		// ���� �ű� ���� 2. false ��, '��ü ������' ȣ��
		// 
		// ���� Alloc, Free
		// true 1. �������� ���� ��, '��ü ������' ȣ�� �� ����
		// true 2. �������� ���� ��, '��ü �Ҹ���' ȣ�� �� �޸� Ǯ�� �߰�
		// true 3. ���α׷� ���� ��, �޸�Ǯ �Ҹ��ڿ��� ��� ��带 ���� ����� �޸𸮸� 'free()' ��Ų��.
		//
		// false 1. �������� ���� ��, �״�� ����
		// false 2. �������� ���� ��, �״�� �޸�Ǯ�� �߰�
		// false 3. ���α׷� ���� ��, �޸�Ǯ �Ҹ��ڿ��� ��� ��带 ���� ��� DATA�� '��ü �Ҹ���' ȣ��. �׸��� ��带 'free()' ��Ų��.
		//
		// bPlacementNew�� TRUE : Alloc() ��, DATA�� Placement new�� ������ ȣ�� �� �ѱ�, Free() ��, DATA�� Placement new�� �Ҹ��� ȣ�� �� �� �޸� Ǯ�� �ִ´�. CMemoryPool�Ҹ��ڿ���, �ش� ��� free
		// bPlacementNew�� FALSE : Alloc() ��, DATA�� ������ ȣ�� ����. Free()�� �Ҹ��� ȣ�� ����. CMemoryPool�Ҹ��ڿ���, �ش� ��� free
		/////////////////////////////////////

		// ��� ���� ����	
		m_iBlockNum = iBlockNum;
		m_bPlacementNew = bPlacementNew;
		m_iAllocCount = m_iUseCount = 0;

		// iBlockNum > 0�̶��,
		if (iBlockNum > 0)
		{
			// ������Ʈ ����, �� ������ ����.
			int iMemSize = sizeof(st_BLOCK_NODE) * m_iBlockNum;

			// ������ ��ŭ �޸� �����Ҵ�.
			char* pMemory = (char*)malloc(iMemSize);

			// pMemory ����صд�. ���� �Ҹ��ڿ��� free�ϱ� ���ؼ�
			m_Memory = pMemory;

			// ù ��ġ�� Top���� ����Ų��. 
			m_pTop = (st_BLOCK_NODE*)pMemory;

			// ���� 1���� ��� ����
			st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pMemory;
			if (bPlacementNew == false)
				new (&pNode->stData) DATA();
			pNode->stpNextBlock = NULL;
			pNode->stMyCode = MEMORYPOOL_ENDCODE;
			pMemory += sizeof(st_BLOCK_NODE);

			m_iAllocCount++;

			//  iBlockNum - 1 �� ��ŭ ��� �ʱ�ȭ
			for (int i = 1; i < iBlockNum; ++i)
			{
				st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pMemory;

				if (bPlacementNew == false)
					new (&pNode->stData) DATA();

				pNode->stpNextBlock = m_pTop;
				m_pTop = pNode;

				/*
				if (i == 0)
					pNode->stpNextBlock = NULL;

				else
				{
					pNode->stpNextBlock = m_pTop;
					m_pTop = pNode;
				}*/

				pNode->stMyCode = MEMORYPOOL_ENDCODE;

				pMemory += sizeof(st_BLOCK_NODE);

				m_iAllocCount++;
			}
		}

		// iBlockNum�� 0���� �۰ų� ���ٸ�, ���� 1���� �����.
		else
		{
			st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
			if (bPlacementNew == false)
				new (&pNode->stData) DATA();
			pNode->stpNextBlock = NULL;
			pNode->stMyCode = MEMORYPOOL_ENDCODE;

			m_pTop = pNode;

			m_iAllocCount++;
		}

	}

	template <typename DATA>
	CMemoryPool<DATA>::~CMemoryPool()
	{
		// �� Ǯ�� �ִ� ��� ��带 ��������
		// �÷��̽���Ʈ ���� ��� �ߵ� ���ߵ� delete�� �ȵ�. �Ҹ��ڰ� ������ ȣ��Ǵϱ�!! free�� �޸� �����ؾ���. 
		// �÷��̽���Ʈ ���� �׳� �ʱ�ȭ�� ��, ���� ������ �Ҵ��ϴ� ������ �ƴ�.
		//
		// �÷��̽���Ʈ ���� ����� ��� : �̹�, Free() �Լ����� �Ҹ��ڸ� ȣ�������� �Ҹ��� ȣ�� ����. 
		// �÷��̽���Ʈ ���� ������ ��� : �̹��� �Ҹ��ڸ� ȣ���Ѵ�. 
		//
		// m_iBlockNum�� 0�̶��, ������ malloc �� ���̴� ������ free �Ѵ�. (Alloc�� ���� ��������. Free()������ �÷��̽���Ʈ ���� ���� �Ҹ��ڸ� ȣ���߾���. �޸� ���� ������)
		// m_iBlockNum�� 0���� ũ�ٸ�, �������� �� ���� �޸� ��ü free �Ѵ�. 
		

		// �� �޸�Ǯ�� �ִ� ��带 ��� 'free()' �Ѵ�.
		while (1)
		{
			if (m_pTop == NULL)
				break;

			st_BLOCK_NODE* deleteNode = (st_BLOCK_NODE*)m_pTop;
			m_pTop = m_pTop->stpNextBlock;

			// �÷��̽���Ʈ ���� false���, Free()�� '��ü �Ҹ���'�� ȣ�� ���� ���̴� ���⼭ ȣ�������� �Ѵ�.
			if (m_bPlacementNew == false)
				deleteNode->stData.~DATA();

			// m_iBlockNum�� 0�̶��, ������ Malloc������(Alloc�� ���� ��������. Free()������ �Ҹ��ڸ� ȣ���ϰ�, �޸� ������ ����.) free�Ѵ�.
			if(m_iBlockNum == 0)
				free(deleteNode);
		}

		//  m_iBlockNum�� 0���� ũ�ٸ�, �� ���� malloc �� ���̴� �ѹ��� free�Ѵ�.
		if (m_iBlockNum > 0)
			free(m_Memory);
	}

	//////////////////////////////////////////////////////////////////////////
	// ���� �ϳ��� �Ҵ�޴´�.
	//
	// Parameters: ����.
	// Return: (DATA *) ����Ÿ ���� ������.
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	DATA*	CMemoryPool<DATA>::Alloc(void)
	{
		//////////////////////////////////
		// m_pTop�� NULL�϶� ó��
		//////////////////////////////////
		if (m_pTop == NULL)
		{
			if (m_iBlockNum > 0)
				return nullptr;

			else
			{
				st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)malloc(sizeof(st_BLOCK_NODE));
				pNode->stpNextBlock = NULL;
				pNode->stMyCode = MEMORYPOOL_ENDCODE;

				// �÷��̽���Ʈ �� ��� ���ο� ���� ����, ������� �Դµ� m_iBlockNum < 0 �̶��, ������ ���� �����ϴ� ��.
				// �÷��̽���Ʈ �� ȣ��
				new (&pNode->stData) DATA();

				m_iAllocCount++;
				m_iUseCount++;

				return (DATA*)&pNode->stData;
			}
		}

		//////////////////////////////////
		// m_pTop�� NULL�� �ƴ� �� ó��
		//////////////////////////////////
		st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)m_pTop;
		m_pTop = pNode->stpNextBlock;

		// �÷��̽���Ʈ ���� ����Ѵٸ� ����ڿ��� �ֱ����� '��ü ������' ȣ��
		if (m_bPlacementNew == true)
			new (&pNode->stData) DATA();

		m_iUseCount++;

		return (DATA*)&pNode->stData;
	}

	//////////////////////////////////////////////////////////////////////////
	// ������̴� ������ �����Ѵ�.
	//
	// Parameters: (DATA *) ���� ������.
	// Return: (BOOL) TRUE, FALSE.
	//////////////////////////////////////////////////////////////////////////
	template <typename DATA>
	bool	CMemoryPool<DATA>::Free(DATA *pData)
	{
		// �̻��� �����Ͱ����� �׳� ����
		if (pData == NULL)
			return false;

		// ���� �Ҵ��� ������ �´��� Ȯ��
		st_BLOCK_NODE* pNode = (st_BLOCK_NODE*)pData;

		if (pNode->stMyCode != MEMORYPOOL_ENDCODE)
			return false;

		//�÷��̽���Ʈ ���� ����Ѵٸ� �޸� Ǯ�� �߰��ϱ� ���� '��ü �Ҹ���' ȣ��
		if (m_bPlacementNew == true)
			pData->~DATA();

		// m_pTop�� �����Ѵ�.
		pNode->stpNextBlock = m_pTop;
		m_pTop = pNode;

		m_iUseCount--;

		return true;
	}
}

#endif // !__OBJECT_POOL_H__