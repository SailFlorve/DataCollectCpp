// dllmain.cpp : 定义 DLL 应用程序的入口点。
// ReSharper disable CppClangTidyConcurrencyMtUnsafe
#include "pch.h"

using namespace std;

void __stdcall start();
void startGetDbHook();
void getDbHookFunc();
void queryMasterAndCreateNewTables();
void selectAndInsertNewTables();
int __cdecl sqliteMasterCallback(void* msg, int colNum, char** colValue, char** colName);
int __cdecl queryTableInfoCallback(void* msg, int colNum, char** colValue, char** colName);
int __cdecl selectCallback(void* msg, int colNum, char** colValue, char** colName);


typedef int (__cdecl* SqliteExec)(sqlite3*, const char*, DWORD*, void*, char**);

struct WeComHookAddress
{
	DWORD hookAddr;
	DWORD reJumpAddr;
	DWORD oldCallAddr;

	DWORD sqlExecAddr;
};

struct TableInfo
{
	string tableName;
	vector<string> columnNames;
	vector<string> columnTypes;
};

struct DbInfo
{
	string dbName;
	sqlite3* pDb = nullptr;
	sqlite3* pCopyDb = nullptr;
	unordered_map<string, TableInfo> tableList;
	string currentTableName;
};

WeComHookAddress currentAddr;
Chook chook;

DWORD lastDbPath;
DWORD lastDbHandle;

SqliteExec sqlExec;

string weComUserDirPath;
string weComId;

unordered_map<string, WeComHookAddress> addrMap = {
	{"4.0.0.6024", {0x1CEEFD, 0x1CEEFD + 0x5, 0x1CE570, 0x177250}}
};
unordered_map<string, DbInfo> dbInfoMap;

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			if (!isProcessRunAsAdmin())
			{
				MessageBox(nullptr, L"没有管理员权限，请右键点击桌面上的“企业微信”图标，选择“属性”，然后打开“兼容性”选项卡，勾选“以管理员身份运行此程序”。",
				           L"采集工具",
				           MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_SYSTEMMODAL);
				system("taskkill /F /IM WXWork.exe");
				return TRUE;
			}

			MessageBox(nullptr, L"请登录并等待程序自动激活。", L"采集工具",
			           MB_OK | MB_TOPMOST | MB_SETFOREGROUND | MB_SYSTEMMODAL);
			auto handle = CreateThread(nullptr, 0,
			                           reinterpret_cast<LPTHREAD_START_ROUTINE>(start), hModule,
			                           NULL, nullptr);
			if (handle != nullptr)
			{
				CloseHandle(handle);
			}
		}
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	default: ;
	}
	return TRUE;
}

void startInternal()
{
	DWORD dllAddress = getDllAddress(L"WXWork.exe");
	outputLog({"Base address: ", to_string(dllAddress)});

	string version = getFileVersion(reinterpret_cast<HMODULE>(dllAddress));
	outputLog({"Version:", version});
	if (addrMap.find(version) == addrMap.end())
	{
		MessageBoxA(nullptr, "此版本未适配", "错误", MB_OK);
		system("taskkill /F /IM WXWork.exe");
		return;
	}

	currentAddr = addrMap[version];
	auto pHookAddr = reinterpret_cast<unsigned long*>(&currentAddr);
	for (unsigned int i = 0; i < sizeof(WeComHookAddress) / sizeof(DWORD); i++)
	{
		(*pHookAddr++) += dllAddress;
	}

	sqlExec = reinterpret_cast<SqliteExec>(currentAddr.sqlExecAddr);

	startGetDbHook();
}

void __stdcall start()
{
	thread t(startInternal);
	t.detach();
}

void startGetDbHook()
{
	chook.HookReady(currentAddr.hookAddr, getDbHookFunc);
	chook.StartHook();
}

void __stdcall onGetDb()
{
	auto dbPath = reinterpret_cast<char*>(lastDbPath - 1);
	auto dbPathStr = string(dbPath); // "D:\Documents\WXWork\1688850237225156\Data\avatar_store_v3.db"

	outputLog({"DbPath:", dbPathStr, "DbHandle:", to_string(lastDbHandle)});

	size_t lastSepPos = dbPathStr.rfind('\\');
	auto parentDirName = dbPathStr.substr(lastSepPos - 4, 4);
	auto dbNameStr = dbPathStr.substr(lastSepPos + 1);

	if (parentDirName == "Data")
	{
		outputLog({"Save Db", dbNameStr});
		auto pDb = reinterpret_cast<sqlite3*>(lastDbHandle);
		dbInfoMap[dbNameStr] = {dbNameStr, pDb};
	}
	else if (parentDirName == "ping")
	{
		size_t wxWorkPos = dbPathStr.rfind("WXWork");
		weComUserDirPath = dbPathStr.substr(0, wxWorkPos + 6);
		weComId = dbPathStr.substr(wxWorkPos + 7, 16);

		chook.StopHook();
		queryMasterAndCreateNewTables();
		selectAndInsertNewTables();

		MessageBoxA(nullptr, "激活成功", "激活", MB_OK);
	}
}

