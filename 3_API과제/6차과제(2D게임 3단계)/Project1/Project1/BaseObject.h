#pragma once
#ifndef __BASE_OBJECT_H__
#define __BASE_OBJECT_H__

class CBaseOBject
{
public:
	virtual void Action() = 0;																		// 키다운 체크 후 이동,로직처리까지 하는 함수
	virtual void Draw(int SpriteIndex, BYTE* bypDest, int iDestWidth, int iDestHeight, int iDestPitch) = 0;			// 실제로 화면에 그리는 함수
};


#endif // !__BASE_OBJECT_H__

