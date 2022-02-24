// ReSharper disable CppClangTidyBugproneNarrowingConversions
#pragma once
#include "pch.h"

string getFileVersion(HMODULE hmodule);

void sendPipeMessage(LPCWSTR pipeName, const vector<string>& messages);

DWORD getDllAddress(const wchar_t* lpModuleName, int maxRetry = 10);

void outputLog(const string& log, bool inConsole = false);

void outputLog(initializer_list<string> logs, bool inConsole = false);

string getKeyStrHex(int len, char* key);

bool isProcessRunAsAdmin();
