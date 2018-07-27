// 파일에 있는 영단어를 한글로 번역해서 보여주기.
#pragma warning(disable:4996)

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
	// 첫 배열 요소의 값을 0으로 만들어서, 복사가 가능하도록 한다.
	cKorBuffer[0] = 0;

	char cWard[WORD_LEN];
	char cInput[10];
	int iFilelen;
	int iWordCount = 0;
	int i = 0;
	int j;

	// 사전 단어집 사전세팅
	Dictionary Dic[WORD_COUNT] =
	{
		{ "english","영어" },
		{ "korea","한국" },
		{ "apple","사과" },
		{ "people","사람" },
		{ "love","사랑" },
		{ "name","이름" },
		{ "you","너" },
		{ "stupid","바보같은" },
		{ "america","미국" },
		{ "bag","가방" },
		{ "money","돈" },
		{ "paper","종이" },
	};

	//FILE* fp = fopen("Test1.txt", "rt");
	
	fputs("번역할 파일 이름을 입력해주세요 : ", stdout);
	scanf("%s", cInput);

	FILE* fp;
	fopen_s(&fp, cInput, "rt");
	

	if (fp == NULL)
	{
		puts("파일 오픈 실패");
		return -1;
	}

	
	while (1)
	{
		i = 0;

		// 1. 파일에서 문자열을 문자 단위로 읽어온다. 파일의 끝까지 읽어온다.
		while ((cEngBuffer[i] = fgetc(fp)) != EOF)
			i++;
		
		// 3. 파일의 총 문자 개수 저장.
		iFilelen = i-1;

		i = 0;

		// 2. 모든 문자 소문자로 변환
		strlwr(cEngBuffer);	

		while (1)
		{
			j = 0;
			// 3. cEngBuffer 에서 1개의 단어 추출.
			while (1)
			{
				if (cEngBuffer[i] == ' ' || cEngBuffer[i] == '\n')
					break;
				cWard[j] = cEngBuffer[i];
				i++;
				j++;
			}

			// 4. 여기까지 왔다는 것은 단어 1개를 모두 찾았다는 것. 마지막에 널을 넣어서 문자열로 만든다.
			cWard[j] = 0;


			// 5. 찾은 단어와 사전의 단어를 비교한다. 문자열이 같다면 해당 영어의 한글을 Kor버퍼에 저장.
			for (j = 0; j < WORD_COUNT; ++j)
			{
				if (0 == strcmp(Dic[j].Eng, cWard))
				{
					// 여기에 온건, 단어 하나를 찾았다는 뜻이니, 단어 카운트 증가.
					iWordCount++;

					strcat_s(cKorBuffer, sizeof(cKorBuffer), Dic[j].Kor);
					cKorBuffer[strlen(cKorBuffer) +1] = 0;

					if (cEngBuffer[i] == ' ')
						cKorBuffer[strlen(cKorBuffer)] = ' ';

					else if (cEngBuffer[i] == '\n')
						cKorBuffer[strlen(cKorBuffer)] = '\n';						

					break;
				}
			}
			// 6. 사전을 마지막까지 뒤졌는데도 일치하는 단어가 없으면, ??를 넣는다.
			if (j == WORD_COUNT)
			{
				strcat_s(cKorBuffer, sizeof(cKorBuffer), "??");
				cKorBuffer[strlen(cKorBuffer) + 1] = 0;

				if (cEngBuffer[i] == ' ')
					cKorBuffer[strlen(cKorBuffer)] = ' ';

				else if (cEngBuffer[i] == '\n')
					cKorBuffer[strlen(cKorBuffer)] = '\n';				
				
			}

			// 7. 만약, 이번 문자열이 마지막이라면 while문 종료.
			if (i == iFilelen)
				break;

			++i;
		}

		// 8. 치환된 문자열 표시
		puts("\n--번역 완료!--");
		printf("번역한 단어 수 : %d\n\n", iWordCount);
		printf("%s\n", cKorBuffer);		

		break;
		
	}	

	fclose(fp);
	return 0;
}