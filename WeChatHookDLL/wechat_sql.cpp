#include "pch.h"
#include "wechat_sql.h"
#include "sqlite3.h"
#include "common_util.h"
#include "sql_copy.h"

using namespace std;

typedef struct SqlHookAddress
{
	DWORD dBHandleHookAddr;
	DWORD dBHandleHookOldCallAddr;
	DWORD dbHandleOldCallReJmpAddr;
	DWORD sqlExecAddr;
	DWORD openDatabaseAddr;
} SqlHookAddress;

void dbHandleHookFunc();

typedef int (__cdecl* OpenDataBase)(const char*, DWORD, unsigned int, const char*);


GetHandleCallback getHandleCallback = nullptr;

// HOOK位置78FDE1EF-78280000=D5E1EF 原始CALL 78FDD780-78280000=D5D780 回跳CALL = HOOK位置+5 exec函数指针 78FB0A00 - 78280000 = D3 0A00
unordered_map<string, SqlHookAddress> addrMap =
{
	{"3.4.0.38", SqlHookAddress{0xD5E1EF, 0xD5D780, 0xD5E1EF + 0x5, 0xD30A00, 0xD5DF10}},
	{"3.4.5.27", SqlHookAddress{0xDA6F3F, 0xDA64D0, 0xDA6F3F + 0x5, 0xD79760, 0xDA6C60}}
};

SqlHookAddress currentAddr;

SqliteExec sqliteExec;
OpenDataBase openDb;

unordered_map<string, DbInfo> handleMap;

Chook sqlHook;
DWORD lastDbPath;

string userPath;
string weChatId;

void startGetDbHandleHook(const string& version, DWORD dllAddress, GetHandleCallback callback)
{
	getHandleCallback = callback;

	currentAddr = addrMap[version];

	auto pHookAddr = reinterpret_cast<unsigned long*>(&currentAddr);
	for (unsigned int i = 0; i < sizeof(SqlHookAddress) / sizeof(DWORD); i++)
	{
		(*pHookAddr++) += dllAddress;
	}

	addrMap[version] = currentAddr;

	sqliteExec = reinterpret_cast<SqliteExec>(currentAddr.sqlExecAddr);
	openDb = reinterpret_cast<OpenDataBase>(currentAddr.openDatabaseAddr);

	sqlHook.HookReady(currentAddr.dBHandleHookAddr, dbHandleHookFunc);
	sqlHook.StartHook();
}

void stopGetDbHandleHook()
{
	sqlHook.StopHook();
}

int startCopyDb()
{
	return startDatabaseCopy(handleMap, userPath, weChatId, sqliteExec);
}

void __stdcall getDb(DWORD pEsi)
{
	char str[0x200] = {0};
	auto path = reinterpret_cast<char*>(lastDbPath);

	//  0            13          25
	// "D:\Documents\WeChat Files\wxid_8rmcj0zs9itk22\Msg\Multi\MSG0.db"
	sprintf_s(str, "DbName:%d Path:%s|", pEsi, path);
	outputLog(str);

	string pathStr = string(path);

	if (userPath.length() == 0)
	{
		size_t posTmp = pathStr.rfind("WeChat Files"); // rfind 字符串右侧开始匹配str，并返回在字符串中的位置
		userPath = pathStr.substr(0, posTmp + 13); // .../WeChat Files/
		weChatId = pathStr.substr(posTmp + 13, 19); // wxid_...
	}

	string::size_type pos = pathStr.find_last_of('\\');

	string dbName(pathStr.substr(pos + 1));
	string::size_type pos1 = dbName.find("MSG");
	string::size_type pos2 = dbName.find("MediaMSG");
	string::size_type pos3 = dbName.find("FTS");
	string::size_type pos4 = dbName.find("Applet");

	if ((pos1 != string::npos || pos2 != string::npos) && pos3 == string::npos)
	{
		sprintf_s(str, "Save %s", dbName.c_str());
		outputLog(str);
		DbInfo info = {dbName, reinterpret_cast<sqlite3*>(pEsi)};
		handleMap[dbName] = info;
	}

	// Applet是最后一个出现的数据库，open它之后即回调获取句柄结束
	if (pos4 != string::npos)
	{
		getHandleCallback(userPath, weChatId);
	}
}

__declspec(naked) void dbHandleHookFunc()
{
	__asm {
		pushad;
		mov lastDbPath, edx;
		popad;
		call currentAddr.dBHandleHookOldCallAddr;
		pushad;
		push esi;
		call getDb;
		popad;
		jmp currentAddr.dbHandleOldCallReJmpAddr;
		}
}
