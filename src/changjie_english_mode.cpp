// Tool 3: If the active IME is ChangJie, switch it to English (alphanumeric) input mode.
//
// Uses: ITfInputProcessorProfileMgr (COM / TSF) for detection,
//       ImmGetDefaultIMEWnd + WM_IME_CONTROL/IMC_SETCONVERSIONMODE (IMM32) for mode change.

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return 1;

    int exitCode = 0;
    if (IsChangjieIMEActive()) {
        hr = SetEnglishMode();
        if (FAILED(hr))
            exitCode = 1;
    }

    CoUninitialize();
    return exitCode;
}
