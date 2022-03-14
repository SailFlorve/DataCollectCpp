// dllmain.cpp : 定义 DLL 应用程序的入口点。
// ReSharper disable CppClangTidyConcurrencyMtUnsafe
#include "pch.h"

using namespace std;

void __stdcall start();
void startGetDbHook();
void getDbHookFunc();

struct WeComHookAddress
{
	DWORD hookAddr;
	DWORD reJumpAddr;
	DWORD oldCallAddr;

	DWORD sqlExecAddr;
};


WeComHookAddress currentAddr;
Chook chook;

DWORD lastDbPath;
DWORD lastDbHandle;

SqliteExec sqlLiteExec;
SqliteOpen sqliteOpen;

string weComUserDirPath;
string weComId;

unordered_map<string, WeComHookAddress> addrMap = {
	{"4.0.0.6024", {0x1CEEFD, 0x1CEEFD + 0x5, 0x1CE570, 0x177250}}
};

unordered_map<string, DbInfo> dbMap;

LPCWSTR pipeName = L"\\\\.\\pipe\\WECOM_TEMP";

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			if (!isProcessRunAsAdmin())
			{
				MessageBox(nullptr, L"没有管理员权限，请右键点击桌面上的“企业微信”图标，选择“属性”，然后打开“兼容性”选项卡，勾选“以管理员身份运行此程序”。",
				           L"采集工具",
				           MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_SYSTEMMODAL);
				system("taskkill /F /IM WXWork.exe");
				return TRUE;
			}

			MessageBox(nullptr, L"请登录并等待程序自动激活。", L"采集工具",
			           MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_SYSTEMMODAL);
			auto handle = CreateThread(nullptr, 0,
			                           reinterpret_cast<LPTHREAD_START_ROUTINE>(start), hModule,
			                           NULL, nullptr);
			if (handle != nullptr)
			{
				CloseHandle(handle);
			}
		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default: ;
	}
	return TRUE;
}

void startInternal()
{
	DWORD dllAddress = getDllAddress(L"WXWork.exe");
	outputLog({"Base address: ", to_string(dllAddress)});

	string version = getFileVersion(reinterpret_cast<HMODULE>(dllAddress));
	outputLog({"Version:", version});
	if (addrMap.find(version) == addrMap.end())
	{
		MessageBoxA(nullptr, "此版本未适配", "错误", MB_OK);
		system("taskkill /F /IM WXWork.exe");
		return;
	}

	currentAddr = addrMap[version];
	auto pHookAddr = reinterpret_cast<unsigned long*>(&currentAddr);
	for (unsigned int i = 0; i < sizeof(WeComHookAddress) / sizeof(DWORD); i++)
	{
		(*pHookAddr++) += dllAddress;
	}

	sqlLiteExec = reinterpret_cast<SqliteExec>(currentAddr.sqlExecAddr);
	sqliteOpen = reinterpret_cast<SqliteOpen>(0x19B920 + dllAddress);

	startGetDbHook();
}

void __stdcall start()
{
	thread t(startInternal);
	t.detach();
}

void startGetDbHook()
{
	chook.HookReady(currentAddr.hookAddr, getDbHookFunc);
	chook.StartHook();
}

void __stdcall onGetDb()
{
	auto dbPath = reinterpret_cast<char*>(lastDbPath - 1);
	auto dbPathStr = string(dbPath); // "D:\Documents\WXWork\1688850237225156\Data\avatar_store_v3.db"

	outputLog({"DbPath:", dbPathStr, "DbHandle:", to_string(lastDbHandle)});

	size_t lastSepPos = dbPathStr.rfind('\\');
	auto parentDirName = dbPathStr.substr(lastSepPos - 4, 4);
	auto dbNameStr = dbPathStr.substr(lastSepPos + 1);

	if (parentDirName == "Data")
	{
		outputLog({"Save Db", dbNameStr});
		auto pDb = reinterpret_cast<sqlite3*>(lastDbHandle);
		dbMap[dbNameStr] = {dbNameStr, pDb};
	}
	else if (parentDirName == "ping")
	{
		size_t wxWorkPos = dbPathStr.rfind("WXWork");
		weComUserDirPath = dbPathStr.substr(0, wxWorkPos + 6);
		weComId = dbPathStr.substr(wxWorkPos + 7, 16);

		chook.StopHook();

		int result = startDatabaseCopy(dbMap, weComUserDirPath, weComId, sqlLiteExec);

		if (result == 0)
		{
			MessageBoxA(nullptr, "激活成功", "激活", MB_OK);
			sendPipeMessage(pipeName, {"SUCCESS", weComUserDirPath, weComId, "null", "\\"});
		}
		else
		{
			string msg = "激活失败，数据库出错。错误码：" + to_string(result);
			MessageBoxA(nullptr, msg.data(), "激活", MB_OK);
		}
	}
}

__declspec(naked) void getDbHookFunc()
{
	__asm {
		pushad;
		mov lastDbPath, ebx
		mov lastDbHandle, ecx;
		call onGetDb;
		popad;
		call currentAddr.oldCallAddr;
		jmp currentAddr.reJumpAddr;
		}
}
