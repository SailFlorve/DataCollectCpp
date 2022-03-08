#pragma once
#include <openssl/aes.h>
#include <string>
class CAES
{
public:

	static unsigned char* aes_128_ecb_decrypt(const char* ciphertext, int text_size, const char* key, int key_size, int& size);
};
int decryptBackupFile(const char* filename, unsigned char* key, int nKey);
int decryptBackupDb(const char* dbfilename, unsigned char* pass, int nKey);
char out_s[1024];