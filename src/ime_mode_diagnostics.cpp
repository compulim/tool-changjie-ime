// Diagnostic tool: Display detailed information about the current IME state.
// This tool helps diagnose issues with Changjie mode detection by showing
// multiple detection methods side-by-side.
//
// Outputs to a message box since this is a WIN32 application.

#include "ime_common.h"
#include <sstream>
#include <iomanip>


// Helper to convert GUID to string
std::wstring GuidToString(const GUID& guid)
{
    wchar_t buffer[64];
    swprintf_s(buffer, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
               guid.Data1, guid.Data2, guid.Data3,
               guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
               guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return buffer;
}

// Helper to format DWORD as hex
std::wstring DwordToHex(DWORD value)
{
    wchar_t buffer[32];
    swprintf_s(buffer, L"0x%08X", value);
    return buffer;
}

// Helper to decode conversion mode flags
std::wstring DecodeConversionMode(DWORD mode)
{
    if (mode == static_cast<DWORD>(-1)) {
        return L"(failed to get mode)";
    }

    std::wstringstream ss;
    ss << DwordToHex(mode) << L"\n";

    if (mode & IME_CMODE_ALPHANUMERIC) ss << L"        ALPHANUMERIC\n";
    if (mode & IME_CMODE_NATIVE) ss << L"        NATIVE (Chinese)\n";
    if (mode & IME_CMODE_KATAKANA) ss << L"        KATAKANA\n";
    if (mode & IME_CMODE_LANGUAGE) ss << L"        LANGUAGE\n";
    if (mode & IME_CMODE_FULLSHAPE) ss << L"        FULLSHAPE\n";
    if (mode & IME_CMODE_ROMAN) ss << L"        ROMAN\n";
    if (mode & IME_CMODE_CHARCODE) ss << L"        CHARCODE\n";
    if (mode & IME_CMODE_HANJACONVERT) ss << L"        HANJACONVERT\n";
    if (mode & IME_CMODE_SOFTKBD) ss << L"        SOFTKBD\n";
    if (mode & IME_CMODE_NOCONVERSION) ss << L"        NOCONVERSION\n";
    if (mode & IME_CMODE_EUDC) ss << L"        EUDC\n";
    if (mode & IME_CMODE_SYMBOL) ss << L"        SYMBOL\n";
    if (mode & IME_CMODE_FIXED) ss << L"        FIXED\n";

    return ss.str();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::wstringstream output;
    output << L"IME Mode Diagnostics\n";
    output << L"====================\n\n";

    // Get foreground window info
    HWND hwndForeground = GetForegroundWindow();
    if (hwndForeground) {
        wchar_t windowTitle[256] = {0};
        GetWindowTextW(hwndForeground, windowTitle, 256);
        output << L"Foreground Window: " << windowTitle << L"\n";
        output << L"HWND: 0x" << std::hex << std::setw(8) << std::setfill(L'0')
               << reinterpret_cast<UINT_PTR>(hwndForeground) << std::dec << L"\n\n";
    } else {
        output << L"No foreground window\n\n";
    }

    // Get keyboard layout info
    if (hwndForeground) {
        DWORD threadId = GetWindowThreadProcessId(hwndForeground, nullptr);
        HKL currentHKL = GetKeyboardLayout(threadId);
        LANGID currentLangID = LOWORD(currentHKL);

        output << L"Keyboard Layout (GetKeyboardLayout):\n";
        output << L"  HKL: 0x" << std::hex << std::setw(8) << std::setfill(L'0')
               << reinterpret_cast<UINT_PTR>(currentHKL) << std::dec << L"\n";
        output << L"  LANGID: 0x" << std::hex << std::setw(4) << std::setfill(L'0')
               << currentLangID << std::dec;

        if (currentLangID == 0x0404) {
            output << L" (zh-TW)\n";
        } else if (currentLangID == 0x0409) {
            output << L" (en-US)\n";
        } else {
            output << L"\n";
        }
        output << L"\n";
    }

    // Get TSF active profile
    TF_INPUTPROCESSORPROFILE profile = {};
    hr = GetActiveProfile(&profile);
    if (SUCCEEDED(hr)) {
        output << L"TSF Active Profile:\n";
        output << L"  Type: ";
        if (profile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR) {
            output << L"INPUTPROCESSOR\n";
        } else if (profile.dwProfileType == TF_PROFILETYPE_KEYBOARDLAYOUT) {
            output << L"KEYBOARDLAYOUT\n";
        } else {
            output << L"Unknown (" << profile.dwProfileType << L")\n";
        }

        output << L"  CLSID: " << GuidToString(profile.clsid) << L"\n";
        output << L"  Profile GUID: " << GuidToString(profile.guidProfile) << L"\n";
        output << L"  LANGID: 0x" << std::hex << std::setw(4) << std::setfill(L'0')
               << profile.langid << std::dec;

        if (profile.langid == 0x0404) {
            output << L" (zh-TW)\n";
        } else if (profile.langid == 0x0409) {
            output << L" (en-US)\n";
        } else {
            output << L"\n";
        }

        // Check if it matches Changjie
        if (IsEqualCLSID(profile.clsid, CLSID_ChangjieIME)) {
            output << L"  ** This is the Changjie IME **\n";
        }
        output << L"\n";
    } else {
        output << L"TSF Active Profile: Failed to get (HRESULT: 0x"
               << std::hex << hr << std::dec << L")\n\n";
    }

    // Get IME window info
    if (hwndForeground) {
        HWND imeWnd = ImmGetDefaultIMEWnd(hwndForeground);
        output << L"IME Window (ImmGetDefaultIMEWnd): ";
        if (imeWnd) {
            output << L"0x" << std::hex << std::setw(8) << std::setfill(L'0')
                   << reinterpret_cast<UINT_PTR>(imeWnd) << std::dec << L"\n\n";
        } else {
            output << L"NULL\n\n";
        }
    }

    // Get IME context info
    if (hwndForeground) {
        HIMC himc = ImmGetContext(hwndForeground);
        output << L"IME Context (ImmGetContext): ";
        if (himc) {
            output << L"0x" << std::hex << std::setw(8) << std::setfill(L'0')
                   << reinterpret_cast<UINT_PTR>(himc) << std::dec << L"\n";

            // Get additional raw context information
            DWORD convMode = 0, sentMode = 0;
            if (ImmGetConversionStatus(himc, &convMode, &sentMode)) {
                output << L"  Raw Conversion Mode (via ImmGetConversionStatus): " << DwordToHex(convMode) << L"\n";
                output << L"  Raw Sentence Mode (via ImmGetConversionStatus): " << DwordToHex(sentMode) << L"\n";
            } else {
                output << L"  ImmGetConversionStatus failed\n";
            }

            ImmReleaseContext(hwndForeground, himc);
        } else {
            output << L"NULL (ImmGetContext not available - this is normal for modern TSF-based IMEs)\n";
        }
        output << L"\n";
    }

    // Get IME open status
    BOOL imeOpen = GetImeOpenStatus();
    output << L"IME Open Status (ImmGetOpenStatus): " << (imeOpen ? L"OPEN" : L"CLOSED");
    output << L" (unreliable for TSF-based IMEs)\n\n";

    // Get conversion mode via WM_IME_CONTROL
    DWORD mode1 = GetCurrentConversionMode();
    output << L"Conversion Mode (WM_IME_CONTROL - Primary Method):\n";
    output << L"  " << DecodeConversionMode(mode1);
    output << L"\n";

    // Get conversion mode via ImmGetConversionStatus
    DWORD mode2 = GetConversionModeViaImmContext();
    output << L"Conversion Mode (ImmGetConversionStatus - Fallback, often fails on TSF IMEs):\n";
    output << L"  " << DecodeConversionMode(mode2);
    output << L"\n";

    // Comparison
    if (mode1 == mode2 && mode1 != static_cast<DWORD>(-1)) {
        output << L"Both methods agree.\n\n";
    } else if (mode1 != static_cast<DWORD>(-1) && mode2 != static_cast<DWORD>(-1)) {
        output << L"WARNING: Methods disagree!\n\n";
    } else if (mode1 != static_cast<DWORD>(-1) && mode2 == static_cast<DWORD>(-1)) {
        output << L"WM_IME_CONTROL works, ImmGetConversionStatus failed (expected for TSF IMEs).\n\n";
    } else {
        output << L"Both methods failed.\n\n";
    }

    // High-level detection results
    output << L"Detection Results:\n";
    output << L"  IsChangjieIMEActive(): " << (IsChangjieIMEActive() ? L"TRUE" : L"FALSE") << L"\n";
    output << L"  IsEnglishUSActive(): " << (IsEnglishUSActive() ? L"TRUE" : L"FALSE") << L"\n";
    output << L"  IsChangjieChineseModeActive(): " << (IsChangjieChineseModeActive() ? L"TRUE" : L"FALSE") << L"\n";
    output << L"  IsChangjieEnglishModeActive(): " << (IsChangjieEnglishModeActive() ? L"TRUE" : L"FALSE") << L"\n";

    CoUninitialize();

    // Show the output in a message box with a monospace font-friendly layout
    MessageBoxW(nullptr, output.str().c_str(), L"IME Mode Diagnostics", MB_OK | MB_ICONINFORMATION);
    return 0;
}
