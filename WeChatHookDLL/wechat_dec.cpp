#include "pch.h"
#include "wechat_dec.h"

#include <utility>
#include "wechat_sql.h"
#include "wechat_bak.h"
#include "wechat_profile.h"

using namespace std;

DWORD dllAddress;
string version;

void getHandleCallback(const string& path, const string& id);
void getProfileCallback(const string& profile);
string getWeChatVersion();
DWORD getWechatWinAddress();
void sendMessage(const vector<string>& messages);
string getUserPath();

unordered_set<string> supportVersionSet = {"3.4.0.38", "3.4.5.27"};

string profileStr;

void __stdcall startWechatDecrypt()
{
	while (TRUE)
	{
		dllAddress = reinterpret_cast<DWORD>(GetModuleHandle(L"WeChatWin.dll"));
		if (dllAddress == 0)
		{
			Sleep(100);
		}
		else
		{
			OutputDebugStringA("Find Base Address Success");
			break;
		}
	}

	version = getWeChatVersion();
	OutputDebugStringA(version.c_str());

	if (supportVersionSet.find(version) == supportVersionSet.end())
	{
		MessageBoxA(nullptr, "此版本未适配", "错误", MB_OK);
		sendMessage({"ERROR"});
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
		sendMessage({"ERROR"});
		return;
	}
	MessageBoxA(nullptr, "激活成功", "激活", MB_OK);
	sendMessage({"SUCCESS", std::move(path), std::move(id), backupKey, profileStr});
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
	OutputDebugStringA(("Get Profile: " + profile).c_str());
	profileStr = profile;
}

DWORD getWechatWinAddress()
{
	return reinterpret_cast<DWORD>(GetModuleHandle(L"WeChatWin.dll"));
}

void sendMessage(const vector<string>& messages)
{
	HANDLE handle = CreateFile(L"\\\\.\\pipe\\WECHAT_TEMP", GENERIC_READ | GENERIC_WRITE,
	                                 FILE_SHARE_WRITE | FILE_SHARE_READ,
	                                 nullptr, CREATE_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
	{
		cerr << "Failed to open the appointed named pipe!Error code:" << GetLastError() << endl;
		return;
	}

	string message;
	for (const auto& msg : messages)
	{
		message += msg;
		message += "\n";
	}

	char buf_msg[1024] = {0};
	strcpy_s(buf_msg, message.c_str());

	DWORD nums_rcv;
	BOOL result = WriteFile(handle, buf_msg, strlen(buf_msg), &nums_rcv, nullptr);
	if (result)
	{
		cout << "Write success" << endl;
	}
	else
	{
		cout << "Write Failed" << endl;
	}
	CloseHandle(handle);
}

string getWeChatVersion()
{
	WCHAR versionFilePath[MAX_PATH];
	if (GetModuleFileName(reinterpret_cast<HMODULE>(dllAddress), versionFilePath, MAX_PATH) == 0)
	{
		return "";
	}

	string versionStr;
	VS_FIXEDFILEINFO* pVsInfo;
	unsigned int iFileInfoSize = sizeof(VS_FIXEDFILEINFO);
	int iVerInfoSize = GetFileVersionInfoSize(versionFilePath, nullptr);
	if (iVerInfoSize != 0)
	{
		char* pBuf = new char[iVerInfoSize];
		if (GetFileVersionInfo(versionFilePath, 0, iVerInfoSize, pBuf))
		{
			if (VerQueryValue(pBuf, TEXT("\\"), reinterpret_cast<void**>(&pVsInfo), &iFileInfoSize))
			{
				//主版本
				//3
				int marjorVersion = (pVsInfo->dwFileVersionMS >> 16) & 0x0000FFFF;
				//4
				int minorVersion = pVsInfo->dwFileVersionMS & 0x0000FFFF;
				//0
				int buildNum = (pVsInfo->dwFileVersionLS >> 16) & 0x0000FFFF;
				//38
				int revisionNum = pVsInfo->dwFileVersionLS & 0x0000FFFF;

				//把版本变成字符串
				strstream weChatVer;
				weChatVer << marjorVersion << "." << minorVersion << "." << buildNum << "." << revisionNum;
				weChatVer >> versionStr;
			}
		}
		delete[] pBuf;
	}

	return versionStr;
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
