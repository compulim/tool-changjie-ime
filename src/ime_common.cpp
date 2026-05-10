#include "ime_common.h"

// ---------------------------------------------------------------------------
// Microsoft ChangJie IME GUIDs
// ---------------------------------------------------------------------------

// CLSID: {4BDF9F03-C7D3-11D4-B2AB-0080C882687E}
const CLSID CLSID_ChangjieIME = {
    0x4BDF9F03, 0xC7D3, 0x11D4,
    { 0xB2, 0xAB, 0x00, 0x80, 0xC8, 0x82, 0x68, 0x7E }
};

// Profile GUID for ChangJie input: {531fdebf-9b4c-4a43-a2aa-960e8fcdc732}
const GUID GUID_ChangjieProfile = {
    0x531FDEBF, 0x9B4C, 0x4A43,
    { 0xA2, 0xAA, 0x96, 0x0E, 0x8F, 0xCD, 0xC7, 0x32 }
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
