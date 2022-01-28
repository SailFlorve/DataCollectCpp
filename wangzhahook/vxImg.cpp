#include "pch.h"
#include "vxImg.h"

#include <fstream>
#include <string>
#include <vector>


using namespace std;



bool MakeImg(wchar_t* filepath)
{
	ifstream ifile(filepath, ifstream::binary);

	if (!ifile.is_open())
	{
		return false;
	}

	ifile.seekg(0);

	vector<UINT>    vbyte;
	wstring    str(filepath);
	int pos = str.find(L".dat");
	wstring refilename = str.substr(0, pos);

	while (!ifile.eof())
	{
		char b;
		ifile.get(b);
		vbyte.push_back(b);
	}

	ifile.clear();
	ifile.close();

	char temp1 = vbyte.at(0);
	char temp2 = vbyte.at(1);
	char key = 0;

	if ((temp1 ^ 0xff) == (temp2 ^ 0xd8))	//jpeg
	{
		key = temp1 ^ 0xff;
		for (size_t idx = 0; idx < vbyte.size(); idx++)
		{
			vbyte.at(idx) ^= key;
		}
		refilename += L".jpg";

	}

	if ((temp1 ^ 0x89) == (temp2 ^ 0x50))	//png
	{
		key = temp1 ^ 0x89;
		for (size_t idx = 0; idx < vbyte.size(); idx++)
		{
			vbyte.at(idx) ^= key;
		}
		refilename += L".png";

	}
	if ((temp1 ^ 0x42) == (temp2 ^ 0x4d))	//bmp
	{
		key = temp1 ^ 0x42;
		for (size_t idx = 0; idx < vbyte.size(); idx++)
		{
			vbyte.at(idx) ^= key;
		}
		refilename += L".bmp";

	}

	ofstream ofile(refilename, ofstream::binary);


	for (size_t idx = 0; idx < vbyte.size(); idx++)
	{

		ofile.put(vbyte.at(idx));
	}

	ofile.close();

	return true;
}

void SendImg(wchar_t* id, wchar_t* imgpath)
{
	stWeChatIdAndText wxid(id);
	stWeChatIdAndText wxpath(imgpath);

	DWORD call1 = getWechatWinAddress() + WxSendImgCallOff1;
	DWORD call2 = getWechatWinAddress() + WxSendImgCallOff2;
	DWORD call3 = getWechatWinAddress() + WxSendImgCallOff3;


	char buf[0x500] = { 0 };
	char buf2[0x500] = { 0 };


	__asm {
		pushad;
		mov edx, edx;
		mov edx, edx;
		mov edx, edx;

		lea edi, wxpath;
		sub esp, 0x14;
		lea eax, buf;
		mov ecx, esp;
		mov dword ptr ss : [ebp - 0x48] , esp;
		push eax;
		call call1;
		push edi;
		lea eax, wxid;
		push eax;
		lea eax, buf2;
		push eax;
		call call2;
		mov ecx, eax;
		call call3;
		popad;
	}


}


