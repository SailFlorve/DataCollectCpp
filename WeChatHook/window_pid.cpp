#include <string>
#include <Windows.h>
#include <iostream>

#include "sqlite3.h"
using namespace std;

HWND GetHwndByPid(DWORD dwProcessID)
{
	//����Z�򶥲��Ĵ��ھ��
	HWND hWnd = GetTopWindow(nullptr);

	while (hWnd)
	{
		DWORD pid = 0;
		//���ݴ��ھ����ȡ����ID
		DWORD dwTheardId = GetWindowThreadProcessId(hWnd, &pid);

		char text[100] = { 0 };

		GetWindowTextA(hWnd, text, 100);

		//����z���е�ǰһ�����һ�����ڵľ��
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
