#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main()
{
	BITMAPFILEHEADER FileHeader1;
	BITMAPFILEHEADER FileHeader2;
	UINT* cFileBuffer_1;

	BITMAPINFOHEADER InfoHeader1;	
	BITMAPINFOHEADER InfoHeader2;
	UINT* cFileBuffer_2;

	UINT Red1, Red2, Green1, Green2, Blue1, Blue2, Alpha1, Alpha2;
	UINT Red3, Green3, Blue3, Alpha3;

	UINT iBreakCheck = 0;
	bool bBreakFlag = false;

	FILE* fp1;
	FILE* fp2;
	FILE* fp3;

	// 1번째 파일, 2번째 파일의 스트림 연결
	fopen_s(&fp1, "aa.bmp", "rb");
	fopen_s(&fp2, "bb.bmp", "rb");

	// 1번 파일의 파일헤더,정보헤더 읽어옴
	fread(&FileHeader1, sizeof(FileHeader1), 1, fp1);
	fread(&InfoHeader1, sizeof(InfoHeader1), 1, fp1);

	// 2번 파일의 파일헤더,정보헤더 읽어옴
	fread(&FileHeader2, sizeof(FileHeader2), 1, fp2);	
	fread(&InfoHeader2, sizeof(InfoHeader2), 1, fp2);

	// 각 파일의 이미지 크기만큼 동적할당
	cFileBuffer_1 = (UINT*)malloc(InfoHeader1.biSizeImage);
	cFileBuffer_2 = (UINT*)malloc(InfoHeader2.biSizeImage);

	printf("1 : %d\n", InfoHeader1.biSizeImage);
	printf("2 : %d\n", InfoHeader2.biSizeImage);

	// 각 파일의 이미지 크기만큼 나머지를 읽어온다.
	fread(cFileBuffer_1, InfoHeader1.biSizeImage, 1, fp1);
	fread(cFileBuffer_2, InfoHeader2.biSizeImage, 1, fp2);

	// 1,2번 파일 읽어오기 끝. 파일 스트림 닫는다.
	fclose(fp1);
	fclose(fp2);

	// 블랜딩 할 파일 스트림 연결
	fopen_s(&fp3, "blend.bmp", "wb");

	// 새로 생성할 파일의 [파일 헤더], [정보 헤더] 보냄
	fwrite(&FileHeader1, sizeof(FileHeader1), 1, fp3);
	fwrite(&InfoHeader1, sizeof(InfoHeader1), 1, fp3);

	// R, G, B, A를 분리해서 각자 계산 후 알파블랜딩한다.
	while (1)
	{
		// iBreakCheck가 파일 크기보다 커지면 마지막 절차인 것.
		if (iBreakCheck > InfoHeader1.biSizeImage)
			bBreakFlag = true;
		
		// 1번 파일의 R,G,B,A를 분리한다.
		Red1 = *cFileBuffer_1 & 0x000000ff;
		Green1 = (*cFileBuffer_1 & 0x0000ff00) >> 8;
		Blue1 = (*cFileBuffer_1 & 0x00ff0000) >> 16;
		Alpha1 = (*cFileBuffer_1  & 0xff000000) >> 24;

		// 2번 파일의 R,G,B,A를 분리한다.
		Red2 = *cFileBuffer_2 & 0x000000ff;
		Green2 = (*cFileBuffer_2 & 0x0000ff00) >> 8;
		Blue2 = (*cFileBuffer_2 & 0x00ff0000) >> 16;
		Alpha2 = (*cFileBuffer_2 & 0xff000000) >> 24;
		
		// 각 파일에 /2 한것을 서로 더한다. 이게 알파블랜딩하는 방법이다 (각 파일의 RGBA값을 절반으로 나눈 후 서로 합침)
		Red3 = (Red1 / 2) + (Red2 / 2);
		Green3 = (Green1 / 2) + (Green2 / 2);
		Blue3 = (Blue1 / 2) + (Blue2 / 2);
		Alpha3 = (Alpha1 / 2) + (Alpha2 / 2);

		// R,G,B,A를 새로운 파일로 보낸다.
		fwrite(&Red3, 1, 1, fp3);
		fwrite(&Green3, 1, 1, fp3);
		fwrite(&Blue3, 1, 1, fp3);
		fwrite(&Alpha3, 1, 1, fp3);

		// 이번이 마지막 절차였으면, while문 종료
		if (bBreakFlag == true)
			break;

		// 각 파일 버퍼의 값을 1씩 증가.
		// 근데, 포인터이기 때문에, 포인트의 형(int)만큼 증가. 즉, 4바이트 증가.
		cFileBuffer_1++;
		cFileBuffer_2++;

		// 종료를 체크할 변수에도 4 증가. 여기는 바이트 단위로 가정한다.
		iBreakCheck += 4;
	}

	// 파일 출력이 끝났으니 파일을 닫는다.
	fclose(fp3);
	
	return 0;
}