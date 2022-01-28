#include "pch.h"
#include "vxSql.h"
using namespace std;

DWORD getWechatWinAddress()
{
	return (DWORD)GetModuleHandle(L"WeChatWin.dll");
}

Chook sqlHook;

DWORD hookAddr = getWechatWinAddress() + WxGetSqlDBHandleHookAddressOff;
DWORD hookOldCallAddr = getWechatWinAddress() + WxGetSqlDBHandleHookOldCallAddressOff;
DWORD hookReJmpAddr = hookAddr + 0x5;

DWORD dbPath;
string sqlStr;


SqliteExec  sqlexec =(SqliteExec) (getWechatWinAddress() + 0xD30A00);

int idx = 0;

void getSqlDBHandleHookStart()
{
	sqlHook.HookReady(hookAddr, getSqlDbHookFunc);
	sqlHook.StartHook();
}

void getSqlDBHandleHookStop()
{
	sqlHook.StopHook();
}

__declspec(naked) void getSqlDbHookFunc()
{

	__asm {
		pushad;
		mov dbPath, edx;
		popad;
		call hookOldCallAddr;
		pushad;
		push esi;
		call getDb;
		popad;
		jmp hookReJmpAddr;
	}
}
void __stdcall  getDb1(DWORD pEsi)
{
	char str[0x100] = { 0 };
	char* Path = (char*)dbPath;

	sprintf_s(str, "vx:db:%d.Path:%s\r\b", pEsi, Path);

	OutputDebugStringA(str);
}

void runSqlExec(DWORD db,char * sqltext)
{
	sqlexec(db, sqltext, (DWORD*)mycallback, 0, 0);
}


int __cdecl mycallback(void* NotUsed, int argc, char** argv, char** azColName) {
	int i;
	
	for (i = 0; i < argc; i++) {
		char str[0x300] = { 0 };
		sprintf_s(str,"vx:%s = %s\r\n", azColName[i], argv[i] ? argv[i] : "NULL");
		OutputDebugStringA(str);
		//sqlStr += str;
	}
	//OutputDebugStringA(sqlStr.c_str());
	return 0;
}