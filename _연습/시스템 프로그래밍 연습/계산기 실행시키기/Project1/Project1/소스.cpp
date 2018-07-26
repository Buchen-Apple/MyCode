#include <stdio.h>
#include <tchar.h>
#include <windows.h>

enum { DIV = 1, MUL, ADD, MIN, ELSE, EXIT };

DWORD ShowMenu();

void Divide(double, double);
void Multiple(double, double);
void Add(double, double);
void Min(double, double);

int _tmain(int argc, TCHAR* argv[])
{
	STARTUPINFO si = { 0, };
	PROCESS_INFORMATION pi;
	si.cb = sizeof(si);

	TCHAR command[] = TEXT("calc.exe");
	SetCurrentDirectory(TEXT("C:\\WINDOW\\system32"));

	DWORD sel;
	double num1, num2;
	while (1)
	{
		sel = ShowMenu();
		if (sel == EXIT)
			return 0;

		if (sel != ELSE)
		{
			_fputts(TEXT("Input Num1 Num2 : "), stdout);
			_tscanf(TEXT("%lf %lf"), &num1, &num2);
		}

		switch (sel)
		{
		case DIV:
			Divide(num1, num2);
			break;

		case MUL:
			Multiple(num1, num2);
			break;

		case ADD:
			Add(num1, num2);
			break;

		case MIN:
			Min(num1, num2);
			break;

		case ELSE:
			ZeroMemory(&pi, sizeof(pi));
			CreateProcess(
				NULL, command, NULL, NULL,
				TRUE, 0, NULL, NULL, &si, &pi);
			break;

		}
	}

	return 0;
}

DWORD ShowMenu()
{
	DWORD sel;

	_fputts(TEXT("-----Menu----- \n"), stdout);
	_fputts(TEXT("num 1 : Divide \n"), stdout);
	_fputts(TEXT("num 2 : Multiple \n"), stdout);
	_fputts(TEXT("num 3 : Add \n"), stdout);
	_fputts(TEXT("num 4 : Minus \n"), stdout);
	_fputts(TEXT("num 5 : Any Other Operation \n"), stdout);
	_fputts(TEXT("num 6 : Exit \n"), stdout);
	_fputts(TEXT("SELECTION >> "), stdout);
	_tscanf(TEXT("%d"), &sel);

	return sel;
}

void Divide(double a, double b)
{
	_tprintf(TEXT("%f / %f = %f \n\n"), a, b, a / b);
}

void Multiple(double a, double b)
{
	_tprintf(TEXT("%f * %f = %f \n\n"), a, b, a * b);
}

void Add(double a, double b)
{
	_tprintf(TEXT("%f + %f = %f \n\n"), a, b, a + b);
}

void Min(double a, double b)
{
	_tprintf(TEXT("%f - %f = %f \n\n"), a, b, a - b);
}