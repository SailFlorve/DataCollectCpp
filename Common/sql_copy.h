#pragma once
#include <string>
#include <unordered_map>
#include "sqlite3.h"

using namespace std;

typedef int (__cdecl* SqliteExec)(sqlite3*, const char*,
                                  int (*callback)(void*, int, char**, char**),
                                  void*, char**);

struct TableInfo;
struct DbInfo;


struct TableInfo
{
	string tableName;
	vector<string> columnNames;
	vector<string> columnTypes;
};

struct DbInfo
{
	std::string dbName;
	sqlite3* pDb = nullptr;
	sqlite3* pCopyDb = nullptr;
	unordered_map<string, TableInfo> tableList;
	string currentTableName;
};

int startDatabaseCopy(unordered_map<string, DbInfo> map, const string& userPath, const string& userId, SqliteExec exec);
