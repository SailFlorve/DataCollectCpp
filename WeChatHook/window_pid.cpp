#include <string>
#include <Windows.h>
#include <iostream>
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

int main(int argc, char* argv[])
{
	GetHwndByPid(0);
}
