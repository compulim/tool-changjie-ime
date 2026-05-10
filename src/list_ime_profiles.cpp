// Diagnostic tool: List all available IME profiles on the system.
// This tool helps users identify the correct GUIDs for the ChangJie IME
// on their specific Windows installation.
//
// Outputs to a message box since this is a WIN32 application.

#include "ime_common.h"
#include <sstream>
#include <vector>
#include <iomanip>

// Helper to convert GUID to string
std::wstring GuidToString(const GUID& guid)
{
    wchar_t buffer[64];
    swprintf_s(buffer, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
               guid.Data1, guid.Data2, guid.Data3,
               guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
               guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return buffer;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ITfInputProcessorProfiles* pProfiles = nullptr;
    hr = CoCreateInstance(
        CLSID_TF_InputProcessorProfiles,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfInputProcessorProfiles,
        reinterpret_cast<void**>(&pProfiles));

    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to create ITfInputProcessorProfiles", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    std::wstringstream output;
    output << L"Available Input Method Profiles:\n\n";

    // Enumerate all language profiles
    IEnumTfLanguageProfiles* pEnum = nullptr;
    hr = pProfiles->EnumLanguageProfiles(LANGID_TraditionalChinese, &pEnum);

    if (SUCCEEDED(hr) && pEnum) {
        output << L"Traditional Chinese (zh-TW, 0x0404) profiles:\n";

        TF_LANGUAGEPROFILE profile;
        ULONG fetched = 0;
        while (pEnum->Next(1, &profile, &fetched) == S_OK) {
            output << L"\n  Type: " << (profile.dwProfileType == TF_PROFILETYPE_INPUTPROCESSOR ? L"Input Processor" : L"Keyboard Layout") << L"\n";
            output << L"  CLSID: " << GuidToString(profile.clsid) << L"\n";
            output << L"  Profile GUID: " << GuidToString(profile.guidProfile) << L"\n";
            output << L"  HKL: 0x" << std::hex << std::setw(8) << std::setfill(L'0')
                   << reinterpret_cast<ULONG_PTR>(profile.hkl) << std::dec << L"\n";

            // Try to get the description
            BSTR bstrDesc = nullptr;
            if (SUCCEEDED(pProfiles->GetLanguageProfileDescription(
                profile.clsid, profile.langid, profile.guidProfile, &bstrDesc))) {
                if (bstrDesc) {
                    output << L"  Description: " << bstrDesc << L"\n";
                    SysFreeString(bstrDesc);
                }
            }
        }
        pEnum->Release();
    }

    // Also enumerate English (US) for comparison
    pEnum = nullptr;
    hr = pProfiles->EnumLanguageProfiles(LANGID_EnglishUS, &pEnum);

    if (SUCCEEDED(hr) && pEnum) {
        output << L"\n\nEnglish (US, 0x0409) profiles:\n";

        TF_LANGUAGEPROFILE profile;
        ULONG fetched = 0;
        int count = 0;
        while (pEnum->Next(1, &profile, &fetched) == S_OK && count < 3) {
            if (profile.dwProfileType == TF_PROFILETYPE_KEYBOARDLAYOUT) {
                output << L"\n  Type: Keyboard Layout\n";
                output << L"  CLSID: " << GuidToString(profile.clsid) << L"\n";
                output << L"  HKL: 0x" << std::hex << std::setw(8) << std::setfill(L'0')
                       << reinterpret_cast<ULONG_PTR>(profile.hkl) << std::dec << L"\n";
                count++;
            }
        }
        pEnum->Release();
    }

    pProfiles->Release();
    CoUninitialize();

    // Show the output in a message box
    MessageBoxW(nullptr, output.str().c_str(), L"IME Profiles", MB_OK | MB_ICONINFORMATION);
    return 0;
}
