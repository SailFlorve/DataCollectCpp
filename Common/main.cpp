#include <cstdlib>
#include <iostream>
#include <ShlObj.h>
#include <TlHelp32.h>
#include <Windows.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include "common_util.h"
#include "sqlite3.h"

using namespace std;
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "Version.lib")

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

sqlite3* pDb;

int __cdecl printCallback(void* msg, int colNum, char** colValue, char** colName)
{
	cout << colValue[1] << "," << colValue[2] << endl;;


	return 0;
}

// int __cdecl selectCallback(void* msg, int colNum, char** colValue, char** colName)
// {
// 	sqlite3_exec(pDb, "insert into Test2 values ('1','2', 'FFD*')", nullptr, nullptr, nullptr);
// 	sqlite3_stmt* stmt;
// 	string sql = "insert into Test2 values (?, ?, ?)";
//
// 	sqlite3_prepare(pDb, sql.data(), sql.length(), &stmt, nullptr);
// 	sqlite3_bind_blob(stmt, 1, colValue[0], 1, nullptr);
// 	sqlite3_bind_blob(stmt, 2, colValue[1], 1, nullptr);
// 	sqlite3_bind_blob(stmt, 3, colValue[2], 16, nullptr);
//
// 	sqlite3_step(stmt);
// 	sqlite3_finalize(stmt);
//
// 	return 0;
// }

bool writeToFile(const char* fileName, void const* buffer, size_t elementSize, size_t elementCount)
{
	try
	{
		FILE* fp;
		int openRes = fopen_s(&fp, fileName, "ab+");
		if (openRes != 0)
		{
			return false;
		}
		{
			fwrite(buffer, elementSize, elementCount, fp);
			fclose(fp);
		}
		return true;
	}
	catch (exception& e)
	{
		printf("%s", e.what());
		return false;
	}
}

int decryptWeChatBackupDb(const char* dbDir, const char* fileName, const char* outputDir, const unsigned char* pass,
                          int nKey)
{
	cout << dbDir;
	cout << fileName;
	cout << outputDir;
	cout << pass;
	cout << nKey;

	FILE* fpdb;
	string filePath;
	filePath += dbDir;
	filePath += "\\";
	filePath += fileName;
	fopen_s(&fpdb, filePath.c_str(), "rb+");
	if (!fpdb)
	{
		printf("Decrypt backup database fail open file error %s\n", filePath.c_str());
		return 1;
	}
	fseek(fpdb, 0, SEEK_END);
	long nFileSize = ftell(fpdb);
	fseek(fpdb, 0, SEEK_SET);
	unsigned char* pDbBuffer = new unsigned char[nFileSize];
	fread(pDbBuffer, 1, nFileSize, fpdb);
	fclose(fpdb);

	unsigned char salt[16] = {0};
	memcpy(salt, pDbBuffer, 16);

#ifndef NO_USE_HMAC_SHA1
	unsigned char mac_salt[16] = {0};
	memcpy(mac_salt, salt, 16);
	for (int i = 0; i < sizeof(salt); i++)
	{
		mac_salt[i] ^= 0x3a;
	}
#endif

	int reserve = IV_SIZE; //校验码长度,PC端每4096字节有48字节
#ifndef NO_USE_HMAC_SHA1
	reserve += HMAC_SHA1_SIZE;
#endif
	reserve = ((reserve % AES_BLOCK_SIZE) == 0) ? reserve : ((reserve / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;

	unsigned char key[KEY_SIZE] = {0};
	unsigned char mac_key[KEY_SIZE] = {0};

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
	while (pTemp < pDbBuffer + nFileSize)
	{
#ifndef NO_USE_HMAC_SHA1
		unsigned char hash_mac[HMAC_SHA1_SIZE] = {0};
		unsigned int hash_len = 0;
		HMAC_CTX hctx;
		HMAC_CTX_init(&hctx);
		HMAC_Init_ex(&hctx, mac_key, sizeof(mac_key), EVP_sha1(), NULL);
		HMAC_Update(&hctx, pTemp + offset, DEFAULT_PAGESIZE - reserve - offset + IV_SIZE);
		HMAC_Update(&hctx, (const unsigned char*)&nPage, sizeof(nPage));
		HMAC_Final(&hctx, hash_mac, &hash_len);
		HMAC_CTX_cleanup(&hctx);
		if (0 != memcmp(hash_mac, pTemp + DEFAULT_PAGESIZE - reserve + IV_SIZE, sizeof(hash_mac)))
		{
			return 2;
			//MessageBoxA(NULL, "ERROR", "hash错误", MB_OK);
		}
#endif
		//
		if (nPage == 1)
		{
			memcpy(pDecryptPerPageBuffer, SQLITE_FILE_HEADER, offset);
		}

		EVP_CIPHER_CTX* ectx = EVP_CIPHER_CTX_new();
		EVP_CipherInit_ex(ectx, EVP_get_cipherbyname("aes-256-cbc"), NULL, NULL, NULL, 0);
		EVP_CIPHER_CTX_set_padding(ectx, 0);
		EVP_CipherInit_ex(ectx, NULL, NULL, key, pTemp + (DEFAULT_PAGESIZE - reserve), 0);

		int nDecryptLen = 0;
		int nTotal = 0;
		EVP_CipherUpdate(ectx, pDecryptPerPageBuffer + offset, &nDecryptLen, pTemp + offset,
		                 DEFAULT_PAGESIZE - reserve - offset);
		nTotal = nDecryptLen;
		EVP_CipherFinal_ex(ectx, pDecryptPerPageBuffer + offset + nDecryptLen, &nDecryptLen);
		nTotal += nDecryptLen;
		EVP_CIPHER_CTX_free(ectx);

		memcpy(pDecryptPerPageBuffer + DEFAULT_PAGESIZE - reserve, pTemp + DEFAULT_PAGESIZE - reserve, reserve);
		char decFile[1024] = {0};
		sprintf_s(decFile, "%s\\decrypt_%s", outputDir, fileName);
		bool writeResult = writeToFile(decFile, pDecryptPerPageBuffer, 1, DEFAULT_PAGESIZE);
		if (!writeResult)
		{
			return 3;
		}

		nPage++;
		offset = 0;
		pTemp += DEFAULT_PAGESIZE;
	}
	//MessageBoxA(NULL, "decrypt db sucess", "解密成功", MB_OK);
	return 0;
}

int main(int argc, char* argv[])
{
	unsigned char key[16] = {
		0xF2, 0x27, 0x55, 0x35, 0x9B, 0xA0, 0x05, 0x44, 0x47, 0x57, 0x76, 0x23, 0x8A, 0x91, 0x58, 0x75
	};
	cout << decryptWeChatBackupDb("D:\\", "message.db", "D:\\WechatDecrypt", key, 16);
}
