#include "stdafx.h"
#include "ScreenDib.h"


//---------------------------
// 생성자 (private)
// --------------------------
CScreenDib::CScreenDib(int iWidth, int iHeight, int iColorBit)
{
	// 각종 멤버변수 초기화
	m_iWidth = iWidth;
	m_iHeight = iHeight;
	m_iColorBit = iColorBit;	

	// 내 백버퍼 생성 함수 호출
	CreateDibBuffer(m_iWidth, m_iHeight, m_iColorBit);
}

//---------------------------
// 파괴자 (public)
// --------------------------
// 가상 파괴자.
CScreenDib::~CScreenDib()
{
	ReleseDibBuffer();	// DibBuffer 삭제
}

//---------------------------
// 싱글톤. 생성자는 private이다.
// --------------------------
CScreenDib *CScreenDib::Getinstance(int iWidth, int iHeight, int iColorBit)
{
	static CScreenDib cScreenDib(iWidth, iHeight, iColorBit);
	return &cScreenDib;
}


//---------------------------
// DibBuffer Create
// --------------------------
void CScreenDib::CreateDibBuffer(int iWidth, int iHeight, int iColorBit)
{
	/////////////////////////////////////////////////////
	// 입력받은 인자의 정보로 맴버변수 정보 셋팅
	////////////////////////////////////////////////////
	// pitch 구하기 (실제 바이트단위 가로 길이)
	m_iPitch = (iWidth * (iColorBit / 8)) + 3 & ~3;

	// 실제 이미지 사이즈 구하기
	m_iBufferSize = m_iPitch * iHeight;

	// BITMAPINFO (BITMAPINFOHEADER)를 셋팅한다.
	m_stDibInfo.bmiHeader.biSize = sizeof(m_stDibInfo.bmiHeader);	// 비트맵 인포 헤더의 사이즈
	m_stDibInfo.bmiHeader.biWidth = iWidth;			// 가로 (픽셀)
	m_stDibInfo.bmiHeader.biHeight = -iHeight;		// 높이 (픽셀). BITMAPINFOHEADER 높이를 -로 넣으면 왠지모르게 알아서 뒤집어준다. 즉, 뒤집힌 비트맵을 정상적으로 나오게 해준다.
	m_stDibInfo.bmiHeader.biPlanes = 1;				// 뭔지 모르겠지만 무조건 1
	m_stDibInfo.bmiHeader.biBitCount = iColorBit;	// 비트 세팅
	m_stDibInfo.bmiHeader.biSizeImage = m_iBufferSize;	// 실제 이미지 사이즈. 실제 비트맵 인포헤더에도 실제 비트 단위 고려해 계산된 이미지 사이즈가 들어가 있음.
	
	// 이미지 사이즈를 계산하여 버퍼용 이미지 동적할당
	m_bypBuffer = new BYTE[m_iBufferSize];
}

//---------------------------
// DibBuffer Relese
// --------------------------
void CScreenDib::ReleseDibBuffer()
{
	// 메모리 해제 ~ 
	delete m_bypBuffer;
}

//---------------------------
// DC에 찍기
// --------------------------
void CScreenDib::DrawBuffer(HWND hWnd, int iX, int iY)
{
	// 입력받은 hWnd 핸들의 DC를 얻어서 DC의 X, Y 위치에 스크린 버퍼 DIB를 출력한다.
	HDC hdc = GetDC(hWnd);
	SetStretchBltMode(hdc, COLORONCOLOR);			// 이미지 축소 시 깨짐모드 방지. COLORONCOLOR : 논리연산 하지 않음. 컬러 비트맵에서 가장 무난한 방법이라고 함.
	StretchDIBits(hdc, iX, iY, m_iWidth, m_iHeight,	// 목적지(DC)의 x,y 좌표와 폭,높이.
		0, 0, m_iWidth, m_iHeight,					// DIB의 x,y 좌표와 폭, 높이. 해당 DIB를 지정한 목적지 크기만큼 출력.
		m_bypBuffer, &m_stDibInfo, DIB_RGB_COLORS, SRCCOPY);	// 실제 이미지가 저장되어 있는 포인터 시작점, BITMAPINFO정보를 넘겨야 하지만, 우린 팔레트 안쓰니 형변환
																		// DIB_RGB_COLORS는 RGB컬러를 쓸건지 팔레트를 쓸건지 선택, 마지막은 출력모드. 즉 우리는 복사.

	ReleaseDC(hWnd,hdc);
}

//---------------------------
// 백 버퍼 얻기
// --------------------------
BYTE* CScreenDib::GetDibBuffer()
{
	return m_bypBuffer;
}

//---------------------------
// 각종 게터
// --------------------------

int CScreenDib::GetWidth()
{
	return m_iWidth;
}
int CScreenDib::GetHeight()
{
	return m_iHeight;
}
int CScreenDib::GetPitch()
{
	return m_iPitch;
}