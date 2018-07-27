#include <stdio.h>
#include <windows.h>

#define HEIGHT_LEN 11
#define WIDTH_LEN 30

void Print(char[][WIDTH_LEN]);
void UpFunc(char[][WIDTH_LEN], int, int);
void DownFunc(char[][WIDTH_LEN], int, int);
void LeftFunc(char[][WIDTH_LEN], int, int);
void RightFunc(char[][WIDTH_LEN], int, int);
void Change(char cBuffer[][WIDTH_LEN], int, int);

int main()
{

	char cOriginal[HEIGHT_LEN][WIDTH_LEN] =
	{
		{ '0', '0', '0', '0', '0', '0', '0', '0','0','0','0','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },
		{ '0', '0', '0', '0', '0', '0', '0', '0','0','0',' ','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },
		{ '0', '0', '0', '0', '0', '0', '0', '0','0','0','0','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },
		{ '0', '0', '0', '0', '0', '0', '0', '0','0','0','0','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },
		{ '0', '0', '0', ' ', '0', '0', ' ', '0','0','0',' ','0','0','0','0',' ' ,'0' ,' ' ,'0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,' ' ,'0' ,'0' },
		{ '0', ' ', '0', ' ', '0', '0', ' ', '0','0','0',' ','0','0','0','0',' ' ,'0' ,'0' ,'0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,' ' ,'0' ,'0' },
		{ '0', '0', '0', ' ', '0', '0', ' ', '0','0','0',' ','0','0','0','0',' ' ,'0' ,' ' ,'0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,' ' ,'0' ,'0' },
		{ '0', '0', '0', '0', '0', '0', '0', '0','0','0','0','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },
		{ '0', '0', '0', '0', '0', '0', '0', '0','0','0','0','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },
		{ '0', '0', '0', '0', '0', '0', '0', ' ','0','0',' ','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },
		{ '0', '0', '0', '0', '0', '0', '0', '0','0','0','0','0','0','0','0','0' ,'0' ,' ' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' },

	};

	char cBuffer[HEIGHT_LEN][WIDTH_LEN];

	int i;
	int j;

	while (1)
	{
		Print(cOriginal);
		Sleep(1000);

		for (i = 0; i < HEIGHT_LEN; ++i)
		{
			for (j = 0; j < WIDTH_LEN; ++j)
			{
				cBuffer[i][j] = cOriginal[i][j];
			}
		}

		Change(cBuffer, 5, 6);
		Sleep(1000);

	}

	return 0;
}

void Print(char cBuffer[][WIDTH_LEN])
{
	int i;
	int j;
	system("cls");

	for (i = 0; i < HEIGHT_LEN; ++i)
	{
		for (j = 0; j < WIDTH_LEN; ++j)
		{
			printf("%c", cBuffer[i][j]);
		}
		fputs("\n", stdout);
	}
	Sleep(2);
}

void Change(char cBuffer[][WIDTH_LEN], int i, int j)
{
	UpFunc(cBuffer, i, j);
	LeftFunc(cBuffer, i, j);
	RightFunc(cBuffer, i, j);
	DownFunc(cBuffer, i, j);
}

void UpFunc(char cBuffer[][WIDTH_LEN], int i, int j)
{
	// 위 체크
	if (i == 0 || cBuffer[i - 1][j] == '*' || cBuffer[i - 1][j] == ' ')
		return;

	if (cBuffer[i - 1][j] == '0')
	{
		cBuffer[i - 1][j] = '*';
		Print(cBuffer);
	}

	if (i != 0)
		i--;
	
	LeftFunc(cBuffer, i, j);
	RightFunc(cBuffer, i, j);
	UpFunc(cBuffer, i, j);
}

void DownFunc(char cBuffer[][WIDTH_LEN], int i, int j)
{
	// 아래 체크
	if (i == HEIGHT_LEN - 1 || cBuffer[i + 1][j] == '*' || cBuffer[i + 1][j] == ' ')
		return;

	if (cBuffer[i + 1][j] == '0')
	{
		cBuffer[i + 1][j] = '*';
		Print(cBuffer);
	}

	if (i < HEIGHT_LEN - 1)
		i++;
		
	LeftFunc(cBuffer, i, j);
	RightFunc(cBuffer, i, j);
	DownFunc(cBuffer, i, j);
}

void LeftFunc(char cBuffer[][WIDTH_LEN], int i, int j)
{
	// 왼쪽 체크
	if (j == 0 || cBuffer[i][j - 1] == '*' || cBuffer[i][j - 1] == ' ')
		return;

	if (cBuffer[i][j - 1] == '0')
	{
		cBuffer[i][j - 1] = '*';
		Print(cBuffer);
	}

	if (j != 0)
		j--;

	
	UpFunc(cBuffer, i, j);
	DownFunc(cBuffer, i, j);
	LeftFunc(cBuffer, i, j);
}

void RightFunc(char cBuffer[][WIDTH_LEN], int i, int j)
{
	// 오른쪽 체크
	if (j == WIDTH_LEN - 1 || cBuffer[i][j + 1] == '*' || cBuffer[i][j + 1] == ' ')
		return;

	if (cBuffer[i][j + 1] == '0')
	{
		cBuffer[i][j + 1] = '*';
		Print(cBuffer);
	}

	if (j < (WIDTH_LEN - 1))
		j++;
		
	UpFunc(cBuffer, i, j);
	DownFunc(cBuffer, i, j);
	RightFunc(cBuffer, i, j);
}