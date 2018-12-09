#ifndef __CONVERTOR_CAL_H__
#define __CONVERTOR_CAL_H__

#include "ListStack.h"

class Convertor_Cal
{

private:
	// 두 연산자의 우선순위 비교
	int WhoPrecOp(char op1, char op2);

	// 인자로 받은 연산자의 우선순위를 정수 형태로 반환
	int GetOpPrec(char op);

	// 인자로 받은 연산자를 이용해, 인자로 받은 2개의 연산자를 연산한다.
	// 연산 후 결과를 리턴한다. (int 형으로)
	char Calculation(char opE, char op1, char op2);


public:
	// 중위 표기법을 후위  표기법으로 변환하는 함수
	void ConvToRPNExp(char exp[]);

	// 인자로 받은 후위표기법을 계산해 결과를 반환하는 함수
	int EvalTPNExp(char exp[]);

};

#endif // !__CONVERTOR_CAL_H__
