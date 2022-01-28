#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <cstring>

using namespace std;

class ProcessMemoryReader {
private:
    DWORD processId = 0;
    HANDLE handle = nullptr;
    BYTE *baseAddress = nullptr;

    void getProcess(const wchar_t *processName) {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 processEntry32 = {0};
        processEntry32.dwSize = sizeof(processEntry32);

        BOOL next = Process32Next(hSnap, &processEntry32);
        while (next == TRUE) {
            if (wcscmp(processEntry32.szExeFile, processName) == 0) {
                processId = processEntry32.th32ProcessID;
                break;
            }
            next = Process32Next(hSnap, &processEntry32);
        }

        if (processId == 0) {
            cout << "没有找到微信" << endl;
            return;
        } else {
            cout << "找到微信 " << processId << endl;
        }
    }

    void openProcess() {
        handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, processId);
        if (handle == nullptr) {
            cout << "打开微信失败" << endl;
        }
    }

    void getProcessModuleHandle(const wchar_t *moduleName) {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
        MODULEENTRY32 moduleEntry32 = {0};
        moduleEntry32.dwSize = sizeof(moduleEntry32);

        while (Module32Next(hSnap, &moduleEntry32)) {
            if (wcscmp(moduleEntry32.szModule, moduleName) == 0) {
                baseAddress = moduleEntry32.modBaseAddr;
            }
        }
    }

public:
    explicit ProcessMemoryReader(const wchar_t *processName, const wchar_t *moduleName) {
        getProcess(processName);
        openProcess();
        getProcessModuleHandle(moduleName);
    }

    LPVOID readMemory(int offset, int size = 100) {
        return readMemory(baseAddress, offset, size);
    }

    LPVOID readMemory(BYTE *base, int offset, int size) {
        void *buf = malloc(sizeof(char) * size);
        ReadProcessMemory(handle, base + offset, buf, size, nullptr);
        return buf;
    }

    LPVOID readPtrMemory(int offset, int size = 100) {
        int *address = static_cast<int *>(readMemory(baseAddress, offset, 4));
        void *buf = readMemory(nullptr, *address, size);
        return buf;
    }
};

const int OFFSET_USERNAME = 0x1E23D5C;
const int OFFSET_PTR_WX_ID = 0X1E23CE4;

//int main() {
//    ProcessMemoryReader reader = ProcessMemoryReader(L"WeChat.exe", L"WeChatWin.dll");
//    char *username = static_cast<char *>(reader.readMemory(OFFSET_USERNAME));
//    char *weChatId = static_cast<char *>(reader.readPtrMemory(OFFSET_PTR_WX_ID));
//
//    cout << "Username: " << username << "\nWeChatID: " << weChatId << endl;
//}
