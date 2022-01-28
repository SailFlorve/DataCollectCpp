#include "pch.h"
#include "wechat_profile.h"

void getProfileHookFunc();
void overrideJnzHookFunc();

GetProfileCallback getProfileCallback = nullptr;

struct ProfileHookAddress {
	DWORD hookAddr;
	DWORD oldCallAddr;
	DWORD reJumpAddr;
	DWORD jnzAddr;
	DWORD jnzReJumpAddr;
};

unordered_map<string, ProfileHookAddress> hookAdddrMap =
{
	{"3.4.0.38", {0x1BA157, 0, 0x1BA157 + 0x5, 0x1BA128, 0x1BA128 + 6}},
	{"3.4.5.27", {0x1C3F87, 0, 0x1C3F87 + 0x5, 0x1C3F58, 0x1C3F58 + 6}}
};

Chook profileHook1;
Chook profileHook2;

ProfileHookAddress currentAddr;

DWORD profileAddr;

void startGetProfileHook(string version, DWORD dllAddress, GetProfileCallback callback)
{
	getProfileCallback = callback;

	currentAddr = hookAdddrMap[version];

	int* pHookAddr = (int*)&currentAddr;

	for (int i = 0; i < sizeof(ProfileHookAddress) / sizeof(DWORD); i++) {
		(*pHookAddr++) += dllAddress;
	}

	profileHook2.HookReady(currentAddr.jnzAddr, overrideJnzHookFunc);
	profileHook2.StartHook();

	profileHook1.HookReady(currentAddr.hookAddr, getProfileHookFunc);
	profileHook1.StartHook();

}

void stopGetProfileHook() {
	profileHook1.StopHook();
	profileHook2.StopHook();
}

void __stdcall getProfile()
{
	if (getProfileCallback != nullptr) {
		wchar_t* profilePath = (wchar_t*)profileAddr;
		char buf[MAX_PATH];
		sprintf_s(buf, "%ws", profilePath);
		getProfileCallback(string(buf));
		OutputDebugStringA("Hook1");
		OutputDebugStringA(buf);
	}
}

__declspec(naked) void getProfileHookFunc() 
{
	__asm {
		add esp, 0x4;
		mov eax, dword ptr ds : [eax] ;

		pushad;
		mov profileAddr, eax;
		call getProfile;
		popad;

		jmp currentAddr.reJumpAddr;
	}
}

__declspec(naked) void overrideJnzHookFunc()
{
	__asm {
		jmp currentAddr.jnzReJumpAddr;
	}
}
