// Tool 6: Toggle between ChangJie IME and English (US) keyboard layout.
//         When landing on ChangJie, also switch to Chinese (native) input mode.
//
// Uses: ITfInputProcessorProfileMgr (COM / TSF) for profile activation,
//       ImmGetDefaultIMEWnd + WM_IME_CONTROL/IMC_SETCONVERSIONMODE (IMM32) for mode change.

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (IsChangjieIMEActive()) {
        // Currently on ChangJie — toggle to English (US).
        ActivateEnglishUS();
    } else {
        // Currently on English (US) or any other source — switch to ChangJie
        // and ensure Chinese input mode is active.
        ActivateChangjieIME();
        Sleep(PROFILE_SWITCH_DELAY_MS); // Allow the foreground app to process the profile switch.
        SetChineseMode();
    }

    CoUninitialize();
    return 0;
}
