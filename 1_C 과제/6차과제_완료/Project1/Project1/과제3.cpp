// 파일 패킹 / 언패킹
#pragma warning(disable:4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	char m_name[30];
	int m_Size;
	int m_Offset;

}PackingFile;

typedef struct
{
	char* cContent;

}PackingContent;

int main()
{
	FILE* fp1;
	FILE* fp2;
	PackingFile* Filelist;
	PackingContent* FileContent;
	int iInput;
	int iFileCount;
	int iNowOffset;
	char c_MyFileCheck[11] = "SongJinGyu";
	char c_FackingFileName[20];
	char c_MyFileCheckSave[11];

	// 1. 패킹 하기, 풀기 선택
	puts("1. 패킹하기");
	puts("2. 패킹풀기");
	scanf("%d", &iInput);

	switch (iInput)
	{
	case 1:
		// 패킹 하기 로직
		// 1. 패킹할 파일 개수 입력
		fputs("패킹할 파일 개수를 입력하세요 : ", stdout);
		scanf("%d", &iFileCount);		

		// 2. 입력한 파일 개수만큼 Filelist에 동적할당.
		Filelist = (PackingFile*)malloc(sizeof(PackingFile) * iFileCount);
		memset(Filelist, 0, sizeof(PackingFile));

		FileContent = (PackingContent*)malloc(sizeof(PackingContent) * iFileCount);
		memset(FileContent, 0, sizeof(PackingContent));

		// 3. 패킹할 파일 이름을 모두 입력.
		for (int i = 0; i < iFileCount; ++i)
		{
			printf("%d번 파일 이름 입력 : ", i);
			scanf("%s", Filelist[i].m_name);
		}

		// 4. 패킹 후 완성본의 파일 이름 입력.
		fputs("패킹 될 파일 이름 입력 : ", stdout);
		scanf("%s", c_FackingFileName);
	

		// 5. 패킹할 파일의 [사이즈], [내용]을 저장한다.
		for (int i = 0; i < iFileCount; ++i)
		{
			// 파일 스트림 연결
			fopen_s(&fp2, Filelist[i].m_name, "rb");

			// 파일의 사이즈를 저장하고.
			fseek(fp2, 0, SEEK_END);
			Filelist[i].m_Size = ftell(fp2);
			fseek(fp2, 0, SEEK_SET);

			// 내용 저장
			FileContent[i].cContent = (char*)malloc(Filelist[i].m_Size);
			fread(FileContent[i].cContent, Filelist[i].m_Size, 1, fp2);

			// 다 썻으니 파일 스트림 닫음.
			fclose(fp2);
		}	

		// 6. 읽어온 파일들의 내용이 들어갈 최초 OffSet을 구한다.
		// 여기까지 되면, iNowOffset이 내용이 들어갈 최초 위치로 세팅된다.

		// [내 파일 마크]가 들어갈 공간 + [파일 개수]가 들어갈 공간 확보.
		iNowOffset = sizeof(c_MyFileCheck);
		iNowOffset += sizeof(iFileCount);

		//  각 파일의 [이름], [사이즈], [오브셋]이 들어갈 공간 확보
		for (int i = 0; i < iFileCount; ++i)
		{
			iNowOffset += sizeof(Filelist[i].m_name);
			iNowOffset += sizeof(Filelist[i].m_Size);
			iNowOffset += sizeof(Filelist[i].m_Offset);
		}

		// 7. Offset의 값을 저장한다.
		for (int i = 0; i < iFileCount; ++i)
		{
			Filelist[i].m_Offset = iNowOffset;
			iNowOffset += Filelist[i].m_Size;
		}


		// 8. 완성본의 출력 스트림 생성
		fopen_s(&fp1, c_FackingFileName, "wb");

		// 9. 완성본에다가 [내 파일 마크], [파일 개수]를 보낸다.
		fwrite(c_MyFileCheck, sizeof(c_MyFileCheck), 1, fp1);
		fwrite(&iFileCount, sizeof(iFileCount), 1, fp1);

		// 10. 읽어온 파일의 [이름], [사이즈], [오브셋]정보를 완성본에 보낸다.
		for (int i = 0; i < iFileCount; ++i)
		{
			// 이름을 보내고.
			fwrite(Filelist[i].m_name, sizeof(Filelist[i].m_name), 1, fp1);

			// 사이즈를 보내고
			fwrite(&Filelist[i].m_Size, sizeof(Filelist[i].m_Size), 1, fp1);
			
			// 오브셋 보낸다.
			fwrite(&Filelist[i].m_Offset, sizeof(Filelist[i].m_Offset), 1, fp1);

		}
		// 11. 파일의 [내용]을 완성본에 보낸다.

		for (int i = 0; i < iFileCount; ++i)
		{
			fwrite(FileContent[i].cContent, Filelist[i].m_Size, 1, fp1);
		}

		// 12. 완성본 스트림 닫음
		fclose(fp1);

		puts("파일 패킹 완료!");

		break;
	case 2:
		// 패킹 풀기 로직
		// 1. 패킹을 풀 파일이름 입력
		fputs("패킹을 풀 파일이름 입력 : ", stdout);
		scanf("%s", c_FackingFileName);

		// 2. 파일을 내려받기 위해 바이너리 읽기모드 오픈
		fopen_s(&fp1, c_FackingFileName, "rb");

		// 3. [내 파일 체크]를 위해 그 만큼만 임시로 내려받는다.
		fread(c_MyFileCheckSave, sizeof(c_MyFileCheckSave), 1, fp1);
		if (0 != strcmp(c_MyFileCheckSave, c_MyFileCheck))
		{
			puts("내 파일이 아닙니다!");
			return -1;
		}

		// 4. 내 파일이라는게 검증되었으니, 이어서 파일 개수를 내려받는다.
		fread((char*)&iFileCount, sizeof(iFileCount), 1, fp1);
		
		// 5. 파일 개수만큼 파일 리스트를 동적할당 후, [이름], [파일 크기], [오브셋]을 내려받는다.
		Filelist = (PackingFile*)malloc(sizeof(PackingFile) * iFileCount);
		memset(Filelist, 0, sizeof(PackingFile));

		for (int i = 0; i < iFileCount; ++i)
		{
			// 이름을 내려받고
			fread(Filelist[i].m_name, sizeof(Filelist[i].m_name), 1, fp1);

			// 파일 크기를 내려받고
			fread(&Filelist[i].m_Size, sizeof(Filelist[i].m_Size), 1, fp1);

			// 오브셋을 내려받는다.
			fread(&Filelist[i].m_Offset, sizeof(Filelist[i].m_Offset), 1, fp1);

		}

		// 6. 내용을 받을 구조체를 파일 개수만큼 동적할당 후, 순서대로 내려받는다.
		FileContent = (PackingContent*)malloc(sizeof(PackingContent) * iFileCount);
		memset(FileContent, 0, sizeof(PackingContent));

		for (int i = 0; i < iFileCount; ++i)
		{
			FileContent[i].cContent = (char*)malloc(Filelist[i].m_Size);
			fread(FileContent[i].cContent, Filelist[i].m_Size, 1, fp1);
		}

		fclose(fp1);

		// 7. 파일 개수만큼 파일을 생성한다.
		for (int i = 0; i < iFileCount; ++i)
		{
			// 생성할 파일을 위한 출력 스트림 오픈. 오픈과 함께 파일 생성.
			fopen_s(&fp2, Filelist[i].m_name, "wb");

			// 파일 내용을 보낸다.
			fwrite(FileContent[i].cContent, Filelist[i].m_Size, 1, fp2);

			// 끝났으니 스트림 닫는다.
			fclose(fp2);
		}

		puts("패킹 풀기 완료");
		
		break;
	}

	return 0;
}