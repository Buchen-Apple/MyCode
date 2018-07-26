#include <iostream>

using namespace std;

class System
{
private:
	int _Data;

public:
	System() :_Data(0) {}
	~System()
	{
		cout << "¼Ò¸êÀÚ È£Ãâ" << endl;
	}

	int GetData() { return _Data; }
	void SetData(int iData) { _Data = iData; }
};

System* GetInstance()
{
	static System _Sys;
	return &_Sys;
}

int main()
{
	System *pSys = GetInstance();

	cout << pSys->GetData() << endl;
	return 0;
}