#include "stdafx.h"

#pragma comment(lib,"ws2_32")
#include <Ws2tcpip.h>

#include <process.h>
#include <Windows.h>
#include <strsafe.h>

#include "NetworkLib.h"
#include "RingBuff\RingBuff.h"
#include "Log\Log.h"
#include "CrashDump\CrashDump.h"


<<<<<<< HEAD
LONG g_llPacketAllocCount = 0;
=======
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04


namespace Library_Jingyu
{
	// ·Î±× ÂïÀ» Àü¿ªº¯¼ö ÇÏ³ª ¹Þ±â.
	CSystemLog* cNetLibLog = CSystemLog::GetInstance();	

	// ´ýÇÁ ³²±æ º¯¼ö ÇÏ³ª ¹Þ±â
	CCrashDump* cNetDump = CCrashDump::GetInstance();


	// Çì´õ »çÀÌÁî
	#define dfNETWORK_PACKET_HEADER_SIZE	2

	// ÇÑ ¹ø¿¡ »÷µåÇÒ ¼ö ÀÖ´Â WSABUFÀÇ Ä«¿îÆ®
	#define dfSENDPOST_MAX_WSABUF			100
<<<<<<< HEAD
	
	void PacketAllocCountAdd()
	{
		InterlockedIncrement(&g_llPacketAllocCount);
	}

	LONG PacketAllocCountSub()
	{
		return InterlockedDecrement(&g_llPacketAllocCount);
	}

	LONG PacketAllocCount_Get()
	{
		return g_llPacketAllocCount;
	}
=======
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04


	// ------------------------------
	// enum°ú ±¸Á¶Ã¼
	// ------------------------------
	// enum class
	enum class CLanServer::euError : int
	{
		NETWORK_LIB_ERROR__NORMAL = 0,					// ¿¡·¯ ¾ø´Â ±âº» »óÅÂ
		NETWORK_LIB_ERROR__WINSTARTUP_FAIL,				// À©¼Ó ÃÊ±âÈ­ ÇÏ´Ù°¡ ¿¡·¯³²
		NETWORK_LIB_ERROR__CREATE_IOCP_PORT,			// IPCP ¸¸µé´Ù°¡ ¿¡·¯³²
		NETWORK_LIB_ERROR__W_THREAD_CREATE_FAIL,		// ¿öÄ¿½º·¹µå ¸¸µé´Ù°¡ ½ÇÆÐ 
		NETWORK_LIB_ERROR__A_THREAD_CREATE_FAIL,		// ¿¢¼ÁÆ® ½º·¹µå ¸¸µé´Ù°¡ ½ÇÆÐ 
		NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL,			// ¼ÒÄÏ »ý¼º ½ÇÆÐ 
		NETWORK_LIB_ERROR__BINDING_FAIL,				// ¹ÙÀÎµù ½ÇÆÐ
		NETWORK_LIB_ERROR__LISTEN_FAIL,					// ¸®½¼ ½ÇÆÐ
		NETWORK_LIB_ERROR__SOCKOPT_FAIL,				// ¼ÒÄÏ ¿É¼Ç º¯°æ ½ÇÆÐ
		NETWORK_LIB_ERROR__IOCP_ERROR,					// IOCP ÀÚÃ¼ ¿¡·¯
		NETWORK_LIB_ERROR__NOT_FIND_CLINET,				// map °Ë»ö µîÀ» ÇÒ¶§ Å¬¶óÀÌ¾ðÆ®¸¦ ¸øÃ£Àº°æ¿ì.
		NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL	,		// Enqueue»çÀÌÁî°¡ ²ËÂù À¯Àú
		NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY,			// Dequeue ½Ã, Å¥°¡ ºñ¾îÀÖ´Â À¯Àú. PeekÀ» ½ÃµµÇÏ´Âµ¥ Å¥°¡ ºñ¾úÀ» »óÈ²Àº ¾øÀ½
		NETWORK_LIB_ERROR__WSASEND_FAIL,				// SendPost¿¡¼­ WSASend ½ÇÆÐ
		NETWORK_LIB_ERROR__WSAENOBUFS,					// WSASend, WSARecv½Ã ¹öÆÛ»çÀÌÁî ºÎÁ·
		NETWORK_LIB_ERROR__EMPTY_RECV_BUFF,				// Recv ¿Ï·áÅëÁö°¡ ¿Ô´Âµ¥, ¸®½Ãºê ¹öÆÛ°¡ ºñ¾îÀÖ´Ù°í ³ª¿À´Â À¯Àú.
		NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT,		// ¿¢¼ÁÆ® ½º·¹µå ºñÁ¤»ó Á¾·á. º¸Åë accept()ÇÔ¼ö¿¡¼­ ÀÌ»óÇÑ ¿¡·¯°¡ ³ª¿Â°Í.
		NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT,		// ¿öÄ¿ ½º·¹µå ºñÁ¤»ó Á¾·á. 
		NETWORK_LIB_ERROR__WFSO_ERROR,					// WaitForSingleObject ¿¡·¯.
		NETWORK_LIB_ERROR__IOCP_IO_FAIL					// IOCP¿¡¼­ I/O ½ÇÆÐ ¿¡·¯. ÀÌ ¶§´Â, ÀÏÁ¤ È½¼ö´Â I/O¸¦ Àç½ÃµµÇÑ´Ù.
	};

	// ¼¼¼Ç ±¸Á¶Ã¼
	struct CLanServer::stSession
	{
		SOCKET m_Client_sock;

		TCHAR m_IP[30];
		USHORT m_prot;

		ULONGLONG m_ullSessionID;

		LONG	m_lIOCount;

		// Send°¡´É »óÅÂÀÎÁö Ã¼Å©. 1ÀÌ¸é SendÁß, 0ÀÌ¸é SendÁß ¾Æ´Ô
		LONG	m_lSendFlag;
		
		// I/O°¡ ¿ÜºÎÀûÀÎ ¿äÀÎ(¹°¸®ÀûÀÎ ¿¬°á ¿¡·¯¶ó´ø°¡..)À¸·Î ½ÇÆÐÇßÀ» ¶§ ¾à 5È¸Á¤µµ Àç½Ãµµ ÇØº»´Ù. 
		// ÀÌ ½ÃµµÇÑ È½¼ö¸¦ ÀúÀåÇÒ º¯¼ö. ½ÇÆÐÇÒ ¶§ ¸¶´Ù 1¾¿ Áõ°¡ÇÏ´Ù°¡, 5°¡µÇ¸é ´õ ÀÌ»ó Á¢¼Ó½Ãµµ ¾ÈÇÔ
		// I/O°¡ ¼º°øÇÏ¸é ±× Áï½Ã 0ÀÌµÈ´Ù.
		//LONG	m_lReyryCount;

		// ÇØ´ç ÀÎµ¦½º ¹è¿­ÀÌ »ç¿ëÁßÀÎÁö Ã¼Å©
		// 1ÀÌ¸é »ç¿ëÁß, 0ÀÌ¸é »ç¿ëÁß ¾Æ´Ô
		LONG m_lUseFlag;

		// ÇØ´ç ¹è¿­ÀÇ ÇöÀç ÀÎµ¦½º
		ULONGLONG m_lIndex;

		// ÇöÀç, WSASend¿¡ ¸î °³ÀÇ µ¥ÀÌÅÍ¸¦ »÷µåÇß´Â°¡. (¹ÙÀÌÆ® ¾Æ´Ô! Ä«¿îÆ®. ÁÖÀÇ!)
		long m_iWSASendCount;
		
		// SendPost¿¡¼­ »÷µå¸¦ ÇÑ µ¥ÀÌÅÍÀÇ Æ÷ÀÎÅÍµé ¹è¿­
		CProtocolBuff* m_cpbufSendPayload[dfSENDPOST_MAX_WSABUF];

		CRingBuff m_RecvQueue;
		CRingBuff m_SendQueue;

		OVERLAPPED m_overRecvOverlapped;
		OVERLAPPED m_overSendOverlapped;

		// »ý¼ºÀÚ
		stSession()
		{			
			m_lIOCount = 0;
			m_lSendFlag = 0;
			m_lIndex = 0;
			m_lUseFlag = FALSE;
			m_iWSASendCount = 0;
		}

		void Reset_Func()
		{
			// -- SendFlag, I/OÄ«¿îÆ®, SendCount ÃÊ±âÈ­
			m_lIOCount = 0;
			m_lSendFlag = 0;

			// Å¥ ÃÊ±âÈ­
			m_RecvQueue.ClearBuffer();
			m_SendQueue.ClearBuffer();
		}

		// ¼Ò¸êÀÚ
		// ÇÊ¿ä¾ø¾îÁü
		//~stSession() {}

	
	};

	// ¹Ì»ç¿ë ÀÎµ¦½º °ü¸® ±¸Á¶Ã¼(½ºÅÃ)
	struct CLanServer::stEmptyStack
	{
		// ½ºÅÃÀº Last-In-First_Out
		int m_iTop;
		int m_iMax;

		ULONGLONG* m_iArray;

		// ÀÎµ¦½º ¾ò±â
		//
		// ¹ÝÈ¯°ª ULONGLONG ---> ÀÌ ¹ÝÈ¯°ªÀ¸·Î ½ÃÇÁÆ® ¿¬»ê ÇÒ ¼öµµ ÀÖ¾î¼­, È¤½Ã ¸ð¸£´Ï unsigned·Î ¸®ÅÏÇÑ´Ù.
		// 0ÀÌ»ó(0Æ÷ÇÔ) : ÀÎµ¦½º Á¤»ó ¹ÝÈ¯
		// 10000000(Ãµ¸¸) : ºó ÀÎµ¦½º ¾øÀ½.
		ULONGLONG Pop()
		{
			// ÀÎµ¦½º ºñ¾ú³ª Ã¼Å© ----------
			// ºó ÀÎµ¦½º°¡ ¾øÀ¸¸é 18,000,000,000,000,000,000 
			// º¸Åë Max·Î ¼³Á¤ÇÑ À¯Àú°¡ ¸ðµÎ µé¾î¿ÔÀ» °æ¿ì, ºó ÀÎµ¦½º°¡ ¾ø´Ù.
			if (m_iTop == 0)
				return 10000000;

			// ºó ÀÎµ¦½º°¡ ÀÖÀ¸¸é, m_iTopÀ» °¨¼Ò ÈÄ ÀÎµ¦½º ¸®ÅÏ.
			m_iTop--;			

			return m_iArray[m_iTop];
		}

