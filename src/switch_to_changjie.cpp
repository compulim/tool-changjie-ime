// Tool 1: Switch to Changjie IME if it is not already the active input source.
//
// Uses: ITfInputProcessorProfileMgr (COM / Text Services Framework)

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!IsChangjieIMEActive())
        ActivateChangjieIME();

    CoUninitialize();
    return 0;
}
