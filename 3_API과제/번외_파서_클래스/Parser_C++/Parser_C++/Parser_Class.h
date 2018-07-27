#pragma once
#ifndef __PARSER_CLASS_H__
#define __PARSER_CLASS_H__

#include <tchar.h>

class Parser
{
private:
	TCHAR* m_cBuffer;		// 파일을 읽어올 버퍼
	TCHAR* m_cAreaBuffer;	// 구역 체크로 지정된 구역. m_cBuffer 중, 현재 필요한 구역만 빼낸 버퍼.
	TCHAR m_cWord[256];		// 매칭되는 단어를 저장할 버퍼
	int m_ilen;				// GetValue 함수들에서 사용하는 문자열 길이.	
	TCHAR m_cSkipWord;		// SkipNoneCommand함수에서 현재 문자가 스킵해야하는 문자인지 구분하기 위한 변수

public:
	// 파일 로드. 
	// 파일오픈 실패 : throw 1 / 파일 데이터 읽어오기실패 : throw 2
	void LoadFile(TCHAR* FileName) throw (int);

	// 구역 체크
	bool AreaCheck(TCHAR* AreaName);

	// GetNextWord에서 단어 스킵 여부를 체크한다.
	// true면 스킵 / false면 스킵이 아니다.
	bool SkipNoneCommand(TCHAR TempSkipWord);

	// 버퍼에서, 다음 단어 찾기
	bool GetNextWord();

	// 파서_원하는 값 찾아오기 (int형)
	bool GetValue_Int(TCHAR* Name, int* value);

	// 파서_원하는 값 찾아오기 (문자열)
	bool GetValue_String(TCHAR* Name, TCHAR* value);

	// 소멸자. m_cBuffer, m_cAreaBuffer딜리트
	~Parser();
};

#endif // !__PARSER_CLASS_H__