		// ÀÎµ¦½º ³Ö±â
		//
		// ¹ÝÈ¯°ª bool
		// true : ÀÎµ¦½º Á¤»óÀ¸·Î µé¾î°¨
		// false : ÀÎµ¦½º µé¾î°¡Áö ¾ÊÀ½ (ÀÌ¹Ì Max¸¸Å­ ²Ë Âü)
		bool Push(ULONGLONG PushIndex)
		{
			// ÀÎµ¦½º°¡ ²ËÃ¡³ª Ã¼Å© ---------
			// ÀÎµ¦½º°¡ ²ËÃ¡À¸¸é false ¸®ÅÏ
			if (m_iTop == m_iMax)
				return false;

			// ÀÎµ¦½º°¡ ²ËÂ÷Áö ¾Ê¾ÒÀ¸¸é, Ãß°¡
			m_iArray[m_iTop] = PushIndex;
			m_iTop++;

			return true;
		}

		stEmptyStack(int Max)
		{
			m_iTop = Max;
			m_iMax = Max;

			m_iArray = new ULONGLONG[Max];

			// ÃÖÃÊ »ý¼º ½Ã, ¸ðµÎ ºó ÀÎµ¦½º
			for (int i = 0; i < Max; ++i)
				m_iArray[i] = i;
		}

		~stEmptyStack()
		{
			delete[] m_iArray;
		}

	};



	// -----------------------------
	// À¯Àú°¡ È£Ãâ ÇÏ´Â ÇÔ¼ö
	// -----------------------------
	// ¼­¹ö ½ÃÀÛ
	// [¿ÀÇÂ IP(¹ÙÀÎµù ÇÒ IP), Æ÷Æ®, ¿öÄ¿½º·¹µå ¼ö, ¿¢¼ÁÆ® ½º·¹µå ¼ö, TCP_NODELAY »ç¿ë ¿©ºÎ(true¸é »ç¿ë), ÃÖ´ë Á¢¼ÓÀÚ ¼ö] ÀÔ·Â¹ÞÀ½.
	//
	// return false : ¿¡·¯ ¹ß»ý ½Ã. ¿¡·¯ÄÚµå ¼ÂÆÃ ÈÄ false ¸®ÅÏ
	// return true : ¼º°ø
	bool CLanServer::Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect)
	{		

		// »õ·Î ½ÃÀÛÇÏ´Ï±î ¿¡·¯ÄÚµåµé ÃÊ±âÈ­
		m_iOSErrorCode = 0;
		m_iMyErrorCode = (euError)0;		

		// À©¼Ó ÃÊ±âÈ­
		WSADATA wsa;
		int retval = WSAStartup(MAKEWORD(2, 2), &wsa);
		if (retval != 0)
		{
			// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
			m_iOSErrorCode = retval;
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WINSTARTUP_FAIL;

			// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
			ExitFunc(m_iW_ThreadCount);

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WSAStartup() Error : NetError(%d), OSError(%d)", 
				(int)m_iMyErrorCode, m_iOSErrorCode);
		
			// false ¸®ÅÏ
			return false;
		}

		// ÀÔÃâ·Â ¿Ï·á Æ÷Æ® »ý¼º
		m_hIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
		if (m_hIOCPHandle == NULL)
		{
			// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__CREATE_IOCP_PORT;

			// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
			ExitFunc(m_iW_ThreadCount);

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> CreateIoCompletionPort() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);
		
			// false ¸®ÅÏ
			return false;
		}

		// ¿öÄ¿ ½º·¹µå »ý¼º
		m_iW_ThreadCount = WorkerThreadCount;
		m_hWorkerHandle = new HANDLE[WorkerThreadCount];

		for (int i = 0; i < m_iW_ThreadCount; ++i)
		{
			m_hWorkerHandle[i] = (HANDLE)_beginthreadex(0, 0, WorkerThread, this, 0, 0);
			if (m_hWorkerHandle[i] == 0)
			{
				// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__W_THREAD_CREATE_FAIL;

				// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
				ExitFunc(i);

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> WorkerThread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);
			
				// false ¸®ÅÏ
				return false;
			}
		}

		// ¼ÒÄÏ »ý¼º
		m_soListen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_soListen_sock == INVALID_SOCKET)
		{
			// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__CREATE_SOCKET_FAIL;

			// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
			ExitFunc(m_iW_ThreadCount);

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> socket() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false ¸®ÅÏ
			return false;
		
		}

		// ¹ÙÀÎµù
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port = htons(port);
		InetPton(AF_INET, bindIP, &serveraddr.sin_addr.s_addr);

		retval = bind(m_soListen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR)
		{
			// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__BINDING_FAIL;

			// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
			ExitFunc(m_iW_ThreadCount);

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> bind() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false ¸®ÅÏ
			return false;
		}

		// ¸®½¼
		retval = listen(m_soListen_sock, SOMAXCONN);
		if (retval == SOCKET_ERROR)
		{
			// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__LISTEN_FAIL;
		
			// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
			ExitFunc(m_iW_ThreadCount);

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> listen() Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false ¸®ÅÏ
			return false;
		}

		// ¿¬°áµÈ ¸®½¼ ¼ÒÄÏÀÇ ¼Û½Å¹öÆÛ Å©±â¸¦ 0À¸·Î º¯°æ. ±×·¡¾ß Á¤»óÀûÀ¸·Î ºñµ¿±â ÀÔÃâ·ÂÀ¸·Î ½ÇÇà
		// ¸®½¼ ¼ÒÄÏ¸¸ ¹Ù²Ù¸é ¸ðµç Å¬¶ó ¼Û½Å¹öÆÛ Å©±â´Â 0ÀÌµÈ´Ù.
		int optval = 0;
		retval = setsockopt(m_soListen_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optval, sizeof(optval));
		if (optval == SOCKET_ERROR)
		{
			// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
			m_iOSErrorCode = WSAGetLastError();
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

			// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
			ExitFunc(m_iW_ThreadCount);

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> setsockopt() SendBuff Size Change Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// false ¸®ÅÏ
			return false;
		}

		// ÀÎÀÚ·Î ¹ÞÀº ³ëµô·¹ÀÌ ¿É¼Ç »ç¿ë ¿©ºÎ¿¡ µû¶ó ³×ÀÌ±Û ¿É¼Ç °áÁ¤
		// ÀÌ°Ô true¸é ³ëµô·¹ÀÌ »ç¿ëÇÏ°Ú´Ù´Â °Í(³×ÀÌ±Û ÁßÁö½ÃÄÑ¾ßÇÔ)
		if (Nodelay == true)
		{
			BOOL optval = TRUE;
			retval = setsockopt(m_soListen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&optval, sizeof(optval));

			if (retval == SOCKET_ERROR)
			{
				// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
				m_iOSErrorCode = WSAGetLastError();
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SOCKOPT_FAIL;

				// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
				ExitFunc(m_iW_ThreadCount);

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> setsockopt() Nodelay apply Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false ¸®ÅÏ
				return false;
			}
		}

		// ÃÖ´ë Á¢¼Ó °¡´É À¯Àú ¼ö ¼ÂÆÃ
		m_iMaxJoinUser = MaxConnect;

		// ¼¼¼Ç ¹è¿­ µ¿ÀûÇÒ´ç, ¹Ì»ç¿ë ¼¼¼Ç °ü¸® ½ºÅÃ µ¿ÀûÇÒ´ç.
		m_stSessionArray = new stSession[MaxConnect];
		m_stEmptyIndexStack = new stEmptyStack(MaxConnect);

		// ¿¢¼ÁÆ® ½º·¹µå »ý¼º
		m_iA_ThreadCount = AcceptThreadCount;
		m_hAcceptHandle = new HANDLE[m_iA_ThreadCount];
		for (int i = 0; i < m_iA_ThreadCount; ++i)
		{
			m_hAcceptHandle[i] = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
			if (m_hAcceptHandle == NULL)
			{
				// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
				m_iOSErrorCode = errno;
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_CREATE_FAIL;

				// °¢Á¾ ÇÚµé ¹ÝÈ¯ ¹× µ¿ÀûÇØÁ¦ ÀýÂ÷.
				ExitFunc(m_iW_ThreadCount);

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Start() --> Accept Thread Create Error : NetError(%d), OSError(%d)",
					(int)m_iMyErrorCode, m_iOSErrorCode);

				// false ¸®ÅÏ
				return false;
			}
		}

		

		// ¼­¹ö ¿­·ÈÀ½ !!
		m_bServerLife = true;		

		// ¼­¹ö ¿ÀÇÂ ·Î±× Âï±â		
		cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerOpen...");

		return true;
	}

	// ¼­¹ö ½ºÅ¾.
	void CLanServer::Stop()
	{	

		// 1. Accept ½º·¹µå Á¾·á. ´õ ÀÌ»ó Á¢¼ÓÀ» ¹ÞÀ¸¸é ¾ÈµÇ´Ï Accept½º·¹µå ¸ÕÀú Á¾·á
		// Accept ½º·¹µå´Â ¸®½¼¼ÒÄÏÀ» closesocketÇÏ¸é µÈ´Ù.
		closesocket(m_soListen_sock);

		// Accept½º·¹µå Á¾·á ´ë±â
		DWORD retval = WaitForMultipleObjects(m_iA_ThreadCount, m_hAcceptHandle, TRUE, INFINITE);

		// ¸®ÅÏ°ªÀÌ [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iW_ThreadCount - 1] °¡ ¾Æ´Ï¶ó¸é, ¹º°¡ ¿¡·¯°¡ ¹ß»ýÇÑ °Í. ¿¡·¯ Âï´Â´Ù
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iW_ThreadCount - 1)
		{
			// ¿¡·¯ °ªÀÌ WAIT_FAILEDÀÏ °æ¿ì, GetLastError()·Î È®ÀÎÇØ¾ßÇÔ.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();

			// ±×°Ô ¾Æ´Ï¶ó¸é ¸®ÅÏ°ª¿¡ ÀÌ¹Ì ¿¡·¯°¡ µé¾îÀÖÀ½.
			else
				m_iOSErrorCode = retval;

			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WFSO_ERROR;

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Accept Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// ¿¡·¯ ¹ß»ý ÇÔ¼ö È£Ãâ
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Accept Thread EXIT Error");
		}

		// 2. ¸ðµç À¯Àú¿¡°Ô Shutdown
		// ¸ðµç À¯Àú¿¡°Ô ¼Ë´Ù¿î ³¯¸²
		for (int i = 0; i < m_iMaxJoinUser; ++i)
		{
			if(m_stSessionArray[i].m_lUseFlag == TRUE)
				shutdown(m_stSessionArray[i].m_Client_sock, SD_BOTH);				
		}

		// ¸ðµç À¯Àú°¡ Á¾·áµÇ¾ú´ÂÁö Ã¼Å©
		while (1)
		{
			if (m_ullJoinUserCount == 0)
				break;

			Sleep(1);
		}

		// 3. ¿öÄ¿ ½º·¹µå Á¾·á
		for (int i = 0; i<m_iW_ThreadCount; ++i)
			PostQueuedCompletionStatus(m_hIOCPHandle, 0, 0, 0);

		// ¿öÄ¿½º·¹µå Á¾·á ´ë±â
		retval = WaitForMultipleObjects(m_iW_ThreadCount, m_hWorkerHandle, TRUE, INFINITE);

		// ¸®ÅÏ°ªÀÌ [WAIT_OBJECT_0 ~ WAIT_OBJECT_0 + m_iW_ThreadCount - 1] °¡ ¾Æ´Ï¶ó¸é, ¹º°¡ ¿¡·¯°¡ ¹ß»ýÇÑ °Í. ¿¡·¯ Âï´Â´Ù
		if (retval < WAIT_OBJECT_0 &&
			retval > WAIT_OBJECT_0 + m_iW_ThreadCount - 1)
		{
			// ¿¡·¯ °ªÀÌ WAIT_FAILEDÀÏ °æ¿ì, GetLastError()·Î È®ÀÎÇØ¾ßÇÔ.
			if (retval == WAIT_FAILED)
				m_iOSErrorCode = GetLastError();
			
			// ±×°Ô ¾Æ´Ï¶ó¸é ¸®ÅÏ°ª¿¡ ÀÌ¹Ì ¿¡·¯°¡ µé¾îÀÖÀ½.
			else
				m_iOSErrorCode = retval;

			// ³» ¿¡·¯ ¼ÂÆÃ
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__W_THREAD_ABNORMAL_EXIT;

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"Stop() --> Worker Thread EXIT Error : NetError(%d), OSError(%d)",
				(int)m_iMyErrorCode, m_iOSErrorCode);

			// ¿¡·¯ ¹ß»ý ÇÔ¼ö È£Ãâ
			OnError((int)euError::NETWORK_LIB_ERROR__WFSO_ERROR, L"Stop() --> Worker Thread EXIT Error");
		}

		// 4. °¢Á¾ ¸®¼Ò½º ¹ÝÈ¯
		// 1) ¿¢¼ÁÆ® ½º·¹µå ÇÚµé ¹ÝÈ¯
		for(int i=0; i<m_iA_ThreadCount; ++i)
			CloseHandle(m_hAcceptHandle[i]);

		// 2) ¿öÄ¿ ½º·¹µå ÇÚµé ¹ÝÈ¯
		for (int i = 0; i < m_iW_ThreadCount; ++i)
			CloseHandle(m_hWorkerHandle[i]);

		// 3) ¿öÄ¿ ½º·¹µå ¹è¿­, ¿¢¼ÁÆ® ½º·¹µå ¹è¿­ µ¿ÀûÇØÁ¦
		delete[] m_hWorkerHandle;
		delete[] m_hAcceptHandle;

		// 4) IOCPÇÚµé ¹ÝÈ¯
		CloseHandle(m_hIOCPHandle);

		// 5) À©¼Ó ÇØÁ¦
		WSACleanup();

		// 6) ¼¼¼Ç ¹è¿­, ¼¼¼Ç ¹Ì»ç¿ë ÀÎµ¦½º °ü¸® ½ºÅÃ µ¿ÀûÇØÁ¦
		delete[] m_stSessionArray;
		delete[] m_stEmptyIndexStack;

		// 5. ¼­¹ö °¡µ¿Áß ¾Æ´Ô »óÅÂ·Î º¯°æ
		m_bServerLife = false;

		// 6. °¢Á¾ º¯¼ö ÃÊ±âÈ­
		Reset();

		// 7. ¼­¹ö Á¾·á ·Î±× Âï±â		
