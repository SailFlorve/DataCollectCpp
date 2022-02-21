// dllmain.cpp : 定义 DLL 应用程序的入口点。
// ReSharper disable CppClangTidyCppcoreguidelinesNarrowingConversions
// ReSharper disable CppClangTidyBugproneNarrowingConversions
#include "pch.h"
#include "common_util.h"

using namespace std;

void __stdcall start();

const wstring pipeName = L"QQ_TEMP";
unordered_set<string> supportVersionSet = {"9.5.5.28104"};

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		MessageBox(nullptr, L"请登录成功后，在菜单中手动选择“聊天记录备份与恢复”。", L"采集工具", MB_OK);
		CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(start), hModule, NULL, nullptr);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default: ;
	}
	return TRUE;
}


void __stdcall start()
{
	DWORD dllAddress = getDllAddress(L"KernelUtil.h");

	string version = getFileVersion(reinterpret_cast<HMODULE>(dllAddress));

	if (supportVersionSet.find(version) == supportVersionSet.end())
	{
		MessageBoxA(nullptr, "此版本未适配", "错误", MB_OK);
		sendPipeMessage(pipeName, {"ERROR"});
		return;
	}

	//TODO
	// startGetDbKeyHook();
	// startGetProfileHook();
}
