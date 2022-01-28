#include "sqlite3.h"
#include <iostream>
#include <string>

using namespace std;

int MyCallback(void* para, int nColumn, char** colValue, char** colName)
{
	cout << "My Callback called" << endl;

	string rowStr = " ";
	for (int i = 0; i < nColumn; i++)
	{
		rowStr += to_string(i);
		rowStr += " ";
		rowStr += string(colName[i]);
		rowStr += " ";
		rowStr += string(colValue[i]);
		rowStr += " ";
	}

	cout << rowStr << endl;
	return 0;
}


int main1() {

	char* errMsg = NULL;

	sqlite3* db = NULL;
	int result = sqlite3_open("D:/test.db", &db);

	if (result != SQLITE_OK)
	{
		printf("open database text.db failed \n");
		return 0;
	}
	else
	{
		printf("open database text.db success \n");
	}

	//result = sqlite3_exec(db, "attach 'D:/COPY1.db' as newDb", MyCallback, 0, &errMsg);
	//if (result != SQLITE_OK)
	//{
	//	cout << "attach fail " << errMsg<< endl;
	//	return 0;
	//}
	//else
	//{
	//	cout << "attach success" << endl;
	//}

	//result = sqlite3_exec(db, "create table newDb.Student1 as select * from Student", MyCallback, 0, &errMsg);
	//if (result != SQLITE_OK)
	//{
	//	cout << "create as fail " << errMsg << endl;
	//	return 0;
	//}

	// Create Table
	const char* sql = "create table Student(t_id integer primary key autoincrement, t_name varchar(15), t_age integer)";

	result = sqlite3_exec(db, sql, NULL, NULL, &errMsg);

	if (result != SQLITE_OK)
	{
		printf("create table Student failed\n");
		printf("error conde %d \t error message:%s", result, errMsg);
	}

	//插入数据
	errMsg = NULL;
	sql = "insert into Student(t_name, t_age) values ('dwb', 23)";
	result = sqlite3_exec(db,sql,NULL,NULL,&errMsg);
	printf("insert message1:%s \n", errMsg);

	errMsg = NULL;
	sql = "insert into Student(t_name, t_age) values ('dhx', 25)";
	result = sqlite3_exec(db,sql,NULL,NULL,&errMsg);
	printf("insert message2:%s \n", errMsg);

	errMsg = NULL;
	sql = "insert into Student(t_name, t_age) values ('dwz', 21)";
	result = sqlite3_exec(db,sql,NULL,NULL,&errMsg);
	printf("insert message3:%s \n", errMsg);

	// 查询数据
	sql = "select * from Student;";
	result = sqlite3_exec(db, sql, MyCallback, NULL, 0);

	return 0;

}