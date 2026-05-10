cmake_minimum_required(VERSION 3.20)
project(changjie-ime-tools VERSION 1.0.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Project-wide settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows only.")
endif()

# Require Windows 10 or later for full Text Services Framework support.
add_compile_definitions(
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# ---------------------------------------------------------------------------
# Shared static library: ime_common
# ---------------------------------------------------------------------------
add_library(ime_common STATIC
    src/ime_common.cpp
)

target_include_directories(ime_common PUBLIC src)

target_link_libraries(ime_common
    ole32
    oleaut32
    imm32
)

# ---------------------------------------------------------------------------
# Helper macro — creates a WIN32 (no-console) executable that links ime_common
# ---------------------------------------------------------------------------
macro(add_ime_tool target_name source_file)
    add_executable(${target_name} WIN32 ${source_file})
    target_link_libraries(${target_name} PRIVATE ime_common)
endmacro()

# ---------------------------------------------------------------------------
# Tool 1: switch-to-changjie
#   Switch to ChangJie IME if it is not already active.
# ---------------------------------------------------------------------------
add_ime_tool(switch-to-changjie src/switch_to_changjie.cpp)

# ---------------------------------------------------------------------------
# Tool 2: changjie-chinese-mode
#   If ChangJie IME is active, switch to Chinese (native) input mode.
# ---------------------------------------------------------------------------
add_ime_tool(changjie-chinese-mode src/changjie_chinese_mode.cpp)

# ---------------------------------------------------------------------------
# Tool 3: changjie-english-mode
#   If ChangJie IME is active, switch to English (alphanumeric) input mode.
# ---------------------------------------------------------------------------
add_ime_tool(changjie-english-mode src/changjie_english_mode.cpp)

# ---------------------------------------------------------------------------
# Tool 4: switch-to-english-us
#   Switch to English (US) keyboard layout if it is not already active.
# ---------------------------------------------------------------------------
add_ime_tool(switch-to-english-us src/switch_to_english_us.cpp)

# ---------------------------------------------------------------------------
# Tool 5: switch-to-changjie-chinese
#   Switch to ChangJie IME (if needed) and set Chinese input mode.
# ---------------------------------------------------------------------------
add_ime_tool(switch-to-changjie-chinese src/switch_to_changjie_chinese.cpp)

# ---------------------------------------------------------------------------
# Tool 6: toggle-changjie-english
#   Toggle between ChangJie IME and English (US).
#   When landing on ChangJie, switch to Chinese input mode.
# ---------------------------------------------------------------------------
add_ime_tool(toggle-changjie-english src/toggle_changjie_english.cpp)

# ---------------------------------------------------------------------------
# Diagnostic tool: list-ime-profiles
#   Display all available IME profiles in a message box.
#   Helps users find the correct GUIDs for their system.
# ---------------------------------------------------------------------------
add_ime_tool(list-ime-profiles src/list_ime_profiles.cpp)

# ---------------------------------------------------------------------------
# Installation rules (used by the GitHub Actions packaging step)
# ---------------------------------------------------------------------------
install(TARGETS
    switch-to-changjie
    changjie-chinese-mode
    changjie-english-mode
    switch-to-english-us
    switch-to-changjie-chinese
    toggle-changjie-english
    list-ime-profiles
    RUNTIME DESTINATION bin
)
