#include <string>
#include <Windows.h>
#include <iostream>
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

int main(int argc, char* argv[])
{
	GetHwndByPid(0);
}
