#pragma once

#include "pch.h"

typedef void (*GetProfileCallback)(const string&);
void startGetProfileHook(string version, DWORD dllAddress, GetProfileCallback callback);
void stopGetProfileHook();