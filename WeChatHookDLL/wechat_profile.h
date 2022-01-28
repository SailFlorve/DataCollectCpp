#pragma once

#include "pch.h"

typedef void (*GetProfileCallback)(const string&);
void startGetProfileHook(const string& version, DWORD dllAddress, GetProfileCallback callback);
void stopGetProfileHook();