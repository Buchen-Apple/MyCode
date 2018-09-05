#include "pch.h"
#include "Parser_Class.h"
#include <iostream>

using namespace std;

// 파일 로드
void Parser::LoadFile(const TCHAR* FileName)
{
	FILE* fp;			// 파일 스트림
	size_t iFileCheck;	// 파일 오픈 여부 체크, 파일의 사이즈 저장. 총 2개의 옹도

	iFileCheck = _tfopen_s(&fp, FileName, _T("rt, ccs=UTF-16LE"));
	if (iFileCheck != 0)
		throw 1;

	fseek(fp, 0, SEEK_END);
	iFileCheck = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	m_cBuffer = new TCHAR[iFileCheck];

	iFileCheck = fread_s(m_cBuffer, iFileCheck, iFileCheck, 1, fp);
	if (iFileCheck != 0)
		throw 2;

	fclose(fp);	// 다 쓴 파일 스트림 닫음
}

// 구역 지정
//
// Parameter : 구역 이름(TCHAR)
// return : 구역 지정 실패 시 false	
bool Parser::AreaCheck(const TCHAR* AreaName)
{
	int i = 0;
	TCHAR cTempWord[256];
	int iStartPos;
	int iEndPos;
	m_ilen = 0;
	bool bWhileBreakCheck = false;

	while (!bWhileBreakCheck)
	{
		cTempWord[0] = '\0';
		i = 0;
		// : 문자를 찾는다.
		while (SkipNoneCommand(m_cBuffer[m_ilen]))
		{
			// len의 값이 cBuffer의 사이즈보다 커지면,  더 이상 : 문자를 못찾은 것.
			if (m_ilen > _tcslen(m_cBuffer))
				return false;

			m_ilen++;
		}

		// 찾은 문자가 : 라면 아래 로직 실행
		if (m_cSkipWord == ':')
		{
			m_ilen++;
			// : 의 다음부터, 완성되는 문자를 찾는다. 즉, 구역 이름을 찾는다.
			while (1)
			{
				// len의 값이 cBuffer의 사이즈보다 커지면, 배열을 벗어난거니 false 리턴.
				if (m_ilen > _tcslen(m_cBuffer))
					return false;

				// 순서대로 콤마, 마침표, 따옴표, 스페이스, 백스페이스, 탭, 라인피드, 캐리지 리턴
				// 이 중 하나라도 해당되면, 문자를 하나 완성한 것. 즉, { 바로 앞의(왼쪽) 위치를 찾은 것.
				else if (m_cBuffer[m_ilen] == ',' || m_cBuffer[m_ilen] == '.' || m_cBuffer[m_ilen] == '"' || m_cBuffer[m_ilen] == 0x20 || m_cBuffer[m_ilen] == 0x08 || m_cBuffer[m_ilen] == 0x09 || m_cBuffer[m_ilen] == 0x0a || m_cBuffer[m_ilen] == 0x0d)
				{
					m_ilen++;
					// 찾은 구역 이름이 내가 찾는 구역이름이 맞다면, 
					if (! _tcscmp(cTempWord, AreaName))
					{
						// { 의 위치를 잡는다.  구역 이름 뒤에 스페이스 등이 있을 수 있기때문에 안전하게 한번 더 체크해주는 로직이다.
						while (SkipNoneCommand(m_cBuffer[m_ilen]))
						{
							// len의 값이 cBuffer의 사이즈보다 커지면, 해당 파일에는 {가 없다는 것
							if (m_ilen > _tcslen(m_cBuffer))
								return false;

							m_ilen++;
						}
						
						// 찾은 문자가 { 라면 {의 한칸 오른쪽으로 이동. 그리고 while 문 종료를 true로 만듦
						if (m_cSkipWord == '{')
						{
							bWhileBreakCheck = true;
							m_ilen++;
						}						
					}	

					break;
				}

				// 값을 넣는다.
				// 그리고 다음 값을 넣기 위해 현재 배열의 위치를 1칸 이동
				cTempWord[i++] = m_cBuffer[m_ilen++];
				cTempWord[i] = '\0';
			}
		}
		else
			m_ilen++;
	}	

	// { 를 찾았으면 이제, }의 위치를 잡아야 한다.
	// { 의 위치를 스타트, 엔드 포지션에 넣는다.
	iStartPos = m_ilen;
	iEndPos = iStartPos;

	while (1)
	{
		// 구역의 마지막 }를 찾는다. }가 나올 때 까지 while문을 반복한다.
		while (SkipNoneCommand(m_cBuffer[m_ilen]))
		{
			// len의 값이 cBuffer의 사이즈보다 커지면, 읽어온 파일을 벗어난 것. false 리턴.
			// 즉, 파일의 끝까지 봤는데 }가 없다는 뜻.
			if (m_ilen > _tcslen(m_cBuffer))
			//if (m_ilen > strlen(m_cBuffer))
				return false;

			m_ilen++;

		}

		// }를 찾았다면, 즉, 구역의 마지막에 도착했다면
		if (m_cBuffer[m_ilen] == '}')
		{
			// 마지막 위치를 지정한다.
			iEndPos = m_ilen;

			// 찾은 iStartPos와 iEndPos를 참조해서 m_cBuffer의 값을 m_cAreaBuffer에 저장한다.
			m_cAreaBuffer = new TCHAR[iEndPos];

			memcpy(m_cAreaBuffer, m_cBuffer + iStartPos, (iEndPos - iStartPos) * sizeof(TCHAR));
			break;
		}
		m_ilen++;

	}

	return true;
}

