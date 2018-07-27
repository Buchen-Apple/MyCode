#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>

typedef struct
{
	char m_iName[30];
	int m_iHP;
	int m_iMP;	
	int m_iAtack;
	int m_iArmor;

}Monster;

// 파일을 읽어올 버퍼
char* cBuffer;

// 몬스터 구조체 정보 보기 함수
void ShowMonster(Monster);

//--------------------------
// 파서 관련 함수
// -------------------------

// 파서_파일 로드
bool Parser_LoadFile(const char* FileName);

// GetNextWord에서 단어 스킵 여부를 체크한다.
// true면 스킵 / false면 스킵이 아니다.
bool SkipNoneCommand(char cTempWord);

// 버퍼에서, 다음 단어 찾기
bool GetNextWord(char cWord[], int* len);

// 파서_원하는 값 찾아오기 (int)
bool Parser_GetValue_Int(const char* Name, int* value);

// 파서_원하는 값 찾아오기 (문자열)
bool Parser_GetValue_String(const char* Name, char* value);


int main(void)
{
	Monster mon;
	
	bool LoadFileCheck;

	//파일 로드
	LoadFileCheck = Parser_LoadFile("Test.txt");
	if (LoadFileCheck == false)
	{
		printf("파일 불러오기 실패\n");
		exit(-1);
	}

	// 원하는 값 찾기
	Parser_GetValue_String("m_iName", mon.m_iName);
	Parser_GetValue_Int("m_iHP", &mon.m_iHP);
	Parser_GetValue_Int("m_iMP", &mon.m_iMP);
	Parser_GetValue_Int("m_iAtack", &mon.m_iAtack);
	Parser_GetValue_Int("m_iArmor", &mon.m_iArmor);

	// 확인하기
	ShowMonster(mon);
	return 0;
}

// 몬스터 구조체 정보 보기 함수
void ShowMonster(Monster mon)
{
	printf("이름 : %s\n", mon.m_iName);
	printf("체력 : %d\n", mon.m_iHP);
	printf("마력 : %d\n", mon.m_iMP);
	printf("공격력 : %d\n", mon.m_iAtack);
	printf("방어력 : %d\n", mon.m_iArmor);
}

//--------------------------
// 파서 관련 함수
// -------------------------

// 파서_파일 로드
bool Parser_LoadFile(const char* FileName)
{
	FILE* fp;
	int iSize;
	size_t iFileCheck;

	iFileCheck = fopen_s(&fp, FileName, "rt");
	if (iFileCheck != 0)
		return false;

	fseek(fp, -7, SEEK_END);
	iSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	cBuffer = (char*)malloc(iSize);

	iFileCheck = fread(cBuffer, iSize, 1, fp);
	if (iFileCheck < 1)
		return false;

	cBuffer[iSize] = 0;

	return true;
}

// GetNextWord에서 단어 스킵 여부를 체크한다.
// true면 스킵 / false면 스킵이 아니다.
bool SkipNoneCommand(char cTempWord)
{
	// 순서대로 콤마, 마침표, 따옴표, 스페이스, 백스페이스, 탭, 라인피드, 캐리지 리턴, / 문자(주석체크용)
	if (cTempWord == ',' || cTempWord == '.' || cTempWord == '"' || cTempWord == 0x20 || cTempWord == 0x08 || cTempWord == 0x09 || cTempWord == 0x0a || cTempWord == 0x0d || cTempWord == '/')
		return true;

	return false;
}

