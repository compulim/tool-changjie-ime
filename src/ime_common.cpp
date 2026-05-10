#include "ime_common.h"

// ---------------------------------------------------------------------------
// Microsoft ChangJie IME GUIDs
// ---------------------------------------------------------------------------

// CLSID: {A76C93D9-5523-4E90-AAFA-4DB112F9AC76}
const CLSID CLSID_ChangjieIME = {
    0xA76C93D9, 0x5523, 0x4E90,
    { 0xAA, 0xFA, 0x4D, 0xB1, 0x12, 0xF9, 0xAC, 0x76 }
};

// Profile GUID for ChangJie input: {B115690A-EA02-48D5-A231-E3578D2FDF80}
const GUID GUID_ChangjieProfile = {
    0xB115690A, 0xEA02, 0x48D5,
    { 0xA2, 0x31, 0xE3, 0x57, 0x8D, 0x2F, 0xDF, 0x80 }
};

// ---------------------------------------------------------------------------
// Profile queries
// ---------------------------------------------------------------------------

HRESULT GetActiveProfile(TF_INPUTPROCESSORPROFILE* pProfile)
{
    if (!pProfile) return E_POINTER;

    ITfInputProcessorProfileMgr* pProfileMgr = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&pProfileMgr));
    if (FAILED(hr)) return hr;

    hr = pProfileMgr->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, pProfile);
    pProfileMgr->Release();
    return hr;
}

bool IsChangjieIMEActive()
{
    TF_INPUTPROCESSORPROFILE profile = {};
    if (FAILED(GetActiveProfile(&profile))) return false;
    return (profile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR)
        && IsEqualCLSID(profile.clsid, CLSID_ChangjieIME);
}

bool IsEnglishUSActive()
{
    TF_INPUTPROCESSORPROFILE profile = {};
    if (FAILED(GetActiveProfile(&profile))) return false;
    return (profile.dwProfileType == TF_PROFILETYPE_KEYBOARDLAYOUT)
        && (profile.langid == LANGID_EnglishUS);
}

// ---------------------------------------------------------------------------
// Profile activation
// ---------------------------------------------------------------------------

HRESULT ActivateChangjieIME()
{
    ITfInputProcessorProfileMgr* pProfileMgr = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&pProfileMgr));
    if (FAILED(hr)) return hr;

    hr = pProfileMgr->ActivateProfile(
        TF_PROFILETYPE_INPUTPROCESSOR,
        LANGID_TraditionalChinese,
        CLSID_ChangjieIME,
        GUID_ChangjieProfile,
        nullptr,
        TF_IPPMF_FORSESSION);

    pProfileMgr->Release();
    return hr;
}

HRESULT ActivateEnglishUS()
{
    // Resolve the HKL for the US English keyboard layout.
    // "00000409" is the standard layout identifier for en-US.
    HKL hklEnUS = LoadKeyboardLayoutW(LAYOUT_ID_ENUS, 0);
    if (!hklEnUS) {
        // Fallback to the well-known HKL value for en-US.
        hklEnUS = reinterpret_cast<HKL>(static_cast<ULONG_PTR>(0x04090409));
    }

    ITfInputProcessorProfileMgr* pProfileMgr = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfileMgr,
        reinterpret_cast<void**>(&pProfileMgr));
    if (FAILED(hr)) return hr;

    hr = pProfileMgr->ActivateProfile(
        TF_PROFILETYPE_KEYBOARDLAYOUT,
        LANGID_EnglishUS,
        CLSID_NULL,
        GUID_NULL,
        hklEnUS,
        TF_IPPMF_FORSESSION);

    pProfileMgr->Release();
    return hr;
}

// ---------------------------------------------------------------------------
// Conversion-mode helpers (IMM32 DLL via WM_IME_CONTROL messages)
//
// We obtain the IME window for the current foreground window via
// ImmGetDefaultIMEWnd() (imm32.dll), then send WM_IME_CONTROL messages to
// it.  This works cross-process because SendMessage delivers to any window.
// ---------------------------------------------------------------------------

DWORD GetCurrentConversionMode()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return static_cast<DWORD>(-1);

    HWND imeWnd = ImmGetDefaultIMEWnd(hwnd);
    if (!imeWnd) return static_cast<DWORD>(-1);

    LRESULT mode = SendMessage(imeWnd, WM_IME_CONTROL, IMC_GETCONVERSIONMODE, 0);
    return static_cast<DWORD>(mode);
}

HRESULT SetConversionMode(DWORD mode)
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return E_FAIL;

    HWND imeWnd = ImmGetDefaultIMEWnd(hwnd);
    if (!imeWnd) return E_FAIL;

    SendMessage(imeWnd, WM_IME_CONTROL, IMC_SETCONVERSIONMODE,
                static_cast<LPARAM>(mode));
    return S_OK;
}

HRESULT SetChineseMode()
{
    DWORD mode = GetCurrentConversionMode();
    if (mode == static_cast<DWORD>(-1)) return E_FAIL;
    return SetConversionMode(mode | IME_CMODE_NATIVE);
}

HRESULT SetEnglishMode()
{
    DWORD mode = GetCurrentConversionMode();
    if (mode == static_cast<DWORD>(-1)) return E_FAIL;
    return SetConversionMode(mode & ~static_cast<DWORD>(IME_CMODE_NATIVE));
}
