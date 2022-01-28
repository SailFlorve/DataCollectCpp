#pragma once
#include "pch.h"
#pragma comment(lib, "Version.lib")

typedef void (*GetHandleCallback)(const string&, const string&);

void startGetDbHandleHook(const string& version, DWORD dllAddress, GetHandleCallback callback);
void stopGetDbHandleHook();
int startCopyDb();