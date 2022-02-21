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
		MessageBoxA(nullptr, "δ�ҵ�ģ��", "����", MB_OK);
		return;
	}

	version = getFileVersion(reinterpret_cast<HMODULE>(dllAddress));
	outputLog(version);

	if (supportVersionSet.find(version) == supportVersionSet.end())
	{
		MessageBoxA(nullptr, "�˰汾δ����", "����", MB_OK);
		sendPipeMessage(pipeName, {"ERROR"});
		return;
	}

	startGetDbHandleHook(version, dllAddress, getHandleCallback);
	startGetProfileHook(version, dllAddress, getProfileCallback);
}

void getHandleCallbackAsync(string path, string id)
{
	// �����Ѿ�����ȥ�ˣ�����Ҫ��ȡͷ�񣬷����ȡ���Ĳ���ͷ��
	stopGetProfileHook();

	startCopyDb();
	// ��һ���ٽ������ݿ⣬ ������ܻ�ȡ������
	Sleep(1000);
	string backupKey = getBackupKey(dllAddress, version);
	if (backupKey.empty())
	{
		MessageBoxA(nullptr, "����ʧ�ܣ��޷���ȡ�������ݿ���Կ���볢�����µ�¼��", "����", MB_OK);
		sendPipeMessage(pipeName, {"ERROR"});
		return;
	}
	MessageBoxA(nullptr, "����ɹ�", "����", MB_OK);
	sendPipeMessage(pipeName, {"SUCCESS", std::move(path), std::move(id), backupKey, profileStr});
	stopGetDbHandleHook();
}

// �˷���Ϊͬ��ִ��
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
