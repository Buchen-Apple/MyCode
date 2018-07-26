#include <iostream>

using namespace std;

class System
{
private:
	int _Data;
	static System _System;
	System() :_Data(0) {}
	~System()
	{
		cout << "¼Ò¸êÀÚ È£Ãâ" << endl;
	}

public:
	static System * GetInstance()
	{
		return &_System;
	}	

	int GetData() { return _Data; }
	void SetData(int iData) { _Data = iData; }
};
System System::_System;

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