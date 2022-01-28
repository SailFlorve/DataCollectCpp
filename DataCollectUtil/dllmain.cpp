﻿// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <TlHelp32.h>
#include <iostream>
#include <Windows.h>
#include <fstream>
#include <Shlobj.h>
#include <stdlib.h>
#include "AES.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>
#include <cassert>

#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libeay32.lib")

#undef _UNICODE
#define SQLITE_FILE_HEADER "SQLite format 3" 
#define IV_SIZE 16
#define HMAC_SHA1_SIZE 20
#define KEY_SIZE 32

#define SL3SIGNLEN 20


#ifndef ANDROID_WECHAT
#define DEFAULT_PAGESIZE 4096       //4048数据 + 16IV + 20 HMAC + 12
#define DEFAULT_ITER 64000
#else
#define NO_USE_HMAC_SHA1
#define DEFAULT_PAGESIZE 1024
#define DEFAULT_ITER 4000
#endif

using namespace std;

extern "C" {
    __declspec(dllexport) void inject(const wchar_t* processName, wchar_t* dllPath, void(*callback)(BOOL, const wchar_t*));
	__declspec(dllexport) DWORD FindProcessId(const wchar_t* processName);
	__declspec(dllexport) int decryptBackup(const char* backupDir, const char* outputDir, int type, const char* bakKey);
	__declspec(dllexport) wchar_t* getDocumentsPath();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void inject(const wchar_t* processName, wchar_t* dllPath, void(*callback)(BOOL, const wchar_t*))
{
    size_t strSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);

    DWORD processId = 0;

	setlocale(LC_ALL, "");
	printf("Inject:%ls, DLLPath:%ls, StrSize:%zu\n", processName, dllPath, strSize);

    // 1. 遍历系统中的进程，找到微信进程
    HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 processEntry32 = { 0 };
    processEntry32.dwSize = sizeof(processEntry32);

    BOOL next = Process32Next(handle, &processEntry32);
    while (next == TRUE) {
        if (wcscmp(processEntry32.szExeFile, processName) == 0) {
            processId = processEntry32.th32ProcessID;
            break;
        }
        next = Process32Next(handle, &processEntry32);
    }

    if (processId == 0) {
        callback(0, L"没有找到进程");
        return;
    }
    else {
        cout << processId << endl;
    }

    // 2.打开进程获取handle
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processId);
    if (hProcess == nullptr) {

		cout << "c01" << endl;
        callback(0, L"打开进程失败");
        return;
    }
    // 3.在进程进程为DLL字符串申请内存空间
    LPVOID allocAddress = VirtualAllocEx(hProcess, nullptr, strSize, MEM_COMMIT, PAGE_READWRITE);
    if (allocAddress == nullptr) {
        callback(0, L"分配空间失败");
        return;
    }
    // 4.把DLL路径写入到申请的内存
    BOOL result = WriteProcessMemory(hProcess, allocAddress, dllPath, strSize, nullptr);
    if (result == FALSE) {
        callback(0, L"写入内存失败");
        return;
    }
    // 5.从Kernel32.dll中获取loadLibraryA的函数地址
    HMODULE hMoudle = GetModuleHandle(L"Kernel32.dll");
    FARPROC farProc = GetProcAddress(hMoudle, "LoadLibraryW");

    if (farProc == nullptr) {
        callback(0, L"查找失败");
        return;
    }
    // 6. 在进程中启动DLL 返回新线程句柄
    HANDLE newThreadHandle = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)farProc, allocAddress, 0, NULL);
    if (newThreadHandle == nullptr) {
        callback(0, L"创建远程线程失败");
        return;
    }
    else {
        callback(1, L"注入成功");
    }
}