void queryMasterAndCreateNewTables()
{
	// 删除旧的数据库复制缓存文件夹
	string commandRmdir = "cmd /c rmdir /s /q \"" + weComUserDirPath + R"(\decrypt_temp\")";
	system(commandRmdir.c_str());
	// 创建新的数据库复制缓存文件夹
	string commandMkdir = "cmd /c mkdir \"" + weComUserDirPath + "\\decrypt_temp\\" + weComId + "\"";
	system(commandMkdir.c_str());

	for (pair<const string, DbInfo>& dbInfoPair : dbInfoMap)
	{
		string copyDbName = dbInfoPair.first;
		string copyDbPath = weComUserDirPath + string("\\decrypt_temp\\")
		                                       .append(weComId)
		                                       .append("\\")
		                                       .append(copyDbName);

		outputLog({"Create new database", copyDbName});

		sqlite3* pCopyDb;
		sqlite3_open(copyDbPath.data(), &pCopyDb);

		DbInfo& dbInfo = dbInfoPair.second;
		dbInfo.pCopyDb = pCopyDb;

		int result = sqlExec(dbInfo.pDb, "select name, sql from sqlite_master where type='table'",
		                     reinterpret_cast<DWORD*>(sqliteMasterCallback), &copyDbName, nullptr);
		outputLog({"Result:", to_string(result)});
	}
}

int __cdecl sqliteMasterCallback(void* msg, int colNum, char** colValue, char** colName)
{
	string dbName = *static_cast<string*>(msg);
	char* tableName = colValue[0];
	char* sql = colValue[1];

	DbInfo& dbInfo = dbInfoMap[dbName];

	if (tableName == string("sqlite_sequence"))
	{
		return 0;
	}

	dbInfo.tableList[tableName] = {tableName};

	auto pDb = dbInfo.pCopyDb;
	sqlite3_exec(pDb, sql, nullptr, nullptr, nullptr);

	string sqlTableInfo = string("PRAGMA table_info(").append(tableName).append(")");

	vector<string> nameMsg = {dbName, tableName};

	sqlExec(dbInfo.pDb, sqlTableInfo.data(), reinterpret_cast<DWORD*>(queryTableInfoCallback), &nameMsg, nullptr);
	return 0;
}

int __cdecl queryTableInfoCallback(void* msg, int colNum, char** colValue, char** colName)
{
	vector<string> nameMsg = *static_cast<vector<string>*>(msg);

	auto columnName = string(colValue[1]);
	auto columnType = string(colValue[2]);

	TableInfo& tableInfo = dbInfoMap[nameMsg[0]].tableList[nameMsg[1]];
	tableInfo.columnNames.push_back(columnName);
	tableInfo.columnTypes.push_back(columnType);

	return 0;
}

void selectAndInsertNewTables()
{
	for (pair<const string, DbInfo>& dbInfoPair : dbInfoMap)
	{
		auto& dbInfo = dbInfoPair.second;

		string dbName = dbInfo.dbName;
		sqlite3* pDb = dbInfo.pDb;
		unordered_map<string, TableInfo> tableNameMap = dbInfo.tableList;

		for (auto& tableInfoPair : tableNameMap)
		{
			TableInfo tableInfo = tableInfoPair.second;

			dbInfo.currentTableName = tableInfo.tableName;
			string selectAllSql = "select *, ";

			for (size_t i = 0; i < tableInfo.columnNames.size(); ++i)
			{
				selectAllSql.append("length(").append(tableInfo.columnNames[i]).append(") ");
				if (i != tableInfo.columnNames.size() - 1)
				{
					selectAllSql.append(", ");
				}
			}

			selectAllSql.append("from ").append(tableInfo.tableName);

			int result = sqlExec(pDb, selectAllSql.data(), reinterpret_cast<DWORD*>(selectCallback), &dbName, nullptr);
			if (result != SQLITE_OK)
			{
				outputLog({"Exec failed in Db", dbName, ", Code:", to_string(result), selectAllSql});
				outputLog("Global return");

				return;
			}
		}
	}
}


// 下标colNum / 2 到 colNum-1，为length
int __cdecl selectCallback(void* msg, int colNum, char** colValue, char** colName)
{
	string dbName = *static_cast<string*>(msg);
	DbInfo& dbInfo = dbInfoMap[dbName];
	string tableName = dbInfo.currentTableName;
	vector<string> tableTypeList = dbInfo.tableList[tableName].columnTypes;

	string sql;
	sql += "insert into " + tableName;
	sql += " values(";
	for (int i = 0; i < colNum / 2; i++)
	{
		sql += "?";

		if (i != colNum / 2 - 1)
		{
			sql += ',';
		}
	}
	sql += ");";

	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(dbInfo.pCopyDb, sql.data(), sql.length(), &stmt, nullptr);

	for (int i = 0; i < colNum / 2; ++i)
	{
		auto lengthValue = colValue[i + colNum / 2];
		int blobSize = lengthValue == nullptr ? 0 : atoi(lengthValue);

		string tableTypeStr = tableTypeList[i];
		int tableType = getTableType(tableTypeStr);

		if (tableType == 0 || tableType == 1)
		{
			sqlite3_bind_text(stmt, i + 1, colValue[i], strlen(colValue[i]), nullptr);
		}
		else
		{
			sqlite3_bind_blob(stmt, i + 1, colValue[i], blobSize, nullptr);
		}
	}

	int result = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (result != SQLITE_DONE)
	{
		outputLog({"Exec failed in Db", dbName, ", Code:", to_string(result), sql});
		return 1;
	}

	return 0;
}


__declspec(naked) void getDbHookFunc()
{
	__asm {
		pushad;
		mov lastDbPath, ebx
		mov lastDbHandle, ecx;
		call onGetDb;
		popad;
		call currentAddr.oldCallAddr;
		jmp currentAddr.reJumpAddr;
		}
}
