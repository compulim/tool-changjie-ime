// Tool 5: Switch to ChangJie IME (if not already active) and then switch to
//         Chinese (native) input mode.
//
// Uses: ITfInputProcessorProfileMgr (COM / TSF) for profile activation,
//       ImmGetDefaultIMEWnd + WM_IME_CONTROL/IMC_SETCONVERSIONMODE (IMM32) for mode change.

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!IsChangjieIMEActive()) {
        ActivateChangjieIME();
        // Give the foreground application a moment to process the profile
        // change before we attempt to set the conversion mode.
        Sleep(PROFILE_SWITCH_DELAY_MS);
    }

    SetChineseMode();

    CoUninitialize();
    return 0;
}
