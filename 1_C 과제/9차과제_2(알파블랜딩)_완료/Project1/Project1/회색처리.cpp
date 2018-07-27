#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main()
{
	BITMAPFILEHEADER FileHeader1;
	BITMAPINFOHEADER InfoHeader1;
	UINT* cFileBuffer_1;

	UINT Red1, Green1, Blue1, Alpha1;
	UINT RGBAverage;

	UINT iBreakCheck = 0;
	bool bBreakFlag = false;

	FILE* fp1;
	FILE* fp3;

	// 파일의 스트림 연결
	fopen_s(&fp1, "aa.bmp", "rb");

	// 파일의 파일헤더,정보헤더 읽어옴
	fread(&FileHeader1, sizeof(FileHeader1), 1, fp1);
	fread(&InfoHeader1, sizeof(InfoHeader1), 1, fp1);

	// 파일의 이미지 크기만큼 동적할당
	cFileBuffer_1 = (UINT*)malloc(InfoHeader1.biSizeImage);

	printf("1 : %d\n", InfoHeader1.biSizeImage);

	// 파일의 이미지 크기만큼 나머지를 읽어온다.
	fread(cFileBuffer_1, InfoHeader1.biSizeImage, 1, fp1);

	// 1,2번 파일 읽어오기 끝. 파일 스트림 닫는다.
	fclose(fp1);

	// 블랜딩 할 파일 스트림 연결
	fopen_s(&fp3, "Gray.bmp", "wb");

	// 새로 생성할 파일의 [파일 헤더], [정보 헤더] 보냄
	fwrite(&FileHeader1, sizeof(FileHeader1), 1, fp3);
	fwrite(&InfoHeader1, sizeof(InfoHeader1), 1, fp3);

	// R, G, B, A를 분리해서 각자 계산 후 알파블랜딩한다.
	while (1)
	{
		// iBreakCheck가 파일 크기보다 커지면 마지막 절차인 것.
		if (iBreakCheck > InfoHeader1.biSizeImage)
			bBreakFlag = true;

		// 파일의 R,G,B,A를 분리한다.
		Red1 = *cFileBuffer_1 & 0x000000ff;
		Green1 = (*cFileBuffer_1 & 0x0000ff00) >> 8;
		Blue1 = (*cFileBuffer_1 & 0x00ff0000) >> 16;
		Alpha1 = (*cFileBuffer_1 & 0xff000000) >> 24;

		// RGB의 평균을 내고 R,G,B 모두 그 값을 같는다. 이게 회색처리하는 방법.
		RGBAverage = (Red1 + Green1 + Blue1) / 3;

		// 기존의 RGBA 자리에 평균계산된 값을 보낸다.
		// Alpha자리에는 그냥 기존의 Alpha를 보낸다.
		fwrite(&RGBAverage, 1, 1, fp3);
		fwrite(&RGBAverage, 1, 1, fp3);
		fwrite(&RGBAverage, 1, 1, fp3);
		fwrite(&Alpha1, 1, 1, fp3);

		// 이번이 마지막 절차였으면, while문 종료
		if (bBreakFlag == true)
			break;

		// 각 파일 버퍼의 값을 1씩 증가.
		// 근데, 포인터이기 때문에, 포인트의 형(int)만큼 증가. 즉, 4바이트 증가.
		cFileBuffer_1++;

		// 종료를 체크할 변수에도 4 증가. 여기는 바이트 단위로 가정한다.
		iBreakCheck += 4;
	}

	// 파일 출력이 끝났으니 파일을 닫는다.
	fclose(fp3);

	return 0;
}