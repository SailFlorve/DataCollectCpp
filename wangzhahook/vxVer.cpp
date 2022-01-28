#include "pch.h"
#include "vxVer.h"

void repairVer(wchar_t * verStr)
{
	DWORD  verAddress = getWechatWinAddress() + WxVerAddressOff;

	int iVer = 0;
	wstring str(verStr);

	int pos1 = str.find(L".", 0);
	wstring str1 = str.substr(0, pos1);
	int int1 = wcstol(str1.c_str(), NULL, 10);

	int pos2 = str.find(L".", pos1 + 1);
	wstring str2 = str.substr(pos1 + 1, str.size());
	int int2 = wcstol(str2.c_str(), NULL, 10);

	int pos3 = str.find(L".", pos2 + 1);
	wstring str3 = str.substr(pos2 + 1, str.size());
	int int3 = wcstol(str3.c_str(), NULL, 10);

	int pos4 = str.find(L".", pos3 + 1);
	wstring str4 = str.substr(pos3 + 1, str.size());
	int int4 = wcstol(str4.c_str(), NULL, 10);
	iVer = 0x60000000;
	iVer += int1 << 24;
	iVer += int2 << 16;
	iVer += int3 << 8;
	iVer += int4;
	
	*(DWORD*)verAddress = iVer;
	
}
