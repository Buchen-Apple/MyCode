#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#define BUF_SIZE 1024
int CommToClient(HANDLE);

int _tmain(int argc, TCHAR* argv[])
{
	LPCTSTR pipeName = _T("\\\\.\\pipe\\simple_pipe");
	HANDLE hPipe;
	while (1)
	{
		hPipe = CreateNamedPipe (
			pipeName,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			BUF_SIZE,
			BUF_SIZE,
			20000,
			NULL);

		if (hPipe == INVALID_HANDLE_VALUE)	// 이름있는 파이프 생성 실패 시
		{
			_tprintf(_T("CreatePipe Fail..\n"));
			return -1;
		}

		BOOL isSuccess = 0;
		isSuccess = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

		if (isSuccess)
			CommToClient(hPipe);
		else
			CloseHandle(hPipe);
	}

	return 1;
}

int CommToClient(HANDLE hPipe)
{
	TCHAR fileName[MAX_PATH];
	TCHAR dataBuffer[BUF_SIZE];

	BOOL isSuccess;
	DWORD fileNameSize;
	isSuccess = ReadFile (
		hPipe,
		fileName,
		MAX_PATH * sizeof(TCHAR),
		&fileNameSize,
		NULL);

	if (!isSuccess || fileNameSize == 0)
	{
		_tprintf(_T("Pipe read message error!\n"));
		return -1;
	}

	FILE* filePtr = _tfopen(fileName, _T("rt, ccs=UNICODE"));
	if (filePtr == NULL)
	{
		_tprintf(_T("File Open fault!\n"));
		return -1;
	}

	DWORD bytesWritten = 0;
	DWORD bytesRead = 0;

	while (!feof(filePtr))
	{
		bytesRead = fread(dataBuffer, 1, BUF_SIZE, filePtr);

		WriteFile(
			hPipe,
			dataBuffer,
			bytesRead,
			&bytesWritten,
			NULL);

		if (bytesRead != bytesWritten)
		{
			_tprintf(_T("Pipe write message error!\n"));
			break;
		}

	}

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	return 1;
}
