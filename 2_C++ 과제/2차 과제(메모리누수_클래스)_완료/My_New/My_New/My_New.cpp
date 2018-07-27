
#include <cstring>
#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <tchar.h>

#include "My_New.h"
#undef new // "My_New.h"에서 쓰던 new 매크로 undef

// 에러 메시지 문자열 상수들
#define ARRAY_ERROR "ARRAY"
#define NOALLOC_ERROR "NOALLOC"
#define LEAK_ERROR "LEAK"

size_t AllocSize = 0;	// 총 알록 용량 저장
size_t AllocCount = 0;	// 총 알록 카운트
time_t nowTime = time(NULL);	// 프로그램 시작 시 시간 저장
tm Tdata;	// 시간을 저장할 구조체 변수
TCHAR FileName[300];	// 파일 이름을 저장할 변수
FILE* wStream;	// 파일 스트림
bool bCheck = false;	// 리스트 성공여부를 체크하는 bool 변수
MemoryCheck CTemp;	// 리스트에 넘겨서 각종 값을 받아오는 객체

// 리스트 선언
LinkedList list;

// 리스트 초기화여부 체크
void NewListInitCheck()
{
	if (AllocCount == 0)
	{
		list.ListInit(&list);
		localtime_s(&Tdata, &nowTime);

		// 현재, FileName가 TCHAR(wchat_s)형태이기 떄문에, 일반 파일 오픈으로 하려면 유니코드 문자를 멀티바이트로 바꿔야함. 그게 귀찮으니, _tfopen_s를 사용한다.
		// _tfopen_s는 문자열이 유니코드라면 _wfopne을 호출 / 문자열이 멀티바이트라면 fopen을 호출
		swprintf_s(FileName, _countof(FileName), TEXT("Alloc_%d%02d%02d_%02d%02d%02d.txt"), Tdata.tm_year + 1900, Tdata.tm_mon + 1, Tdata.tm_mday, Tdata.tm_hour, Tdata.tm_min, Tdata.tm_sec);
		_tfopen_s(&wStream, FileName, _T("wt"));

		// 아래 2개는 FileName를 char로 선언할 경우 사용 가능
		//strftime(FileName, 300, "Alloc_%Y%m%d_%H%M%S.txt", &data);
		//fopen_s(&wStream, FileNameCopy, "wt");		
	}
}

// operator new 전역함수. MemoryCheck와 frined
void* operator new (size_t size, const char* File, int Line)
{
	// 리스트 초기화여부 체크 함수 호출. 최초 동적할당이면 리스트 초기화
	NewListInitCheck();

	// 객체 동적할당.
	MemoryCheck* mem = new MemoryCheck;

	// 동적할당 받은 객체 세팅
	mem->size = size;
	strcpy_s(mem->_name, strlen(File) + 1, File);
	mem->_line = Line;
	mem->_address = malloc(size);
	mem->ArrayCheck = false;

	// 할당받은 동적할당 공간과 횟수 저장
	AllocSize += mem->size;
	AllocCount++;

	// 리스트에 넣는다.
	list.ListInsert(&list, mem);

	return mem->_address;
}

// operator new[] 전역함수. MemoryCheck와 frined
void* operator new[](size_t size, const char* File, int Line)
{
	// 리스트 초기화여부 체크 함수 호출. 최초 동적할당이면 리스트 초기화
	NewListInitCheck();

	MemoryCheck* mem = new MemoryCheck;

	mem->size = size;
	strcpy_s(mem->_name, strlen(File) + 1, File);
	mem->_line = Line;
	mem->_address = malloc(size);
	mem->ArrayCheck = true;

	// 할당받은 동적할당 공간과 횟수 저장
	AllocSize += mem->size;
	AllocCount++;

	// 리스트에 넣는다.
	list.ListInsert(&list, mem);

	return mem->_address;
}

