#ifndef __NETWORK_LIB_H__
#define __NETWORK_LIB_H__
#include <windows.h>
#include <map>
#include "ProtocolBuff\ProtocolBuff.h"

using namespace std;


<<<<<<< HEAD
#include <unordered_map>

=======
>>>>>>> parent of 4721bdb... 218-08-03 7ì°¨ê³¼ì œ ver 04
namespace Library_Jingyu
{
	void PacketAllocCountAdd();
	LONG PacketAllocCountSub();
	LONG PacketAllocCount_Get();


	// --------------
	// CLanServer Å¬·¡½º´Â, ³»ºÎ ¼­¹ö °£ Åë½Å¿¡ »ç¿ëµÈ´Ù.
	// ³»ºÎ ¼­¹ö°£ Åë½ÅÀº, Á¢¼Ó ¹Þ´ÂÂÊÀ» ¼­¹ö / Á¢¼ÓÇÏ´Â ÂÊÀ» Å¬¶ó·Î ³ª´²¼­ »ý°¢ÇÑ´Ù (°³³äÀûÀ¸·Î)
	// ±× Áß ¼­¹ö ºÎºÐÀÌ´Ù.
	// --------------
	class CLanServer
	{
		//!!
	public:
		SRWLOCK m_srwLeak;
		typedef std::unordered_map<UINT_PTR, CProtocolBuff *> UMAP_LEAK;
		UMAP_LEAK m_umapLeak;
		void LeakIn(CProtocolBuff *_p)
		{
			AcquireSRWLockExclusive(&m_srwLeak);

			UINT_PTR g = (UINT_PTR)_p;
			m_umapLeak.insert(UMAP_LEAK::value_type(g, _p));

			ReleaseSRWLockExclusive(&m_srwLeak);
		}
		void LeakDel(CProtocolBuff *_p)
		{
			AcquireSRWLockExclusive(&m_srwLeak);

			size_t g = m_umapLeak.erase((UINT_PTR)_p);
			if (0 == g)
			{
				int a = 0;
			}

			ReleaseSRWLockExclusive(&m_srwLeak);
		}
		//!!
	private:
		// ----------------------
		// private ±¸Á¶Ã¼ or enum Àü¹æ¼±¾ð
		// ----------------------
		// ¿¡·¯ enum°ªµé
		enum class euError : int;

		// Session±¸Á¶Ã¼ Àü¹æ¼±¾ð
		struct stSession;

		// ¹Ì»ç¿ë ÀÎµ¦½º °ü¸® ±¸Á¶Ã¼(½ºÅÃ)
		struct stEmptyStack;


		// ----------------------
		// private º¯¼öµé
		// ----------------------
		// ¿¢¼ÁÆ® ½º·¹µå ÇÚµé
		HANDLE*  m_hAcceptHandle;

		// ¿öÄ¿½º·¹µå ÇÚµé ¹è¿­, 
		HANDLE* m_hWorkerHandle;

		// ¿öÄ¿½º·¹µå ¼ö, ¿¢¼ÁÆ® ½º·¹µå ¼ö, ¶óÀÌÇÁ Ã¼Å© ½º·¹µå ¼ö
		int m_iW_ThreadCount;
		int m_iA_ThreadCount;

		// IOCP ÇÚµé
		HANDLE m_hIOCPHandle;

		// ¸®½¼¼ÒÄÏ
		SOCKET m_soListen_sock;


		// ----- ¼¼¼Ç °ü¸®¿ë -------
		// ¼¼¼Ç °ü¸® ¹è¿­
		stSession* m_stSessionArray;

		// ¹Ì»ç¿ë ÀÎµ¦½º °ü¸® ½ºÅÃ
		stEmptyStack* m_stEmptyIndexStack;

		// ¹Ì¼¼¿ë ¼¼¼Ç °ü¸® ½ºÅÃÀÇ SRWLock
		SRWLOCK m_srwSession_stack_srwl;

