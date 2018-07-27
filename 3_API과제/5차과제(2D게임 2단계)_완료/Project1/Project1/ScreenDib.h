#pragma once
#ifndef __SCREENDIB_H__
#define __SCREENDIB_H__

#include <Windows.h>

class CScreenDib
{
private:
	//---------------------------
	// 싱글톤을 위해 생성자를 private로 선언.
	// --------------------------	
	CScreenDib(int iWidth, int iHeight, int iColorBit);

public:
	//---------------------------
	// 파괴자
	// --------------------------	
	virtual ~CScreenDib();

	//---------------------------
	// 싱글톤 생성 함수
	// --------------------------
	static CScreenDib* Getinstance(int iWidth, int iHeight, int iColorBit);


protected:
	//---------------------------
	// 내 백버퍼 동적할당 / 동적할당 해제 함수
	// 생성자와 소멸자에서 호출한다.
	// --------------------------
	void CreateDibBuffer(int iWidth, int iHeight, int iColorBit);
	void ReleseDibBuffer();

public:
	//---------------------------
	// Flip 함수
	// --------------------------
	void DrawBuffer(HWND hWnd, int iX = 0, int iY = 0);

	//---------------------------
	// 각종 게터함수
	// --------------------------
	BYTE *GetDibBuffer();	// m_bypBuffer를 반환하는 함수
	int GetWidth();			// m_iWidth를 반환하는 함수
	int GetHeight();		// m_iHeight를 반환하는 함수
	int GetPitch();			// m_iPitch를 반환하는 함수

protected:
	//---------------------------
	// 멤버 변수
	// --------------------------
	BITMAPINFO	m_stDibInfo;	// 내가 만든 BITMAPINFO
	BYTE		*m_bypBuffer;	// 내 백버퍼.
	
	int m_iWidth;				// 백버퍼의 넓이. 픽셀 단위.
	int m_iHeight;				// 백버퍼의 높이. 픽셀단위
	int m_iPitch;				// 백버퍼의 피치. (바이트 단위 가로 크기)
	int m_iColorBit;			// 컬러 깊이. 8bit / 16bit 등..
	int m_iBufferSize;			// 실제 이미지 사이즈. 바이트 단위.
	
};

#endif // !__SCREENDIB_H__