<<<<<<< HEAD
		_tprintf_s(L"PacketCount : [%d]\n", g_llPacketAllocCount);

		//g_llPacketAllocCount = 0;
=======
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04
		cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_SYSTEM, L"ServerClose...");
	}

	// ¿ÜºÎ¿¡¼­, ¾î¶² µ¥ÀÌÅÍ¸¦ º¸³»°í ½ÍÀ»¶§ È£ÃâÇÏ´Â ÇÔ¼ö.
	// SendPacketÀº ±×³É ¾Æ¹«¶§³ª ÇÏ¸é µÈ´Ù.
	// ÇØ´ç À¯ÀúÀÇ SendQ¿¡ ³Ö¾îµ×´Ù°¡ ¶§°¡ µÇ¸é º¸³½´Ù.
	//
	// return true : SendQ¿¡ ¼º°øÀûÀ¸·Î µ¥ÀÌÅÍ ³ÖÀ½.
	// return false : SendQ¿¡ µ¥ÀÌÅÍ ³Ö±â ½ÇÆÐ or ¿øÇÏ´ø À¯Àú ¸øÃ£À½
	bool CLanServer::SendPacket(ULONGLONG ClinetID, CProtocolBuff* payloadBuff)
	{
		// 1. ClinetID·Î ¼¼¼ÇÀÇ Index ¾Ë¾Æ¿À±â
		ULONGLONG wArrayIndex = GetSessionIndex(ClinetID);

		// 2. ¹Ì»ç¿ë ¹è¿­ÀÌ¸é ¹º°¡ Àß¸øµÈ °ÍÀÌ´Ï false ¸®ÅÏ
		if (m_stSessionArray[wArrayIndex].m_lUseFlag == FALSE)
		{
			// À¯Àú°¡ È£ÃâÇÑ ÇÔ¼ö´Â, ¿¡·¯ È®ÀÎÀÌ °¡´ÉÇÏ±â ¶§¹®¿¡ OnErrorÇÔ¼ö È£Ãâ ¾ÈÇÔ.
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__NOT_FIND_CLINET;

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			// ÇÊ¿äÇÏ¸é Âï´Â´Ù. Áö±Ý ¾Æ·¡ÀÖ´Â°Ç °ú°Å¿¡ ¾²´ø°Å
			//cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Not Fine Clinet :  NetError(%d)",(int)m_iMyErrorCode);

			return false;
		}		
	
		// 3. Çì´õ¸¦ ³Ö¾î¼­, ÆÐÅ¶ ¿Ï¼ºÇÏ±â
		SetProtocolBuff_HeaderSet(payloadBuff);

		// 4. ÀÎÅ¥. ÆÐÅ¶ÀÇ "ÁÖ¼Ò"¸¦ ÀÎÅ¥ÇÑ´Ù(8¹ÙÀÌÆ®)
		void* payloadBuffAddr = payloadBuff;

		int EnqueueCheck = m_stSessionArray[wArrayIndex].m_SendQueue.Enqueue((char*)&payloadBuffAddr, sizeof(void*));
		if (EnqueueCheck == -1)
		{
			// À¯Àú°¡ È£ÃâÇÑ ÇÔ¼ö´Â, ¿¡·¯ È®ÀÎÀÌ °¡´ÉÇÏ±â ¶§¹®¿¡ OnErrorÇÔ¼ö È£Ãâ ¾ÈÇÔ.
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__SEND_QUEUE_SIZE_FULL;

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Send Queue Full Clinet : [%s : %d] NetError(%d)",
				m_stSessionArray[wArrayIndex].m_IP, m_stSessionArray[wArrayIndex].m_prot, (int)m_iMyErrorCode);

			// ÇØ´ç À¯Àú´Â Á¢¼ÓÀ» ²÷´Â´Ù.
			shutdown(m_stSessionArray[wArrayIndex].m_Client_sock, SD_BOTH);
			
			return false;
		}		

		// 4. SendPost½Ãµµ
		bool Check = SendPost(&m_stSessionArray[wArrayIndex]);		

		return Check;
	}

	// ÁöÁ¤ÇÑ À¯Àú¸¦ ²÷À» ¶§ È£ÃâÇÏ´Â ÇÔ¼ö. ¿ÜºÎ ¿¡¼­ »ç¿ë.
	// ¶óÀÌºê·¯¸®ÇÑÅ× ²÷¾îÁà!¶ó°í ¿äÃ»ÇÏ´Â °Í »Ó
	//
	// return true : ÇØ´ç À¯Àú¿¡°Ô ¼Ë´Ù¿î Àß ³¯¸².
	// return false : Á¢¼ÓÁßÀÌÁö ¾ÊÀº À¯Àú¸¦ Á¢¼ÓÇØÁ¦ÇÏ·Á°í ÇÔ.
	bool CLanServer::Disconnect(ULONGLONG ClinetID)
	{
		// 1. ClinetID·Î ¼¼¼ÇÀÇ Index ¾Ë¾Æ¿À±â
		ULONGLONG wArrayIndex = GetSessionIndex(ClinetID);

		// 2. ³»°¡ Ã£´ø À¯Àú°¡ ¾Æ´Ï°Å³ª, ¹Ì»ç¿ë ¹è¿­ÀÌ¸é ¹º°¡ Àß¸øµÈ °ÍÀÌ´Ï false ¸®ÅÏ
		if (m_stSessionArray[wArrayIndex].m_lIndex != wArrayIndex || m_stSessionArray[wArrayIndex].m_lUseFlag == FALSE)
		{
			// ³» ¿¡·¯ ³²±è. (À©µµ¿ì ¿¡·¯´Â ¾øÀ½)
			m_iMyErrorCode = euError::NETWORK_LIB_ERROR__NOT_FIND_CLINET;

			// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"SendPacket() --> Not Fine Clinet : NetError(%d)", (int)m_iMyErrorCode);

			return false;
		}

		// ²÷¾î¾ßÇÏ´Â À¯Àú´Â ¼Ë´Ù¿î ³¯¸°´Ù.
		// Â÷ÈÄ ÀÚ¿¬½º·´°Ô I/OÄ«¿îÆ®°¡ °¨¼ÒµÇ¾î¼­ µð½ºÄ¿³ØÆ®µÈ´Ù.
		shutdown(m_stSessionArray[wArrayIndex].m_Client_sock, SD_BOTH);

		return true;
	}

	///// °¢Á¾ °ÔÅÍÇÔ¼öµé /////
	// À©µµ¿ì ¿¡·¯ ¾ò±â
	int CLanServer::WinGetLastError()
	{
		return m_iOSErrorCode;
	}

	// ³» ¿¡·¯ ¾ò±â
	int CLanServer::NetLibGetLastError()
	{
		return (int)m_iMyErrorCode;
	}

	// ÇöÀç Á¢¼ÓÀÚ ¼ö ¾ò±â
	ULONGLONG CLanServer::GetClientCount()
	{
		return m_ullJoinUserCount;
	}

	// ¼­¹ö °¡µ¿»óÅÂ ¾ò±â
	// return true : °¡µ¿Áß
	// return false : °¡µ¿Áß ¾Æ´Ô
	bool CLanServer::GetServerState()
	{
		return m_bServerLife;
	}
	





	// -----------------------------
	// Lib ³»ºÎ¿¡¼­¸¸ »ç¿ëÇÏ´Â ÇÔ¼ö
	// -----------------------------
	// »ý¼ºÀÚ
	CLanServer::CLanServer()
	{	
		// ·Î±× ÃÊ±âÈ­
		cNetLibLog->SetDirectory(L"LanServer");
		cNetLibLog->SetLogLeve(CSystemLog::en_LogLevel::LEVEL_DEBUG);

		// ¼­¹ö °¡µ¿»óÅÂ false·Î ½ÃÀÛ 
		m_bServerLife = false;

		// SRWLock ÃÊ±âÈ­
		InitializeSRWLock(&m_srwSession_stack_srwl);
	}

	// ¼Ò¸êÀÚ
	CLanServer::~CLanServer()
	{
		// ¼­¹ö°¡ °¡µ¿ÁßÀÌ¾úÀ¸¸é, »óÅÂ¸¦ false ·Î ¹Ù²Ù°í, ¼­¹ö Á¾·áÀýÂ÷ ÁøÇà
		if (m_bServerLife == true )
		{
			m_bServerLife = false;
			Stop();
		}
	}

	// ½ÇÁ¦·Î Á¢¼ÓÁ¾·á ½ÃÅ°´Â ÇÔ¼ö
	void CLanServer::InDisconnect(stSession* DeleteSession)
