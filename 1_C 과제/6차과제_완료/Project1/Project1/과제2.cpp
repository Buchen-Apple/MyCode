// 파일 암호화/복호화

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

int main()
{
	char cMyFileCheck[11] = "SongJinGyu";
	int iFileSize;
	int iFileReadCheck;
	char* cFileSave;
	char* cRefIleSave;
	unsigned int iMask = 32457;
	int j = 0;
	char cInput[30];

	// false면 암호화 , true면 복호화
	bool bCryptography = false;

	FILE* fp;

	// 1. 암호화/복호화 할 파일이름 입력
	fputs("암호화/복호화 할 파일 이름을 입력해주세요 : ", stdout);
	scanf("%s", cInput);


	// 2. 입력 스트림을 바이너리 모드로 오픈
	fopen_s(&fp, cInput, "rb");


	// 3. 읽어온 파일의 크기를 구한다.
	fseek(fp, 0, SEEK_END);
	iFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);


	// 4. 파일 크기 만큼 동적할당. 
	cFileSave = (char*)malloc(iFileSize);


	// 5. 동적할당 한 메모리에 파일을 읽어온다.
	iFileReadCheck = fread(cFileSave, iFileSize, 1, fp);

	// 6. 읽어오기 실패하면 에러메시지 발생
	if (iFileReadCheck != 1)
	{
		puts("뭔가 문제가 있네요!");
		return -1;
	}

	// 7. 입력 스트림 오프
	fclose(fp);


	// 8. 암호화, 복호화 체크
	// 두 문자열을 strlen(cMyFileCheck) 길이만큼 비교.
	// 둘이 같다면, SongJinGyu 라는 문자가 있는거니, 암호화 되어 있는것으로 판단. 즉, 복호화를 해야 한다.
	if (0 == strncmp(cFileSave, cMyFileCheck, strlen(cMyFileCheck)))
		bCryptography = true;

	// 9. 출력 스트림 오픈	
	fopen_s(&fp, "test2.txt", "wb");

	// 8번 절차에서, bCryptography가 false일 경우 암호화 로직 진행
	if (bCryptography == false)
	{
		// 일단 내 파일이라는것을 표시하는 문자열을 먼저 파일에 넣는다.
		// sizeof(cMyFileCheck)에 -1을 하는 이유는, cMyFileCheck에는 널을 포함한 문자열이 들어가 있기 때문에, 널을 빼고 넣는것이다.
		fwrite(cMyFileCheck, sizeof(cMyFileCheck)-1, 1, fp);
		
		// 파일에서 읽어온 문자열을 iMask와 xor해서 암호화 한다.
		// 반복문을 통해 하나하나 암호화.
		for (int i = 0; i < iFileSize; ++i)
		{
			cFileSave[i] ^= iMask;
			iMask = iMask >> 1;
			j++;
			if (j == 32)
			{
				iMask = 32457;
				j = 0;
			}
		}

		// 암호화 한 문자열을 바이너리 모드로 파일로 보낸다.
		fwrite(cFileSave, iFileSize, 1, fp);
		
		puts("암호화 완료!");
	}

	// 8번 절차에서 bCryptography가 true일 경우 복호화 로직 진행
	else 	
	{
		// 복호화 할 정보를 저장할 메모리 공간 동적할당.
		// 메모리 공간은 읽어온 파일의 크기 - 내 파일표시 문자열이다.
		// 내 파일표시 문자열에서 -1을 하는 이유는 위와 마찬가지로 널때문이다.
		cRefIleSave = (char*)malloc(iFileSize - (sizeof(cMyFileCheck) -1));

		for (int i = sizeof(cMyFileCheck) -1; i < iFileSize; ++i)
		{
			cRefIleSave[i - (sizeof(cMyFileCheck) -1)] = (cFileSave[i] ^ iMask);
			
			iMask = iMask >> 1;
			j++;
			if (j == 32)
			{
				iMask = 32457;
				j = 0;
			}
		}

		// 복호화 한 문자열을 바이너리 모드로 파일로 보낸다.
		fwrite(cRefIleSave, iFileSize - (sizeof(cMyFileCheck) -1), 1, fp);

		puts("복호화 완료!");
	}

	// 10. 출력 스트림 오프
	fclose(fp);

	return 0;
}