// Tool 4: Switch to English (US) keyboard layout if it is not already active.
//
// Uses: ITfInputProcessorProfileMgr (COM / Text Services Framework)

#include "ime_common.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!IsEnglishUSActive())
        ActivateEnglishUS();

    CoUninitialize();
    return 0;
}
