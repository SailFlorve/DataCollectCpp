// dllmain.cpp : 定义 DLL 应用程序的入口点。
// ReSharper disable CppClangTidyCppcoreguidelinesNarrowingConversions
// ReSharper disable CppClangTidyBugproneNarrowingConversions
// ReSharper disable CppClangTidyConcurrencyMtUnsafe
#include "pch.h"

#include <thread>

#include "common_util.h"
#include "Chook.h"

using namespace std;

void __stdcall start();
void startGetDbKeyHook();
void bakDbKeyHookFunc();

struct QQHookAddress
{
	DWORD bakDbKeyHookAddr;
	DWORD innerOpenAddr;
	DWORD bakDbKeyHookReJmpAddr;
};


const wchar_t* pipeName = L"\\\\.\\pipe\\QQ_TEMP";

unordered_set<string> supportVersionSet = {"9.5.5.28104"};
unordered_map<string, QQHookAddress> addrMap = {
	{
		"9.5.5.28104", {0x32C42, 0x33121, 0x32C42 + 0x5}
	}
};

unordered_map<string, string> dbKeyMap;

Chook chook;
QQHookAddress currentAddr;

DWORD lastDbPath;
DWORD lastDbKey;

int bakDbKeyNum = 0;
string userId;
string userPath;
string profilePath = "/";

extern "C" {

}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

		if (!isProcessRunAsAdmin())
		{
			MessageBox(nullptr, L"没有管理员权限，请右键点击QQ，选择“属性”，打开“兼容性”选项卡，勾选“以管理员身份运行此程序”。",
			           L"采集工具",
			           MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_SYSTEMMODAL);
			system("taskkill /F /IM QQ.exe");
			return TRUE;
		}

		MessageBox(nullptr, L"请登录成功后，在菜单中手动选择“聊天记录备份与恢复”。", L"采集工具", MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_SYSTEMMODAL);
		CreateThread(nullptr, 0,
		             reinterpret_cast<LPTHREAD_START_ROUTINE>(start), hModule,
		             NULL, nullptr);
	// if (handle != nullptr)
	// {
	// 	CloseHandle(handle);
	// }
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
	DWORD dllAddress = getDllAddress(L"KernelUtil.dll");

	string version = getFileVersion(reinterpret_cast<HMODULE>(dllAddress));

	if (supportVersionSet.find(version) == supportVersionSet.end())
	{
		MessageBoxA(nullptr, "此版本未适配", "错误", MB_OK);
		sendPipeMessage(pipeName, {"ERROR"});
		return;
	}

	currentAddr = addrMap[version];

	auto pHookAddr = reinterpret_cast<unsigned long*>(&currentAddr);
	for (unsigned int i = 0; i < sizeof(QQHookAddress) / sizeof(DWORD); i++)
	{
		(*pHookAddr++) += dllAddress;
	}

	startGetDbKeyHook();
	// startGetProfileHook();
}

void startGetDbKeyHook()
{
	chook.HookReady(currentAddr.bakDbKeyHookAddr, bakDbKeyHookFunc);
	chook.StartHook();
}

void sendMsg()
{
	bool isSuccess = userPath.length() != 0 && bakDbKeyNum > 0 && !dbKeyMap.empty();
	if (!isSuccess)
	{
		outputLog("Send Error");
		sendPipeMessage(pipeName, {"ERROR"});
		return;
	}

	string dbKeyStr;
	for (const auto& dbKeyPair : dbKeyMap)
	{
		dbKeyStr += dbKeyPair.first;
		dbKeyStr += "&";
		dbKeyStr += dbKeyPair.second;
		dbKeyStr += "#";
	}

	outputLog({"Send ", userPath, userId, dbKeyStr}, true);
	sendPipeMessage(pipeName, {"SUCCESS", userPath, userId, dbKeyStr, profilePath});
}

void __stdcall handleGetDbKey()
{
	auto path = string(reinterpret_cast<char*>(lastDbPath));
	auto bakKey = reinterpret_cast<char*>(lastDbKey);

	string keyStr = getKeyStrHex(16, bakKey);

	outputLog({path, keyStr}, true);

	dbKeyMap[path] = keyStr;

	string::size_type pos = path.find("MsgBackup");

	if (pos != string::npos)
	{
		bakDbKeyNum += 1;
		userPath = path.substr(0, pos - 1);
		size_t lastSepIndex = userPath.rfind('\\');
		userId = userPath.substr(lastSepIndex + 1, userPath.length());
	}

	if (bakDbKeyNum == 4)
	{
		outputLog({"Hook Stop. UserPath:", userPath}, true);
		MessageBoxA(nullptr, "激活成功", "激活", MB_OK);
		sendMsg();
		chook.StopHook();
	}
}

__declspec(naked) void bakDbKeyHookFunc()
{
	__asm
		{

		pushad
		mov lastDbPath, eax
		mov lastDbKey, esi
		call handleGetDbKey
		popad;

		push dword ptr ss : [ebp + 0x1c]
		push edi
		push esi

		jmp currentAddr.bakDbKeyHookReJmpAddr

		}
}
