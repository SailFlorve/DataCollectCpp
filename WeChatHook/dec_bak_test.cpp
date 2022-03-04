#include <string>
#include <unordered_map>
#include <unordered_set>
#include <strstream>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <Windows.h>
#include "AES.h"
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>
#include <cassert>
#include <ShlObj.h>
#include <fstream>

using namespace std;

#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libeay32.lib")

char fileStorage[] = "D:/WeChatDecrypt/";

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


unsigned char* CAES::aes_128_ecb_decrypt(const char* ciphertext, int text_size, const char* key, int key_size,
                                         int& size)
{
	EVP_CIPHER_CTX* ctx;
	ctx = EVP_CIPHER_CTX_new();
	int ret = EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, (const unsigned char*)key, NULL);
	EVP_CIPHER_CTX_set_padding(ctx, 0);
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

int decryptBackupFile(const char* backupPath, const char* backupFile, const char* outputDir, unsigned char* key,
                      int nKey)
{
	FILE* fpdb;
	string fileName;
	fileName += backupPath;
	fileName += "\\";
	fileName += backupFile;

	fopen_s(&fpdb, fileName.c_str(), "rb+");
	if (!fpdb)
	{
		//MessageBoxA(NULL, "ERROR", "打开文件错误", MB_OK);
		printf("DecryptBackupFile Open file error %s\n", fileName.c_str());
		return 1;
	}
	fseek(fpdb, 0, SEEK_END);
	long long nFileSize = ftell(fpdb);
	

	for (long long i = 0; i < nFileSize; i += 204800000L)
	{
		_fseeki64(fpdb, i, SEEK_SET);
		int mallocSize = 204800000;
		if (i + 204800000L > nFileSize)
		{
			mallocSize = nFileSize - i + 1L;
		}
		unsigned char* pDeBuffer = new unsigned char[mallocSize];
		fread(pDeBuffer, 1, mallocSize, fpdb);
		int size = 0;
		unsigned char* decrypt = CAES::aes_128_ecb_decrypt((const char*)pDeBuffer, mallocSize, (const char*)key, nKey,
		                                                   size);

		char decFile[1024] = {0};
		sprintf_s(decFile, "%s\\decrypt_%s", outputDir, backupFile);
		bool writeResult = writeToFile(decFile, decrypt, 1, size);
		if (!writeResult)
		{
			return 3;
		}
		delete[] pDeBuffer;
		delete[] decrypt;
	}


	fclose(fpdb);

	//MessageBoxA(NULL, "decrypt file sucess", "解密成功", MB_OK);
	return 0;
}

// 0 成功 1 打开的文件不存在 2 key错误 3 写入文件出错
int decryptWeChatBackupDb(const char* dbDir, const char* fileName, const char* outputDir, const unsigned char* pass,
	int nKey)
{
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

	unsigned char salt[16] = { 0 };
	memcpy(salt, pDbBuffer, 16);

#ifndef NO_USE_HMAC_SHA1
	unsigned char mac_salt[16] = { 0 };
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
	while (pTemp < pDbBuffer + nFileSize)
	{
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
		char decFile[1024] = { 0 };
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
	decryptWeChatBackupDb(
		R"(D:\Documents\WeChat Files\wxid_8rmcj0zs9itk22\BackupFiles\android_90c24abb4ed18109e5af20919b4cc64b\)",
		"Backup.db",
		R"(D:\WeChatDecrypt\wxid_8rmcj0zs9itk22)", (unsigned char*)"99c27b53665b225f114a7a08e811990e", 0x20);
}
