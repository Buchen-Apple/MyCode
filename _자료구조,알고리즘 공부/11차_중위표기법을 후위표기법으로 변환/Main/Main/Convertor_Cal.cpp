#include "pch.h"
#include "Convertor_Cal.h"
#include "windows.h"

// 중위 표기법을 후위  표기법으로 변환하는 함수
void Convertor_Cal::ConvToRPNExp(char exp[])
{
	// 변화된 후위연산자를 보관할 배열
	int expLen = (int)strlen(exp);
	char* pResult = new char[expLen + 1];
	ZeroMemory(pResult, expLen+1);

	// 중간에 쟁반 역활 스택
	listStack stack;
	stack.Init();

	// exp안에 있는 문자를 하나하나 보면서 작업 진행
	int TempIndex = 0;
	char cTok;
	int i = 0;

	while (i < expLen)
	{
		// 문자 하나 골라내기.
		cTok = exp[i];

		// 이번 문자가 숫자인지 확인
		// 숫자라면 true 리턴
		if (isdigit(cTok))
		{
			// 숫자라면 바로 배열에 넣고 끝
			pResult[TempIndex] = cTok;
			TempIndex++;
		}
		
		// 숫자가 아니면 연산자.
		// 연산자일 경우 로직
		else
		{
			switch (cTok)
			{
				// 연산자가 ( 라면 소괄호 시작.
			case '(':
			{
				// 스택에 소괄호 연산자를 넣어둔다.
				stack.Push(cTok);
			}
			break;

			// 연산자가 )라면 소괄호 끝.
			case ')':
			{
				char PopData;

				// 스택에 있는 모든 연산자를 꺼내서 결과 배열에 넣는다.
				while (1)
				{
					stack.Pop(&PopData);

					// 다시 시작 연산자를 만날 때 까지
					if (PopData == '(')
						break;

					pResult[TempIndex] = PopData;
					TempIndex++;
				}
			}
			break;

			// 연산자가 +, -, *, /라면
			case '+':
			case '-':
			case '*':
			case '/':
			{
				// 스택에 연산자가 있을 경우, 스택 안의 연산자와 이번 연산자를 비교.		
				char op1;

				if (stack.Peek(&op1) == true)
				{
					// 스택 안의 연산자의 우선순위가 더 높거나 같을 경우, 스택에서 기존 연산자를 뺀 후, 결과 배열에 넣는다.
					while (stack.IsEmpty() == false && WhoPrecOp(op1, cTok) >= 0)
					{
						stack.Pop(&op1);
						pResult[TempIndex] = op1;
						TempIndex++;

						stack.Peek(&op1);
					}
				}

				// 비교 완료 후, 현재 연산자를 스택에 넣는다.
				stack.Push(cTok);
				break;
			}

			}			
		}

		i++;
	}

	// 스택에 남아있는 연산자 모두를 결과배열로 이동
	while (stack.Pop(&cTok))
	{
		pResult[TempIndex] = cTok;
		TempIndex++;
	}

	// 변환된 후위연산자 수식을, 매개변수에 넣는다.
	strcpy_s(exp, expLen+1, pResult);

	// 기존 결과배열은 메모리 해제
	delete[] pResult;
}

// 두 연산자의 우선순위 비교
int Convertor_Cal::WhoPrecOp(char op1, char op2)
{
	// op1과 op2의 우선순위 알아오기
	int op1Prec = GetOpPrec(op1);
	int op2Prec = GetOpPrec(op2);

	// op1이 더 우선순위가 높다면 1
	if (op1Prec > op2Prec)
		return 1;

	// op2가 더 우선순위가 높다면 -1
	else if (op1Prec < op2Prec)
		return -1;

	// 우선순위가 같다면 0
	else
		return 0;
}

// 인자로 받은 연산자의 우선순위를 정수 형태로 반환
int Convertor_Cal::GetOpPrec(char op)
{	
	switch (op)
	{
		// *와 /는 우선순위 5
	case '*':
	case '/':
		return 5;

		// +와 -는 우선순위 3
	case '+':
	case '-':
		return 3;

		// (는 1. 가장 낮음
	case '(':
		return 1;
	}

	// 이 외에 없는 연산자는 -1
	return -1;
}

// 인자로 받은 후위표기법를 계산해 결과를 반환하는 함수
int Convertor_Cal::EvalTPNExp(char exp[])
{
	listStack stack;
	stack.Init();

	// exp에서 하나씩 빼서 계산한다.
	int expLen = (int)strlen(exp);
	char Tok;
	
	int i = 0;
	while (i < expLen)
	{
		// 하나 빼오기
		Tok = exp[i];

		// 빼 온 문자가 숫자인지 체크
		if (isdigit(Tok))
		{
			// 숫자라면 스택에 넣고 끝.
			// '-'을 뺌으로써, 아스키문자가 아니라 정말 정수형 값을 저장한다.
			stack.Push(Tok - '0');
		}

		// 숫자가 아니라 연산자라면
		else
		{
			// 스택에서 피연산자 2개 빼온다.
			// 먼저 나오는 피연산자가 op2가 된다.
			// op1이랑 op2는 순서상 나란히 배치된다. (ex. op1 / op2)
			// 그 이유는, 나누기, 빼기 등은 순서가 중요하기 때문이다.
			char op1, op2;

			stack.Pop(&op2);
			stack.Pop(&op1);

			// 계산 후 결과값을 스택에 Push.
			// 다시 피연산자가 된다
			stack.Push(Calculation(Tok, op1, op2));
		}

		i++;
	}

	// 스택에는 결과만 남아있게 된다.
	char Result;
	stack.Pop(&Result);

	return Result;
}

// 인자로 받은 연산자를 이용해, 인자로 받은 2개의 연산자를 연산한다.
// 연산 후 결과를 리턴한다. (char 형으로)
char Convertor_Cal::Calculation(char opE, char op1, char op2)
{
	switch (opE)
	{
	case '+':
		return op1 + op2;

	case '-':
		return op1 - op2;

	case '*':
		return op1 * op2;

	case '/':
		return op1 / op2;
	}	
}