// operator delete 전역함수. MemoryCheck와 frined
void operator delete (void* ptr)
{
	bCheck = list.ListSearch(&list, ptr, &CTemp);

	// 리스트에서 무언가를 가져왔다면.
	if (bCheck)
	{
		// 해당 공간을 리스트에서 제외
		bCheck = list.ListDelete(&list, ptr);

		// 해제됐던 공간이 일반 new로 할당된 공간이라면
		if (CTemp.ArrayCheck == false)
		{
			puts("일반 딜리트");
			return;
		}

		// 해당 공간이 배열 new로 할당된 공간일 경우 (ARRAY에러를 파일에 저장)
		else
		{
			fprintf_s(wStream, "%s    [0x%X]  [    %zd]  %s : %d\n", ARRAY_ERROR, CTemp._address, CTemp.size, CTemp._name, CTemp._line);
			puts("배열을 일반으로 해제 시도함.");
			return;
		}
	}

	// 리스트에 원하는 파일이 없다면 (NOALLOC에러를 파일에 저장)
	else
	{
		fprintf_s(wStream, "%s  [0x%X]\n", NOALLOC_ERROR, ptr);
	}

}

// operator delete[] 전역함수. MemoryCheck와 frined
void operator delete[](void* ptr)
{
	bCheck = list.ListSearch(&list, ptr, &CTemp);

	// 리스트에서 무언가를 가져왔다면.
	if (bCheck)
	{
		// 해당 공간을 리스트에서 제외
		bCheck = list.ListDelete(&list, ptr);

		// 해제됐던 공간이 배열 new로 할당된 공간이라면
		if (CTemp.ArrayCheck == true)
		{
			puts("배열 딜리트");
			return;
		}

		//해당 공간이 일반 new로 생성된 공간이라면 (ARRAY 에러를 파일로 보냄)
		else
		{
			fprintf_s(wStream, "%s    [0x%X]  [    %zd]  %s : %d\n", ARRAY_ERROR, CTemp._address, CTemp.size, CTemp._name, CTemp._line);
			puts("일반을 배열으로 해제 시도함.");
			return;
		}
	}

	// 리스트에 원하는 파일이 없다면 (NOALLOC에러를 파일에 저장)
	else
	{
		fprintf_s(wStream, "%s  [0x%X]\n", NOALLOC_ERROR, ptr);
	}

}

// 소멸자
MemoryCheck::~MemoryCheck()
{
	// 리스트에 남아있는 노드들을 하나씩 검색. 
	// 첫 노드 검색
	if (list.ListSearchFirst(&list, &CTemp))
	{
		// 노드가 있는 경우, 해당 노드의 메모리 릭 메시지를 파일에 저장
		fprintf_s(wStream, "%s     [0x%X]  [    %zd]  %s : %d\n", LEAK_ERROR, CTemp._address, CTemp.size, CTemp._name, CTemp._line);

		// 다음 노드 검색. 리스트의 끝에 도착할 때 까지 검색
		while (list.ListSearchSecond(&list, &CTemp))
			// 다음 노드가 있으면, 해당 노드의 메모리 릭 메시지를 파일에 저장
			fprintf_s(wStream, "%s     [0x%X]  [    %zd]  %s : %d\n", LEAK_ERROR, CTemp._address, CTemp.size, CTemp._name, CTemp._line);
	}

	// 총 알록 카운트 / 용량을 파일로 보냄
	fprintf_s(wStream, "\n총 Alloc 횟수 : %d\n총 Alloc 용량 : %d\n", AllocCount, AllocSize);

	// 할거 다 했으니 파일 스트림 닫음
	fclose(wStream);
}

// 원하는 주소(temp)의 리스트를 찾는다. ListDelete / ListSearch에서 사용
bool LinkedList::SearchFunc(LinkedList* list, void* temp)
{
	// temp(주소)를 체크할 노드를 헤드 위치로 이동
	list->_cur = list->_head->_next;

	// 헤드 위치부터 한칸씩 >>쪽으로 이동하면서 data._address 검사.
	while (1)
	{
		// 현재 노드가 꼬리더미라면, 가장 끝에 도착했는데도 원하는 주소를 못찾은거니 false 반환;
		if (list->_cur == list->_tail)
			return false;

		// 현재 노드의 주소가 temp와 같으면 원하는 주소를 찾은것이니 break;
		if(list->_cur->_data)
		if (list->_cur->_data->_address == temp)
			break;

		// 위 상황이 둘 다 아니면 >>한칸 이동
		list->_cur = list->_cur->_next;
	}

	return true;
}

