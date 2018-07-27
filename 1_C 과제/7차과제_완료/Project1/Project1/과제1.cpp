// 해시테이블
#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include "LinkedLIst.h"

#define ListMax 10

enum 
{
	INSERT = 1, DELETE, ALL_SHOW, SEARCH 
};

int HashFunt(int);

int main()
{

	int iInput;
	int j;
	int iInputKey;
	int iIndex;
	char cInputData[100];
	bool NextCheck = true;
	LinkedList List[ListMax];
	Node* ShowNode = (Node*)malloc(sizeof(Node));

	// 리스트 초기화
	for (int i = 0; i < ListMax; ++i)
	{
		Init(&List[i]);
	}

	while (1)
	{
		// 1. 메뉴 선택
		system("cls");
		puts("##MENU##");
		puts("1. 데이터 추가");
		puts("2. 데이터 삭제");
		puts("3. 전체보기");
		puts("4. 찾기");
		fputs(": ", stdout);
		scanf("%d", &iInput);

		// 2. 메뉴에 따라 로직 진행
		switch (iInput)
		{
		case INSERT:
			// 데이터 추가 로직 시작
			// 1. 키값과 데이터를 입력받는다.
			fputs("KEY (숫자) : ", stdout);
			scanf("%d", &iInputKey);
			fputs("Data : ", stdout);
			scanf("%s", cInputData);

			// 2. 키 값을 해시함수를 이용해 인덱스로 치환함.
			iIndex = HashFunt(iInputKey);

			// 3. 해당 인덱스에, 링크드 리스트로 값 추가
			ListInsert_Head(&List[iIndex], iInputKey, cInputData);

			// 4. 추가 완료 스트링
			puts("인서트 완료!");
			system("pause");
			break;

		case DELETE:
			// 데이터 삭제하기 로직
			// 1. 삭제할 데이터의 Key값을 입력받는다.
			fputs("FIne Key(숫자) : ", stdout);
			scanf("%d", &iInputKey);

			// 2. 입력받은 키를 해시함수를 이용해 인덱스로 치환
			iIndex = HashFunt(iInputKey);

			// 3. 치환된 인덱스로 해당 인덱스에 연결되어 있는 노드를 순회하며 Key를 비교한다. 그리고 그 key를 갖고 있는 노드를 리스트에서 빼고 free한다.
			for (j = 0; j < ListMax; ++j)
			{				
				NextCheck = ListDelete(&List[j], iInputKey);
				// NextCheck가 true면 원하는 값을 찾아서 삭제를 완료했다는 것. 그럼 반복문 종료.
				if (NextCheck == true)
					break;
			}

			// 4. 여기까지 왔을 때, NextCheck가 true라면 원하는것을 찾아서 나온것. 그러니 삭제 완료! 스트링을 표시한다.
			if (NextCheck == true)
				puts("삭제 완료!");
			// 5. 여기까지 왔을 때, NextCheck가 false라면 원하는것을 못찾은거니, 찾는 값이 없습니다 스트링 표시.
			else
				puts("찾는 키가 없습니다");

			system("pause");			
			break;

		case ALL_SHOW:
			// 데이터 보여주기 로직
			// 1. 리스트 순서대로 전부 랜더링.
			for (int i = 0; i < ListMax; ++i)
			{
				printf("[%02d] : ", i);
				// 2. 각 리스트는 모두, 가장 앞에 더미가 있기 때문에 1칸 >>이쪽으로 움직인 후 체크해야 함.
				// 그래서 j의 기본 값을 1로 한다.
				j = 1;
				while (1)
				{
					// ListPeek을 이용해, ShowNode에다가 보여줄 노드의 값을 저장한다.
					// j가 인덱스이다.
					NextCheck = ListShow(&List[i], ShowNode, j);
					// 만약 NextCheck가 false면 꼬리에 도착한 것이니 다음 순서의 리스트를 보여준다.
					if (NextCheck == false)
						break;

					printf(">> Key: %d, ", ShowNode->Key);
					printf("Data: %s ", ShowNode->Data);
					j++;
				}
				fputs("\n", stdout);
			}

			puts("전체보기 완료!");
			system("pause");
			break;

		case SEARCH:
			// 데이터 찾기 로직
			// 1. key를 입력받는다.
			fputs("FIne Key(숫자) : ", stdout);
			scanf("%d", &iInputKey);

			// 2. 입력받은 키를 해시함수를 이용해 인덱스로 치환
			iIndex = HashFunt(iInputKey);

			// 3. 치환된 인덱스로 해당 인덱스에 연결되있는 리스트를 순서대로 검색하며, key값을 비교한다.
			NextCheck = ListPeek(&List[iIndex], cInputData, iInputKey);

			// 4. 값을 찾았으면 찾은 값을 보여준다.
			if (NextCheck == true)
			{
				printf("Fine Data : %s\n", cInputData);
				puts("찾기 성공!");
			}

			// 5. 못찾으면 못찾았다고 보여준다.
			else
			{
				puts("없는 데이터입니다.");			
			}

			system("pause");
			break;

		}
	}	

	return 0;
}

int HashFunt(int iInputKey)
{
	iInputKey = ((iInputKey * 3 + 130) % ListMax);
	return iInputKey;
}