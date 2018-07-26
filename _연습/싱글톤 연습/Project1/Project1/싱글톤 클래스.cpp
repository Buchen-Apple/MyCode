#include <iostream>

using namespace std;

class System
{
private:
	int _Data;
	System() :_Data(0) {}

public:	
	static System * GetInstance()
	{
		static System _Sys;
		return &_Sys;
	}

	~System()
	{
		cout << "¼Ò¸êÀÚ È£Ãâ" << endl;
	}

	int GetData() { return _Data; }
	void SetData(int iData) { _Data = iData; }
};


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