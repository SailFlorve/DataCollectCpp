#include "PCH.h"
#include "Chook.h"


Chook::Chook()
{
}


Chook::~Chook()
{
	if (m_hWHND) { CloseHandle(m_hWHND); }
	
}


VOID Chook::StartHook()
{

	if (ReadProcessMemory(m_hWHND, (LPVOID)m_hookAdd, m_backCode, HOOK_LEN, NULL) == 0) {
		MessageBoxA(NULL, "hookµÿ÷∑µƒ ˝æ›∂¡»° ß∞‹", "∂¡»° ß∞‹", MB_OK);
		return;
	}
	
	if (WriteProcessMemory(m_hWHND, (LPVOID)m_hookAdd, m_JmpCode, HOOK_LEN, NULL) == 0) {
		MessageBoxA(NULL, "hook–¥»Î ß∞‹£¨∫Ø ˝ÃÊªª ß∞‹", "¥ÌŒÛ", MB_OK);
		return;
	}

}
void Chook::StopHook() {

	if (WriteProcessMemory(m_hWHND, (LPVOID)m_hookAdd, m_backCode, HOOK_LEN, NULL) == 0) {
		MessageBoxA(NULL, "hook–¥»Î ß∞‹£¨∫Ø ˝ÃÊªª ß∞‹", "¥ÌŒÛ", MB_OK);
		return;
	}
}

void Chook::HookReady(DWORD hookAdd, LPVOID jmpAdd)
{
		m_hWHND = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
		m_hookAdd = hookAdd;
		m_jmpAdd = jmpAdd;
		m_JmpCode[0] = 0xE9;
		*(DWORD *)&m_JmpCode[1] = (DWORD)m_jmpAdd - m_hookAdd - HOOK_LEN;	
}