		// --------------------------

		// À©µµ¿ì ¿¡·¯ º¸°ü º¯¼ö, ³»°¡ ÁöÁ¤ÇÑ ¿¡·¯ º¸°ü º¯¼ö
		int m_iOSErrorCode;
		euError m_iMyErrorCode;

		// ÇöÀç Á¢¼ÓÁßÀÎ À¯Àú ¼ö
		ULONGLONG m_ullJoinUserCount;

		// ÃÖ´ë Á¢¼Ó °¡´É À¯Àú ¼ö
		ULONGLONG m_iMaxJoinUser;

		// ¼­¹ö °¡µ¿ ¿©ºÎ. true¸é ÀÛµ¿Áß / false¸é ÀÛµ¿Áß ¾Æ´Ô
		bool m_bServerLife;




		// ¹Ì»ç¿ë ¼¼¼Ç °ü¸® ½ºÅÃ¿ë Exclusive ¶ô °É±â, Ç®±â
#define	Lock_Exclusive_Stack()		LockStack_Exclusive_Func()
#define Unlock_Exclusive_Stack()	UnlockStack_Exclusive_Func()


		// ¼¼¼Ç ¹è¿­¿ë Shared  ¶ô °É±â, Ç®±â
#define	Lock_Shared_Stack()		LockStack_Shared_Func()
#define Unlock_Shared_Stack()	UnlockStack_Shared_Func()


	private:
		// ----------------------
		// private ÇÔ¼öµé
		// ----------------------
		
		// ¹Ì»ç¿ë ¼¼¼Ç °ü¸® ½ºÅÃ¿¡ Exclusive ¶ô °É±â, ¶ô Ç®±â
		void LockStack_Exclusive_Func();
		void UnlockStack_Exclusive_Func();

		// ¹Ì»ç¿ë ¼¼¼Ç °ü¸® ½ºÅÃ¿¡ Shared ¶ô °É±â, ¶ô Ç®±â
		void LockStack_Shared_Func();
		void UnlockStack_Shared_Func();

		// Á¶ÇÕµÈ Å°¸¦ ÀÔ·Â¹ÞÀ¸¸é, Index ¸®ÅÏÇÏ´Â ÇÔ¼ö
		WORD GetSessionIndex(ULONGLONG MixKey);

		// Á¶ÇÕµÈ Å°¸¦ ÀÔ·Â¹ÞÀ¸¸é, ÁøÂ¥ ¼¼¼ÇÅ°¸¦ ¸®ÅÏÇÏ´Â ÇÔ¼ö.
		ULONGLONG GetRealSessionKey(ULONGLONG MixKey);

		// CProtocolBuff¿¡ Çì´õ ³Ö´Â ÇÔ¼ö
		void SetProtocolBuff_HeaderSet(CProtocolBuff* Packet);

		// ¿öÄ¿ ½º·¹µå
		static UINT	WINAPI	WorkerThread(LPVOID lParam);

		// Accept ½º·¹µå
		static UINT	WINAPI	AcceptThread(LPVOID lParam);

		// Áß°£¿¡ ¹«¾ð°¡ ¿¡·¯³µÀ»¶§ È£ÃâÇÏ´Â ÇÔ¼ö
		// 1. À©¼Ó ÇØÁ¦
		// 2. ÀÔÃâ·Â ¿Ï·áÆ÷Æ® ÇÚµé ¹ÝÈ¯
		// 3. ¿öÄ¿½º·¹µå Á¾·á, ÇÚµé¹ÝÈ¯, ÇÚµé ¹è¿­ µ¿ÀûÇØÁ¦
		// 4. ¸®½¼¼ÒÄÏ ´Ý±â
		void ExitFunc(int w_ThreadCount);

		// RecvProc ÇÔ¼ö. Å¥ÀÇ ³»¿ë Ã¼Å© ÈÄ PacketProcÀ¸·Î ³Ñ±ä´Ù.
		void RecvProc(stSession* NowSession);

