#pragma once
#include <Windows.h>

#define  HOOK_LEN 5
class Chook
{
public:
	Chook();
	~Chook();
	DWORD m_hookAdd{};//Ҫ����HOOK�ĺ�����ַ
	LPVOID m_jmpAdd{};//myFUNC
	HANDLE m_hWHND{};// ���̾��
	BYTE m_backCode[HOOK_LEN]= { 0 };//ԭʼ����
	BYTE m_JmpCode[HOOK_LEN] = { 0 };//��ת����
	void StartHook();
	void StopHook() const;
	void HookReady(DWORD hookAdd, LPVOID jmpAdd);
};

