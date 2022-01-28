#pragma once
#include "pch.h"
#include <string>
using namespace std;



void GetWcNameById(wchar_t* wxid, wchar_t* strname);

struct stCardXmlDate
{
	wchar_t* pstr;
	int ilen;
	int ilen2;
	int temp1; int temp2; int temp3; int temp4; int temp5; int temp6; int temp7;
	stCardXmlDate(wchar_t* id)
	{
		pstr = id;
		ilen = wcslen(pstr);
		ilen2 = ilen + 0x20;
		temp1 = temp2 = temp3 = temp4 = temp5 = temp6 = temp7 = 0;	
	}
};
struct stWeChatIdAndText
{
	wchar_t* pstr;
	int ilen;
	int ilenMax;
	int temp;
	int temp2;
	stWeChatIdAndText(wchar_t* date)
	{
		pstr = date;
		ilen = wcslen(pstr) + 1;
		ilenMax = ilen;
		temp = 0;
		temp2 = 0;	
	}

};

struct  ReceMsg
{
	DWORD dwType;
	bool bRoomMsg;
	wchar_t* WxId;
	wchar_t* WxText;
	wchar_t* imgPath;
	wchar_t* sendIdByRoom;
	wchar_t* srcText;
	wchar_t* sendIdName;
	wchar_t* roomIdName;
	ReceMsg(DWORD type, stWeChatIdAndText* id, stWeChatIdAndText* text,stWeChatIdAndText* img, stWeChatIdAndText* roomid, stWeChatIdAndText* src)
	{
		dwType = type;
		WxId = new wchar_t[id->ilen+1];
		memcpy(WxId, id->pstr,(id->ilen + 1)*2);
		wstring str(WxId);
		
		bRoomMsg = str.find(L"chatroom")!= wstring::npos ? true : false;


		//---------------------消息内容
		if (text->ilen !=0 && text->pstr !=0)
		{
			WxText= new wchar_t[text->ilen + 1];
			memcpy(WxText, text->pstr, (text->ilen + 1)*2);
		}
		else 
		{
			WxText = new wchar_t[5];
			wcscpy_s(WxText,5,L"NULL");
		}
		//---------------------图片路径
		if (img->ilen != 0 && img->pstr != 0)
		{
			imgPath = new wchar_t[img->ilen + 1];
			memcpy(imgPath, img->pstr, (img->ilen + 1) * 2);
		}
		else
		{
			imgPath = new wchar_t[5];
			wcscpy_s(imgPath, 5, L"NULL");
		}
		//---------------------群发送者ID
		if (roomid->ilen != 0 && roomid->pstr != 0)
		{
			sendIdByRoom = new wchar_t[roomid->ilen + 1];
			memcpy(sendIdByRoom, roomid->pstr, (roomid->ilen + 1) * 2);
		}
		else
		{
			sendIdByRoom = new wchar_t[5];
			wcscpy_s(sendIdByRoom, 5, L"NULL");
		}
		//---------------------原始消息
		/*if (src->ilen != 0 && src->pstr != 0)
		{
			srcText = new wchar_t[src->ilen + 1];
			memcpy(srcText, src->pstr, src->ilen + 1);
		}
		else
		{*/
			srcText = new wchar_t[5];
			wcscpy_s(srcText, 5, L"NULL");
		//}
			if (bRoomMsg)//判断是否是群消息  并获取用户名字；
			{
				wchar_t  roomname[100] = { 0 };
				GetWcNameById(WxId, roomname);
				roomIdName = new wchar_t[wcslen(roomname)];
				memcpy(roomIdName, roomname, (wcslen(roomname) + 1) * 2);

				wchar_t  idname[100] = { 0 };
				GetWcNameById(sendIdByRoom, idname);
				sendIdName = new wchar_t[wcslen(idname)];
				memcpy(sendIdName, idname, (wcslen(idname) + 1) * 2);

			}
			else
			{
				wchar_t  idname[100] = { 0 };
				GetWcNameById(WxId, idname);
				sendIdName = new wchar_t[wcslen(idname)];
				memcpy(sendIdName, idname, (wcslen(idname) + 1) * 2);

				roomIdName = new wchar_t[5];
				wcscpy_s(roomIdName, 5, L"NULL");
			}
	}


};

DWORD getWechatWinAddress();
void sendTextMsg(wchar_t* id, wchar_t* text);

void receiveStartHook();
void receiveStopHook();


 void ReceiveHookFunc();
void __stdcall getReceMsg(DWORD  pEcx);
void sendFriedCard(wchar_t* reID, wchar_t* strXml);