		// RecvPostÇÔ¼ö
		//
		// return true : ¼º°øÀûÀ¸·Î WSARecv() ¿Ï·á or ¾îÂ¶µç Á¾·áµÈ À¯Àú´Â ¾Æ´Ô
		// return false : I/OÄ«¿îÆ®°¡ 0ÀÌµÇ¾î¼­ Á¾·áµÈ À¯ÀúÀÓ
		bool RecvPost(stSession* NowSession);

		// SendPostÇÔ¼ö
		//
		// return true : ¼º°øÀûÀ¸·Î WSASend() ¿Ï·á or ¾îÂ¶µç Á¾·áµÈ À¯Àú´Â ¾Æ´Ô
		// return false : I/OÄ«¿îÆ®°¡ 0ÀÌµÇ¾î¼­ Á¾·áµÈ À¯ÀúÀÓ
		bool SendPost(stSession* NowSession);

		// ³»ºÎ¿¡¼­ ½ÇÁ¦·Î À¯Àú¸¦ ²÷´Â ÇÔ¼ö.
		void InDisconnect(stSession* NowSession);

		// °¢Á¾ º¯¼öµéÀ» ¸®¼Â½ÃÅ²´Ù.
		void Reset();


	public:
		// -----------------------
		// »ý¼ºÀÚ¿Í ¼Ò¸êÀÚ
		// -----------------------
		CLanServer();
		~CLanServer();

	public:
		// -----------------------
		// ¿ÜºÎ¿¡¼­ È£Ãâ °¡´ÉÇÑ ÇÔ¼ö
		// -----------------------

		// ----------------------------- ±â´É ÇÔ¼öµé ---------------------------
		// ¼­¹ö ½ÃÀÛ
		// [¿ÀÇÂ IP(¹ÙÀÎµù ÇÒ IP), Æ÷Æ®, ¿öÄ¿½º·¹µå ¼ö, ¿¢¼ÁÆ® ½º·¹µå ¼ö, TCP_NODELAY »ç¿ë ¿©ºÎ(true¸é »ç¿ë), ÃÖ´ë Á¢¼ÓÀÚ ¼ö] ÀÔ·Â¹ÞÀ½.
		//
		// return false : ¿¡·¯ ¹ß»ý ½Ã. ¿¡·¯ÄÚµå ¼ÂÆÃ ÈÄ false ¸®ÅÏ
		// return true : ¼º°ø
		bool Start(const TCHAR* bindIP, USHORT port, int WorkerThreadCount, int AcceptThreadCount, bool Nodelay, int MaxConnect);

		// ¼­¹ö ½ºÅ¾.
		void Stop();

		// ÁöÁ¤ÇÑ À¯Àú¸¦ ²÷À» ¶§ È£ÃâÇÏ´Â ÇÔ¼ö. ¿ÜºÎ ¿¡¼­ »ç¿ë.
		// ¶óÀÌºê·¯¸®ÇÑÅ× ²÷¾îÁà!¶ó°í ¿äÃ»ÇÏ´Â °Í »Ó
		//
		// return true : ÇØ´ç À¯Àú¿¡°Ô ¼Ë´Ù¿î Àß ³¯¸².
		// return false : Á¢¼ÓÁßÀÌÁö ¾ÊÀº À¯Àú¸¦ Á¢¼ÓÇØÁ¦ÇÏ·Á°í ÇÔ.
		bool Disconnect(ULONGLONG ClinetID);

