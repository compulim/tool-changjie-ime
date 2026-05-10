// Tool 6: Toggle between ChangJie IME and English (US) keyboard layout.
//         When landing on ChangJie, also switch to Chinese (native) input mode.
//
// Uses: ITfInputProcessorProfileMgr (COM / TSF) for profile activation,
//       ImmGetDefaultIMEWnd + WM_IME_CONTROL/IMC_SETCONVERSIONMODE (IMM32) for mode change.

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return 1;

    int exitCode = 0;
    if (IsChangjieIMEActive()) {
        // Currently on ChangJie (in either Chinese or English mode) — toggle to English (US).
        hr = ActivateEnglishUS();
        if (FAILED(hr))
            exitCode = 1;
    } else {
        // Currently on English (US) or any other source — switch to ChangJie
        // and ensure Chinese input mode is active.
        hr = ActivateChangjieIME();
        if (FAILED(hr)) {
            CoUninitialize();
            return 1;
        }
        DWORD delay = ParseDelayArgument(lpCmdLine);
        Sleep(delay); // Allow the foreground app to process the profile switch.
        hr = SetChineseMode();
        if (FAILED(hr))
            exitCode = 1;
    }

    CoUninitialize();
    return exitCode;
}