// GetNextWord에서 단어 스킵 여부를 체크한다.
// true면 스킵 / false면 스킵이 아니다.
bool Parser::SkipNoneCommand(TCHAR TempSkipWord)
{
	m_cSkipWord = TempSkipWord;

	// 순서대로 콤마, 따옴표, 스페이스, 백스페이스, 탭, 라인피드, 캐리지 리턴, / 문자(주석체크용)
	if (m_cSkipWord == ',' || m_cSkipWord == '"' || m_cSkipWord == 0x20 || m_cSkipWord == 0x08 || m_cSkipWord == 0x09 || m_cSkipWord == 0x0a || m_cSkipWord == 0x0d || m_cSkipWord == '/')
		return true;

	return false;
}

// 버퍼에서, 다음 단어 찾기
bool Parser::GetNextWord()
{
	int i = 0;
	TCHAR cTempWord[256];

	// 단어 시작 위치를 찾는다. true면 계속 스킵된다.
	while (SkipNoneCommand(m_cAreaBuffer[m_ilen]))
	{
		// len의 값이 cBuffer의 사이즈보다 커지면, 배열을 벗어난거니 false 리턴.
		if (m_ilen > _tcslen(m_cAreaBuffer))
			return false;

		m_ilen++;
	}

	// 시작위치를 찾았다. 이제 끝 위치를 찾아야한다.
	while (1)
	{
		// len의 값이 cBuffer의 사이즈보다 커지면, 배열을 벗어난거니 false 리턴.
		if (m_ilen > _tcslen(m_cBuffer))
			return false;

		// 순서대로 콤마, 마침표, 따옴표, 스페이스, 백스페이스, 탭, 라인피드, 캐리지 리턴
		// 이 중 하나라도 해당되면, 끝 위치를 찾은거니 while문을 나간다.
		else if (m_cAreaBuffer[m_ilen] == ',' || m_cAreaBuffer[m_ilen] == '"' || m_cAreaBuffer[m_ilen] == 0x20 || m_cAreaBuffer[m_ilen] == 0x08 || m_cAreaBuffer[m_ilen] == 0x09 || m_cAreaBuffer[m_ilen] == 0x0a || m_cAreaBuffer[m_ilen] == 0x0d)
			break;

		// 값을 넣는다.
		// 그리고 다음 값을 넣기 위해 현재 배열의 위치를 1칸 이동
		cTempWord[i++] = m_cAreaBuffer[m_ilen++];

	}

	// 단어의 마지막에 널문자를 넣어 문자열로 완성.
	cTempWord[i] = '\0';
	m_ilen++;

	// 문자열 복사.
	memset(m_cWord, 0, 256);
	_tcscpy_s(m_cWord, sizeof(cTempWord), cTempWord);

	return true;
}

