# tool-changjie-ime

A set of small Windows utilities for controlling the **Microsoft ChangJie (倉頡) IME** and the **English (US) keyboard layout** through the [Text Services Framework (TSF)](https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework) COM interfaces and the IMM32 API.

Each utility compiles to a single, self-contained EXE with no console window.  They are designed to be bound to hotkeys (e.g. with AutoHotKey, PowerToys, or Windows keyboard shortcuts) and return almost instantly.

---

## Tools

| Executable | Description |
|---|---|
| `switch-to-changjie.exe` | Switch to ChangJie IME if it is not already active. |
| `changjie-chinese-mode.exe` | If ChangJie IME is active, switch to **Chinese** (native) input mode. |
| `changjie-english-mode.exe` | If ChangJie IME is active, switch to **English** (alphanumeric) input mode. |
| `switch-to-english-us.exe` | Switch to the English (US) keyboard layout if it is not already active. |
| `switch-to-changjie-chinese.exe` | Switch to ChangJie IME (if needed) **and** set Chinese input mode. |
| `toggle-changjie-english.exe` | Toggle between ChangJie IME and English (US). When landing on ChangJie, Chinese mode is activated automatically. |

---

## Implementation notes

* **Profile switching** – Uses `ITfInputProcessorProfileMgr::ActivateProfile` (COM, `msctf.dll`) with `TF_IPPMF_FORSESSION` so the change applies session-wide to every window.
* **Conversion-mode switching** – Uses `ImmGetDefaultIMEWnd` (IMM32, `imm32.dll`) to obtain the foreground window's IME window handle, then sends `WM_IME_CONTROL / IMC_SETCONVERSIONMODE` to it.  This works cross-process.

### ChangJie IME GUIDs

The default GUIDs for the Microsoft ChangJie IME shipped with Windows are:

| Item | GUID |
|---|---|
| CLSID | `{4BDF9F03-C7D3-11D4-B2AB-0080C882687E}` |
| Profile (ChangJie input) | `{531FDEBF-9B4C-4A43-A2AA-960E8FCDC732}` |
| Language | `0x0404` (zh-TW, Traditional Chinese) |

You can verify these on your machine under:
```
HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\CTF\TIP\{CLSID}\LanguageProfile\{langid}\{ProfileGUID}
```

---

## Building

### Prerequisites

* Windows 10 or later  
* [CMake](https://cmake.org/) ≥ 3.20  
* Visual Studio 2022 with the **Desktop development with C++** workload (or Build Tools for Visual Studio 2022)

### Steps

```powershell
# Configure (64-bit Release)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# Binaries land in build\Release\
```

---

## Releases

Pre-built 64-bit binaries are published automatically by the GitHub Actions workflow whenever a `v*` tag is pushed:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The workflow builds with MSVC on `windows-latest`, then attaches each EXE and a zip archive (`changjie-ime-tools.zip`) to the GitHub Release.
