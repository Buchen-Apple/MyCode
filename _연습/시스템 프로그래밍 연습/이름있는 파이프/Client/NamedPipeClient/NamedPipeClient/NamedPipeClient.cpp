#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#define BUF_SIZE 1024

int _tmain(int argc, TCHAR* argv[])
{
	HANDLE hPipe;
	TCHAR readDataBuf[BUF_SIZE + 1];
	LPCTSTR pipeName = _T("\\\\.\\pipe\\simple_pipe");

	while (1)
	{
		// 클라이언트 입장에서 서버와 파이프 연결
		hPipe = CreateFile(
			pipeName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		// 정상적으로 연결되었으면 while문 종료
		if (hPipe != INVALID_HANDLE_VALUE)	
			break;

		// 연결되지 않았고, 마지막에러가 ERROR_PIPE_BUSY(연결 요청 대기상태)가 아니라면
		// 전혀 이 상한 에러가 발생한 것. 프로그램 종료
		else if (GetLastError() != ERROR_PIPE_BUSY)															
		{
			_tprintf(_T("Could not open pipe \n"));
			return 0;
		}

		// 여기까지 왔으면 마지막 에러가 ERROR_PIPE_BUSY라는 것.
		// WaitNamedPipe함수는 지정한 파이프 이름(첫 인자)의 연결 대기 상태를 일정 시간(두번째 인자) 동안 기다린다.
		// 일정 시간 안에 연결 가능상태가 되면 true 반환 / 일정 시간이 끝났는데도 연결 요청 대기상태라면 false 반환
		// true가 반환되면 다시 While문 처음으로 돌아가서 파이프 연결 시도.
		else if (!WaitNamedPipe(pipeName, NMPWAIT_NOWAIT))
		{
			_tprintf(_T("Could not open pipe \n"));
			return 0;
		}
	}

	DWORD pipeMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;	// 메시지 기반으로 모드 변경
	BOOL isSuccess = SetNamedPipeHandleState(hPipe, &pipeMode, NULL, NULL);

	if (!isSuccess)
	{
		_tprintf(_T("SetNamedPipeHandleState failed \n"));
		return 0;
	}

	LPCTSTR fileName = _T("Test.txt");
	DWORD bytesWritten = 0;

	isSuccess = WriteFile(
		hPipe,
		fileName,
		(_tcslen(fileName) + 1) * sizeof(TCHAR),
		&bytesWritten,
		NULL);

	if (!isSuccess)
	{
		_tprintf(_T("WriteFile failed \n"));
		return 0;
	}

	DWORD bytesRead = 0;
	while (1)
	{
		isSuccess = ReadFile(
			hPipe,
			readDataBuf,
			BUF_SIZE * sizeof(TCHAR),
			&bytesRead,
			NULL);

		if (!isSuccess || GetLastError() == ERROR_MORE_DATA)
			break;

		readDataBuf[bytesRead / 2] = 0;
		_tprintf(_T("%s \n"), readDataBuf);

	}

	CloseHandle(hPipe);
	return 0;
}