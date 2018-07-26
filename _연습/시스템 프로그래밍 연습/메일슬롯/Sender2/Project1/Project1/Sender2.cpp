#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <locale.h>

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


	// 핸들을 얻는 코드.
	FILE* fp;
	_tfopen_s(&fp, _T("InheritableHandle.txt"), _T("rt"));
	_ftscanf(fp, _T("%d"), &hMailSlot);
	fclose(fp);
	_tprintf(_T("Inheritable Handle : %d \n"), hMailSlot);


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