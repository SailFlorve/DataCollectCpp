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

    // 1. ����ϵͳ�еĽ��̣��ҵ�΢�Ž���
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
        callback(0, L"û���ҵ�΢��");
        return;
    }

    // 2.��΢�Ż�ȡhandle
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, wechatProcessId);
    if (hProcess == nullptr) {
        callback(0, L"��΢��ʧ��");
        return;
    }

    // 3.��΢�Ž���ΪDLL�ַ��������ڴ�ռ�
    LPVOID allocAddress = VirtualAllocEx(hProcess, nullptr, strSize, MEM_COMMIT, PAGE_READWRITE);
    if (allocAddress == nullptr) {
        callback(0, L"����ռ�ʧ��");
        return;
    }

    // 4.��DLL·��д�뵽������ڴ�
    BOOL result = WriteProcessMemory(hProcess, allocAddress, dllPath, strSize, nullptr);
    if (result == FALSE) {
        callback(0, L"д���ڴ�ʧ��");
        return;
    }

    // 5.��Kernel32.dll�л�ȡloadLibraryA�ĺ�����ַ
    HMODULE hMoudle = GetModuleHandle(L"Kernel32.dll");
    FARPROC farProc = GetProcAddress(hMoudle, "LoadLibraryA");

    if (farProc == nullptr) {
        callback(0, L"����ʧ��");
        return;
    }

    // 6. ��΢��������DLL �������߳̾��
    HANDLE newThreadHandle = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)farProc, allocAddress, 0, NULL);
    if (newThreadHandle == nullptr) {
        callback(0, L"����Զ���߳�ʧ��");
        return;
    }
    else {
        callback(1, L"ע��ɹ�");
    }

}

void injectL(const wchar_t* processName, const wchar_t* dllPath, void(*callback)(BOOL, const wchar_t*))
{
    size_t strSize = (wcslen(dllPath) + 1);

    DWORD processId = 0;

    setlocale(LC_ALL, "");
    printf("Inject:%ls, DLLPath:%s, StrSize:%zu\n", processName, dllPath, strSize);

    // 1. ����ϵͳ�еĽ��̣��ҵ�΢�Ž���
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
        callback(0, L"û���ҵ�����");
        return;
    }
    else {
        cout << processId << endl;
    }

    // 2.�򿪽��̻�ȡhandle
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == nullptr) {

        cout << "c01" << endl;
        callback(0, L"�򿪽���ʧ��");
        return;
    }
    // 3.�ڽ��̽���ΪDLL�ַ��������ڴ�ռ�
    LPVOID allocAddress = VirtualAllocEx(hProcess, nullptr, strSize, MEM_COMMIT, PAGE_READWRITE);
    if (allocAddress == nullptr) {
        callback(0, L"����ռ�ʧ��");
        return;
    }
    // 4.��DLL·��д�뵽������ڴ�
    BOOL result = WriteProcessMemory(hProcess, allocAddress, dllPath, strSize, nullptr);
    if (result == FALSE) {
        callback(0, L"д���ڴ�ʧ��");
        return;
    }
    // 5.��Kernel32.dll�л�ȡloadLibraryA�ĺ�����ַ
    HMODULE hMoudle = GetModuleHandle(L"kernel32.dll");
    FARPROC farProc = GetProcAddress(hMoudle, "LoadLibraryW");

    if (farProc == nullptr) {
        callback(0, L"����ʧ��");
        return;
    }
    // 6. �ڽ���������DLL �������߳̾��
    HANDLE newThreadHandle = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)farProc, allocAddress, 0, NULL);
    if (newThreadHandle == nullptr) {
        callback(0, L"����Զ���߳�ʧ��");
        return;
    }
    else {
        callback(1, L"ע��ɹ�");
    }
}

BOOL InjectDll(DWORD dwPID, LPCTSTR szDllPath)
{
    HANDLE                  hProcess = NULL;//����Ŀ����̵ľ��
    LPVOID                  pRemoteBuf = NULL;//Ŀ����̿��ٵ��ڴ����ʼ��ַ
    DWORD                   dwBufSize = (DWORD)(_tcslen(szDllPath) + 1) * sizeof(TCHAR);//���ٵ��ڴ�Ĵ�С
    LPTHREAD_START_ROUTINE  pThreadProc = NULL;//loadLibreayW��������ʼ��ַ
    HMODULE                 hMod = NULL;//kernel32.dllģ��ľ��
    BOOL                    bRet = FALSE;
    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))//��Ŀ����̣���þ��
    {
        _tprintf(L"InjectDll() : OpenProcess(%d) failed!!! [%d]\n",
            dwPID, GetLastError());
        goto INJECTDLL_EXIT;
    }
    pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize,
        MEM_COMMIT, PAGE_READWRITE);//��Ŀ����̿ռ俪��һ���ڴ�
    if (pRemoteBuf == NULL)
    {
        _tprintf(L"InjectDll() : VirtualAllocEx() failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    if (!WriteProcessMemory(hProcess, pRemoteBuf,
        (LPVOID)szDllPath, dwBufSize, NULL))//�򿪱ٵ��ڴ渴��dll��·��
    {
        _tprintf(L"InjectDll() : WriteProcessMemory() failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    hMod = GetModuleHandle(L"kernel32.dll");//��ñ�����kernel32.dll��ģ����
    if (hMod == NULL)
    {
        _tprintf(L"InjectDll() : GetModuleHandle(\"kernel32.dll\") failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryW");//���LoadLibraryW��������ʼ��ַ
    if (pThreadProc == NULL)
    {
        _tprintf(L"InjectDll() : GetProcAddress(\"LoadLibraryW\") failed!!! [%d]\n",
            GetLastError());
        goto INJECTDLL_EXIT;
    }
    if (!CreateRemoteThread(hProcess, NULL, 0, pThreadProc, pRemoteBuf, 0, NULL))//ִ��Զ���߳�
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
