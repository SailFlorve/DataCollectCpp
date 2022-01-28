#include "pch.h"
#include "Chook.h"


Chook::Chook()
= default;


Chook::~Chook()
{
	if (m_hWHND) { CloseHandle(m_hWHND); }
}


VOID Chook::StartHook()
{
	if (ReadProcessMemory(m_hWHND, (LPVOID)m_hookAdd, m_backCode, HOOK_LEN, nullptr) == 0)
	{
		MessageBoxA(nullptr, "hookµÿ÷∑µƒ ˝æ›∂¡»° ß∞‹", "∂¡»° ß∞‹", MB_OK);
		return;
	}
	if (WriteProcessMemory(m_hWHND, (LPVOID)m_hookAdd, m_JmpCode, HOOK_LEN, nullptr) == 0)
	{
		MessageBoxA(nullptr, "hook–¥»Î ß∞‹£¨∫Ø ˝ÃÊªª ß∞‹", "¥ÌŒÛ", MB_OK);
		return;
	}
}

void Chook::StopHook() const
{
	if (WriteProcessMemory(m_hWHND, (LPVOID)m_hookAdd, m_backCode, HOOK_LEN, nullptr) == 0)
	{
		MessageBoxA(nullptr, "hook–¥»Î ß∞‹£¨∫Ø ˝ÃÊªª ß∞‹", "¥ÌŒÛ", MB_OK);
		return;
	}
}

void Chook::HookReady(DWORD hookAdd, LPVOID jmpAdd)
{
	m_hWHND = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
	m_hookAdd = hookAdd;
	m_jmpAdd = jmpAdd;
	m_JmpCode[0] = 0xE9;
	*reinterpret_cast<DWORD*>(&m_JmpCode[1]) = reinterpret_cast<DWORD>(m_jmpAdd) - m_hookAdd - HOOK_LEN;
}
