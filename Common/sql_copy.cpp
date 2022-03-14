#include "sql_copy.h"

#include <utility>

#include "common_util.h"

int sqliteMasterCallback(void* msg, int colNum, char** colValue, char** colName);
int queryTableInfoCallback(void* msg, int colNum, char** colValue, char** colName);
int selectCallback(void* msg, int colNum, char** colValue, char** colName);
int queryMasterAndCreateNewTables();
int selectAndInsertNewTables();


unordered_map<string, DbInfo> dbInfoMap;
string outputPath;
SqliteExec sqlExec;

int startDatabaseCopy(unordered_map<string, DbInfo> map, const string& userPath, const string& userId, SqliteExec exec)
{
	dbInfoMap = std::move(map);
	outputPath = userPath + "\\decrypt_temp\\" + userId + "\\";
	sqlExec = exec;

	int result = queryMasterAndCreateNewTables();
	if (result != 0)
	{
		return result;
	}

	result = selectAndInsertNewTables();

	return result;
}

int queryMasterAndCreateNewTables()
{
	// 删除旧的数据库复制缓存文件夹
	string commandRmdir = "cmd /c rmdir /s /q \"" + outputPath + R"(")";
	system(commandRmdir.c_str());
	// 创建新的数据库复制缓存文件夹
	string commandMkdir = "cmd /c mkdir \"" + outputPath + "\"";
	system(commandMkdir.c_str());

	for (pair<const string, DbInfo>& dbInfoPair : dbInfoMap)
	{
		string dbName = dbInfoPair.first;
		string copyDbName = generateCopyDbName(dbInfoPair.first);
		string copyDbPath = outputPath + string("\\").append(copyDbName);

		outputLog({"Create new database", copyDbName});

		sqlite3* pCopyDb;
		sqlite3_open(copyDbPath.data(), &pCopyDb);

		DbInfo& dbInfo = dbInfoPair.second;
		dbInfo.pCopyDb = pCopyDb;

		int result = sqlExec(dbInfo.pDb, "select name, sql from sqlite_master where type='table'",
		                     sqliteMasterCallback, &dbName, nullptr);
		outputLog({"Result:", to_string(result)});

		if (result != SQLITE_OK)
		{
			return result;
		}
	}

	return 0;
}

int sqliteMasterCallback(void* msg, int colNum, char** colValue, char** colName)
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

	int result = sqlExec(dbInfo.pDb, sqlTableInfo.data(), queryTableInfoCallback, &nameMsg, nullptr);
	if (result != SQLITE_OK)
	{
		outputLog({"Exec failed: code", to_string(result), sqlTableInfo});
	}

	return result;
}

int queryTableInfoCallback(void* msg, int colNum, char** colValue, char** colName)
{
	vector<string> nameMsg = *static_cast<vector<string>*>(msg);

	auto columnName = string(colValue[1]);
	auto columnType = string(colValue[2]);

	TableInfo& tableInfo = dbInfoMap[nameMsg[0]].tableList[nameMsg[1]];
	tableInfo.columnNames.push_back(columnName);
	tableInfo.columnTypes.push_back(columnType);

	return 0;
}

int selectAndInsertNewTables()
{
	for (pair<const string, DbInfo>& dbInfoPair : dbInfoMap)
	{
		auto& dbInfo = dbInfoPair.second;

		string dbName = dbInfo.dbName;
		sqlite3* pDb = dbInfo.pDb;
		unordered_map<string, TableInfo> tableNameMap = dbInfo.tableList;

		sqlite3_exec(dbInfo.pCopyDb, "begin;", nullptr, nullptr, nullptr);

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

			int result = sqlExec(pDb, selectAllSql.data(), selectCallback, &dbName, nullptr);
			if (result != SQLITE_OK)
			{
				outputLog({"Exec failed in Db", dbName, ", Code:", to_string(result), selectAllSql});
				outputLog("Global return");

				return result;
			}
		}

		sqlite3_exec(dbInfo.pCopyDb, "commit;", nullptr, nullptr, nullptr);
		sqlite3_close(dbInfo.pCopyDb);
	}

	return 0;
}

int selectCallback(void* msg, int colNum, char** colValue, char** colName)
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
			int valueLen = colValue[i] == nullptr ? 0 : strlen(colValue[i]);
			sqlite3_bind_text(stmt, i + 1, colValue[i], valueLen, nullptr);
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
		return result;
	}

	return 0;
}
