// Changjie English Watcher: System tray application that monitors and auto-switches
// from Changjie English mode to English US.
//
// This tool runs in the background with a system tray icon. It checks the IME state
// at a configurable interval (default: 1000ms) and switches to English US whenever
// it detects that Changjie is in English mode.
//
// The polling interval can be configured via command line argument (in milliseconds).
//
// Usage:
//   changjie-english-watcher.exe [interval_ms]
//
// Examples:
//   changjie-english-watcher.exe         - Use default interval (1000ms)
//   changjie-english-watcher.exe 500     - Check every 500ms
//   changjie-english-watcher.exe 2000    - Check every 2 seconds

#include "ime_common.h"
#include <shellapi.h>

// Define the system tray icon ID and messages
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ICON 1
#define ID_TRAY_EXIT 2001
#define WM_CHECK_IME (WM_USER + 2)

// Global variables
HINSTANCE g_hInstance = nullptr;
HWND g_hwndMain = nullptr;
NOTIFYICONDATAW g_nid = {};
UINT_PTR g_timerId = 0;
DWORD g_checkIntervalMs = 1000; // Default: check every 1 second
HANDLE g_hMutex = nullptr;

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateTrayIcon(HWND hwnd);
void RemoveTrayIcon();
void ShowContextMenu(HWND hwnd);
void CheckAndSwitchIME();
void UpdateTrayTooltip();


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int)
{
    g_hInstance = hInstance;

    // Create a named mutex to prevent multiple instances
    g_hMutex = CreateMutexW(nullptr, TRUE, L"ChangjieEnglishWatcher_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr,
            L"Changjie English Watcher is already running.\n\nOnly one instance can run at a time.",
            L"Already Running",
            MB_OK | MB_ICONINFORMATION);
        if (g_hMutex) {
            CloseHandle(g_hMutex);
        }
        return 1;
    }

    // Parse command-line argument for check interval
    if (lpCmdLine && *lpCmdLine) {
        // Skip leading whitespace
        while (*lpCmdLine == ' ' || *lpCmdLine == '\t') {
            lpCmdLine++;
        }

        // Try to parse as a number
        char* endPtr = nullptr;
        long value = strtol(lpCmdLine, &endPtr, 10);

        // If parsing succeeded and the value is valid, use it
        if (endPtr != lpCmdLine && value >= 100 && value <= 60000) {
            g_checkIntervalMs = static_cast<DWORD>(value);
        }
    }

    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        if (g_hMutex) {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex);
        }
        return 1;
    }

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ChangjieEnglishWatcherClass";

    if (!RegisterClassExW(&wc)) {
        CoUninitialize();
        if (g_hMutex) {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex);
        }
        return 1;
    }

    // Create a hidden window for message handling
    g_hwndMain = CreateWindowExW(
        0,
        L"ChangjieEnglishWatcherClass",
        L"Changjie English Watcher",
        WS_OVERLAPPED,  // Use WS_OVERLAPPED instead of 0 to ensure proper message handling
        0, 0, 0, 0,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_hwndMain) {
        CoUninitialize();
        if (g_hMutex) {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex);
        }
        return 1;
    }

    // Create system tray icon
    CreateTrayIcon(g_hwndMain);

    // Start the timer for periodic checking
    g_timerId = SetTimer(g_hwndMain, 1, g_checkIntervalMs, nullptr);
    if (!g_timerId) {
        MessageBoxW(nullptr, L"Failed to create timer", L"Error", MB_OK | MB_ICONERROR);
        RemoveTrayIcon();
        DestroyWindow(g_hwndMain);
        CoUninitialize();
        if (g_hMutex) {
            ReleaseMutex(g_hMutex);
            CloseHandle(g_hMutex);
        }
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    if (g_timerId) {
        KillTimer(g_hwndMain, g_timerId);
    }
    RemoveTrayIcon();
    CoUninitialize();

    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_TIMER:
            if (wParam == 1) {
                CheckAndSwitchIME();
                UpdateTrayTooltip();
            }
            return 0;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                ShowContextMenu(hwnd);
            }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                PostQuitMessage(0);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void CreateTrayIcon(HWND hwnd)
{
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = ID_TRAY_ICON;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"Changjie English Watcher");

    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

void ShowContextMenu(HWND hwnd)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    POINT pt;
    GetCursorPos(&pt);

    // Required for menu to close when clicking outside
    SetForegroundWindow(hwnd);

    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);

    DestroyMenu(hMenu);
}

void CheckAndSwitchIME()
{
    // Check if we're in Changjie English mode
    if (IsChangjieEnglishModeActive()) {
        // Switch to English US
        ActivateEnglishUS();
    }
}

void UpdateTrayTooltip()
{
    // Get current conversion mode for debugging
    DWORD convMode = GetCurrentConversionMode();
    DWORD sentMode = GetCurrentSentenceMode();

    // Build tooltip with diagnostic info
    wchar_t tooltip[128];
    swprintf_s(tooltip, L"Changjie Watcher | Conv:0x%08X Sent:0x%08X", convMode, sentMode);

    // Update tray icon tooltip
    wcscpy_s(g_nid.szTip, tooltip);
    g_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}
