#include <iostream>
#include <stdlib.h>

using namespace std;

template <typename T>
class TemplateSingleton
{
private:
	static T* m_pInstance;

protected:
	TemplateSingleton() {}
	virtual ~TemplateSingleton()
	{
		cout << "¼Ò¸êÀÚ È£Ãâ TemplateSingleton" << endl;
	}

public:
	static T* GetInstance()
	{
		if (m_pInstance == NULL)
		{
			m_pInstance = new T;
			atexit(DestroyInstance);
		}

		return m_pInstance;
	}

	static void DestroyInstance()
	{
		if (m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	}
};
template <typename T>
T* TemplateSingleton<T>::m_pInstance = 0;


class System	:public TemplateSingleton<System>
{
public:
	int _Data;
	System() :_Data(0) {}
	~System() { cout << "¼Ò¸êÀÚÈ£Ãâ Syste" << endl; }

public:

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