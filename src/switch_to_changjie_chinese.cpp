// Tool 5: Switch to ChangJie IME (if not already active) and then switch to
//         Chinese (native) input mode.
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
    if (!IsChangjieIMEActive()) {
        hr = ActivateChangjieIME();
        if (FAILED(hr)) {
            CoUninitialize();
            return 1;
        }
        // Give the foreground application a moment to process the profile
        // change before we attempt to set the conversion mode.
        DWORD delay = ParseDelayArgument(lpCmdLine);
        Sleep(delay);
    }

    hr = SetChineseMode();
    if (FAILED(hr))
        exitCode = 1;

    CoUninitialize();
    return exitCode;
}
