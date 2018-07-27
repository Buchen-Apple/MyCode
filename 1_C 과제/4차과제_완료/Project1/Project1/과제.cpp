#include <stdio.h>
#include <string.h>
#define BUFFER_LEN	250
#define WORD_LEN	100
#define WORD_COUNT	30

typedef struct
{
	char Eng[WORD_LEN];
	char Kor[WORD_LEN];

}Dictionary;

int main()
{
	char cEngBuffer[BUFFER_LEN];
	char cKorBuffer[BUFFER_LEN];
	// 첫 배열의 값을 0으로 만들어서, 첫 시작 복사가 가능하도록 한다.
	cKorBuffer[0] = 0;

	char cWard[WORD_LEN];
	int i = 0;
	int j;
	bool ibreakFlag = false;

	// 사전 단어집 사전세팅
	Dictionary Dic[WORD_COUNT] =
	{
	{"english","영어"},
	{"korea","한국"},
	{"apple","사과"},
	{"people","사람"},
	{"love","사랑"},
	{"name","이름"},
	{"you","너"},
	{"stupid","바보같은"},
	{"america","미국"},
	{"bag","가방"},
	{"money","돈"},
	{"paper","종이"},
	};

	// 1. 영어 문자열 입력 후, cEngBuffer에 저장
	fputs("영어 문자열 입력 : ", stdout);
	gets_s(cEngBuffer, BUFFER_LEN);

	// 2. 모든 문자를 소문자로 변환.	
	strlwr(cEngBuffer);

	// 3. cEngBuffer에서 문자를 공백 단위로 쪼개서 단어저장 버퍼에 저장.
	while (1)
	{
		j = 0;
		// 4. 입력받은 문자열에서 1개의 단어 추출.
		while (1)
		{
			if (cEngBuffer[i] == ' ' || cEngBuffer[i] == 0)
				break;
			cWard[j] = cEngBuffer[i];
			i++;
			j++;
		}
		
		// 5. 마지막 문자였다면, while문을 빠져나가야 하기 때문에 flag를 true로 만듦.
		if (cEngBuffer[i] == 0)
			ibreakFlag = true;

		// 6. 여기까지 왔다는 것은 단어 1개를 모두 찾았다는 것. 단어 마지막에 널문자를 넣어준다.
		cWard[j] = 0;


		// 7. 찾은 단어와 사전의 단어를 비교한다. 문자열이 같다면 해당 영어의 한글을 Kor버퍼에 저장.
		for (j = 0; j < WORD_COUNT; ++j)
		{
			if (0 == strcmp(Dic[j].Eng, cWard))
			{
				strcat_s(cKorBuffer, sizeof(cKorBuffer), Dic[j].Kor);
				cKorBuffer[strlen(cKorBuffer) + 1] = 0;
				cKorBuffer[strlen(cKorBuffer)] = ' ';
				
				break;
			}
		}
		// 8. 사전을 마지막까지 뒤졌는데도 일치하는 단어가 없으면, ??를 넣는다.
		if (j == WORD_COUNT)
		{
			strcat_s(cKorBuffer, sizeof(cKorBuffer), "??");
			cKorBuffer[strlen(cKorBuffer) + 1] = 0;
			cKorBuffer[strlen(cKorBuffer)] = ' ';
		}

		// 9. 마지막 문자였다면 (5번 절차에서 체크함)  while문 종료
		if (ibreakFlag == true)
			break;
		i++;
	}

	// 10. 치환된 문자열 표시
	printf("치환 한국어 표시 : %s\n", cKorBuffer);	

	return 0;
}