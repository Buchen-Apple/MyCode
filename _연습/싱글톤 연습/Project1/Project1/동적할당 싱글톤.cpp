#include <iostream>
#include <stdlib.h>

using namespace std;

class System
{
private:
	int _Data;
	static System* _pSystem;
	System() :_Data(0) {}
	~System()
	{
		cout << "¼Ò¸êÀÚ È£Ãâ" << endl;
	}

public:
	static System * GetInstance()
	{
		if (_pSystem == NULL)
		{
			_pSystem = new System;
			atexit(Destroy);
		}

		return _pSystem;
	}
	static void Destroy()
	{
		delete _pSystem;
	}

	int GetData() { return _Data; }
	void SetData(int iData) { _Data = iData; }
};
System* System::_pSystem = NULL;

int main()
{
	System *pSys = System::GetInstance();
	cout << pSys->GetData() << endl;
	pSys->SetData(10);
	cout << pSys->GetData() << endl;

	System *pSys2 = System::GetInstance();
	cout << pSys2->GetData() << endl;
	return 0;
}