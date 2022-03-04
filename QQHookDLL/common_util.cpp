// ReSharper disable CppClangTidyBugproneNarrowingConversions
#include "pch.h"
#include "common_util.h"

#pragma comment(lib, "Version.lib")

string getFileVersion(HMODULE hmodule)
{
	WCHAR versionFilePath[MAX_PATH];
	if (GetModuleFileName(hmodule, versionFilePath, MAX_PATH) == 0)
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
				int marjorVersion = pVsInfo->dwFileVersionMS >> 16 & 0x0000FFFF;
				//4
				int minorVersion = pVsInfo->dwFileVersionMS & 0x0000FFFF;
				//0
				int buildNum = pVsInfo->dwFileVersionLS >> 16 & 0x0000FFFF;
				//38
				int revisionNum = pVsInfo->dwFileVersionLS & 0x0000FFFF;

				//把版本变成字符串
				strstream verSs;
				verSs << marjorVersion << "." << minorVersion << "." << buildNum << "." << revisionNum;
				verSs >> versionStr;
			}
		}
		delete[] pBuf;
	}

	return versionStr;
}

void sendPipeMessage(LPCWSTR pipeName, const vector<string>& messages)
{
	HANDLE handle = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE,
	                           FILE_SHARE_WRITE | FILE_SHARE_READ,
	                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (handle == INVALID_HANDLE_VALUE)
	{
		char errMsg[100] = {0};
		sprintf_s(errMsg, "Failed to open the appointed named pipe!Error code: %lu", GetLastError());
		outputLog(errMsg);
		return;
	}

	string message;
	for (const auto& msg : messages)
	{
		message += msg;
		message += "\n";
	}

	char buf_msg[4096] = {0};
	strcpy_s(buf_msg, message.c_str());

	outputLog({"Send", buf_msg});
	DWORD nums_rcv;
	BOOL result = WriteFile(handle, buf_msg, strlen(buf_msg), &nums_rcv, nullptr);
	if (result)
	{
		outputLog("Write success");
	}
	else
	{
		outputLog("Write Failed");
	}
	CloseHandle(handle);
}

DWORD getDllAddress(const wchar_t* lpModuleName, int maxRetry)
{
	DWORD dllAddress = 0;
	for (int i = 0; i < maxRetry; ++i)
	{
		dllAddress = reinterpret_cast<DWORD>(GetModuleHandle(lpModuleName));
		if (dllAddress == 0)
		{
			Sleep(100);
		}
		else
		{
			break;
		}
	}
	return dllAddress;
}

void outputLog(const string& log, bool inConsole)
{
	outputLog({log}, inConsole);
}

void outputLog(initializer_list<string> logs, bool inConsole)
{
	string logStr;
	for (const string& log : logs)
	{
		logStr += log;
		logStr += " ";
	}
	OutputDebugStringA(logStr.c_str());
	if (inConsole)
	{
		cout << logStr << endl;
	}
}

string getKeyStrHex(int len, char* key)
{
	char change[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	string result;
	for (int i = 0; i < len; i++)
	{
		result += "0x";
		unsigned char high_4bit = (key[i] & 0xf0) >> 4;
		unsigned char low_4bit = key[i] & 0x0f;
		result += change[high_4bit];
		result += change[low_4bit];
		if (i == len - 1)
			break;
		result += ",";
	}
	return result;
}

bool isProcessRunAsAdmin()
{
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	BOOL b = AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup);
	if (b)
	{
		CheckTokenMembership(nullptr, AdministratorsGroup, &b);
		FreeSid(AdministratorsGroup);
	}
	return b == TRUE;
}

wchar_t* getDocumentsPath()
{
	static wchar_t homePath[MAX_PATH] = { 0 };
	if (!SHGetSpecialFolderPath(nullptr, homePath, CSIDL_PERSONAL, false))
	{
		return nullptr;
	}
	return homePath;
}