// 버퍼에서, 다음 단어 찾기
bool GetNextWord(char cWord[], int* len)
{
	int i = 0;
	char cTempWord[256];

	// 단어 시작 위치를 찾는다. true면 계속 스킵된다.
	while (SkipNoneCommand(cBuffer[*len]))
	{
		// len의 값이 cBuffer의 사이즈보다 커지면, 배열을 벗어난거니 false 리턴.
		if (*len > strlen(cBuffer))
			return false;

		(*len)++;
	}

	// 시작위치를 찾았다. 이제 끝 위치를 찾아야한다.
	while (1)
	{
		// len의 값이 cBuffer의 사이즈보다 커지면, 배열을 벗어난거니 false 리턴.
		if (*len > strlen(cBuffer))
			return false;

		// 순서대로 콤마, 마침표, 따옴표, 스페이스, 백스페이스, 탭, 라인피드, 캐리지 리턴
		// 이 중 하나라도 해당되면, 끝 위치를 찾은거니 while문을 나간다.
		else if (cBuffer[*len] == ',' || cBuffer[*len] == '.' || cBuffer[*len] == '"' || cBuffer[*len] == 0x20 || cBuffer[*len] == 0x08 || cBuffer[*len] == 0x09 || cBuffer[*len] == 0x0a || cBuffer[*len] == 0x0d)
			break;

		// 값을 넣는다.
		// 그리고 다음 값을 넣기 위해 현재 배열의 위치를 1칸 이동
		cTempWord[i++] = cBuffer[(*len)++];

	}

	// 단어의 마지막에 널문자를 넣어 문자열로 완성.
	cTempWord[i] = '\0';
	(*len)++;

	// 문자열 복사.
	memset(cWord, 0, 256);
	strcpy_s(cWord, sizeof(cTempWord), cTempWord);

	return true;
}

// 파서_원하는 값 찾아오기 (int형)
bool Parser_GetValue_Int(const char* Name, int* value)
{
	int len = 0;
	char cWord[256];

	// cBuffer의 문자열을 cWord에 옮긴다. 실행될 때 마다 다음 문자열을 받아온다.
	// 받아오는데 성공하면 true, 받아오는데 실패하면 false이다. 받아오는데 실패하는 경우는, 마지막까지 모두 검사를 한 것이다.
	while (GetNextWord(cWord, &len))
	{
		// 찾은 문자열이 내가 원하는 문자열인지 확인. 내가 찾는 문자열이 아니라면 다시 위 while문으로 문자 확인.
		if (strcmp(cWord, Name) == 0)
		{
			// 내가 찾는 문자열이라면, 다음 문자로 = 를 찾는다.
			// if (GetNextWord(cWord, &len))가 true라면 일단 다음 문자열을 찾은 것이다.
			if (GetNextWord(cWord, &len))
			{
				// 문자열을 찾았으니 이 문자열이 진짜 = 인지 확인하자.
				if (strcmp(cWord, "=") == 0)
				{
					// = 문자열이라면, 다음 문자열을 찾아온다. 이번에 찾는것은 정말 Value이다.
					if (GetNextWord(cWord, &len))
					{
						// 여기까지 왔으면, Value도 찾은 것이니 이제 값을 넣자.
						*value = atoi(cWord);
						return true;
					}

				}

			}

		}

	}

	// 여기까지 온건, cBuffer의 모든 데이터를 찾았는데도 내가 원하는 데이터가 없는 것이니, false 반환.
	return false;
}

// 파서_원하는 값 찾아오기 (문자열)
bool Parser_GetValue_String(const char* Name, char* value)
{
	int len = 0;
	char cWord[256];

	// cBuffer의 문자열을 cWord에 옮긴다. 실행될 때 마다 다음 문자열을 받아온다.
	// 받아오는데 성공하면 true, 받아오는데 실패하면 false이다. 받아오는데 실패하는 경우는, 마지막까지 모두 검사를 한 것이다.
	while (GetNextWord(cWord, &len))
	{
		// 찾은 문자열이 내가 원하는 문자열인지 확인. 내가 찾는 문자열이 아니라면 다시 위 while문으로 문자 확인.
		if (strcmp(cWord, Name) == 0)
		{
			// 내가 찾는 문자열이라면, 다음 문자로 = 를 찾는다.
			// if (GetNextWord(cWord, &len))가 true라면 일단 다음 문자열을 찾은 것이다.
			if (GetNextWord(cWord, &len))
			{
				// 문자열을 찾았으니 이 문자열이 진짜 = 인지 확인하자.
				if (strcmp(cWord, "=") == 0)
				{
					// = 문자열이라면, 다음 문자열을 찾아온다. 이번에 찾는것은 정말 Value이다.
					if (GetNextWord(cWord, &len))
					{
						// 여기까지 왔으면, Value도 찾은 것이니 이제 값을 넣자.
						strcpy_s(value, sizeof(cWord), cWord);
						return true;
					}

				}

			}

		}

	}

	// 여기까지 온건, cBuffer의 모든 데이터를 찾았는데도 내가 원하는 데이터가 없는 것이니, false 반환.
	return false;
}