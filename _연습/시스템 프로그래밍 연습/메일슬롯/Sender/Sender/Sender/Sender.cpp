#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <locale.h>

#define SLOT_NAME	_T("\\\\.\\mailslot\\mailbox")

int _tmain(int argc, TCHAR* argv[])
{
	_tsetlocale(LC_ALL, _T("korean"));

	HANDLE hMailSlot;
	TCHAR MsgBox[50];
	DWORD byteWrite;
	TCHAR str[50];

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;	// 핸들의 상속 여부(Y,N). TRUE면 Y

	// MailSlot 스트림 연결 //
	hMailSlot = CreateFile(
		SLOT_NAME,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		&sa,	// 정의한 sa를 넣었다. 이제 생성되는 데이터 스트림 리소스의 핸들은 상속 Y상태가 되었다.
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (hMailSlot == INVALID_HANDLE_VALUE)
	{
		_tcscpy_s(str, _countof(str), _T("메일슬롯 스트림 연결 실패!"));
		wprintf(_T("%s \n"), str);
		return 1;
	}

	_tprintf(_T("Inheritable Handle : %d \n"), hMailSlot);

	// 핸들 정보를 파일을 이용해 자식 프로세스에게 전달한다.
	FILE* fp;
	_tfopen_s(&fp, _T("InheritableHandle.txt"), _T("wt"));
	_ftprintf(fp, _T("%d"), hMailSlot);
	fclose(fp);

	STARTUPINFO si = { 0, };
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;

	TCHAR command[] = _T("Sender2.exe");

	CreateProcess(NULL, command,
		NULL, NULL,
		TRUE,
		CREATE_NEW_CONSOLE,
		NULL, NULL,
		&si, &pi);

	// Receiver의 메일슬롯으로 데이터 보내기
	while (1)
	{
		_tcscpy_s(str, _countof(str), _T("나의 CMD>"));
		wprintf(_T("%s "), str);
		_fgetts(MsgBox, sizeof(MsgBox) / sizeof(TCHAR), stdin);

		if (!WriteFile(hMailSlot, MsgBox, _tcslen(MsgBox) * sizeof(TCHAR), &byteWrite, NULL))
		{
			_tcscpy_s(str, _countof(str), _T("데이터 보내기 실패!"));
			wprintf(_T("%s \n"), str);
			CloseHandle(hMailSlot);
			return 1;
		}

		if (!_tcscmp(MsgBox, _T("exit")))
		{
			_tcscpy_s(str, _countof(str), _T("잘가!"));
			wprintf(_T("%s \n"), str);
			break;
		}
	}

	CloseHandle(hMailSlot);
	return 0;
}