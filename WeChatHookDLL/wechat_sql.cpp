#include "pch.h"
#include "wechat_sql.h"
#include "sqlite3.h"

using namespace std;

typedef struct SqlHookAddress
{
	DWORD dBHandleHookAddr;
	DWORD dBHandleHookOldCallAddr;
	DWORD dbHandleOldCallReJmpAddr;
	DWORD sqlExecAddr;
	DWORD openDatabaseAddr;
} SqlHookAddress;

typedef struct DbInfo
{
	string dbName;
	DWORD pDb;
	sqlite3* pCopyDb;
	vector<string> tableList;
	vector<int> tableCountList;
	string tableName;
	int count;
	int currentCount;
} DbInfo;

void dbHandleHookFunc();
int runSqlExec(DWORD pDb, const char* sql, string dbName, DWORD* callback);
string generateCopyDbName(const string& dbName);

typedef int (__cdecl* SqliteExec)(DWORD, const char*, DWORD*, void*, char**);
typedef int (__cdecl* OpenDataBase)(const char*, DWORD, unsigned int, const char*);


GetHandleCallback getHandleCallback = nullptr;

// HOOK位置78FDE1EF-78280000=D5E1EF 原始CALL 78FDD780-78280000=D5D780 回跳CALL = HOOK位置+5 exec函数指针 78FB0A00 - 78280000 = D3 0A00
unordered_map<string, SqlHookAddress> addrMap =
{
	{"3.4.0.38", SqlHookAddress{0xD5E1EF, 0xD5D780, 0xD5E1EF + 0x5, 0xD30A00, 0xD5DF10}},
	{"3.4.5.27", SqlHookAddress{0xDA6F3F, 0xDA64D0, 0xDA6F3F + 0x5, 0xD79760, 0xDA6C60}}
};

SqlHookAddress currentAddr;

SqliteExec sqlExec;
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

	sqlExec = reinterpret_cast<SqliteExec>(currentAddr.sqlExecAddr);
	openDb = reinterpret_cast<OpenDataBase>(currentAddr.openDatabaseAddr);

	sqlHook.HookReady(currentAddr.dBHandleHookAddr, dbHandleHookFunc);
	sqlHook.StartHook();
}

void stopGetDbHandleHook()
{
	sqlHook.StopHook();
}

int __cdecl queryTableCountCallback(void* msg, int colNum, char** colValue, char** colName)
{
	if (colNum == 1 && colValue)
	{
		*static_cast<int*>(msg) = atoi(colValue[0]);
		return 0;
	}
	return 1;
}

// 1. 传入参数 2.列号 3.内容 4.列名
// 此回调用于建表
int __cdecl sqliteMasterCallback(void* msg, int colNum, char** colValue, char** colName)
{
	// select name, sql from sqlite_master where type='table'
	string log;
	string dbName = *static_cast<string*>(msg);
	DbInfo& info = handleMap[dbName];

	log += "DBNname: ";
	log += info.dbName;

	for (int i = 0; i < colNum; i++)
	{
		log += " ";
		log += colName[i];
		log += ":";
		if (colValue[i] != nullptr)
		{
			log += string(colValue[i]);
		}
		log += "||";
	}
	OutputDebugStringA(log.c_str());

	if (colNum != 2)
	{
		OutputDebugStringA("Col num is not 2");
		return 0;
	}

	string tableName = string(colValue[0]);

	if (tableName == "sqlite_sequence")
	{
		return 0;
	}

	char* pErrMsg = {nullptr};

	// 执行建表语句
	char logChar[100] = {0};
	int createTableResult = sqlite3_exec(info.pCopyDb, colValue[1], nullptr, nullptr, &pErrMsg);
	if (createTableResult != 0)
	{
		sprintf_s(logChar, "Create table %s fail, %d, %s", tableName.c_str(), createTableResult, pErrMsg);
		OutputDebugStringA(logChar);
		return 1;
	}
	else
	{
		sprintf_s(logChar, "Create table %s success", tableName.c_str());
		OutputDebugStringA(logChar);
	}

	// 更新table vector
	info.tableList.push_back(tableName);

	// 查询count
	int count = -1;
	string sql = "select count(*) from " + tableName;
	int countResult = sqlExec(info.pDb, sql.c_str(), reinterpret_cast<DWORD*>(queryTableCountCallback), &count,
	                          &pErrMsg);
	if (countResult == 0)
	{
		sprintf_s(logChar, "Select count success, count: %d", count);
		OutputDebugStringA(logChar);
		info.tableCountList.push_back(count);
	}
	else
	{
		sprintf_s(logChar, "Select count %s fail, %s", tableName.c_str(), pErrMsg);
		OutputDebugStringA(logChar);
		return 1;
	}

	return 0;
}

