// Tool 6: Toggle between ChangJie IME in Chinese mode and English (US) keyboard layout.
//         When landing on ChangJie, also switch to Chinese (native) input mode.
//
// Uses: ITfInputProcessorProfileMgr (COM / TSF) for profile activation,
//       ImmGetDefaultIMEWnd + WM_IME_CONTROL/IMC_SETCONVERSIONMODE (IMM32) for mode change.

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return 1;

    int exitCode = 0;
    if (IsChangjieChineseModeActive()) {
        // Currently on ChangJie in Chinese mode — toggle to English (US).
        hr = ActivateEnglishUS();
        if (FAILED(hr))
            exitCode = 1;
    } else {
        // Currently on English (US) or ChangJie in English mode or any other source
        // — switch to ChangJie and ensure Chinese input mode is active.
        hr = ActivateChangjieIME();
        if (FAILED(hr)) {
            CoUninitialize();
            return 1;
        }
        Sleep(PROFILE_SWITCH_DELAY_MS); // Allow the foreground app to process the profile switch.
        hr = SetChineseMode();
        if (FAILED(hr))
            exitCode = 1;
    }

    CoUninitialize();
    return exitCode;
}
