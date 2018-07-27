#include "Scene_Class.h"

int main()
{	
	cs_Initial();	// 화면의 커서 안보이게 처리 및 핸들 선언
	system("mode con cols=83 lines=33");
	while (1)
	{
		hSceneHandle.run();
	}

	return 0;
}

