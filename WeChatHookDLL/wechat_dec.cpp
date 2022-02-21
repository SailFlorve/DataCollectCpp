#include "pch.h"
#include "wechat_dec.h"

#include "common_util.h"
#include <utility>
#include "wechat_sql.h"
#include "wechat_bak.h"
#include "wechat_profile.h"

using namespace std;

DWORD dllAddress;
string version;

void getHandleCallback(const string& path, const string& id);
void getProfileCallback(const string& profile);
string getUserPath();

LPCWSTR pipeName = L"\\\\.\\pipe\\WECHAT_TEMP";

unordered_set<string> supportVersionSet = {"3.4.0.38", "3.4.5.27"};

string profileStr;

void __stdcall startWechatDecrypt()
{
	dllAddress = getDllAddress(L"WeChatWin.dll");
	if (dllAddress == 0)
	{
		MessageBoxA(nullptr, "未找到模块", "错误", MB_OK);
		return;
	}

	version = getFileVersion(reinterpret_cast<HMODULE>(dllAddress));
	outputLog(version);

	if (supportVersionSet.find(version) == supportVersionSet.end())
	{
		MessageBoxA(nullptr, "此版本未适配", "错误", MB_OK);
		sendPipeMessage(pipeName, {"ERROR"});
		return;
	}

	startGetDbHandleHook(version, dllAddress, getHandleCallback);
	startGetProfileHook(version, dllAddress, getProfileCallback);
}

void getHandleCallbackAsync(string path, string id)
{
	// 这里已经登上去了，不需要获取头像，否则获取到的不是头像
	stopGetProfileHook();

	startCopyDb();
	// 等一会再解密数据库， 否则可能获取不到？
	Sleep(1000);
	string backupKey = getBackupKey(dllAddress, version);
	if (backupKey.empty())
	{
		MessageBoxA(nullptr, "激活失败，无法获取备份数据库密钥，请尝试重新登录。", "激活", MB_OK);
		sendPipeMessage(pipeName, {"ERROR"});
		return;
	}
	MessageBoxA(nullptr, "激活成功", "激活", MB_OK);
	sendPipeMessage(pipeName, {"SUCCESS", std::move(path), std::move(id), backupKey, profileStr});
	stopGetDbHandleHook();
}

// 此方法为同步执行
void getHandleCallback(const string& path, const string& id)
{
	thread t(getHandleCallbackAsync, path, id);
	t.detach();
}

void getProfileCallback(const string& profile)
{
	outputLog({"Get Profile: ", profile});
	profileStr = profile;
}

string getUserPath()
{
	char homePath[MAX_PATH] = {0};
	if (!SHGetSpecialFolderPathA(nullptr, homePath, CSIDL_PERSONAL, false))
	{
		return "";
	}
	return homePath;
}