int __cdecl selectAllCallback(void* msg, int colNum, char** colValue, char** colName)
{
	string dbName = *static_cast<string*>(msg);
	DbInfo& info = handleMap[dbName];
	string tableName = info.tableName;

	string sql;
	sql += "insert into " + tableName;
	sql += " values(";
	for (int i = 0; i < colNum; i++)
	{
		sql += "'";
		sql += colValue[i] == nullptr ? "NULL" : colValue[i];
		sql += "'";

		if (i != colNum - 1)
		{
			sql += ',';
		}
	}
	sql += ");";

	char* errMsg = {nullptr};
	int insertResult = sqlite3_exec(info.pCopyDb, sql.c_str(), nullptr, nullptr, &errMsg);

	if (insertResult != 0)
	{
		OutputDebugStringA(sql.c_str());
		OutputDebugStringA("insert failed");
		OutputDebugStringA(errMsg);
	}
	else
	{
		info.currentCount += 1;
		//string log;
		//log += to_string(info.currentCount) + "/" + to_string(info.count) + " ";
		//log += "insert success";
		//OutputDebugStringA(log.c_str());
	}

	return 0;
}

int startCopyDb()
{
	// 删除旧的数据库复制缓存文件夹
	string commandRmdir = "cmd /c rmdir /s /q \"" + userPath + R"(\decrypt_temp\")";
	system(commandRmdir.c_str());
	// 创建新的数据库复制缓存文件夹
	string commandMkdir = "cmd /c mkdir \"" + userPath + "decrypt_temp\\" + weChatId + "\"";
	system(commandMkdir.c_str());

	// 查询sqlite_master， 建表、查询数量
	for (auto it = handleMap.begin(); it != handleMap.end(); ++it)
	{
		DbInfo& dbInfo = it->second;

		string copyDbName = generateCopyDbName(dbInfo.dbName);
		string copyDbPath = userPath + string("decrypt_temp\\").append(weChatId).append("\\").append(copyDbName);

		sqlite3* pCopyDb;
		int openResult = sqlite3_open(copyDbPath.c_str(), &pCopyDb);

		if (openResult != 0)
		{
			OutputDebugStringA((string("Open db ") + copyDbPath + "failed").c_str());
			continue;
		}
		else
		{
			OutputDebugStringA((string("Open db ") + copyDbPath + " success").c_str());
			dbInfo.pCopyDb = pCopyDb;
		}

		runSqlExec(dbInfo.pDb, "select name, sql from sqlite_master where type='table'",
		           it->first,
		           reinterpret_cast<DWORD*>(sqliteMasterCallback));
	}

	OutputDebugStringA("query sqlite master finish");

	// 遍历表
	for (auto it = handleMap.begin(); it != handleMap.end(); ++it)
	{
		DbInfo& dbInfo = it->second;
		vector<string> tableList = dbInfo.tableList;

		sqlite3_exec(dbInfo.pCopyDb, "begin;", nullptr, nullptr, nullptr);

		char log[100] = {0};
		sprintf_s(log, "DB %s table count: %d", dbInfo.dbName.c_str(), tableList.size());
		OutputDebugStringA(log);

		int index = 0;
		for (auto& tableName : tableList)
		{
			sprintf_s(log, "Select All From %s.%s", dbInfo.dbName.c_str(), tableName.c_str());
			OutputDebugStringA(log);

			dbInfo.tableName = tableName;
			dbInfo.count = dbInfo.tableCountList[index];
			dbInfo.currentCount = 0;
			string sql = "select * from " + tableName;
			runSqlExec(dbInfo.pDb, sql.c_str(), dbInfo.dbName, reinterpret_cast<DWORD*>(selectAllCallback));

			index++;
			sprintf_s(log, "Select All and Insert Finish");
			OutputDebugStringA(log);
		}

		sqlite3_exec(dbInfo.pCopyDb, "commit;", nullptr, nullptr, nullptr);
		sqlite3_close(dbInfo.pCopyDb);
	}

	return 0;
}

int runSqlExec(DWORD pDb, const char* sql, string dbName, DWORD* callback)
{
	char* errMsg = nullptr;
	int result = sqlExec(pDb, sql, callback, &dbName, &errMsg);
	string resultStr = "Exec result: ";
	resultStr += to_string(result);
	if (errMsg != nullptr)
	{
		resultStr += errMsg;
	}
	resultStr += sql;
	OutputDebugStringA(resultStr.c_str());

	return result;
}

void __stdcall getDb(DWORD pEsi)
{
	char str[0x200] = {0};
	auto path = reinterpret_cast<char*>(lastDbPath);

	//  0            13          25
	// "D:\Documents\WeChat Files\wxid_8rmcj0zs9itk22\Msg\Multi\MSG0.db"
	sprintf_s(str, "DbName:%d Path:%s|", pEsi, path);
	OutputDebugStringA(str);

	string pathStr = string(path);

	if (userPath.length() == 0)
	{
		size_t posTmp = pathStr.rfind("WeChat Files");
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
		OutputDebugStringA(str);
		DbInfo info = {dbName, pEsi};
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

string generateCopyDbName(const string& dbName)
{
	return string("decrypt_") + dbName;
}
