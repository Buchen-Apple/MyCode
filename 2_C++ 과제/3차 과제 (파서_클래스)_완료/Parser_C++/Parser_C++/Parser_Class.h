#ifndef __PARSER_CLASS_H__
#define __PARSER_CLASS_H__

namespace Library_Jingyu
{
	class Parser
	{
	private:
		char* m_cBuffer;		// 파일을 읽어올 버퍼
		char* m_cAreaBuffer;	// m_cBuffer 중, 현재 필요한 영역만 빼낸 버퍼. {와 }를 기준으로.
		char m_cWord[256];	// 매칭되는 단어를 저장할 버퍼
		int m_ilen;			// GetValue 함수들에서 사용하는 문자열 길이.	
		char m_cSkipWord;		// SkipNoneCommand함수에서 현재 문자가 스킵해야하는 문자인지 구분하기 위한 변수

	public:
		// 파일 로드. 
		// 파일오픈 실패 : throw 1 / 파일 데이터 읽어오기실패 : throw 2
		void LoadFile(const char* FileName) throw (int);

		// 구역 체크
		bool AreaCheck(const char*);

		// GetNextWord에서 단어 스킵 여부를 체크한다.
		// true면 스킵 / false면 스킵이 아니다.
		bool SkipNoneCommand(const char);

		// 버퍼에서, 다음 단어 찾기
		bool GetNextWord();

		// 파서_원하는 값 찾아오기 (int형)
		bool GetValue_Int(const char* Name, int* value);

		// 파서_원하는 값 찾아오기 (문자열)
		bool GetValue_String(const char* Name, char* value);

		// 소멸자. m_cBuffer딜리트
		~Parser();
	};
}

#endif // !__PARSER_CLASS_H__

