#pragma once
#include "pch.h"

void getSqlDBHandleHookStart();
void getSqlDBHandleHookStop();

void getSqlDbHookFunc();
void __stdcall  getDb1(DWORD pEsi);
typedef int(__cdecl* SqliteExec)(DWORD, char*, DWORD*, void*, char**);


int __cdecl mycallback(void* NotUsed, int argc, char** argv, char** azColName);
//extern string sqlStr;
//extern sqlite_exec  sqlexec;
void runSqlExec(DWORD db, char* sqltext);