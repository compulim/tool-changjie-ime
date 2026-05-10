// Tool 4: Switch to English (US) keyboard layout if it is not already active.
//
// Uses: ITfInputProcessorProfileMgr (COM / Text Services Framework)

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return 1;

    int exitCode = 0;
    if (!IsEnglishUSActive()) {
        hr = ActivateEnglishUS();
        if (FAILED(hr))
            exitCode = 1;
        // Brief delay to allow the profile switch to complete.
        Sleep(PROFILE_SWITCH_DELAY_MS);
    }

    CoUninitialize();
    return exitCode;
}
