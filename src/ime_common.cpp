#include "ime_common.h"

// ---------------------------------------------------------------------------
// Microsoft ChangJie IME GUIDs
//
// NOTE: These GUIDs may vary between Windows versions and system configurations.
// If the tools are not working (no-op behavior), the GUIDs may be incorrect for
// your system. To find the correct GUIDs for your installation:
//
// 1. Open Registry Editor (regedit.exe)
// 2. Navigate to: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\CTF\TIP
// 3. Look for an entry with "ChangJie" or "倉頡" in the description
// 4. Note the CLSID (the GUID under TIP)
// 5. Under that CLSID, navigate to: LanguageProfile\0404 (zh-TW)
// 6. Note the Profile GUID
//
// Update the GUIDs below to match your system.
// ---------------------------------------------------------------------------

// CLSID: {531FDEBF-9B4C-4A43-A2AA-960E8FCDC732}
const CLSID CLSID_ChangjieIME = {
    0x531FDEBF, 0x9B4C, 0x4A43,
    { 0xA2, 0xAA, 0x96, 0x0E, 0x8F, 0xCD, 0xC7, 0x32 }
};

// Profile GUID for ChangJie input: {4BDF9F03-C7D3-11D4-B2AB-0080C882687E}
const GUID GUID_ChangjieProfile = {
    0x4BDF9F03, 0xC7D3, 0x11D4,
    { 0xB2, 0xAB, 0x00, 0x80, 0xC8, 0x82, 0x68, 0x7E }
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
    // First check the foreground window's actual keyboard layout
    // This is the most reliable indicator of what the user will type
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        DWORD threadId = GetWindowThreadProcessId(hwnd, nullptr);
        HKL currentHKL = GetKeyboardLayout(threadId);
        LANGID currentLangID = LOWORD(currentHKL);

        // If the foreground window has English US, we're done
        if (currentLangID == LANGID_EnglishUS) {
            return true;
        }
    }

    // Fallback: check TSF profile (might be out of sync with foreground window)
    TF_INPUTPROCESSORPROFILE profile = {};
    if (FAILED(GetActiveProfile(&profile))) return false;

    return (profile.dwProfileType == TF_PROFILETYPE_KEYBOARDLAYOUT)
        && (profile.langid == LANGID_EnglishUS);
}

bool IsChangjieChineseModeActive()
{
    if (!IsChangjieIMEActive()) return false;

    // When the IME is in English mode (not open), it's not in Chinese mode
    // The open status indicates whether the IME is actively accepting native input
    BOOL imeOpen = GetImeOpenStatus();
    if (!imeOpen) return false;

    // Try to get conversion mode via WM_IME_CONTROL first (primary method)
    DWORD mode = GetCurrentConversionMode();

    // If that fails or returns an unexpected value, try the fallback method
    if (mode == static_cast<DWORD>(-1) || mode == 0) {
        mode = GetConversionModeViaImmContext();
    }

    if (mode == static_cast<DWORD>(-1)) return false;

    // Check if IME_CMODE_NATIVE is set (indicates Chinese character input mode)
    return (mode & IME_CMODE_NATIVE) != 0;
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

    // Use TSF to activate the profile session-wide first
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

    // Also send WM_INPUTLANGCHANGEREQUEST to the foreground window to ensure immediate effect
    HWND hwndForeground = GetForegroundWindow();
    if (hwndForeground) {
        // Post the message asynchronously to avoid blocking
        PostMessageW(hwndForeground, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(hklEnUS));
    }

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

DWORD GetConversionModeViaImmContext()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return static_cast<DWORD>(-1);

    HIMC himc = ImmGetContext(hwnd);
    if (!himc) return static_cast<DWORD>(-1);

    DWORD conversionMode = 0;
    DWORD sentenceMode = 0;
    BOOL result = ImmGetConversionStatus(himc, &conversionMode, &sentenceMode);

    ImmReleaseContext(hwnd, himc);

    return result ? conversionMode : static_cast<DWORD>(-1);
}

BOOL GetImeOpenStatus()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return FALSE;

    HIMC himc = ImmGetContext(hwnd);
    if (!himc) return FALSE;

    BOOL isOpen = ImmGetOpenStatus(himc);

    ImmReleaseContext(hwnd, himc);

    return isOpen;
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

// ---------------------------------------------------------------------------
// Command-line argument parsing
// ---------------------------------------------------------------------------

DWORD ParseDelayArgument(LPSTR lpCmdLine)
{
    if (!lpCmdLine || !*lpCmdLine) {
        return PROFILE_SWITCH_DELAY_MS;
    }

    // Skip leading whitespace
    while (*lpCmdLine == ' ' || *lpCmdLine == '\t') {
        lpCmdLine++;
    }

    // Try to parse as a number
    char* endPtr = nullptr;
    long value = strtol(lpCmdLine, &endPtr, 10);

    // If parsing succeeded and the value is valid, use it
    if (endPtr != lpCmdLine && value >= 0 && value <= 10000) {
        return static_cast<DWORD>(value);
    }

    // Otherwise, return the default
    return PROFILE_SWITCH_DELAY_MS;
}
