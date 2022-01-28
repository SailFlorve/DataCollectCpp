#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include<stdlib.h>
#include <Windows.h>
#include <strstream>

#pragma comment(lib, "Version.lib")

using namespace std;

class FileUtil {
private:
	fstream fs;
	const char* fileName;

public:
	FileUtil(const char* fileName) {
		fs.open(fileName, ios::in | ios::out | ios::app);
		this->fileName = fileName;
	}

	void writeLine(const char* content) {
		fs.seekg(ios::end);
		fs << content << endl;
	}

	vector<string> readAll() {
		fs.seekg(0, ios::beg);
		string str;
		vector<string> result;
		while (getline(fs, str)) {
			result.push_back(str);
		}
		return result;
	}
};

//int main() {
//	const WCHAR* versionFilePath = L"C:/Program Files (x86)/Tencent/WeChat/WeChatWin.dll";
//
//	string version = "";
//	VS_FIXEDFILEINFO* pVsInfo;
//	unsigned int iFileInfoSize = sizeof(VS_FIXEDFILEINFO);
//	int iVerInfoSize = GetFileVersionInfoSize(versionFilePath, NULL);
//	if (iVerInfoSize != 0) {
//		char* pBuf = new char[iVerInfoSize];
//		if (GetFileVersionInfo(versionFilePath, 0, iVerInfoSize, pBuf)) {
//			if (VerQueryValue(pBuf, TEXT("\\"), (void**)&pVsInfo, &iFileInfoSize)) {
//				//主版本
//				//3
//				int marjorVersion = (pVsInfo->dwFileVersionMS >> 16) & 0x0000FFFF;
//				//4
//				int minorVersion = pVsInfo->dwFileVersionMS & 0x0000FFFF;
//				//0
//				int buildNum = (pVsInfo->dwFileVersionLS >> 16) & 0x0000FFFF;
//				//38
//				int revisionNum = pVsInfo->dwFileVersionLS & 0x0000FFFF;
//
//				//把版本变成字符串
//				strstream weChatVer;
//				weChatVer << marjorVersion << "." << minorVersion << "." << buildNum << "." << revisionNum;
//				weChatVer >> version;
//			}
//		}
//		delete[] pBuf;
//	}
//}