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

int decryptBackupFile(const char* defilename, unsigned char* key, int nKey)
{
	FILE* fpdb;
	char filename[1024];
	memset(filename, 0, sizeof(filename));
	memcpy(filename, fileStorage, sizeof(fileStorage) - 1);
	memcpy(filename + sizeof(fileStorage) - 1, defilename, strlen(defilename));
	memset(filename + sizeof(fileStorage) - 1 + strlen(defilename), 0, 1);
	fopen_s(&fpdb, filename, "rb+");
	if (!fpdb) {
		MessageBoxA(NULL, "ERROR", "打开文件错误", MB_OK);
		return 0;
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
	sprintf_s(decFile, "%sdec_%s", fileStorage, defilename);
	FILE* fp;
	fopen_s(&fp, decFile, "ab+");
	{
		fwrite(decrypt, 1, size, fp);
		fclose(fp);
	}
	MessageBoxA(NULL, "decrypt file sucess", "解密成功", MB_OK);
	return 0;
}

//int main() {
//	const char* key = "c7313dcbec5f4145bed13ed51e134bb2";
//
//	decryptBackupFile("BAK_0_TEXT", (unsigned char*)key, 0x10);
//}