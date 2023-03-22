// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "wechat_bak.h"

#include "common_util.h"

using namespace std;

#define WxDatabaseKey  0x126DCC0  //数据库密钥基址
#define Wx_id 0x127F3C0           //id基址

#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libeay32.lib")

#if _MSC_VER>=1900
#include <cstdio>
_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned);
#ifdef __cplusplus
extern "C"
#endif
FILE* __cdecl __iob_func(unsigned i)
{
	return __acrt_iob_func(i);
}
#endif /* _MSC_VER>=1900 */


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

//pc端密码是经过OllyDbg得到的64位pass,是64位，不是网上传的32位，这里是个坑
char fileStorage[] = "D:/WeChatDecrypt/";

struct BackupKeyAddress
{
	DWORD backupKeyAddrFirstOffset;
	DWORD backupKeyAddrSecondOffset;
};

unordered_map<string, BackupKeyAddress> addrMap = {
	{"3.4.0.38", {0x01E26A20, 0x130}},
	{"3.4.5.27", {0x01EA802C, 0x130}},
	{"3.6.0.18", {0x02257b1c, 0x130}}
};

unsigned char* CAES::aes_128_ecb_decrypt(const char* ciphertext, int text_size, const char* key, int key_size,
                                         int& size)
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

int decryptBackupDb(const char* filename, unsigned char* pass, int nKey)
{
	FILE* fpdb;
	char dbfilename[1024];
	memset(dbfilename, 0, sizeof(dbfilename));
	memcpy(dbfilename, fileStorage, sizeof(fileStorage) - 1);
	memcpy(dbfilename + sizeof(fileStorage) - 1, filename, strlen(filename));
	memset(dbfilename + sizeof(fileStorage) - 1 + strlen(filename), 0, 1);
	fopen_s(&fpdb, dbfilename, "rb+");
	if (!fpdb)
	{
		MessageBoxA(NULL, "ERROR", "打开文件错误", MB_OK);
		return 0;
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
			MessageBoxA(NULL, "ERROR", "hash错误", MB_OK);
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
		sprintf_s(decFile, "%sdec_%s", fileStorage, filename);
		FILE* fp;
		fopen_s(&fp, decFile, "ab+");
		{
			fwrite(pDecryptPerPageBuffer, 1, DEFAULT_PAGESIZE, fp);
			fclose(fp);
		}

		nPage++;
		offset = 0;
		pTemp += DEFAULT_PAGESIZE;
	}
	OutputDebugStringA("解密成功");
	//MessageBoxA(NULL, "decrypt db sucess", "解密成功", MB_OK);
	return 0;
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
	if (!fpdb)
	{
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
	char decFile[1024] = {0};
	sprintf_s(decFile, "%sdec_%s", fileStorage, defilename);
	FILE* fp;
	fopen_s(&fp, decFile, "ab+");
	{
		fwrite(decrypt, 1, size, fp);
		fclose(fp);
	}
	//MessageBoxA(NULL, "decrypt file sucess", "解密成功", MB_OK);
	return 0;
}

char* Get_DB_Key(char* databasekey)
{
	memset(databasekey, 0, 0x20);
	//获取WeChatWin的基址
	DWORD dwKeyAddr = (DWORD)GetModuleHandle(L"WeChatWin.dll") + WxDatabaseKey;
	//MessageBoxA(NULL, "Step1", "获得密钥", MB_OK);
	LPVOID* pAddr = (LPVOID*)(*(DWORD*)dwKeyAddr);
	DWORD dwOldAttr = 0;
	VirtualProtect(pAddr, 0x20, PAGE_EXECUTE_READWRITE, &dwOldAttr);
	//MessageBoxA(NULL, "Step2", "获得密钥", MB_OK);
	memcpy(databasekey, pAddr, 0x20);
	//MessageBoxA(NULL, "Step3", "获得密钥", MB_OK);
	VirtualProtect(pAddr, 0x20, dwOldAttr, &dwOldAttr);
	//MessageBoxA(NULL, "Step4", "获得密钥", MB_OK);
	return databasekey;
}

char* Get_Wx_id(char* wxid)
{
	memset(wxid, 0, 20);
	//获取WeChatWin的基址
	DWORD dwKeyAddr = (DWORD)GetModuleHandle(L"WeChatWin.dll") + Wx_id;
	//MessageBoxA(NULL, "Step1", "获得密钥", MB_OK);
	LPVOID* pAddr = (LPVOID*)(*(DWORD*)dwKeyAddr);
	DWORD dwOldAttr = 0;
	VirtualProtect(pAddr, 19, PAGE_EXECUTE_READWRITE, &dwOldAttr);
	//MessageBoxA(NULL, "Step2", "获得密钥", MB_OK);
	memcpy(wxid, pAddr, 19);
	//MessageBoxA(NULL, "Step3", "获得密钥", MB_OK);
	VirtualProtect(pAddr, 19, dwOldAttr, &dwOldAttr);
	//MessageBoxA(NULL, "Step4", "获得密钥", MB_OK);
	MessageBoxA(NULL, wxid, "获得id", MB_OK);
	return wxid;
}

char* Get_BAK_Key(DWORD baseAddr, char* bakkey, BackupKeyAddress addr)
{
	DWORD BAK_keyaddrbase = addr.backupKeyAddrFirstOffset;
	DWORD BAK_offset = addr.backupKeyAddrSecondOffset;


	DWORD bak_f_Addr = baseAddr + BAK_keyaddrbase;
	DWORD* bak_f = (DWORD*)((*(DWORD*)bak_f_Addr) + BAK_offset);
	LPVOID* pAddr = (LPVOID*)(*bak_f);
	DWORD dwOldAttr = 0;
	printf("Get key addr: %p\n", pAddr);
	memcpy(bakkey, pAddr, 0x20);
	return bakkey;
}

int outputKey(char* BAK_Key)
{
	char change[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	string output_Key;
	for (int i = 0; i < 0x20; i++)
	{
		output_Key = output_Key + "0x";
		unsigned char high_4bit = (BAK_Key[i] & 0xf0) >> 4;
		unsigned char low_4bit = BAK_Key[i] & 0x0f;
		output_Key = output_Key + change[high_4bit];
		output_Key = output_Key + change[low_4bit];
		if (i == 0x20 - 1)
			break;
		output_Key = output_Key + ",";
	}
	MessageBoxA(NULL, output_Key.c_str(), "获得密钥", MB_OK);
	return 1;
}

string getKeyStr(char* BAK_Key)
{
	char change[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	string result;
	for (int i = 0; i < 0x20; i++)
	{
		string output_Key;
		unsigned char high_4bit = (BAK_Key[i] & 0xf0) >> 4;
		unsigned char low_4bit = BAK_Key[i] & 0x0f;
		output_Key = output_Key + change[high_4bit];
		output_Key = output_Key + change[low_4bit];
		char c = stoi(output_Key, 0, 16);
		result += c;
		if (i == 0x20 - 1)
			break;
	}
	return result;
}

// deprecated 
void Start()
{
	MessageBoxA(NULL, "ATTACH", "dll已注入", MB_OK);
	char BAK_Key[0x20] = {0};
	char DB_Key[0x20] = {0};
	char WX_id[20] = {0};
	//Get_BAK_Key(BAK_Key);
	//Get_DB_Key(DB_Key);
	//Get_Wx_id(WX_id);
	decryptBackupFile("BAK_0_TEXT", (unsigned char*)BAK_Key, 0x10);
	decryptBackupFile("BAK_0_MEDIA", (unsigned char*)BAK_Key, 0x10);
	decryptBackupDb("Backup.db", (unsigned char*)BAK_Key, 0x20);
	outputKey(BAK_Key);
	outputKey(DB_Key);
}

void startDectyptBak(DWORD baseAddr, string version)
{
	char BAK_Key[0x20] = {0};
	BackupKeyAddress addr = addrMap[version];
	Get_BAK_Key(baseAddr, BAK_Key, addr);
	decryptBackupFile("BAK_0_TEXT", (unsigned char*)BAK_Key, 0x10);
	decryptBackupFile("BAK_0_MEDIA", (unsigned char*)BAK_Key, 0x10);
	decryptBackupDb("Backup.db", (unsigned char*)BAK_Key, 0x20);
}

string getBackupKey(DWORD baseAddr, string version)
{
	outputLog("P-getBackupKey");
	char log[100];
	sprintf_s(log, "getBackupKey %ld ", baseAddr);
	OutputDebugStringA(log);
	char BAK_Key[0x20] = {0};
	BackupKeyAddress addr = addrMap[version];

	try
	{
		Get_BAK_Key(baseAddr, BAK_Key, addr);
	}
	catch (exception& e)
	{
		return "";
	}
	string keyStr = getKeyStr(BAK_Key);
	return keyStr;
}
