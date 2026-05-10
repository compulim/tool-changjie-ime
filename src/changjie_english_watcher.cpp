// ChangJie English Mode Watcher
//
// This program runs in the background with a system tray icon and monitors
// the ChangJie IME state. When it detects that ChangJie IME is in English mode,
// it automatically switches to the English (US) keyboard layout.
//
// Features:
// - Configurable check interval (default: 1000ms, can be set via command-line)
// - System tray icon with Exit menu
// - Single-instance enforcement (prevents multiple copies from running)
// - Uses icon from %SystemRoot%\SYSTEM32\InputMethod\Shared\ResourceDll.dll index 3

#include "ime_common.h"
#include <shellapi.h>
#include <strsafe.h>

// Window class name for the hidden window
static const wchar_t* WINDOW_CLASS_NAME = L"ChangjieEnglishWatcherWindow";

// Mutex name for single-instance enforcement
static const wchar_t* MUTEX_NAME = L"Global\\ChangjieEnglishWatcherMutex";

// System tray notification ID
static const UINT TRAY_ID = 1;

// Custom window messages
static const UINT WM_TRAYICON = WM_USER + 1;

// Timer ID
static const UINT_PTR TIMER_ID = 1;

// Menu command IDs
static const UINT ID_EXIT = 1001;

// Default check interval in milliseconds
static const DWORD DEFAULT_CHECK_INTERVAL_MS = 1000;

// Global variables
static HINSTANCE g_hInstance = nullptr;
static HWND g_hwnd = nullptr;
static NOTIFYICONDATAW g_nid = {};
static DWORD g_checkIntervalMs = DEFAULT_CHECK_INTERVAL_MS;

// Parse check interval from command-line arguments
DWORD ParseCheckInterval(LPSTR lpCmdLine)
{
    if (!lpCmdLine || !*lpCmdLine) {
        return DEFAULT_CHECK_INTERVAL_MS;
    }

    // Skip leading whitespace
    while (*lpCmdLine == ' ' || *lpCmdLine == '\t') {
        lpCmdLine++;
    }

    // Try to parse as a number
    char* endPtr = nullptr;
    long value = strtol(lpCmdLine, &endPtr, 10);

    // If parsing succeeded and the value is valid (50ms to 60000ms), use it
    if (endPtr != lpCmdLine && value >= 50 && value <= 60000) {
        return static_cast<DWORD>(value);
    }

    // Otherwise, return the default
    return DEFAULT_CHECK_INTERVAL_MS;
}

// Load icon from ResourceDll.dll
HICON LoadResourceDllIcon()
{
    wchar_t systemPath[MAX_PATH];
    if (GetSystemDirectoryW(systemPath, MAX_PATH) == 0) {
        return nullptr;
    }

    wchar_t dllPath[MAX_PATH];
    StringCchPrintfW(dllPath, MAX_PATH, L"%s\\InputMethod\\Shared\\ResourceDll.dll", systemPath);

    // Extract icon at index 3
    HICON hIcon = nullptr;
    ExtractIconExW(dllPath, 3, nullptr, &hIcon, 1);
    return hIcon;
}

// Add system tray icon
bool AddTrayIcon(HWND hwnd)
{
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hwnd;
    g_nid.uID = TRAY_ID;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;

    // Load icon from ResourceDll.dll
    g_nid.hIcon = LoadResourceDllIcon();
    if (!g_nid.hIcon) {
        // Fallback to default application icon if loading fails
        g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    StringCchCopyW(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"ChangJie English Watcher");

    return Shell_NotifyIconW(NIM_ADD, &g_nid) != FALSE;
}

// Remove system tray icon
void RemoveTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_nid.hIcon) {
        DestroyIcon(g_nid.hIcon);
        g_nid.hIcon = nullptr;
    }
}

// Show context menu for tray icon
void ShowTrayMenu(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING, ID_EXIT, L"Exit");

    // Required for proper menu behavior with tray icons
    SetForegroundWindow(hwnd);

    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);

    DestroyMenu(hMenu);
}

// Timer callback - checks ChangJie IME state
void OnTimer(HWND hwnd)
{
    // Check if ChangJie IME is in English mode
    if (IsChangjieEnglishModeActive()) {
        // Switch to English US keyboard
        ActivateEnglishUS();
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            // Initialize COM for this window's thread
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);

            // Start the timer
            SetTimer(hwnd, TIMER_ID, g_checkIntervalMs, nullptr);

            // Add tray icon
            if (!AddTrayIcon(hwnd)) {
                MessageBoxW(hwnd, L"Failed to add system tray icon", L"Error", MB_OK | MB_ICONERROR);
                return -1;
            }
            break;

        case WM_TIMER:
            if (wParam == TIMER_ID) {
                OnTimer(hwnd);
            }
            break;

        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
                ShowTrayMenu(hwnd);
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_EXIT) {
                PostQuitMessage(0);
            }
            break;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ID);
            RemoveTrayIcon();
            CoUninitialize();
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Register window class
bool RegisterWindowClass()
{
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = g_hInstance;
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);

    return RegisterClassExW(&wcex) != 0;
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int)
{
    g_hInstance = hInstance;

    // Parse command-line arguments for check interval
    g_checkIntervalMs = ParseCheckInterval(lpCmdLine);

    // Single-instance enforcement using a named mutex
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, MUTEX_NAME);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is already running - just exit silently
        if (hMutex) {
            CloseHandle(hMutex);
        }
        return 0;
    }

    // Register window class
    if (!RegisterWindowClass()) {
        CloseHandle(hMutex);
        return 1;
    }

    // Create hidden window for message processing
    g_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"ChangJie English Watcher",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        g_hInstance,
        nullptr);

    if (!g_hwnd) {
        CloseHandle(hMutex);
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup
    CloseHandle(hMutex);
    return static_cast<int>(msg.wParam);
}