		// ¿ÜºÎ¿¡¼­, ¾î¶² µ¥ÀÌÅÍ¸¦ º¸³»°í ½ÍÀ»¶§ È£ÃâÇÏ´Â ÇÔ¼ö.
		// SendPacketÀº ±×³É ¾Æ¹«¶§³ª ÇÏ¸é µÈ´Ù.
		// ÇØ´ç À¯ÀúÀÇ SendQ¿¡ ³Ö¾îµ×´Ù°¡ ¶§°¡ µÇ¸é º¸³½´Ù.
		//
		// return true : SendQ¿¡ ¼º°øÀûÀ¸·Î µ¥ÀÌÅÍ ³ÖÀ½.
		// return true : SendQ¿¡ µ¥ÀÌÅÍ ³Ö±â ½ÇÆÐ.
		bool SendPacket(ULONGLONG ClinetID, CProtocolBuff* payloadBuff);


		// ----------------------------- °ÔÅÍ ÇÔ¼öµé ---------------------------
		// À©µµ¿ì ¿¡·¯ ¾ò±â
		int WinGetLastError();

		// ³» ¿¡·¯ ¾ò±â 
		int NetLibGetLastError();

		// ÇöÀç Á¢¼ÓÀÚ ¼ö ¾ò±â
		ULONGLONG GetClientCount();

		// ¼­¹ö °¡µ¿»óÅÂ ¾ò±â
		// return true : °¡µ¿Áß
		// return false : °¡µ¿Áß ¾Æ´Ô
		bool GetServerState();

	public:
		// -----------------------
		// ¼ø¼ö °¡»óÇÔ¼ö
		// -----------------------

		// Accept Á÷ÈÄ, È£ÃâµÈ´Ù.
		//
		// return false : Å¬¶óÀÌ¾ðÆ® Á¢¼Ó °ÅºÎ
		// return true : Á¢¼Ó Çã¿ë
		virtual bool OnConnectionRequest(TCHAR* IP, USHORT port) = 0;

		// Accept ÈÄ, Á¢¼ÓÃ³¸®±îÁö ´Ù ¿Ï·áµÈ ÈÄ È£ÃâµÈ´Ù
		virtual void OnClientJoin(ULONGLONG ClinetID) = 0;

		// InDisconnect ÈÄ È£ÃâµÈ´Ù. (½ÇÁ¦ ³»ºÎ¿¡¼­ µð½ºÄ¿³ØÆ® ÈÄ È£Ãâ)
		virtual void OnClientLeave(ULONGLONG ClinetID) = 0;

		// ÆÐÅ¶ ¼ö½Å ¿Ï·á ÈÄ È£ÃâµÇ´Â ÇÔ¼ö.
		// ¿Ï¼ºµÈ ÆÐÅ¶ 1°³°¡ »ý±â¸é È£ÃâµÊ. PacketProcÀ¸·Î »ý°¢ÇÏ¸é µÈ´Ù.
		virtual void OnRecv(ULONGLONG ClinetID, CProtocolBuff* Payload) = 0;

		// ÆÐÅ¶ ¼Û½Å ¿Ï·á ÈÄ È£ÃâµÇ´Â ÇÔ¼ö
		// »÷µå ¿Ï·áÅëÁö¸¦ ¹Þ¾ÒÀ» ¶§, ¸µ¹öÆÛ ÀÌµ¿ µî ´Ù ÇÏ°í È£ÃâµÈ´Ù.
		virtual void OnSend(ULONGLONG ClinetID, DWORD SendSize) = 0;

		// ¿öÄ¿ ½º·¹µå GQCS ¹Ù·Î ÇÏ´Ü¿¡¼­ È£Ãâ
		virtual void OnWorkerThreadBegin() = 0;

		// ¿öÄ¿½º·¹µå 1·çÇÁ Á¾·á ÈÄ È£ÃâµÇ´Â ÇÔ¼ö.
		virtual void OnWorkerThreadEnd() = 0;

		// ¿¡·¯ ¹ß»ý ½Ã È£ÃâµÇ´Â ÇÔ¼ö.
		virtual void OnError(int error, const TCHAR* errorStr) = 0;

	};

}






#endif // !__NETWORK_LIB_H__
