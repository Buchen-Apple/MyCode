#pragma once
#ifndef __STACK_H__
#define __STACK_H__

typedef int Data;

#define STACK_LEN 330

typedef struct
{
	Data m_xpos[STACK_LEN];
	Data m_ypos[STACK_LEN];
	int topIndex;

}Stack;

void StackInit(Stack* pstack)
{
	pstack->topIndex =  - 1;
}

bool Push(Stack* pstack, Data xpos, Data ypos)
{
	// 만약, 현재 스택 +1스택이 max라면 false 반환.
	if (pstack->topIndex + 1 >= STACK_LEN)
		return false;

	// 스택 1 증가
	pstack->topIndex += 1;

	// x,y값 넣음.
	pstack->m_xpos[pstack->topIndex] = xpos;
	pstack->m_ypos[pstack->topIndex] = ypos;

	return true;
}

bool Pop(Stack* pstack, Data* xpos, Data* ypos)
{
	// 만약, 스택이 0보다 작은 상태라면(즉, -1이라면. -1은 초기상태) 더 이상 뺼 데이터가 없기 때문에 false반환
	if (pstack->topIndex < 0)
		return false;

	// x,y좌표를 직접 입력
	*xpos = pstack->m_xpos[pstack->topIndex];
	*ypos = pstack->m_ypos[pstack->topIndex];

	// 스택1 감소
	pstack->topIndex -= 1;

	return true;
}


#endif // !__STACK_H__

