#include <string>
#include <Windows.h>
#include <iostream>

#include "sqlite3.h"
using namespace std;

HWND GetHwndByPid(DWORD dwProcessID)
{
	//返回Z序顶部的窗口句柄
	HWND hWnd = GetTopWindow(nullptr);

	while (hWnd)
	{
		DWORD pid = 0;
		//根据窗口句柄获取进程ID
		DWORD dwTheardId = GetWindowThreadProcessId(hWnd, &pid);

		char text[100] = { 0 };

		GetWindowTextA(hWnd, text, 100);

		//返回z序中的前一个或后一个窗口的句柄
		hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT);

		
		if (pid == 18676)
		{
			cout << strlen(text)<<" "<<text << " " << pid << endl;
		}
	}
	return hWnd;
}

int aaamain(int argc, char* argv[])
{
	const char* s = "123123123";
	cout << s << endl;
	sqlite3* sq;
	sqlite3_open("D:/Projects/VSProjects/DataCollectCpp/WeChatHook/test.db", &sq);
	system("pause");
	return 1;
}
