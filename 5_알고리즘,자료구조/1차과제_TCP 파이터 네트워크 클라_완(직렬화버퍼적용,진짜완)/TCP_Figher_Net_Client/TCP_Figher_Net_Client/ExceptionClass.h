#ifndef __EXCEPTION_CLASS_H__
#define __EXCEPTION_CLASS_H__


// Recv()패킷 처리 중, 예외 발생 시 던지는 예외클래스이다.
class CException
{
private:
	wchar_t ExceptionText[100];

public:
	// 생성자
	CException(const wchar_t* str);

	// 예외 텍스트 포인터 반환
	char* GetExceptionText();
	
};

#endif // !__EXCEPTION_CLASS_H__
