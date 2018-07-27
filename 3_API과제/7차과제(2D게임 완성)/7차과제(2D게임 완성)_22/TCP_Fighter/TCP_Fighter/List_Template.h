#pragma once
#ifndef __CLIST_TEMPLATE_H__
#define __CLIST_TEMPLATE_H__

using namespace std;

template <typename T>
class CList
{
public:
	struct Node
	{
		T _data;
		Node* _Prev;
		Node* _Next;
	};

	class iterator
	{
	private:
		Node * _node;

	public:
		// iterator: 생성자
		iterator(Node *node = nullptr)
		{
			//인자로 들어온 Node 포인터를 저장
			_node = node;
		}

		// iterator: 복사 생성자
		iterator(const iterator& ref)
		{
			_node = ref._node;
		}

		// iterator: 비어있음 체크
		bool Empty(iterator itor)
		{
			if (itor._node == NULL)
				return true;

			return false;
		}

		// iterator: == 비교 연산자
		bool operator==(const iterator& ref)
		{
			if (_node == ref._node)
				return true;

			return false;
		}

		// iterator: != 비교 연산자
		bool operator!=(const iterator& ref)
		{
			if (_node->_data != ref._node->_data)
				return true;

			return false;
		}

		// iterator: 대입 연산자
		iterator& operator = (const iterator& ref)
		{
			_node = ref._node;
			return *this;
		}

		//iterator: 현재 노드를 다음 노드( >>> )로 이동. 후위 증가
		iterator& operator ++(int)
		{
			_node = _node->_Next;
			return *this;
		}

		// iterator: 현재 노드를 이전 노드( <<< )로 이동. 전위 증가
		iterator& operator ++()
		{
			_node = _node->_Prev;
			return *this;
		}

		// iterator: 데이터 리턴
		T& operator *()
		{
			//현재 노드의 데이터를 뽑음
			return _node->_data;
		}

		// iterator: 현재 노드 삭제. 삭제할 노드 기준, 좌 우 노드가 서로 연결됨.
		void NodeDelete()
		{
			_node->_Prev->_Next = _node->_Next;
			_node->_Next->_Prev = _node->_Prev;

		}

		// iterator: 현재 이터레이터의 노드와 Next의 위치를 서로 바꾼다.
		void ListSwap(iterator Next)
		{
			T temp = _node->_data;
			_node->_data = Next._node->_data;
			Next._node->_data = temp;
		}


		friend iterator CList<T>::erase(iterator&) throw(int);
	};

private:
	int _size = 0;
	Node* _head;	// 더미 노드를 가리킨다.
	Node* _tail;	// 더미 노드를 가리킨다.

public:
	// CList: 생성자
	CList()
	{
		_head = new Node;
		_tail = new Node;

		_head->_Next = _tail;
		_head->_Prev = NULL;

		_tail->_Prev = _head;
		_tail->_Next = NULL;
	}

	// CList: 복사 생성자
	CList(const CList& ref)
	{
		_head = ref._head;
		_tail = ref._tail;
		_size = ref._size;
	}

	// CList: 대입 연산자
	CList& operator = (const CList& ref)
	{
		_head = ref._head;
		_tail = ref._tail;
		_size = ref._size;
		return *this;
	}

	// CList: 첫번째 노드를 가리키는 이터레이터 리턴
	iterator begin() const
	{
		iterator itor;
		itor = _head->_Next;
		return itor;
	}

	// CList:  Tail 노드를 가리키는(데이터가 없는 진짜 끝 노드) 이터레이터를 리턴
	// 또는 끝으로 인지할 수 있는 이터레이터를 리턴
	iterator end() const
	{
		iterator itor;
		itor = _tail;
		return itor;
	}

	// CList: 새로운 노드 추가 (헤드)
	void push_front(T data)
	{
		Node* newNode = new Node;
		newNode->_data = data;

		if (_size == 0)
		{
			newNode->_Next = _tail;
			newNode->_Prev = _head;

			_head->_Next = newNode;
			_tail->_Prev = newNode;

			_size++;
			return;
		}

		_size++;
		newNode->_Next = _head->_Next;
		newNode->_Prev = _head;

		_head->_Next->_Prev = newNode;
		_head->_Next = newNode;
	}

	// CList: 새로운 노드 추가 (꼬리)
	void push_back(T data)
	{
		Node* newNode = new Node;
		newNode->_data = data;

		if (_size == 0)
		{
			newNode->_Next = _tail;
			newNode->_Prev = _head;

			_head->_Next = newNode;
			_tail->_Prev = newNode;

			_size++;
			return;
		}

		_size++;
		newNode->_Next = _tail;
		newNode->_Prev = _tail->_Prev;

		_tail->_Prev->_Next = newNode;
		_tail->_Prev = newNode;
	}

	// CList: 리스트 클리어
	void clear()
	{
		_head->_Next = _tail;	// 헤드 더미가 꼬리를 가리킴
		_tail->_Prev = _head;	// 꼬리 더미가 헤드를 가리킴. 즉, 서로 더미끼리 가리키게 해서 초기상태로 만듦.
		_size = 0;
	}

	// CList: 노드 갯수 리턴
	int size() const { return _size; };

	// CList: 현재 노드가 비었는지 체크
	bool is_empty(iterator itor)
	{
		bool bTemp = itor.Empty(itor);
		return bTemp;
	}

	// CList: 이터레이터의 그 노드를 지움.
	// 그리고 지운 노드의 다음 노드를 카리키는 이터레이터 리턴
	iterator erase(iterator& itor)
	{
		if (itor == ++(CList::begin()) || itor == CList::end())
			throw 1;

		iterator deleteiotr = itor;
		itor++;
		deleteiotr.NodeDelete();
		_size--;

		delete deleteiotr._node;
		return itor;
	}

	// CList: 소멸자
	~CList()
	{}
};

#endif // !__CLIST_TEMPLATE_H__

