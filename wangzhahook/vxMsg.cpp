#include "pch.h"



Chook reMsgHook;

DWORD gReMsgCallAddress = getWechatWinAddress() + WxReceiveMsgOldCalladdress;
DWORD gReMsgJmpNextAddress = getWechatWinAddress() + WxReceiveMsgHOOKaddress+5;



DWORD getWechatWinAddress()
{
	
	return (DWORD)GetModuleHandle(L"WeChatWin.dll");
}

void sendTextMsg(wchar_t* id, wchar_t* text)
{
	DWORD call1 = getWechatWinAddress() + WxSendTextMsgOffset;
	stWeChatIdAndText sid(id);
	stWeChatIdAndText stext(text);
	char buf[0x34] = { 0 };
	char buf2[0x600] = { 0 };

	__asm {	
		push 1;
		lea eax, buf;
		push eax;
		lea edi, stext;
		push edi;
		lea edx, sid;
		lea ecx, buf2;
		call call1;
		add esp, 0xC;		
	}
}





void receiveStartHook()
{
	reMsgHook.HookReady(getWechatWinAddress() + WxReceiveMsgHOOKaddress, ReceiveHookFunc);
	reMsgHook.StartHook();
}

void receiveStopHook()
{
	reMsgHook.StopHook();
}

_declspec(naked) void ReceiveHookFunc()
{

	__asm {
		pushad;
		mov ecx, [ecx];
		push ecx;
		call getReceMsg;
		popad;
		call gReMsgCallAddress;
		jmp gReMsgJmpNextAddress;
	

	}

}

void __stdcall getReceMsg(DWORD pEcx )
{
	try
	{

	
	DWORD type = *((DWORD*)(pEcx + 0x38));
	stWeChatIdAndText* temp = (stWeChatIdAndText*)(pEcx + 0x48);

	ReceMsg* msg = new  ReceMsg(type, (stWeChatIdAndText*)(pEcx+0x48), (stWeChatIdAndText*)(pEcx + 0x70), 
								(stWeChatIdAndText*)(pEcx + 0x1ac),(stWeChatIdAndText*)(pEcx + 0x170), 
								(stWeChatIdAndText*)(pEcx + 0x1d8));

	wchar_t str[0x500] = { 0 };
	wsprintf(str, L"vx:dwType=%d,WxId:%s.sendIdName:%s.WxText:%s,imgPath:%s.sendIdByRoom:%s.roomIdName:%s.srcText:%s\r\n",
		msg->dwType, msg->WxId, msg->sendIdName,msg->WxText, msg->imgPath, msg->sendIdByRoom,msg->roomIdName, msg->srcText);

	if (msg->dwType == 3)//是否是图片消息
	{
		int itime = 0;
		wstring str(WxRootPath);
		str += msg->imgPath;
		while (!MakeImg((wchar_t *)str.c_str()))
		{
			if (itime > 5)break;		
			Sleep(500);
			itime++;
		}		
	}




	OutputDebugString(str);


	delete msg;
		msg = NULL;
	}
	catch (...)
	{
		OutputDebugString(L"vx:接收消息处理函数异常！");
	}


}



void GetWcNameById(wchar_t* wxid, wchar_t* strname)
{

	
	stWeChatIdAndText id(wxid);
	DWORD call1 = getWechatWinAddress() + WxGetNameByIdCallOff1;
	DWORD call2 = getWechatWinAddress() + WxGetNameByIdCallOff2;
	DWORD call3 = getWechatWinAddress() + WxGetNameByIdCallOff3;
	char buf[0x1000] = { 0 };
	DWORD  pedi = 0;

	__asm {
		mov ecx, ecx; 
		mov ecx, ecx;
		
		lea edi, buf;
		push edi;
		sub esp, 0x14;
		lea eax, id;
		mov ecx, esp;
		push eax;
		call call1;
		
		call call2;
		
		call call3;
		mov pedi, edi;
		
	}
	wchar_t* str = (wchar_t*)(*((DWORD*)(pedi + 0x6c)));
	if (str==0)
	{
		wcscpy_s(strname, 5, L"NULL");
	}
	else
	{
		int len = wcslen(str) * 2;
		memcpy(strname, (wchar_t*)(*((DWORD*)(pedi + 0x6c))), len);
	}	
}



void sendFriedCard(wchar_t* reID ,wchar_t * strXml)
{

	DWORD call1 = getWechatWinAddress() + WxSendFriendCardCallOff;

	stCardXmlDate xml(strXml);
	stWeChatIdAndText id(reID);
	char buf[0x800] = { 0 };

	__asm {
		pushad;
		push 0x2A;
		lea eax, xml;
		lea esi, id;
		mov edx, esi;
		mov ebx, 0;
		push ebx;
		push eax;
		lea ecx, buf;
		call call1;
		add esp, 0xC;
		popad;
	
	}


}