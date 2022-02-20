#include <iostream>
#include <fstream>

#include "sqlite3.h"

using namespace std;

ofstream os(R"(D:\WeChatDecrypt\1020716466.txt)");

bool checkResult(const string& op, int result, const string& msg = "")
{
	if (result != SQLITE_OK)
	{
		cout << op << " " << "failed " << result << " " << msg << endl;
		return false;
	}
	return true;
}

int callback(void* msg, int colNum, char** colVal, char** colName)
{
	
	if (os.is_open())
	{
		os << colVal[7];
		os << endl;
		os << endl;
	}
	return 0;
}

int main()
{
	sqlite3* db;
	int rec = sqlite3_open(R"(D:\WeChatDecrypt\BD82E3508DBEE52F432A731EA5FC3228)", &db);
	if (!checkResult("Open", rec)) return 1;
	
	sqlite3_stmt* stmt;
	string sql = "select * from msg_3_1020716466";
	sqlite3_prepare(db, sql.c_str(), sql.length(), &stmt, nullptr);
	rec = sqlite3_step(stmt);
	cout << rec << endl;
	while(rec == SQLITE_ROW)
	{
		const unsigned char* type = sqlite3_column_text(stmt, 3);
		const void* pBolbData = sqlite3_column_blob(stmt, 7);
		int len = sqlite3_column_bytes(stmt, 7);

		os << type << endl;
		os.write(static_cast<const char*>(pBolbData), len);
		os << endl;
		rec = sqlite3_step(stmt);
		cout << rec << endl;
	}
	sqlite3_finalize(stmt);

	os.close();
	return 0;
}
