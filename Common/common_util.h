// ReSharper disable CppClangTidyBugproneNarrowingConversions
#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include <Windows.h>
#include <strstream>
#include <iostream>
#include <ShlObj.h>
#include <io.h>


std::string getFileVersion(HMODULE hModule);

void sendPipeMessage(LPCWSTR pipeName, const std::vector<std::string>& messages);

DWORD getDllAddress(const wchar_t* lpModuleName, int maxRetry = 10);

void outputLog(const std::string& log, bool inConsole = false);

void outputLog(std::initializer_list<std::string> logs, bool inConsole = true);

std::string getKeyStrHex(int len, char* key);

bool isProcessRunAsAdmin();

wchar_t* getDocumentsPath();

std::string generateCopyDbName(const std::string& dbName);

int getTableType(const std::string& tableType);

bool isDirectoryExists(const std::string& path);