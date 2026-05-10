// Diagnostic tool: Display detailed information about the current IME state.
// This tool helps diagnose issues with Changjie mode detection by showing
// multiple detection methods side-by-side.
//
// Outputs to a message box since this is a WIN32 application.

#include "ime_common.h"
#include <sstream>
#include <iomanip>

// Helper to format DWORD as hex
std::wstring DwordToHex(DWORD value)
{
    wchar_t buffer[32];
    swprintf_s(buffer, L"0x%08X", value);
    return buffer;
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
        output << L"Window: " << windowTitle << L"\n\n";
    } else {
        output << L"ERROR: No foreground window\n\n";
    }

    // Get TSF active profile
    TF_INPUTPROCESSORPROFILE profile = {};
    hr = GetActiveProfile(&profile);
    if (SUCCEEDED(hr)) {
        output << L"Active Profile: ";
        if (profile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR) {
            output << L"IME";
        } else if (profile.dwProfileType == TF_PROFILETYPE_KEYBOARDLAYOUT) {
            output << L"Keyboard";
        }
        output << L" (LANGID: 0x" << std::hex << std::setw(4) << std::setfill(L'0')
               << profile.langid << std::dec << L")";
        if (IsEqualCLSID(profile.clsid, CLSID_ChangjieIME)) {
            output << L" [Changjie]";
        }
        output << L"\n\n";
    } else {
        output << L"ERROR: GetActiveProfile failed (HRESULT: 0x"
               << std::hex << hr << std::dec << L")\n\n";
    }

    // WM_IME_CONTROL method
    output << L"WM_IME_CONTROL Method:\n";
    HWND imeWnd = hwndForeground ? ImmGetDefaultIMEWnd(hwndForeground) : nullptr;
    if (!hwndForeground) {
        output << L"  ERROR: No foreground window\n";
    } else if (!imeWnd) {
        output << L"  ERROR: ImmGetDefaultIMEWnd returned NULL\n";
    } else {
        DWORD modeWMI = GetCurrentConversionMode();
        DWORD sentWMI = GetCurrentSentenceMode();

        if (modeWMI == static_cast<DWORD>(-1)) {
            output << L"  Conversion: FAILED\n";
        } else {
            output << L"  Conversion: " << DwordToHex(modeWMI);
            if (modeWMI & IME_CMODE_NATIVE) {
                output << L" [NATIVE]";
            }
            output << L"\n";
        }

        if (sentWMI == static_cast<DWORD>(-1)) {
            output << L"  Sentence:   FAILED\n";
        } else {
            output << L"  Sentence:   " << DwordToHex(sentWMI) << L"\n";
        }
    }
    output << L"\n";

    // ImmGetContext method
    output << L"ImmGetContext Method:\n";
    if (!hwndForeground) {
        output << L"  ERROR: No foreground window\n";
    } else {
        HIMC himc = ImmGetContext(hwndForeground);
        if (!himc) {
            output << L"  ERROR: ImmGetContext returned NULL\n";
            output << L"  (This is normal for TSF-based IMEs)\n";
        } else {
            DWORD convMode = 0, sentMode = 0;
            if (ImmGetConversionStatus(himc, &convMode, &sentMode)) {
                output << L"  Conversion: " << DwordToHex(convMode);
                if (convMode & IME_CMODE_NATIVE) {
                    output << L" [NATIVE]";
                }
                output << L"\n  Sentence:   " << DwordToHex(sentMode) << L"\n";
            } else {
                output << L"  ERROR: ImmGetConversionStatus failed\n";
            }
            ImmReleaseContext(hwndForeground, himc);
        }
    }
    output << L"\n";

    // Detection results
    output << L"Detection:\n";
    output << L"  Changjie Active:   " << (IsChangjieIMEActive() ? L"YES" : L"NO") << L"\n";
    output << L"  Chinese Mode:      " << (IsChangjieChineseModeActive() ? L"YES" : L"NO") << L"\n";
    output << L"  English Mode:      " << (IsChangjieEnglishModeActive() ? L"YES" : L"NO") << L"\n";
    output << L"  English US Active: " << (IsEnglishUSActive() ? L"YES" : L"NO") << L"\n";

    CoUninitialize();

    // Show the output in a message box with a monospace font-friendly layout
    MessageBoxW(nullptr, output.str().c_str(), L"IME Mode Diagnostics", MB_OK | MB_ICONINFORMATION);
    return 0;
}