<<<<<<< HEAD
	{	
=======
	{

		int TempCount = DeleteSession->m_iWSASendCount;
		DeleteSession->m_iWSASendCount = 0;  // º¸³½ Ä«¿îÆ® 0À¸·Î ¸¸µë.
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04

		ULONGLONG sessionID = DeleteSession->m_ullSessionID;

		OnClientLeave(sessionID);

		int TempCount = DeleteSession->m_iWSASendCount;
		InterlockedExchange(&DeleteSession->m_iWSASendCount, 0);  // º¸³½ Ä«¿îÆ® 0À¸·Î ¸¸µë.		

		// º¸³½ Ä«¿îÆ®°¡ ÀÖÀ¸¸é ¸ðµÎ µ¿ÀûÇØÁ¦ ÇÑ´Ù.
		if (TempCount > 0)
		{
<<<<<<< HEAD
<<<<<<< HEAD
			for (int i = 0; i < dfSENDPOST_MAX_WSABUF; ++i)
=======
			for (int i = 0; i < TempCount; ++i)
>>>>>>> parent of 8d940d8... ì„±ì¤€ì”¨ë²„ì „ ver 1
			{
				delete DeleteSession->m_cpbufSendPayload[i];
				PacketAllocCountSub();
			}
		}			

		// »÷µå Å¥¿¡ µ¥ÀÌÅÍ°¡ ÀÖÀ¸¸é µ¿ÀûÇØÁ¦ ÇÑ´Ù.
		CProtocolBuff* Payload[1024];
		int UseSize = DeleteSession->m_SendQueue.GetUseSize();
		int DequeueSize = DeleteSession->m_SendQueue.Dequeue((char*)Payload, UseSize);

		DequeueSize = DequeueSize / 8;

		if (DequeueSize != UseSize / 8)
		{
			// ¿¡·¯ ·Î±× Âï±â
			// ·Î±× Âï±â(·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"InDisconnect(). DequeueSize, UseSize Not SAME! : Rear(%d), Front(%d)",
				DequeueSize, UseSize / 8);
		}

		// ²¨³½ Dequeue¸¸Å­ µ¹¸é¼­ »èÁ¦ÇÑ´Ù.
		for (int i = 0; i < DequeueSize; ++i)
		{
			delete Payload[i];
			if (PacketAllocCountSub() != 0)
			{
				// ¿¡·¯ ·Î±× Âï±â
				// ·Î±× Âï±â(·Î±× ·¹º§ : ¿¡·¯)
				//cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"InDisconnect(). PacketAllocCount Not 0 : %d",
				//	g_llPacketAllocCount);
			}
		}



		//int UseSize = DeleteSession->m_SendQueue.GetUseSize();

		//while (UseSize > 0)
		//{
		//	int Temp = UseSize;

		//	if (Temp > 8000)
		//		Temp = 8000;

		//	// UseSize »çÀÌÁî ¸¸Å­ µðÅ¥
		//	CProtocolBuff* Payload[1000];
		//	int DequeueSize = DeleteSession->m_SendQueue.Dequeue((char*)Payload, Temp);

		//	UseSize -= DequeueSize;

		//	DequeueSize = DequeueSize / 8;

		//	// ²¨³½ Dequeue¸¸Å­ µ¹¸é¼­ »èÁ¦ÇÑ´Ù.
		//	for (int i = 0; i < DequeueSize; ++i)
		//	{
		//		delete Payload[i];
		//		PacketAllocCountSub();
		//	}
		//}

		int* RearValue = (int*)DeleteSession->m_SendQueue.GetWriteBufferPtr();
		int* FrontValue = (int*)DeleteSession->m_SendQueue.GetReadBufferPtr();

		if (*RearValue != *FrontValue)
		{
			// ¿¡·¯ ·Î±× Âï±â
			// ·Î±× Âï±â(·Î±× ·¹º§ : ¿¡·¯)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"InDisconnect(). Rear, Front Not SAME! : Rear(%d), Front(%d)",
				RearValue, FrontValue);
		}

		// ¸®¼Â
		DeleteSession->Reset_Func();

		// Å¬·ÎÁî ¼ÒÄÏ
		closesocket(DeleteSession->m_Client_sock);		
