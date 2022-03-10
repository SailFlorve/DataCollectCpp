#include <string>
#include <iostream>

#include "sqlite3.h"
using namespace std;

sqlite3* pDb;

int __cdecl printCallback(void* msg, int colNum, char** colValue, char** colName)
{
	cout << colValue[1] << "," << colValue[2] << endl;;


	return 0;
}

int __cdecl selectCallback(void* msg, int colNum, char** colValue, char** colName)
{
	sqlite3_exec(pDb, "insert into Test2 values ('1','2', 'FFD*')", nullptr, nullptr, nullptr);
	sqlite3_stmt* stmt;
	string sql = "insert into Test2 values (?, ?, ?)";

	sqlite3_prepare(pDb, sql.data(), sql.length(), &stmt, nullptr);
	sqlite3_bind_blob(stmt, 1, colValue[0], 1, nullptr);
	sqlite3_bind_blob(stmt, 2, colValue[1], 1, nullptr);
	sqlite3_bind_blob(stmt, 3, colValue[2], 16, nullptr);

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	return 0;
}

int main(int argc, char* argv[])
{
	sqlite3_open("D:\\test.db", &pDb);

	sqlite3_exec(pDb, "select *, length(c1), length(c2), length(c3) from Test", printCallback, nullptr, nullptr);
}
