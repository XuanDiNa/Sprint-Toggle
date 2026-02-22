#include <windows.h>
#include <cstdio>

#pragma comment(lib, "user32.lib")

namespace {

bool g_autoSprintEnabled = false;
bool g_autoCtrlHeld = false;
HANDLE g_thread = nullptr;
bool g_running = false;

DWORD WINAPI InputThread(LPVOID) {
    while (g_running) {
        bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        
        if (ctrlPressed && !g_autoCtrlHeld) {
            g_autoSprintEnabled = !g_autoSprintEnabled;
            g_autoCtrlHeld = true;
            char buf[256];
            sprintf_s(buf, "[AutoSprint] Mode: %s\n", g_autoSprintEnabled ? "ON" : "OFF");
            OutputDebugStringA(buf);
        }
        
        if (!ctrlPressed && g_autoCtrlHeld) {
            g_autoCtrlHeld = false;
        }
        
        if (g_autoSprintEnabled) {
            bool wPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
            
            if (wPressed && !g_autoCtrlHeld) {
                keybd_event(VK_CONTROL, 0, 0, 0);
                g_autoCtrlHeld = true;
            } else if (!wPressed && g_autoCtrlHeld) {
                keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                g_autoCtrlHeld = false;
            }
        }
        
        Sleep(10);
    }
    return 0;
}

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            OutputDebugStringA("[AutoSprint] Loaded! Press Ctrl to toggle\n");
            g_running = true;
            g_thread = CreateThread(nullptr, 0, InputThread, nullptr, 0, nullptr);
            break;
        case DLL_PROCESS_DETACH:
            g_running = false;
            if (g_thread) {
                WaitForSingleObject(g_thread, 1000);
                CloseHandle(g_thread);
            }
            break;
    }
    return TRUE;
}
