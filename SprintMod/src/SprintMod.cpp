#include <windows.h>
#include <cstdio>

#pragma comment(lib, "user32.lib")

namespace {

bool g_autoSprintEnabled = false;
bool g_autoCtrlHeld = false;
HANDLE g_thread = nullptr;
bool g_running = false;

bool IsMinecraftActive() {
    char title[256];
    HWND hwnd = GetForegroundWindow();
    GetWindowTextA(hwnd, title, sizeof(title));
    return strstr(title, "Minecraft") != nullptr;
}

bool IsInGame() {
    HWND hwnd = GetForegroundWindow();
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    
    if (strstr(title, "Minecraft") == nullptr) return false;
    if (strstr(title, "Play") != nullptr) return false;
    if (strstr(title, "Settings") != nullptr) return false;
    if (strstr(title, "Options") != nullptr) return false;
    if (strstr(title, "Chat") != nullptr) return false;
    if (strstr(title, "World") != nullptr) return false;
    if (strstr(title, "Server") != nullptr) return false;
    if (strstr(title, "Menu") != nullptr) return false;
    
    return true;
}

DWORD WINAPI InputThread(LPVOID) {
    while (g_running) {
        if (IsInGame()) {
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
