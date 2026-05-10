// Tool 1: Switch to Changjie IME if it is not already the active input source.
//
// Uses: ITfInputProcessorProfileMgr (COM / Text Services Framework)

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return 1;

    int exitCode = 0;
    if (!IsChangjieIMEActive()) {
        hr = ActivateChangjieIME();
        if (FAILED(hr))
            exitCode = 1;
    }

    CoUninitialize();
    return exitCode;
}
