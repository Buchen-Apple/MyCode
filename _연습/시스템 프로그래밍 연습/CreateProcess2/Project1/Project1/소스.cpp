#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#define DIR_LEN	MAX_PATH+1

int _tmain(int argc, TCHAR* argv[])
{
	STARTUPINFO si = { 0, };
	PROCESS_INFORMATION pi;

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USEPOSITION | STARTF_USESIZE;
	si.dwX = 100;
	si.dwY = 200;
	si.dwXSize = 300;
	si.dwYSize = 200;
	si.lpTitle = TEXT("I am a boy!");

	TCHAR command[] = TEXT("Project1.exe 10 20");
	TCHAR cDir[DIR_LEN];
	BOOL state;

	GetCurrentDirectory(DIR_LEN, cDir);
	_fputts(cDir, stdout);
	_fputts(TEXT("\n"), stdout);

	//SetCurrentDirectory(TEXT("C:\\WinSystem"));
	SetCurrentDirectory(TEXT("C:\\Users\Administrator\\Desktop\\VS\\시스템 프로그래밍 연습\\CreateProcess\\Project1\\x64\Release"));

	GetCurrentDirectory(DIR_LEN, cDir);
	_fputts(cDir, stdout);
	_fputts(TEXT("\n"), stdout);

	state = CreateProcess(
		NULL,
		command,
		NULL, NULL, TRUE,
		CREATE_NEW_CONSOLE,
		NULL, NULL, &si, &pi);

	if (state != 0)
		_fputts(TEXT("Create OK! \n"), stdout);
	else
		_fputts(TEXT("Create Error! \n"), stdout);

	return 0;
}