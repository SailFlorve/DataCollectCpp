#include<Windows.h>
#include<iostream>
#include"sqlite3.h"
#include <vector>
using namespace std;

typedef int (*qq_splite3_key)(void* pDB, void* pKey, int nSize);
typedef int (*qq_sqlite3_open)(const char* fName, void* ppDB);
typedef int (*qq_sqlite3_rekey)(void* pDB, void* pKey, int nSize);
typedef int (*qq_sqlite3_exec)(void* pDB, const char* sql, void* callback, void* para, char** errMsg);

static int show_table(void* data, int argc, char** argv, char** azColName);

sqlite3* newDb;
vector<string> tableVec;

static int show_table(void* data, int argc, char** argv, char** azColName)
{
	for (int i = 0; i < argc; i++)
	{
		cout << azColName[i] << "=" << (argv[i] ? argv[i] : "NULL") << endl;
	}
	printf("\n");
	sqlite3_exec(newDb, argv[1], nullptr, nullptr, nullptr);
	tableVec.push_back(argv[2]);
	return 0;
}

int selectAllCallback(void* msg, int colNum, char** colValue, char** colName)
{
	string sql = "INSERT INTO ";
	char* tableName = (char*)msg;
	sql += tableName;
	sql += " VALUES (";
	for (int i = 0; i < colNum; ++i)
	{
		sql += "?";
		if (i != colNum - 1)
		{
			sql += ",";
		}
	}
	sql += ")";

	sqlite3_stmt* stmt = NULL;

	//插入的sql语句，这里用?占位，代表BLOB数据
	sqlite3_prepare_v2(newDb, sql.c_str(), -1, &stmt, NULL);
	for (int i = 0; i < colNum; ++i)
	{
		sqlite3_bind_text(stmt, i + 1, colValue[0], strlen(colValue[0]), NULL);
	}

	char* errMsg = {nullptr};
	int rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE)
		printf("execution failed: %s %s\n", sqlite3_errmsg(newDb), sql.c_str());
	else
		printf("exec success\n");

	return 0;
}

int abcmain()
{
	unsigned char key[] = {
		0x35, 0x37, 0x30, 0x39, 0x34, 0x36, 0x38, 0x36, 0x32, 0x32, 0x38, 0x44, 0x34, 0x34, 0x42, 0x30
	};
	unsigned char keyC3D[] = {
		0xA0, 0x79, 0xA1, 0x80, 0x73, 0x4E, 0x0A, 0xD8, 0xC1, 0xEE, 0x03, 0xF3, 0x2E, 0x79, 0xFE, 0xB0
	};
	unsigned char keyBD8[] = {
		0x4A, 0xD0, 0xE0, 0xE2, 0x6C, 0x62, 0xA9, 0xD4, 0xB0, 0x4E, 0x3E, 0xDA, 0x9E, 0xBF, 0x95, 0x16
	};
	unsigned char key8CC[] = {
		0x36, 0x59, 0xB5, 0x17, 0xFD, 0x69, 0xD7, 0xE5, 0xF3, 0x26, 0x52, 0x9F, 0xE9, 0x86, 0x06, 0xC0
	};
	unsigned char key561[] = {
		0xAF, 0x89, 0x25, 0xF0, 0x0F, 0x33, 0x3F, 0xC3, 0xFC, 0x24, 0x54, 0xC9, 0x26, 0x05, 0x89, 0x16
	};

	unsigned char keyMsg3[] = {
		0xF8, 0x9A, 0x6C, 0x97, 0x26, 0x53, 0x19, 0xA3, 0x26, 0x1E, 0x48, 0x74, 0x7D, 0xFC, 0x3D, 0x32
	};

	unsigned char rekey[16];
	memset(rekey, 0, 16);

	HMODULE hMoudle = LoadLibraryEx(L"C:\\Users\\Hang\\Desktop\\QQ\\Bin\\KernelUtil.dll", nullptr,
	                                LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hMoudle)
	{
		cout << GetLastError() << endl;
		return 0;
	}
	//cout << (DWORD)hMoudle << endl;

	sqlite3_open("D:\\WechatDecrypt\\new.db", &newDb);

	qq_splite3_key sql3_key = qq_splite3_key((DWORD)hMoudle + 0x86E86);
	qq_sqlite3_rekey sql3_rekey = qq_sqlite3_rekey((DWORD)hMoudle + 0x87045);
	qq_sqlite3_open sql3_open = qq_sqlite3_open((DWORD)hMoudle + 0x365A6);
	qq_sqlite3_exec sql3_exec = qq_sqlite3_exec((DWORD)hMoudle + 0x35967);
	sqlite3* pDB = NULL;
	int nResult = sql3_open(R"(D:\WeChatDecrypt\Msg3.0.db)", &pDB);
	cout << "OPEN " << nResult << endl;
	if (nResult != SQLITE_OK)
	{
		cout << "open db error" << endl;
		return 0;
	}
	cout << "KEY " << sql3_key(pDB, keyMsg3, 16) << endl;

	// char* errMsg = NULL;
	// cout << sql3_exec(pDB, "SELECT type, sql, name FROM sqlite_master where type='table'", show_table, NULL, &errMsg) << endl;
	// printf("EXEC: %s\n", errMsg);
	//
	// for (auto table : tableVec)
	// {
	// 	string sql = "SELECT * FROM " + table;
	// 	cout << "Exec sql:" << sql << endl;
	// 	cout << sql3_exec(pDB, sql.c_str(), selectAllCallback, (void*)table.c_str(), &errMsg) << endl;
	// }

	cout << "REKEY" << sql3_rekey(pDB, rekey, 16) << endl;
	// cout << "CLOSE" << sqlite3_close(pDB) << endl;
	return 1;
}