=======
			for (int i = 0; i < TempCount; ++i)
				delete DeleteSession->m_cpbufSendPayload[i];			
		}	

		DeleteSession->m_lUseFlag = FALSE;
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04

		DeleteSession->m_lUseFlag = FALSE;

		// ¹Ì»ç¿ë ÀÎµ¦½º ½ºÅÃ¿¡ ¹Ý³³
		Lock_Exclusive_Stack();
		if (m_stEmptyIndexStack->Push(DeleteSession->m_lIndex) == false)
		{
			// ¿¡·¯ ·Î±× Âï±â
			// ·Î±× Âï±â(·Î±× ·¹º§ : µð¹ö±×)
			cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_DEBUG, L"InDisconnect(). Stack_Index_Full! : NowJoinUser(%d), MaxUser(%d)",
				m_ullJoinUserCount, m_iMaxJoinUser);
		}
		Unlock_Exclusive_Stack();

		// Å¬·ÎÁî ¼ÒÄÏ
		closesocket(DeleteSession->m_Client_sock);

		// À¯Àú ¼ö °¨¼Ò
		InterlockedDecrement(&m_ullJoinUserCount);	

		
		return;
	}

	// Start¿¡¼­ ¿¡·¯°¡ ³¯ ½Ã È£ÃâÇÏ´Â ÇÔ¼ö.
	// 1. ÀÔÃâ·Â ¿Ï·áÆ÷Æ® ÇÚµé ¹ÝÈ¯
	// 2. ¿öÄ¿½º·¹µå Á¾·á, ÇÚµé¹ÝÈ¯, ÇÚµé ¹è¿­ µ¿ÀûÇØÁ¦
	// 3. ¿¢¼ÁÆ® ½º·¹µå Á¾·á, ÇÚµé ¹ÝÈ¯
	// 4. ¸®½¼¼ÒÄÏ ´Ý±â
	// 5. À©¼Ó ÇØÁ¦
	void CLanServer::ExitFunc(int w_ThreadCount)
	{
		// 1. ÀÔÃâ·Â ¿Ï·áÆ÷Æ® ÇØÁ¦ ÀýÂ÷.
		// nullÀÌ ¾Æ´Ò¶§¸¸! (Áï, ÀÔÃâ·Â ¿Ï·áÆ÷Æ®¸¦ ¸¸µé¾úÀ» °æ¿ì!)
		// ÇØ´ç ÇÔ¼ö´Â ¾îµð¼­³ª È£ÃâµÇ±â¶§¹®¿¡, Áö±Ý ÀÌ°É ÇØ¾ßÇÏ´ÂÁö Ç×»ó È®ÀÎ.
		if (m_hIOCPHandle != NULL)
			CloseHandle(m_hIOCPHandle);

		// 2. ¿öÄ¿½º·¹µå Á¾·á ÀýÂ÷. Count°¡ 0ÀÌ ¾Æ´Ò¶§¸¸!
		if (w_ThreadCount > 0)
		{
			// Á¾·á ¸Þ½ÃÁö¸¦ º¸³½´Ù
			for (int h = 0; h < w_ThreadCount; ++h)
				PostQueuedCompletionStatus(m_hIOCPHandle, 0, 0, 0);

			// ¸ðµç ¿öÄ¿½º·¹µå Á¾·á ´ë±â.
			WaitForMultipleObjects(w_ThreadCount, m_hWorkerHandle, TRUE, INFINITE);

			// ¸ðµç ¿öÄ¿½º·¹µå°¡ Á¾·áµÆÀ¸¸é ÇÚµé ¹ÝÈ¯
			for (int h = 0; h < w_ThreadCount; ++h)
				CloseHandle(m_hWorkerHandle[h]);

			// ¿öÄ¿½º·¹µå ÇÚµé ¹è¿­ µ¿ÀûÇØÁ¦. 
			// Count°¡ 0º¸´Ù Å©´Ù¸é ¹«Á¶°Ç µ¿ÀûÇÒ´çÀ» ÇÑ ÀûÀÌ ÀÖÀ½
			delete[] m_hWorkerHandle;
		}

		// 3. ¿¢¼ÁÆ® ½º·¹µå Á¾·á, ÇÚµé ¹ÝÈ¯
		if (m_iA_ThreadCount > 0)
			CloseHandle(m_hAcceptHandle);

		// 4. ¸®½¼¼ÒÄÏ ´Ý±â
		if (m_soListen_sock != NULL)
			closesocket(m_soListen_sock);

		// 5. À©¼Ó ÇØÁ¦
		// À©¼Ó ÃÊ±âÈ­ ÇÏÁö ¾ÊÀº »óÅÂ¿¡¼­ WSAClenup() È£ÃâÇØµµ ¾Æ¹« ¹®Á¦ ¾øÀ½
		WSACleanup();
	}

	// ¿öÄ¿½º·¹µå
	UINT WINAPI	CLanServer::WorkerThread(LPVOID lParam)
	{
		DWORD cbTransferred;
		stSession* stNowSession;
		OVERLAPPED* overlapped;

		CLanServer* g_This = (CLanServer*)lParam;

		while (1)
		{
			// º¯¼öµé ÃÊ±âÈ­
			cbTransferred = 0;
			stNowSession = nullptr;
			overlapped = nullptr;

			// GQCS ¿Ï·á ½Ã ÇÔ¼ö È£Ãâ
			g_This->OnWorkerThreadEnd();

			// ºñµ¿±â ÀÔÃâ·Â ¿Ï·á ´ë±â
			// GQCS ´ë±â
			GetQueuedCompletionStatus(g_This->m_hIOCPHandle, &cbTransferred, (PULONG_PTR)&stNowSession, &overlapped, INFINITE);

			// GQCS ±ú¾î³¯ ½Ã ÇÔ¼öÈ£Ãâ
			g_This->OnWorkerThreadBegin();

			// --------------
			// ¿Ï·á Ã¼Å©
			// --------------
			// overlapped°¡ nullptrÀÌ¶ó¸é, IOCP ¿¡·¯ÀÓ.
			if (overlapped == NULL)
			{
				// ¼Â ´Ù 0ÀÌ¸é ½º·¹µå Á¾·á.
				if (cbTransferred == 0 && stNowSession == NULL)
				{
					// printf("¿öÄ¿ ½º·¹µå Á¤»óÁ¾·á\n");

					break;
				}
				// ±×°Ô ¾Æ´Ï¸é IOCP ¿¡·¯ ¹ß»ýÇÑ °Í

				// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
				g_This->m_iOSErrorCode = GetLastError();
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__IOCP_ERROR;

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"IOCP_Error --> GQCS return Error : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);


				// ¿¡·¯ ¹ß»ý ÇÔ¼ö È£Ãâ
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__IOCP_ERROR, L"IOCP_Error");

				break;
			}

			// -----------------
			// Recv ·ÎÁ÷
			// -----------------
			// WSArecv()°¡ ¿Ï·áµÈ °æ¿ì, ¹ÞÀº µ¥ÀÌÅÍ°¡ 0ÀÌ ¾Æ´Ï¸é ·ÎÁ÷ Ã³¸®
			if (&stNowSession->m_overRecvOverlapped == overlapped && cbTransferred > 0)
			{
				// rear ÀÌµ¿
				stNowSession->m_RecvQueue.MoveWritePos(cbTransferred);

				// 1. ¸µ¹öÆÛ ÈÄ, ÆÐÅ¶ Ã³¸®
				g_This->RecvProc(stNowSession);

				// 2. ¸®½Ãºê ´Ù½Ã °É±â. false°¡ ¸®ÅÏµÇ¸é Á¾·áµÈ À¯ÀúÀÌ´Ï ´Ù½Ã À§·Î ¿Ã¶ó°£´Ù.
				if (g_This->RecvPost(stNowSession) == false)
					continue;
			}

			// -----------------
			// Send ·ÎÁ÷
			// -----------------
			// WSAsend()°¡ ¿Ï·áµÈ °æ¿ì, ¹ÞÀº µ¥ÀÌÅÍ°¡ 0ÀÌ ¾Æ´Ï¸é ·ÎÁ÷Ã³¸®
			else if (&stNowSession->m_overSendOverlapped == overlapped && cbTransferred > 0)
			{
				// 1. »÷µå ¿Ï·áµÆ´Ù°í ÄÁÅÙÃ÷¿¡ ¾Ë·ÁÁÜ
				g_This->OnSend(stNowSession->m_ullSessionID, cbTransferred);			

				// 2. º¸³Â´ø Á÷·ÄÈ­¹öÆÛ »èÁ¦
				int TempCount = stNowSession->m_iWSASendCount;
				InterlockedExchange(&stNowSession->m_iWSASendCount, 0);  // º¸³½ Ä«¿îÆ® 0À¸·Î ¸¸µë.

<<<<<<< HEAD
<<<<<<< HEAD
				for (int i = 0; i < dfSENDPOST_MAX_WSABUF; ++i)
=======
				for (int i = 0; i < TempCount; ++i)
>>>>>>> parent of 8d940d8... ì„±ì¤€ì”¨ë²„ì „ ver 1
				{
					delete stNowSession->m_cpbufSendPayload[i];
					PacketAllocCountSub();
				}
=======
				for (int i = 0; i < TempCount; ++i)
					delete stNowSession->m_cpbufSendPayload[i];		
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04

				// 4. »÷µå °¡´É »óÅÂ·Î º¯°æ
				InterlockedExchange(&stNowSession->m_lSendFlag, FALSE);

				// 5. ´Ù½Ã »÷µå ½Ãµµ. false°¡ ¸®ÅÏµÇ¸é Á¾·áµÈ À¯ÀúÀÌ´Ï ´Ù½Ã À§·Î ¿Ã¶ó°£´Ù.
				if (g_This->SendPost(stNowSession) == false)
					continue;				
			}

			// -----------------
			// I/OÄ«¿îÆ® °¨¼Ò ¹× »èÁ¦ Ã³¸®
			// -----------------
			// I/OÄ«¿îÆ® °¨¼Ò ÈÄ, 0ÀÌ¶ó¸éÁ¢¼Ó Á¾·á
			if (InterlockedDecrement(&stNowSession->m_lIOCount) == 0)
				g_This->InDisconnect(stNowSession);

		}
		return 0;
	}

	// Accept ½º·¹µå
	UINT WINAPI	CLanServer::AcceptThread(LPVOID lParam)
	{
		// --------------------------
		// Accept ÆÄÆ®
		// --------------------------
		SOCKET client_sock;
		SOCKADDR_IN	clientaddr;
		int addrlen;

		CLanServer* g_This = (CLanServer*)lParam;

		// À¯Àú°¡ Á¢¼ÓÇÒ ¶§ ¸¶´Ù 1¾¿ Áõ°¡ÇÏ´Â °íÀ¯ÇÑ Å°.
		ULONGLONG ullUniqueSessionID = 0;

		while (1)
		{
			ZeroMemory(&clientaddr, sizeof(clientaddr));
			addrlen = sizeof(clientaddr);
			client_sock = accept(g_This->m_soListen_sock, (SOCKADDR*)&clientaddr, &addrlen);
			if (client_sock == INVALID_SOCKET)
			{
				int Error = WSAGetLastError();
				// 10004¹ø(WSAEINTR) ¿¡·¯¸é Á¤»óÁ¾·á. ÇÔ¼öÈ£ÃâÀÌ Áß´ÜµÇ¾ú´Ù´Â °Í.
				// 10038¹ø(WSAEINTR) Àº ¼ÒÄÏÀÌ ¾Æ´Ñ Ç×¸ñ¿¡ ÀÛ¾÷À» ½ÃµµÇß´Ù´Â °Í. ÀÌ¹Ì ¸®½¼¼ÒÄÏÀº closesocketµÈ°ÍÀÌ´Ï ¿¡·¯ ¾Æ´Ô.
				if (Error == WSAEINTR || Error == WSAENOTSOCK)
				{
					// Accept ½º·¹µå Á¤»ó Á¾·á
					break;
				}

				// ±×°Ô ¾Æ´Ï¶ó¸é OnError ÇÔ¼ö È£Ãâ
				// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
				g_This->m_iOSErrorCode = Error;
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT;

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"accpet(). Abnormal_exit : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// ¿¡·¯ ¹ß»ý ÇÔ¼ö È£Ãâ
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT, L"accpet(). Abnormal_exit");

				break;
			}	

			// ------------------
			// ÃÖ´ë Á¢¼ÓÀÚ ¼ö ÀÌ»ó Á¢¼Ó ºÒ°¡
			// ------------------
			if (g_This->m_iMaxJoinUser <= g_This->m_ullJoinUserCount)
			{
				closesocket(client_sock);
<<<<<<< HEAD

				// ¿¡·¯ ·Î±× Âï±â (·Î±× ·¹º§ : µð¹ö±×)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_DEBUG, L"accpet(). User_Max... : NowJoinUser(%d), MaxUser(%d)",
					g_This->m_ullJoinUserCount, g_This->m_iMaxJoinUser);

=======
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04
				continue;
			}
				


			// ------------------
			// IP¿Í Æ÷Æ® ¾Ë¾Æ¿À±â.
			// ------------------
			TCHAR tcTempIP[30];
			InetNtop(AF_INET, &clientaddr.sin_addr, tcTempIP, 30);
			USHORT port = ntohs(clientaddr.sin_port);




			// ------------------
			// Á¢¼Ó Á÷ÈÄ, IPµîÀ» ÆÇ´ÜÇØ¼­ ¹«¾ð°¡ Ãß°¡ ÀÛ¾÷ÀÌ ÇÊ¿äÇÒ °æ¿ì°¡ ÀÖÀ» ¼öµµ ÀÖÀ¸´Ï È£Ãâ
			// ------------------		
			// false¸é Á¢¼Ó°ÅºÎ, 
			// true¸é Á¢¼Ó °è¼Ó ÁøÇà. true¿¡¼­ ÇÒ°Ô ÀÖÀ¸¸é OnConnectionRequestÇÔ¼ö ÀÎÀÚ·Î ¹º°¡¸¦ ´øÁø´Ù.
			if (g_This->OnConnectionRequest(tcTempIP, port) == false)
				continue;




			// ------------------
			// ¼¼¼Ç ±¸Á¶Ã¼ »ý¼º ÈÄ ¼ÂÆÃ
			// ------------------
			// 1) ¹Ì»ç¿ë ÀÎµ¦½º ¾Ë¾Æ¿À±â
			g_This->Lock_Exclusive_Stack(); // ¹Ì»ç¿ë ÀÎµ¦½º ½ºÅÃ ¶ô -----------
			ULONGLONG iIndex = g_This->m_stEmptyIndexStack->Pop();
			if (iIndex == 10000000)
			{
				g_This->Unlock_Exclusive_Stack(); // ¹Ì»ç¿ë ÀÎµ¦½º ½ºÅÃ ¶ô ÇØÁ¦ -----------

				// ¿¡·¯ ·Î±× Âï±â
				// ·Î±× Âï±â(·Î±× ·¹º§ : µð¹ö±×)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_DEBUG, L"accpet(). Stack_Index_Empty... : NowJoinUser(%d), MaxUser(%d)",
					g_This->m_ullJoinUserCount, g_This->m_iMaxJoinUser);

				break;
			}
			g_This->Unlock_Exclusive_Stack(); // ¹Ì»ç¿ë ÀÎµ¦½º ½ºÅÃ ¶ô ÇØÁ¦ -----------

			// 2) ÇØ´ç ¼¼¼Ç ¹è¿­, »ç¿ëÁßÀ¸·Î º¯°æ
			g_This->m_stSessionArray[iIndex].m_lUseFlag = TRUE;

			// 3) ÃÊ±âÈ­ÇÏ±â
			// -- ¼ÒÄÏ
			g_This->m_stSessionArray[iIndex].m_Client_sock = client_sock;

			// -- ¼¼¼ÇÅ°(¹Í½º Å°)¿Í ÀÎµ¦½º
			ULONGLONG MixKey = InterlockedIncrement(&ullUniqueSessionID) | (iIndex << 48);
			g_This->m_stSessionArray[iIndex].m_ullSessionID = MixKey;
			g_This->m_stSessionArray[iIndex].m_lIndex = iIndex;

			// -- IP¿Í Port
			StringCchCopy(g_This->m_stSessionArray[iIndex].m_IP, _MyCountof(g_This->m_stSessionArray[iIndex].m_IP), tcTempIP);
			g_This->m_stSessionArray[iIndex].m_prot = port;
			
			// -- SendFlag, I/OÄ«¿îÆ®, Å¥ ÃÊ±âÈ­ ÃÊ±âÈ­
			g_This->m_stSessionArray[iIndex].Reset_Func();			
							



			// ------------------
			// IOCP ¿¬°á
			// ------------------
			// ¼ÒÄÏ°ú IOCP ¿¬°á
			if (CreateIoCompletionPort((HANDLE)client_sock, g_This->m_hIOCPHandle, (ULONG_PTR)&g_This->m_stSessionArray[iIndex], 0) == NULL)
			{
				// À©µµ¿ì ¿¡·¯, ³» ¿¡·¯ º¸°ü
				g_This->m_iOSErrorCode = WSAGetLastError();
				g_This->m_iMyErrorCode = euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT;

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"accpet(). Abonormal_exit : NetError(%d), OSError(%d)",
					(int)g_This->m_iMyErrorCode, g_This->m_iOSErrorCode);

				// ¿¡·¯ ¹ß»ý ÇÔ¼ö È£Ãâ
				g_This->OnError((int)euError::NETWORK_LIB_ERROR__A_THREAD_ABNORMAL_EXIT, L"accpet(). Abonormal_exit");

				break;
			}
					
			// Á¢¼ÓÀÚ ¼ö Áõ°¡. disconnect¿¡¼­µµ »ç¿ëµÇ´Â º¯¼öÀÌ±â ¶§¹®¿¡ ÀÎÅÍ¶ô »ç¿ë
			InterlockedIncrement(&g_This->m_ullJoinUserCount);



			// ------------------
			// ¸ðµç Á¢¼ÓÀýÂ÷°¡ ¿Ï·áµÇ¾úÀ¸´Ï Á¢¼Ó ÈÄ Ã³¸® ÇÔ¼ö È£Ãâ.
			// ------------------
			g_This->m_stSessionArray[iIndex].m_lIOCount++; // I/OÄ«¿îÆ® ++. µé¾î°¡±âÀü¿¡ 1¿¡¼­ ½ÃÀÛ. ¾ÆÁ÷ recv,send ±× ¾î¶²°Íµµ ¾È°É¾ú±â ¶§¹®¿¡ ±×³É ++ÇØµµ ¾ÈÀü!
			g_This->OnClientJoin(MixKey);

			

			// ------------------
			// ºñµ¿±â ÀÔÃâ·Â ½ÃÀÛ
			// ------------------
			// ¹ÝÈ¯°ªÀÌ false¶ó¸é, ÀÌ ¾È¿¡¼­ Á¾·áµÈ À¯ÀúÀÓ. ±Ùµ¥ ¾È¹ÞÀ½
			g_This->RecvPost(&g_This->m_stSessionArray[iIndex]);

			// I/OÄ«¿îÆ® --. 0ÀÌ¶ó¸é »èÁ¦Ã³¸®
			if (InterlockedDecrement(&g_This->m_stSessionArray[iIndex].m_lIOCount) == 0)
				g_This->InDisconnect(&g_This->m_stSessionArray[iIndex]);
			
		}

		return 0;
	}

	// ¹Ì»ç¿ë ¼¼¼Ç °ü¸® ½ºÅÃ¿¡ Exclusive ¶ô °É±â, ¶ô Ç®±â
	void CLanServer::LockStack_Exclusive_Func()
	{
		AcquireSRWLockExclusive(&m_srwSession_stack_srwl);
	}

	void CLanServer::UnlockStack_Exclusive_Func()
	{
		ReleaseSRWLockExclusive(&m_srwSession_stack_srwl);
	}

	// ¹Ì»ç¿ë ¼¼¼Ç °ü¸® ½ºÅÃ¿¡ Shared ¶ô °É±â, ¶ô Ç®±â
	void CLanServer::LockStack_Shared_Func()
	{
		AcquireSRWLockShared(&m_srwSession_stack_srwl);
	}

	void CLanServer::UnlockStack_Shared_Func()
	{
		ReleaseSRWLockShared(&m_srwSession_stack_srwl);
	}



	// Á¶ÇÕµÈ Å°¸¦ ÀÔ·Â¹ÞÀ¸¸é, Index ¸®ÅÏÇÏ´Â ÇÔ¼ö
	WORD CLanServer::GetSessionIndex(ULONGLONG MixKey)
	{
		return MixKey >> 48;		
	}

	// Á¶ÇÕµÈ Å°¸¦ ÀÔ·Â¹ÞÀ¸¸é, ÁøÂ¥ ¼¼¼ÇÅ°¸¦ ¸®ÅÏÇÏ´Â ÇÔ¼ö.
	ULONGLONG CLanServer::GetRealSessionKey(ULONGLONG MixKey)
	{
		return MixKey & 0x0000ffffffffffff;
	}

	// CProtocolBuff¿¡ Çì´õ ³Ö´Â ÇÔ¼ö
	void CLanServer::SetProtocolBuff_HeaderSet(CProtocolBuff* Packet)
	{
		WORD wHeader = Packet->GetUseSize() - dfNETWORK_PACKET_HEADER_SIZE;
		memcpy_s(&Packet->GetBufferPtr()[0], dfNETWORK_PACKET_HEADER_SIZE, &wHeader, dfNETWORK_PACKET_HEADER_SIZE);
	}



	// °¢Á¾ º¯¼öµéÀ» ¸®¼Â½ÃÅ²´Ù.
	void CLanServer::Reset()
	{	
		m_iW_ThreadCount = 0;
		m_iA_ThreadCount = 0;
		m_hWorkerHandle = nullptr;
		m_hIOCPHandle = 0;
		m_soListen_sock = 0;
		m_iMaxJoinUser = 0;
		m_ullJoinUserCount = 0;				
	}






	// ------------
	// Lib ³»ºÎ¿¡¼­¸¸ »ç¿ëÇÏ´Â ¸®½Ãºê °ü·Ã ÇÔ¼öµé
	// ------------
	// RecvProc ÇÔ¼ö. Å¥ÀÇ ³»¿ë Ã¼Å© ÈÄ PacketProcÀ¸·Î ³Ñ±ä´Ù.
	void CLanServer::RecvProc(stSession* NowSession)
	{
		// -----------------
		// Recv Å¥ °ü·Ã Ã³¸®
		// -----------------

		while (1)
		{
			// 1. RecvBuff¿¡ ÃÖ¼ÒÇÑÀÇ »çÀÌÁî°¡ ÀÖ´ÂÁö Ã¼Å©. (Á¶°Ç = Çì´õ »çÀÌÁî¿Í °°°Å³ª ÃÊ°ú. Áï, ÀÏ´Ü Çì´õ¸¸Å­ÀÇ Å©±â°¡ ÀÖ´ÂÁö Ã¼Å©)	
			WORD Header_PaylaodSize;

			// RecvBuffÀÇ »ç¿ë ÁßÀÎ ¹öÆÛ Å©±â°¡ Çì´õ Å©±âº¸´Ù ÀÛ´Ù¸é, ¿Ï·á ÆÐÅ¶ÀÌ ¾ø´Ù´Â °ÍÀÌ´Ï while¹® Á¾·á.
			int UseSize = NowSession->m_RecvQueue.GetUseSize();
			if (UseSize < dfNETWORK_PACKET_HEADER_SIZE)
			{
				break;
			}

			// 2. Çì´õ¸¦ PeekÀ¸·Î È®ÀÎÇÑ´Ù.  Peek ¾È¿¡¼­´Â, ¾î¶»°Ô ÇØ¼­µçÁö len¸¸Å­ ÀÐ´Â´Ù. 
			// ¹öÆÛ°¡ ºñ¾îÀÖÀ¸¸é Á¢¼Ó ²÷À½.
			if (NowSession->m_RecvQueue.Peek((char*)&Header_PaylaodSize, dfNETWORK_PACKET_HEADER_SIZE) == -1)
			{
				// ÀÏ´Ü ²÷¾î¾ßÇÏ´Ï ¼Ë´Ù¿î È£Ãâ
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// ³» ¿¡·¯ º¸°ü. À©µµ¿ì ¿¡·¯´Â ¾øÀ½.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF;

				// ¿¡·¯ ½ºÆ®¸µ ¸¸µé°í
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_Empry.UserID : %d, [%s:%d]"), 
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"%s, NetError(%d)",
					tcErrorString, (int)m_iMyErrorCode);

				// ¿¡·¯ ÇÔ¼ö È£Ãâ
				OnError((int)euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF, tcErrorString);
			
				// Á¢¼ÓÀÌ ²÷±æ À¯ÀúÀÌ´Ï ´õ´Â ¾Æ¹«°Íµµ ¾ÈÇÏ°í ¸®ÅÏ
				return;
			}

			// 3. ¿Ï¼ºµÈ ÆÐÅ¶ÀÌ ÀÖ´ÂÁö È®ÀÎ. (¿Ï¼º ÆÐÅ¶ »çÀÌÁî = Çì´õ »çÀÌÁî + ÆäÀÌ·Îµå Size)
			// °è»ê °á°ú, ¿Ï¼º ÆÐÅ¶ »çÀÌÁî°¡ ¾ÈµÇ¸é while¹® Á¾·á.
			if (UseSize < (dfNETWORK_PACKET_HEADER_SIZE + Header_PaylaodSize))
			{
				break;
			}

			// 4. RecvBuff¿¡¼­ PeekÇß´ø Çì´õ¸¦ Áö¿ì°í (ÀÌ¹Ì PeekÇßÀ¸´Ï, ±×³É RemoveÇÑ´Ù)
			NowSession->m_RecvQueue.RemoveData(dfNETWORK_PACKET_HEADER_SIZE);

			// 5. RecvBuff¿¡¼­ ÆäÀÌ·Îµå Size ¸¸Å­ ÆäÀÌ·Îµå Á÷·ÄÈ­ ¹öÆÛ·Î »Ì´Â´Ù. (µðÅ¥ÀÌ´Ù. Peek ¾Æ´Ô)
			CProtocolBuff PayloadBuff;
			PayloadBuff.MoveReadPos(dfNETWORK_PACKET_HEADER_SIZE);

			int DequeueSize = NowSession->m_RecvQueue.Dequeue(&PayloadBuff.GetBufferPtr()[dfNETWORK_PACKET_HEADER_SIZE], Header_PaylaodSize);
			// ¹öÆÛ°¡ ºñ¾îÀÖÀ¸¸é Á¢¼Ó ²÷À½
			if (DequeueSize == -1)
			{
				// ÀÏ´Ü ²÷¾î¾ßÇÏ´Ï ¼Ë´Ù¿î È£Ãâ
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// ³» ¿¡·¯ º¸°ü. À©µµ¿ì ¿¡·¯´Â ¾øÀ½.
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF;

				// ¿¡·¯ ½ºÆ®¸µ ¸¸µé°í
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("RecvRingBuff_Empry.UserID : %d, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// ¿¡·¯ ÇÔ¼ö È£Ãâ
				OnError((int)euError::NETWORK_LIB_ERROR__EMPTY_RECV_BUFF, tcErrorString);

				// Á¢¼ÓÀÌ ²÷±æ À¯ÀúÀÌ´Ï ´õ´Â ¾Æ¹«°Íµµ ¾ÈÇÏ°í ¸®ÅÏ
				return;
			}
			PayloadBuff.MoveWritePos(DequeueSize);

			// 9. Çì´õ¿¡ µé¾îÀÖ´Â Å¸ÀÔ¿¡ µû¶ó ºÐ±âÃ³¸®.
			OnRecv(NowSession->m_ullSessionID, &PayloadBuff);

		}

		return;
	}

	// RecvPost ÇÔ¼ö. ºñµ¿±â ÀÔÃâ·Â ½ÃÀÛ
	//
	// return true : ¼º°øÀûÀ¸·Î WSARecv() ¿Ï·á or ¾îÂ¶µç Á¾·áµÈ À¯Àú´Â ¾Æ´Ô
	// return false : I/OÄ«¿îÆ®°¡ 0ÀÌµÇ¾î¼­ Á¾·áµÈ À¯ÀúÀÓ
	bool CLanServer::RecvPost(stSession* NowSession)
	{
		// ------------------
		// ºñµ¿±â ÀÔÃâ·Â ½ÃÀÛ
		// ------------------
		// 1. WSABUF ¼ÂÆÃ.
		WSABUF wsabuf[2];
		int wsabufCount = 0;

		int FreeSize = NowSession->m_RecvQueue.GetFreeSize();
		int Size = NowSession->m_RecvQueue.GetNotBrokenPutSize();

		if (Size < FreeSize)
		{
			wsabuf[0].buf = NowSession->m_RecvQueue.GetRearBufferPtr();
			wsabuf[0].len = Size;

			wsabuf[1].buf = NowSession->m_RecvQueue.GetBufferPtr();
			wsabuf[1].len = FreeSize - Size;
			wsabufCount = 2;

		}
		else
		{
			wsabuf[0].buf = NowSession->m_RecvQueue.GetRearBufferPtr();
			wsabuf[0].len = Size;

			wsabufCount = 1;
		}

		// 2. Overlapped ±¸Á¶Ã¼ ÃÊ±âÈ­
		ZeroMemory(&NowSession->m_overRecvOverlapped, sizeof(NowSession->m_overRecvOverlapped));

		// 3. WSARecv()
		DWORD recvBytes = 0, flags = 0;
		InterlockedIncrement(&NowSession->m_lIOCount);

		// 4. ¿¡·¯ Ã³¸®
		if (WSARecv(NowSession->m_Client_sock, wsabuf, wsabufCount, &recvBytes, &flags, &NowSession->m_overRecvOverlapped, NULL) == SOCKET_ERROR)
		{
			int Error = WSAGetLastError();

			// ºñµ¿±â ÀÔÃâ·ÂÀÌ ½ÃÀÛµÈ°Ô ¾Æ´Ï¶ó¸é
			if (Error != WSA_IO_PENDING)
			{
				// I/OÄ«¿îÆ® 1°¨¼Ò.I/O Ä«¿îÆ®°¡ 0ÀÌ¶ó¸é Á¢¼Ó Á¾·á.
				if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
				{
					InDisconnect(NowSession);
					return false;
				}

				// ¿¡·¯°¡ ¹öÆÛ ºÎÁ·ÀÌ¶ó¸é, I/OÄ«¿îÆ® Â÷°¨ÀÌ ³¡ÀÌ ¾Æ´Ï¶ó ²÷¾î¾ßÇÑ´Ù.
				if (Error == WSAENOBUFS)
				{			
					// ÀÏ´Ü ²÷¾î¾ßÇÏ´Ï ¼Ë´Ù¿î È£Ãâ
					shutdown(NowSession->m_Client_sock, SD_BOTH);

					// ³» ¿¡·¯, À©µµ¿ì¿¡·¯ º¸°ü
					m_iOSErrorCode = Error;
					m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

					// ¿¡·¯ ½ºÆ®¸µ ¸¸µé°í
					TCHAR tcErrorString[300];
					StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %d, [%s:%d]"),
						NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

					// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
					cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSARecv --> %s : NetError(%d), OSError(%d)",
						tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

					// ¿¡·¯ ÇÔ¼ö È£Ãâ
					OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);
				}
			}
		}

		return true;
	}






	// ------------
	// Lib ³»ºÎ¿¡¼­¸¸ »ç¿ëÇÏ´Â »÷µå °ü·Ã ÇÔ¼öµé
	// ------------
	// »÷µå ¹öÆÛÀÇ µ¥ÀÌÅÍ WSASend() ÇÏ±â
	//
	// return true : ¼º°øÀûÀ¸·Î WSASend() ¿Ï·á or ¾îÂ¶µç Á¾·áµÈ À¯Àú´Â ¾Æ´Ô
	// return false : I/OÄ«¿îÆ®°¡ 0ÀÌµÇ¾î¼­ Á¾·áµÈ À¯ÀúÀÓ
	bool CLanServer::SendPost(stSession* NowSession)
	{
		while (1)
		{
			// ------------------
			// send °¡´É »óÅÂÀÎÁö Ã¼Å©
			// ------------------
			// 1. SendFlag(1¹øÀÎÀÚ)°¡ 0(3¹øÀÎÀÚ)°ú °°´Ù¸é, SendFlag(1¹øÀÎÀÚ)¸¦ 1(2¹øÀÎÀÚ)À¸·Î º¯°æ
			// ¿©±â¼­ TRUE°¡ ¸®ÅÏµÇ´Â °ÍÀº, ÀÌ¹Ì NowSession->m_SendFlag°¡ 1(»÷µå Áß)ÀÌ¾ú´Ù´Â °Í.
			if (InterlockedCompareExchange(&NowSession->m_lSendFlag, TRUE, FALSE) == TRUE)
				return true;

			// 2. SendBuff¿¡ µ¥ÀÌÅÍ°¡ ÀÖ´ÂÁö È®ÀÎ
			// ¿©±â¼­ ±¸ÇÑ UseSize´Â ÀÌÁ¦ ½º³À¼¦ ÀÌ´Ù. 
			int UseSize = NowSession->m_SendQueue.GetUseSize();
			if (UseSize == 0)
			{
				// WSASend ¾È°É¾ú±â ¶§¹®¿¡, »÷µå °¡´É »óÅÂ·Î ´Ù½Ã µ¹¸².
				NowSession->m_lSendFlag = 0;

				// 3. ÁøÂ¥·Î »çÀÌÁî°¡ ¾ø´ÂÁö ´Ù½ÃÇÑ¹ø Ã¼Å©. ¶ô Ç®°í ¿Ô´Âµ¥, ÄÁÅØ½ºÆ® ½ºÀ§Äª ÀÏ¾î³ª¼­ ´Ù¸¥ ½º·¹µå°¡ °Çµå·ÈÀ» °¡´É¼º
				// »çÀÌÁî ÀÖÀ¸¸é À§·Î ¿Ã¶ó°¡¼­ ÇÑ¹ø ´õ ½Ãµµ
				if (NowSession->m_SendQueue.GetUseSize() > 0)
					continue;

				break;
			}

			// ------------------
			// Send ÁØºñÇÏ±â
			// ------------------
			// 1. WSABUF ¼ÂÆÃ.
			WSABUF wsabuf[dfSENDPOST_MAX_WSABUF];		

			if (UseSize > dfSENDPOST_MAX_WSABUF * 8)
				UseSize = dfSENDPOST_MAX_WSABUF * 8;

<<<<<<< HEAD
<<<<<<< HEAD
			// 2. ÇÑ ¹ø¿¡ 100°³ÀÇ Æ÷ÀÎÅÍ(ÃÑ 800¹ÙÀÌÆ®)¸¦ ²¨³»µµ·Ï ½Ãµµ
=======
			// 2. ÇÑ ¹ø¿¡ 100°³ÀÇ Æ÷ÀÎÅÍ(ÃÑ 800¹ÙÀÌÆ®)¸¦ ²¨³»µµ·Ï ½Ãµµ			
>>>>>>> parent of 8d940d8... ì„±ì¤€ì”¨ë²„ì „ ver 1
			int wsabufByte = (NowSession->m_SendQueue.Dequeue((char*)NowSession->m_cpbufSendPayload, UseSize));
			if (wsabufByte == -1)
=======
			// 3. ÇÑ ¹ø¿¡ 100°³ÀÇ Æ÷ÀÎÅÍ(ÃÑ 800¹ÙÀÌÆ®)¸¦ ²¨³»µµ·Ï ½Ãµµ			
			int wsabufCount = (NowSession->m_SendQueue.Dequeue((char*)NowSession->m_cpbufSendPayload, UseSize));
			if (wsabufCount == -1)
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04
			{
				// Å¥°¡ ÅÖ ºñ¾îÀÖÀ½. À§¿¡¼­ ÀÖ´Ù°í ¿Ô´Âµ¥ ¿©±â¼­ ¾ø´Â°ÍÀº ÁøÂ¥ ¸»µµ¾ÈµÇ´Â ¿¡·¯!
				// ³» ¿¡·¯º¸°ü
				m_iMyErrorCode = euError::NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY;

				// ¿¡·¯ ½ºÆ®¸µ ¸¸µé°í
				TCHAR tcErrorString[300];
				StringCchPrintf(tcErrorString, 300, _T("QUEUE_PEEK_EMPTY. UserID : %d, [%s:%d]"),
					NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

				// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
				cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d)",
					tcErrorString, (int)m_iMyErrorCode);

				// ²÷´Â´Ù.
				shutdown(NowSession->m_Client_sock, SD_BOTH);

				// »÷µå Flag 0À¸·Î º¯°æ (»÷µå °¡´É»óÅÂ)
				InterlockedExchange(&NowSession->m_lSendFlag, FALSE);

				// ¿¡·¯ ÇÔ¼ö È£Ãâ
				OnError((int)euError::NETWORK_LIB_ERROR__QUEUE_DEQUEUE_EMPTY, tcErrorString);

				return false;
			}
<<<<<<< HEAD
<<<<<<< HEAD
=======
			NowSession->m_iWSASendCount = wsabufCount / 8;
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04

			if (0 != wsabufByte % 8)
			{
				int a = 0;
			}

			int iMax = wsabufByte / 8;
			InterlockedExchange(&NowSession->m_iWSASendCount, iMax);
=======
			InterlockedExchange(&NowSession->m_iWSASendCount, wsabufByte / 8);
>>>>>>> parent of 8d940d8... ì„±ì¤€ì”¨ë²„ì „ ver 1

			// 3. ½ÇÁ¦·Î ²¨³½ Æ÷ÀÎÆ® ¼ö(¹ÙÀÌÆ® ¾Æ´Ô! ÁÖÀÇ)¸¸Å­ µ¹¸é¼­ WSABUF±¸Á¶Ã¼¿¡ ÇÒ´ç
			for (int i = 0; i < NowSession->m_iWSASendCount; i++)
			{				
				wsabuf[i].buf = NowSession->m_cpbufSendPayload[i]->GetBufferPtr();
				wsabuf[i].len = NowSession->m_cpbufSendPayload[i]->GetUseSize();
			}

			// 4. Overlapped ±¸Á¶Ã¼ ÃÊ±âÈ­
			ZeroMemory(&NowSession->m_overSendOverlapped, sizeof(NowSession->m_overSendOverlapped));

			// 5. WSASend()
			DWORD SendBytes = 0, flags = 0;
			InterlockedIncrement(&NowSession->m_lIOCount);

			// 6. ¿¡·¯ Ã³¸®
			if (WSASend(NowSession->m_Client_sock, wsabuf, NowSession->m_iWSASendCount, &SendBytes, flags, &NowSession->m_overSendOverlapped, NULL) == SOCKET_ERROR)
			{
				int Error = WSAGetLastError();

				// ºñµ¿±â ÀÔÃâ·ÂÀÌ ½ÃÀÛµÈ°Ô ¾Æ´Ï¶ó¸é
				if (Error != WSA_IO_PENDING)
				{	
					// »÷µå Flag 0À¸·Î º¯°æ (»÷µå °¡´É»óÅÂ)
					InterlockedExchange(&NowSession->m_lSendFlag, FALSE);

					// IOcount ÇÏ³ª °¨¼Ò. I/O Ä«¿îÆ®°¡ 0ÀÌ¶ó¸é Á¢¼Ó Á¾·á.
					if (InterlockedDecrement(&NowSession->m_lIOCount) == 0)
					{
						InDisconnect(NowSession);
						return false;
					}					

					// ¿¡·¯°¡ ¹öÆÛ ºÎÁ·ÀÌ¶ó¸é
					if (Error == WSAENOBUFS)
					{
						// ³» ¿¡·¯, À©µµ¿ì¿¡·¯ º¸°ü
						m_iOSErrorCode = Error;
						m_iMyErrorCode = euError::NETWORK_LIB_ERROR__WSAENOBUFS;

						// ¿¡·¯ ½ºÆ®¸µ ¸¸µé°í
						TCHAR tcErrorString[300];
						StringCchPrintf(tcErrorString, 300, _T("WSANOBUFS. UserID : %d, [%s:%d]"),
							NowSession->m_ullSessionID, NowSession->m_IP, NowSession->m_prot);

						// ·Î±× Âï±â (·Î±× ·¹º§ : ¿¡·¯)
						cNetLibLog->LogSave(L"LanServer", CSystemLog::en_LogLevel::LEVEL_ERROR, L"WSASend --> %s : NetError(%d), OSError(%d)",
							tcErrorString, (int)m_iMyErrorCode, m_iOSErrorCode);

						// ²÷´Â´Ù.
						shutdown(NowSession->m_Client_sock, SD_BOTH);

						// ¿¡·¯ ÇÔ¼ö È£Ãâ
						OnError((int)euError::NETWORK_LIB_ERROR__WSAENOBUFS, tcErrorString);
					}
				}
			}
			break;
		}

		return true;
	}

}


