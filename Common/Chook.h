#pragma once
#include <Windows.h>

#define  HOOK_LEN 5
class Chook
{
public:
	Chook();
	~Chook();
	DWORD m_hookAdd{};//要进行HOOK的函数地址
	LPVOID m_jmpAdd{};//myFUNC
	HANDLE m_hWHND{};// 进程句柄
	BYTE m_backCode[HOOK_LEN]= { 0 };//原始数据
	BYTE m_JmpCode[HOOK_LEN] = { 0 };//跳转数据
	void StartHook();
	void StopHook() const;
	void HookReady(DWORD hookAdd, LPVOID jmpAdd);
};