// 파서_원하는 값 찾아오기 (int형)
bool Parser::GetValue_Int(const TCHAR* Name, int* value)
{
	m_ilen = 0;

	// cBuffer의 문자열을 cWord에 옮긴다. 실행될 때 마다 다음 문자열을 받아온다.
	// 받아오는데 성공하면 true, 받아오는데 실패하면 false이다. 받아오는데 실패하는 경우는, 마지막까지 모두 검사를 한 것이다.
	while (GetNextWord())
	{
		// 찾은 문자열이 내가 원하는 문자열인지 확인. 내가 찾는 문자열이 아니라면 다시 위 while문으로 문자 확인.
		if (_tcscmp(m_cWord, Name) == 0)
		{
			// 내가 찾는 문자열이라면, 다음 문자로 = 를 찾는다.
			// if (GetNextWord(cWord, &len))가 true라면 일단 다음 문자열을 찾은 것이다.
			if (GetNextWord())
			{
				// 문자열을 찾았으니 이 문자열이 진짜 = 인지 확인하자.
				if (_tcscmp(m_cWord, _T("=")) == 0)
				{
					// = 문자열이라면, 다음 문자열을 찾아온다. 이번에 찾는것은 정말 Value이다.
					if (GetNextWord())
					{
						// 여기까지 왔으면, Value도 찾은 것이니 이제 값을 넣자.
						*value = _ttoi(m_cWord);
						//*value = atoi(m_cWord);
						return true;
					}

				}

			}

		}

	}

	// 여기까지 온건, cBuffer의 모든 데이터를 찾았는데도 내가 원하는 데이터가 없는 것이니, false 반환.
	return false;
}

// 파서_원하는 값 찾아오기 (문자열)
bool Parser::GetValue_String(const TCHAR* Name, TCHAR* value)
{
	m_ilen = 0;

	// cBuffer의 문자열을 cWord에 옮긴다. 실행될 때 마다 다음 문자열을 받아온다.
	// 받아오는데 성공하면 true, 받아오는데 실패하면 false이다. 받아오는데 실패하는 경우는, 마지막까지 모두 검사를 한 것이다.
	while (GetNextWord())
	{
		// 찾은 문자열이 내가 원하는 문자열인지 확인. 내가 찾는 문자열이 아니라면 다시 위 while문으로 문자 확인.
		if (_tcscmp(m_cWord, Name) == 0)
		{
			// 내가 찾는 문자열이라면, 다음 문자로 = 를 찾는다.
			// if (GetNextWord(cWord, &len))가 true라면 일단 다음 문자열을 찾은 것이다.
			if (GetNextWord())
			{
				// 문자열을 찾았으니 이 문자열이 진짜 = 인지 확인하자.
				if (_tcscmp(m_cWord, _T("=")) == 0)
				{
					// = 문자열이라면, 다음 문자열을 찾아온다. 이번에 찾는것은 정말 Value이다.
					if (GetNextWord())
					{
						// 여기까지 왔으면, Value도 찾은 것이니 이제 값을 넣자.						
						_tcscpy_s(value, sizeof(m_cWord), m_cWord);
						return true;
					}

				}

			}

		}

	}

	// 여기까지 온건, cBuffer의 모든 데이터를 찾았는데도 내가 원하는 데이터가 없는 것이니, false 반환.
	return false;
}

Parser::~Parser()
{
	delete m_cBuffer;
	delete m_cAreaBuffer;
}