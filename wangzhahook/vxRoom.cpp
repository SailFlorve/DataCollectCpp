#include "pch.h"

#include "vxRoom.h"

#include <vector>

#pragma  warning(disable:4996)



void getRoomFriend(wchar_t* roomid, vector<string>& date)
{//23462183857@chatroom
	stWeChatIdAndText id(roomid);
	DWORD call0 = getWechatWinAddress() + WxGetRoomFriendCallOff0;
	DWORD call1 = getWechatWinAddress() + WxGetRoomFriendCallOff1;
	DWORD call2 = getWechatWinAddress() + WxGetRoomFriendCallOff2;
	
	char buf[0x1500] = { 0 };

	__asm {
	
		mov edx, edx;
		mov edx, edx;
		mov edx, edx;
		pushad;

		lea ecx, buf;
		call call0;
		lea eax, buf;
		push eax;
		lea ebx, id;
		push ebx;
		call call1;
		mov ecx, eax;
		call call2;
		popad;

	}

	DWORD idLen = *((DWORD*)(buf + 0x2c));
	char * stdId = new char[idLen] {0};
	char * sourStr = (char *)(*((DWORD*)(buf + 0x1c)));
	memcpy(stdId, sourStr, idLen);
	const char *  strFlag= "^G";
	char* p = strtok(stdId, strFlag);

	while (p)
	{
		date.push_back(string(p));
		p = strtok(NULL, strFlag);
	}
	//delete[] stdId;
}