DWORD FindProcessId(const wchar_t* processName)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    DWORD result = NULL;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hProcessSnap) return(FALSE);

    pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

                                          // Retrieve information about the first process,
                                          // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return NULL;
    }

    do
    {
        if (0 == wcscmp(processName, pe32.szExeFile))
        {
            result = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    return result;
}

wchar_t* getDocumentsPath() {
	static wchar_t homePath[MAX_PATH] = { 0 };
    if (!SHGetSpecialFolderPath(NULL, homePath, CSIDL_PERSONAL, false)) {
        return 0;
    }
    return homePath;
}

unsigned char* CAES::aes_128_ecb_decrypt(const char* ciphertext, int text_size, const char* key, int key_size, int& size)
{
	EVP_CIPHER_CTX* ctx;
	ctx = EVP_CIPHER_CTX_new();
	int ret = EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, (const unsigned char*)key, NULL);
	//assert(ret == 1);
	unsigned char* result = new unsigned char[text_size + 64]; // 弄个足够大的空间
	int len1 = 0;
	ret = EVP_DecryptUpdate(ctx, result, &len1, (const unsigned char*)ciphertext, text_size);
	//assert(ret == 1);
	int len2 = 0;
	ret = EVP_DecryptFinal_ex(ctx, result + len1, &len2);
	//assert(ret == 1);
	ret = EVP_CIPHER_CTX_cleanup(ctx);
	//assert(ret == 1);
	EVP_CIPHER_CTX_free(ctx);
	size = len1 + len2;
	unsigned char* res = new unsigned char[size];
	memcpy(res, result, size);
	delete[] result;
	return res;
}

bool writeToFile(const char* fileName, void const* buffer, size_t elementSize, size_t elementCount) {
	try {


		FILE* fp;
		int openRes = fopen_s(&fp, fileName, "ab+");
		if (openRes != 0) {
			return false;
		}
		{
			fwrite(buffer, elementSize, elementCount, fp);
			fclose(fp);
		}
		return true;
	}
	catch (exception& e) {
		printf("%s", e.what());
		return false;
	}

}

