#pragma once

// Require Windows 10 or later for full TSF support
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <msctf.h>
#include <imm.h>

// Define IMM32 WM_IME_CONTROL message constants if not already defined
#ifndef IMC_GETCONVERSIONMODE
#define IMC_GETCONVERSIONMODE 1
#endif
#ifndef IMC_SETCONVERSIONMODE
#define IMC_SETCONVERSIONMODE 2
#endif
#ifndef IMC_GETSENTENCEMODE
#define IMC_GETSENTENCEMODE 3
#endif

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "imm32.lib")

// ---------------------------------------------------------------------------
// Microsoft ChangJie IME (Traditional Chinese, zh-TW) GUIDs
//
// These GUIDs identify the Microsoft ChangJie (倉頡) input method installed
// as part of the Windows Traditional Chinese language pack.
//
// To look them up on your machine:
//   HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\CTF\TIP\{CLSID}
//     \LanguageProfile\{langid}\{ProfileGUID}
//
// CLSID: {531FDEBF-9B4C-4A43-A2AA-960E8FCDC732}
// Profile GUID (ChangJie): {4BDF9F03-C7D3-11D4-B2AB-0080C882687E}
// ---------------------------------------------------------------------------
extern const CLSID CLSID_ChangjieIME;
extern const GUID  GUID_ChangjieProfile;

static const LANGID LANGID_TraditionalChinese = 0x0404; // zh-TW
static const LANGID LANGID_EnglishUS          = 0x0409; // en-US

// Windows keyboard layout identifier string for the US English keyboard.
// Used with LoadKeyboardLayoutW(); defined in the system registry under
// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Keyboard Layouts.
static const wchar_t* LAYOUT_ID_ENUS = L"00000409";

// Milliseconds to wait after requesting a TSF profile switch before sending
// WM_IME_CONTROL to the foreground window.  The profile switch is
// asynchronous; the brief delay lets the target application process the
// WM_INPUTLANGCHANGE notification before we set the conversion mode.
static const DWORD PROFILE_SWITCH_DELAY_MS = 20;

// Parse the delay value from command-line arguments. If a numeric argument
// is provided, use it; otherwise, return the default PROFILE_SWITCH_DELAY_MS.
DWORD ParseDelayArgument(LPSTR lpCmdLine);

// ---------------------------------------------------------------------------
// Profile queries (use ITfInputProcessorProfileMgr via COM)
// ---------------------------------------------------------------------------

// Fill *pProfile with the currently active input processor profile.
HRESULT GetActiveProfile(TF_INPUTPROCESSORPROFILE* pProfile);

// Return true if the Microsoft ChangJie IME is the active input processor.
bool IsChangjieIMEActive();

// Return true if an English (US) keyboard layout is the active input source.
bool IsEnglishUSActive();

// Return true if the ChangJie IME is active AND in Chinese (native) input mode.
bool IsChangjieChineseModeActive();

// Return true if the ChangJie IME is active AND in English (alphanumeric) input mode.
bool IsChangjieEnglishModeActive();

// Get conversion mode using ImmGetConversionStatus (alternative to WM_IME_CONTROL).
DWORD GetConversionModeViaImmContext();

// Check if the IME is open (active for input).
BOOL GetImeOpenStatus();

// ---------------------------------------------------------------------------
// Profile activation (use ITfInputProcessorProfileMgr via COM)
// ---------------------------------------------------------------------------

// Activate the Microsoft ChangJie IME session-wide.
HRESULT ActivateChangjieIME();

// Activate the English (US) keyboard layout session-wide.
HRESULT ActivateEnglishUS();

// ---------------------------------------------------------------------------
// Conversion-mode helpers (use IMM32 DLL via ImmGetDefaultIMEWnd /
//   WM_IME_CONTROL messages sent to the foreground window's IME window)
// ---------------------------------------------------------------------------

// Return the foreground window's current IME conversion mode flags,
// or (DWORD)-1 on failure.
DWORD GetCurrentConversionMode();

// Return the foreground window's current IME sentence mode flags,
// or (DWORD)-1 on failure.
DWORD GetCurrentSentenceMode();

// Set the foreground window's IME conversion mode to |mode|.
HRESULT SetConversionMode(DWORD mode);

// Ensure IME_CMODE_NATIVE is set (Chinese character input).
HRESULT SetChineseMode();

// Ensure IME_CMODE_NATIVE is cleared (alphanumeric / English input).
HRESULT SetEnglishMode();
