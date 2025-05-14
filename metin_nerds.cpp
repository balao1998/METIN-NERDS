#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iostream>

struct HandleData {
    DWORD pid;
    HWND hwnd;
};

BOOL CALLBACK EnumWindowsCallback(HWND hWnd, LPARAM lParam) {
    HandleData& data = *(HandleData*)lParam;
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hWnd, &windowPid);
    if (windowPid == data.pid && IsWindowVisible(hWnd)) {
        data.hwnd = hWnd;
        return FALSE; // Found, stop enumerating
    }
    return TRUE;
}

HWND FindWindowByProcessName(LPCTSTR processName, bool verbose = true) {
    HWND hwnd = NULL;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    bool processFound = false;

    if (verbose) {
        std::wcout << L"Searching for process: " << processName << std::endl;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error: Could not create process snapshot" << std::endl;
        return NULL;
    }

    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_tcsicmp(pe32.szExeFile, processName) == 0) {
                processFound = true;
                if (verbose) {
                    std::wcout << L"  Found process: " << pe32.szExeFile 
                               << L" (PID: " << pe32.th32ProcessID << L")" << std::endl;
                }

                HandleData data = { pe32.th32ProcessID, NULL };
                EnumWindows(EnumWindowsCallback, (LPARAM)&data);
                hwnd = data.hwnd;

                if (hwnd && verbose) {
                    TCHAR windowTitle[256];
                    GetWindowText(hwnd, windowTitle, 256);
                    std::wcout << L"  Found visible window: \"" 
                               << windowTitle << L"\" (HWND: " 
                               << hwnd << L")" << std::endl;
                }

                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    if (verbose) {
        if (!processFound) {
            std::wcout << L"  Process not found: " << processName << std::endl;
        } else if (!hwnd) {
            std::wcout << L"  Process found but no visible window detected" << std::endl;
        }
    }

    return hwnd;
}


void SendSpaceToWindow(HWND hwnd, int count = 1, DWORD delay = 500) {
    if (!hwnd) {
        std::wcout << L"Cannot send keys - no target window" << std::endl;
        return;
    }

    std::wcout << L"Sending " << count << L" space keys to window (HWND: "
               << hwnd << L")" << std::endl;

    MessageBox(NULL, L"Click OK to start sending space keys", L"Foreground Permission", MB_OK);

    if (!SetForegroundWindow(hwnd)) {
        std::wcerr << L"Warning: Failed to bring window to foreground" << std::endl;
    }
    Sleep(100);

    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    for (int i = 0; i < count; i++) {
        input.ki.wVk = VK_SPACE;
        input.ki.dwFlags = 0;
        SendInput(1, &input, sizeof(INPUT));

        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));

        std::wcout << L"  Sent space key (" << i + 1 << L"/" << count << L")" << std::endl;

        if (i < count - 1) {
            Sleep(delay);
        }
    }
}

int _tmain(int argc, TCHAR* argv[]) {
    LPCTSTR processName = TEXT("notepad.exe");
    int count = 5;
    DWORD delay = 500;

    if (argc > 1) processName = argv[1];
    if (argc > 2) count = _ttoi(argv[2]);
    if (argc > 3) delay = _ttoi(argv[3]);

    std::wcout << L"\n=== Space Key Sender ===" << std::endl;
    std::wcout << L"Target process: " << processName << std::endl;
    std::wcout << L"Key count: " << count << std::endl;
    std::wcout << L"Delay: " << delay << L" ms\n" << std::endl;

    

    HWND hwnd = FindWindowByProcessName(processName);
    SendSpaceToWindow(hwnd, count, delay);

    std::wcout << L"\nOperation completed" << std::endl;
    return 0;
}