// 0 成功 1 打开的文件不存在 2 key错误 3 写入文件出错
int decryptBackupDb(const char* dbDir, const char* fileName, const char* outputDir, unsigned char* pass, int nKey) {

	FILE* fpdb;
	string filePath;
	filePath += dbDir;
	filePath += "\\";
	filePath += fileName;
	fopen_s(&fpdb, filePath.c_str(), "rb+");
	if (!fpdb) {
		printf("Decrypt backup database fail open file error %s\n", filePath.c_str());
		return 1;
	}
	fseek(fpdb, 0, SEEK_END);
	long nFileSize = ftell(fpdb);
	fseek(fpdb, 0, SEEK_SET);
	unsigned char* pDbBuffer = new unsigned char[nFileSize];
	fread(pDbBuffer, 1, nFileSize, fpdb);
	fclose(fpdb);

	unsigned char salt[16] = { 0 };
	memcpy(salt, pDbBuffer, 16);

#ifndef NO_USE_HMAC_SHA1
	unsigned char mac_salt[16] = { 0 };
	memcpy(mac_salt, salt, 16);
	for (int i = 0; i < sizeof(salt); i++) {
		mac_salt[i] ^= 0x3a;
	}
#endif

	int reserve = IV_SIZE;      //校验码长度,PC端每4096字节有48字节
#ifndef NO_USE_HMAC_SHA1
	reserve += HMAC_SHA1_SIZE;
#endif
	reserve = ((reserve % AES_BLOCK_SIZE) == 0) ? reserve : ((reserve / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;

	unsigned char key[KEY_SIZE] = { 0 };
	unsigned char mac_key[KEY_SIZE] = { 0 };

	OpenSSL_add_all_algorithms();
	PKCS5_PBKDF2_HMAC_SHA1((const char*)pass, nKey, salt, sizeof(salt), DEFAULT_ITER, sizeof(key), key);
#ifndef NO_USE_HMAC_SHA1
	//此处源码，怀凝可能有错，pass 数组才是密码
	//PKCS5_PBKDF2_HMAC_SHA1((const char*)key, sizeof(key), mac_salt, sizeof(mac_salt), 2, sizeof(mac_key), mac_key);
	PKCS5_PBKDF2_HMAC_SHA1((const char*)key, sizeof(key), mac_salt, sizeof(mac_salt), 2, sizeof(mac_key), mac_key);
#endif

	unsigned char* pTemp = pDbBuffer;
	unsigned char pDecryptPerPageBuffer[DEFAULT_PAGESIZE];
	int nPage = 1;
	int offset = 16;
	while (pTemp < pDbBuffer + nFileSize) {

#ifndef NO_USE_HMAC_SHA1
		unsigned char hash_mac[HMAC_SHA1_SIZE] = { 0 };
		unsigned int hash_len = 0;
		HMAC_CTX hctx;
		HMAC_CTX_init(&hctx);
		HMAC_Init_ex(&hctx, mac_key, sizeof(mac_key), EVP_sha1(), NULL);
		HMAC_Update(&hctx, pTemp + offset, DEFAULT_PAGESIZE - reserve - offset + IV_SIZE);
		HMAC_Update(&hctx, (const unsigned char*)&nPage, sizeof(nPage));
		HMAC_Final(&hctx, hash_mac, &hash_len);
		HMAC_CTX_cleanup(&hctx);
		if (0 != memcmp(hash_mac, pTemp + DEFAULT_PAGESIZE - reserve + IV_SIZE, sizeof(hash_mac))) {
			return 2;
			//MessageBoxA(NULL, "ERROR", "hash错误", MB_OK);
		}
#endif
		//
		if (nPage == 1) {
			memcpy(pDecryptPerPageBuffer, SQLITE_FILE_HEADER, offset);
		}

		EVP_CIPHER_CTX* ectx = EVP_CIPHER_CTX_new();
		EVP_CipherInit_ex(ectx, EVP_get_cipherbyname("aes-256-cbc"), NULL, NULL, NULL, 0);
		EVP_CIPHER_CTX_set_padding(ectx, 0);
		EVP_CipherInit_ex(ectx, NULL, NULL, key, pTemp + (DEFAULT_PAGESIZE - reserve), 0);

		int nDecryptLen = 0;
		int nTotal = 0;
		EVP_CipherUpdate(ectx, pDecryptPerPageBuffer + offset, &nDecryptLen, pTemp + offset, DEFAULT_PAGESIZE - reserve - offset);
		nTotal = nDecryptLen;
		EVP_CipherFinal_ex(ectx, pDecryptPerPageBuffer + offset + nDecryptLen, &nDecryptLen);
		nTotal += nDecryptLen;
		EVP_CIPHER_CTX_free(ectx);

		memcpy(pDecryptPerPageBuffer + DEFAULT_PAGESIZE - reserve, pTemp + DEFAULT_PAGESIZE - reserve, reserve);
		char decFile[1024] = { 0 };
		sprintf_s(decFile, "%s\\decrypt_%s", outputDir, fileName);
		bool writeResult = writeToFile(decFile, pDecryptPerPageBuffer, 1, DEFAULT_PAGESIZE);
		if (!writeResult) {
			return 3;
		}

		nPage++;
		offset = 0;
		pTemp += DEFAULT_PAGESIZE;
	}
	//MessageBoxA(NULL, "decrypt db sucess", "解密成功", MB_OK);
	return 0;
}

// 01 文件不存在 11 写入错误
int decryptBackupFile(const char* backupPath, const char* backupFile, const char* outputDir, unsigned char* key, int nKey)
{
	FILE* fpdb;
	string fileName;
	fileName += backupPath;
	fileName += "\\";
	fileName += backupFile;

	fopen_s(&fpdb, fileName.c_str(), "rb+");
	if (!fpdb) {
		//MessageBoxA(NULL, "ERROR", "打开文件错误", MB_OK);
		printf("DecryptBackupFile Open file error %s\n", fileName.c_str());
		return 1;
	}
	fseek(fpdb, 0, SEEK_END);
	long nFileSize = ftell(fpdb);
	fseek(fpdb, 0, SEEK_SET);
	unsigned char* pDeBuffer = new unsigned char[nFileSize];
	fread(pDeBuffer, 1, nFileSize, fpdb);
	fclose(fpdb);
	int size = 0;
	unsigned char* decrypt = CAES::aes_128_ecb_decrypt((const char*)pDeBuffer, nFileSize, (const char*)key, nKey, size);
	char decFile[1024] = { 0 };
	sprintf_s(decFile, "%s\\decrypt_%s", outputDir, backupFile);
	bool writeResult = writeToFile(decFile, decrypt, 1, size);
	if (!writeResult) {
		return 3;
	}
	//MessageBoxA(NULL, "decrypt file sucess", "解密成功", MB_OK);
	return 0;
}

int decryptBackup(const char* backupDir, const char* outputDir, int type, const char* bakKey) {
	// result: 0 success 1 文件不存在 2 Key错误

	switch (type)
	{
	case 1:
	{
		int result = decryptBackupDb(backupDir, "Backup.db", outputDir, (unsigned char*)bakKey, 0x20);
		if (result != 0) {
			return result;
		}
		result = decryptBackupFile(backupDir, "BAK_0_TEXT", outputDir, (unsigned char*)bakKey, 0x10);
		result = result | decryptBackupFile(backupDir, "BAK_0_MEDIA", outputDir, (unsigned char*)bakKey, 0x10);
		return result;
	}
	default:
		break;
	}
	return 0;
}
