#include <stdio.h>


// 1. 작은 원반 n-1개를 A에서 B로 이동
// 2. 큰 원반 1개를 A에서 C로 이동
// 3. 작은 원반 n-1개를 B에서 C로 이동

void hanoi(int num, char one, char two, char three)
{
	// 하노이 타워 탈출조건 : 이동할 원반이 1개면, 마지막 원반(가장 작은 원반. 1 원반)을 one에서 three로 이동하는 것이니 탈출
	if (num == 1)
	{
		printf("원반 1을 %c에서 %c로 이동\n", one, three);
	}

	// 이동할 원반이 1이 아니라면, 절차에 따라 원반 이동
	else
	{
		// num-1번 원반을 one에서 three를 거쳐 two로 이동시킨다. 
		hanoi(num - 1, one, three, two);
		printf("원반 %d을(를) %c에서 %c로 이동\n", num, one, three);

		// num-1번 원반을, two에서 one을 거쳐 three로 이동시킨다.
		hanoi(num - 1, two, one, three);
	}

}

int main()
{
	hanoi(2, 'A', 'B', 'C');
	return 0;
}