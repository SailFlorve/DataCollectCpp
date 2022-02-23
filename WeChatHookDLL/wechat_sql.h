#pragma once
#include "pch.h"

typedef void (*GetHandleCallback)(const string&, const string&);

void startGetDbHandleHook(const string& version, DWORD dllAddress, GetHandleCallback callback);
void stopGetDbHandleHook();
int startCopyDb();