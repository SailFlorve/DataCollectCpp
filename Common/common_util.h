// ReSharper disable CppClangTidyBugproneNarrowingConversions
#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include <Windows.h>
#include <strstream>
#include <iostream>
#include <ShlObj.h>


using namespace std;

string getFileVersion(HMODULE hmodule);

void sendPipeMessage(LPCWSTR pipeName, const vector<string>& messages);

DWORD getDllAddress(const wchar_t* lpModuleName, int maxRetry = 10);

void outputLog(const string& log, bool inConsole = false);

void outputLog(initializer_list<string> logs, bool inConsole = false);

string getKeyStrHex(int len, char* key);

bool isProcessRunAsAdmin();

wchar_t* getDocumentsPath();
