// ReSharper disable CppClangTidyBugproneNarrowingConversions
#include "pch.h"
#include "common_util.h"

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

	char buf_msg[1024] = { 0 };
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
	outputLog({ log }, inConsole);
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
