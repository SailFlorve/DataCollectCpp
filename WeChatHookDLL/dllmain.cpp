// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "wechat_dec.h"

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		MessageBox(nullptr, L"需要进行扫码登录，点击确定开始登录。登录后会自动进行激活，请耐心等待。", L"采集工具", MB_OK);
		CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(startWechatDecrypt), hModule, NULL, nullptr);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default: ;
	}
	return TRUE;
}
