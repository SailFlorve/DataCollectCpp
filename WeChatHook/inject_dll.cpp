#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <io.h>
#include <tchar.h>

using namespace std;


void injectDll(const wchar_t* processName, const char* dllPath, void(*callback)(BOOL, const wchar_t*))
{
    size_t strSize = strlen(dllPath) + 1;

    DWORD wechatProcessId = 0;

    // 1. 遍历系统中的进程，找到微信进程
    HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 processEntry32 = { 0 };
    processEntry32.dwSize = sizeof(processEntry32);

    BOOL next = Process32Next(handle, &processEntry32);
    while (next == TRUE) {
        if (wcscmp(processEntry32.szExeFile, processName) == 0) {
            wechatProcessId = processEntry32.th32ProcessID;
            break;
        }
        next = Process32Next(handle, &processEntry32);
    }

    if (wechatProcessId == 0) {
        callback(0, L"没有找到微信");
        return;
    }

    // 2.打开微信获取handle
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, wechatProcessId);
    if (hProcess == nullptr) {
        callback(0, L"打开微信失败");
        return;
    }

    // 3.在微信进程为DLL字符串申请内存空间
    LPVOID allocAddress = VirtualAllocEx(hProcess, nullptr, strSize, MEM_COMMIT, PAGE_READWRITE);
    if (allocAddress == nullptr) {
        callback(0, L"分配空间失败");
        return;
    }

    // 4.把DLL路径写入到申请的内存
    BOOL result = WriteProcessMemory(hProcess, allocAddress, dllPath, strSize, nullptr);
    if (result == FALSE) {
        callback(0, L"写入内存失败");
        return;
    }

    // 5.从Kernel32.dll中获取loadLibraryA的函数地址
    HMODULE hMoudle = GetModuleHandle(L"Kernel32.dll");
    FARPROC farProc = GetProcAddress(hMoudle, "LoadLibraryA");

    if (farProc == nullptr) {
        callback(0, L"查找失败");
        return;
    }

    // 6. 在微信中启动DLL 返回新线程句柄
    HANDLE newThreadHandle = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)farProc, allocAddress, 0, NULL);
    if (newThreadHandle == nullptr) {
        callback(0, L"创建远程线程失败");
        return;
    }
    else {
        callback(1, L"注入成功");
    }

}

void injectL(const wchar_t* processName, const wchar_t* dllPath, void(*callback)(BOOL, const wchar_t*))
{
    size_t strSize = (wcslen(dllPath) + 1);

    DWORD processId = 0;

    setlocale(LC_ALL, "");
    printf("Inject:%ls, DLLPath:%s, StrSize:%zu\n", processName, dllPath, strSize);

    // 1. 遍历系统中的进程，找到微信进程
    HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 processEntry32 = { 0 };
    processEntry32.dwSize = sizeof(processEntry32);

    BOOL next = Process32Next(handle, &processEntry32);
    while (next == TRUE) {
        if (wcscmp(processEntry32.szExeFile, processName) == 0) {
            processId = processEntry32.th32ProcessID;
            break;
        }
        next = Process32Next(handle, &processEntry32);
    }

    if (processId == 0) {
        callback(0, L"没有找到进程");
        return;
    }
    else {
        cout << processId << endl;
    }

    // 2.打开进程获取handle
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == nullptr) {

        cout << "c01" << endl;
        callback(0, L"打开进程失败");
        return;
    }
    // 3.在进程进程为DLL字符串申请内存空间
    LPVOID allocAddress = VirtualAllocEx(hProcess, nullptr, strSize, MEM_COMMIT, PAGE_READWRITE);
    if (allocAddress == nullptr) {
        callback(0, L"分配空间失败");
        return;
    }
    // 4.把DLL路径写入到申请的内存
    BOOL result = WriteProcessMemory(hProcess, allocAddress, dllPath, strSize, nullptr);
    if (result == FALSE) {
        callback(0, L"写入内存失败");
        return;
    }
    // 5.从Kernel32.dll中获取loadLibraryA的函数地址
    HMODULE hMoudle = GetModuleHandle(L"kernel32.dll");
    FARPROC farProc = GetProcAddress(hMoudle, "LoadLibraryW");

    if (farProc == nullptr) {
        callback(0, L"查找失败");
        return;
    }
    // 6. 在进程中启动DLL 返回新线程句柄
    HANDLE newThreadHandle = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)farProc, allocAddress, 0, NULL);
    if (newThreadHandle == nullptr) {
        callback(0, L"创建远程线程失败");
        return;
    }
    else {
        callback(1, L"注入成功");
    }
}

BOOL InjectDll(DWORD dwPID, LPCTSTR szDllPath)
{
    HANDLE                  hProcess = NULL;//保存目标进程的句柄
    LPVOID                  pRemoteBuf = NULL;//目标进程开辟的内存的起始地址
    DWORD                   dwBufSize = (DWORD)(_tcslen(szDllPath) + 1) * sizeof(TCHAR);//开辟的内存的大小
    LPTHREAD_START_ROUTINE  pThreadProc = NULL;//loadLibreayW函数的起始地址
    HMODULE                 hMod = NULL;//kernel32.dll模块的句柄
    BOOL                    bRet = FALSE;
    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))//打开目标进程，获得句柄
    {
        _tprintf(L"InjectDll() : OpenProcess(%d) failed!!! [%d]\n",
            dwPID, GetLastError());
        goto INJECTDLL_EXIT;
    }
    pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize,
        MEM_COMMIT, PAGE_READWRITE);//在目标进程空间开辟一块内存
    if (pRemoteBuf == NULL)
    {
        _tprintf(L"InjectDll() : VirtualAllocEx() failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    if (!WriteProcessMemory(hProcess, pRemoteBuf,
        (LPVOID)szDllPath, dwBufSize, NULL))//向开辟的内存复制dll的路径
    {
        _tprintf(L"InjectDll() : WriteProcessMemory() failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    hMod = GetModuleHandle(L"kernel32.dll");//获得本进程kernel32.dll的模块句柄
    if (hMod == NULL)
    {
        _tprintf(L"InjectDll() : GetModuleHandle(\"kernel32.dll\") failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryW");//获得LoadLibraryW函数的起始地址
    if (pThreadProc == NULL)
    {
        _tprintf(L"InjectDll() : GetProcAddress(\"LoadLibraryW\") failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    if (!CreateRemoteThread(hProcess, NULL, 0, pThreadProc, pRemoteBuf, 0, NULL))//执行远程线程
    {
        _tprintf(L"InjectDll() : MyCreateRemoteThread() failed!!!\n");
        goto INJECTDLL_EXIT;
    }
INJECTDLL_EXIT:
    return FALSE;
}

void f(BOOL res, const wchar_t* msg) {
    cout << res << endl;
}

int main() {
    injectL(L"WeChat.exe", L"D:\\Projects\\VSProjects\\WeChatHook\\Debug\\WeChatHookDLL.dll", f);
    // InjectDll(48420, L"D:\\Projects\\VSProjects\\WeChatHook\\Debug\\WeChatHookDLL.dll");

}