// 리스트 초기화
void LinkedList::ListInit(LinkedList* list)
{
	// 헤드, 꼬리 더미 생성
	Node* headDumy = new Node;
	Node* tailDumy = new Node;

	// 헤드, 꼬리 더미가 서로를 가리킨다.
	headDumy->_prev = 0;
	headDumy->_next = tailDumy;

	tailDumy->_prev = headDumy;
	tailDumy->_next = 0;

	// 리스트의 헤드와 꼬리가 각각 더미를 가리킴
	list->_head = headDumy;
	list->_tail = tailDumy;
	list->_cur = list->_head;
}

// 리스트 추가 (꼬리에 추가)
void LinkedList::ListInsert(LinkedList* list, Data* data)
{
	// 새로운 노드 세팅
	Node* newNode = new Node;

	// 새로운 노드, _data에 data의 값을 넣는다.
	newNode->_data = data;

	// 새로운 노드, _data에 data의 값을 넣는다.

	// 해당 노드를 뒤에서부터 추가한다. 즉, 꼬리에서 추가. 
	// 첫 노드 추가일때랑 그 이후랑은 절차가 다르다.
	if (list->_tail->_prev == list->_head)	// 첫 노드라면
	{
		newNode->_prev = list->_head;
		newNode->_next = list->_tail;

		list->_head->_next = newNode;
		list->_tail->_prev = newNode;
	}

	else  // 첫 노드가 아니라면 
	{
		list->_tail->_prev->_next = newNode;
		newNode->_prev = list->_tail->_prev;

		newNode->_next = list->_tail;
		list->_tail->_prev = newNode;
	}

}

// 리스트 삭제. 원하는(temp) 주소의 노드를 리스트에서 제외
bool LinkedList::ListDelete(LinkedList* list, void* temp)
{
	// 원하는 주소를 찾는다. 원하는 주소(temp)가 없으면 false / 있으면 true 반환
	bool bCheck = list->SearchFunc(list, temp);

	// 주소를 찾았다면
	if (bCheck == true)
	{
		// 현재 노드의 이전과 현재 노드의 다음이 서로를 연결한다.
		list->_cur->_prev->_next = list->_cur->_next;
		list->_cur->_next->_prev = list->_cur->_prev;

		return true;
	}

	return false;
}

// 리스트 검색. 원하는(temp) 주소가 리스트에 있으면 해당 리스트의 _data를 data에 넣음. 원하는 주소의 리스트가 없으면 false 반환
bool LinkedList::ListSearch(LinkedList* list, void* temp, Data* data)
{
	// 원하는 주소를 찾는다. 원하는 주소(temp)가 없으면 false / 있으면 true 반환
	bool bCheck = list->SearchFunc(list, temp);

	// 주소를 찾았다면
	if (bCheck == true)
	{
		memcpy(data, list->_cur->_data, sizeof(Data));
		return true;
	}

	return false;
}

// 리스트 순회(First). 첫 노드가 있으면 true / 없으면 flase 반환
bool LinkedList::ListSearchFirst(LinkedList* list, Data* data)
{
	list->_cur = list->_head->_next;

	if (list->_cur == list->_tail)
		return false;

	memcpy(data, list->_cur->_data, sizeof(Data));

	return true;
}

// 리스트 순회(Next). 다음 노드가 있으면 true / 없으면 false 반환
bool LinkedList::ListSearchSecond(LinkedList* list, Data* data)
{
	list->_cur = list->_cur->_next;

	if (list->_cur == list->_tail)
		return false;

	memcpy(data, list->_cur->_data, sizeof(Data));

	return true;
}