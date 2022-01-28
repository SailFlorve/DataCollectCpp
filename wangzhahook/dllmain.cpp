// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "resource.h"
#include <vector>


wchar_t stdid[20] = { 0 };
wchar_t stdtext[100] = { 0 };




VOID ShowDemoUI(HMODULE hModule);


INT_PTR CALLBACK DialogProc(_In_ HWND   hwndDlg, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
VOID UnInject();

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
         CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShowDemoUI, hModule, NULL, 0);
     
        break;
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

VOID ShowDemoUI(HMODULE hModule)
{


    //获取WeChatWin.dll的基址
     
    if ((DWORD)GetModuleHandle(L"WeChatWin.dll") == 0)
    {
        FreeLibrary(hModule);
        return;
    }

    //启动窗口
    DialogBox(hModule, MAKEINTRESOURCE(IDD_MAIN), NULL, &DialogProc);
}
INT_PTR CALLBACK DialogProc(_In_ HWND   hwndDlg, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
   
        break;
    }
    case WM_CLOSE:
    {
        //关闭窗口事件
        UnInject();
        EndDialog(hwndDlg, 0);
        break;
    }
    case WM_COMMAND:
    {
       if (wParam == IDC_BUTTON_TEST)
        {
           
           
           GetDlgItemText(hwndDlg, IDC_EDIT_ID, stdid, 20);
           GetDlgItemText(hwndDlg, IDC_EDIT_TEXT, stdtext, 100);

           sendTextMsg(stdid, stdtext);
           break;
        }
       if (wParam == IDC_BUTTON_RECEMSGHOOK_START)
       {
           receiveStartHook();
           break;
       }
	   if (wParam == IDC_BUTTON_RECEMSGHOOK_STOP)
	   {
		   receiveStopHook();
		   break;
	   }
	   if (wParam == IDC_BUTTON_GETNAMEBYID)
	   {
          
           wchar_t id[MAX_PATH] = { 0 };
           wchar_t name[MAX_PATH] = { 0 };
           GetDlgItemText(hwndDlg, IDC_EDIT_ID, id, MAX_PATH);        
           GetWcNameById(id, name);       
          SetDlgItemText(hwndDlg, IDC_EDIT_WXNAME,name);
		   break;
	   }
	   if (wParam == IDC_BUTTON_SENDIMG)
	   {
		   wchar_t id[MAX_PATH] = { 0 };	
           wchar_t path[MAX_PATH] = { 0 };
           GetDlgItemText(hwndDlg, IDC_EDIT_ID, id, MAX_PATH);
           GetDlgItemText(hwndDlg, IDC_EDIT_IMGPATH, path, MAX_PATH);
           SendImg(id, path);
		   break;
	   }
	   if (wParam == IDC_BUTTON_GETROOMFRIEND)
	   {
		   wchar_t id[MAX_PATH] = { 0 };
		   wchar_t path[MAX_PATH] = { 0 };
		   GetDlgItemText(hwndDlg, IDC_EDIT_ID, id, MAX_PATH);
           vector<string> date;
           getRoomFriend(id, date);
          
           for (int idx=0;idx<date.size();idx++)
           {
               OutputDebugStringA(date.at(idx).c_str());             
           }

		   break;
	   }
	   if (wParam == IDC_BUTTON_GETSQLDBSTART)
	   {
           getSqlDBHandleHookStart();	
		   break;
	   }
	   if (wParam == IDC_BUTTON_GETSQLDBSTOP)
	   {
           
           getSqlDBHandleHookStop();
		   break;
	   }
	   if (wParam == IDC_BUTTON_RUNSQLEXEC)
	   {
		   char sqltext[0x30] = { 0 };
		   DWORD db = GetDlgItemInt(hwndDlg, IDC_EDIT_SQLDB, NULL, false);
           GetDlgItemTextA(hwndDlg, IDC_EDIT_SQLTEXT, sqltext, 30);
           runSqlExec(db, sqltext);

		   break;
	   }
	   if (wParam == IDC_BUTTON_SENDCARD)
	   {
		   wchar_t id[MAX_PATH] = { 0 };
		   wchar_t xml[0x800] = { 0 };
		   GetDlgItemText(hwndDlg, IDC_EDIT_ID, id, MAX_PATH);
		   GetDlgItemText(hwndDlg, IDC_EDIT_CARDXML, xml, 0x800);
           sendFriedCard(id, xml);
		   break;
	   }
	   if (wParam == IDC_BUTTON_REPAIRVER)
	   {
		   wchar_t ver[MAX_PATH] = { 0 };
		   GetDlgItemText(hwndDlg, IDC_EDIT_VER, ver, MAX_PATH);
           
           repairVer(ver);

		   break;
	   }
       
        break;
    }
    default:
        break;
    }
    return FALSE;
}
VOID UnInject()
{
    HMODULE hModule = NULL;

 
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPWSTR)&UnInject, &hModule);

    if (hModule != 0)
    {


        //减少一次引用计数
        FreeLibrary(hModule);
        //从内存中卸载
        FreeLibraryAndExitThread(hModule, 0);
    }
}