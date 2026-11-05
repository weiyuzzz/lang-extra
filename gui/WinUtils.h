#pragma once

#include <windows.h>

#include <string>

namespace win {

// UTF-8 is the only internal encoding. Conversion happens exclusively at the
// Win32 boundary, so Chinese/Arabic paths never get round-tripped through the
// ANSI code page.
std::wstring utf8ToWide(const std::string& utf8);
std::string wideToUtf8(const std::wstring& wide);

// Absolute directory of the running executable (no dependency on CWD).
std::wstring exeDirectory();

void enablePerMonitorDpi();

class OleGuard {
public:
    OleGuard();
    ~OleGuard();
    OleGuard(const OleGuard&) = delete;
    OleGuard& operator=(const OleGuard&) = delete;
    bool ok() const { return initialized_; }

private:
    bool initialized_ = false;
};

// SHBrowseForFolderW + SHGetPathFromIDListW with proper CoTaskMemFree.
bool pickFolder(HWND owner, std::wstring& out);

// Single GetSaveFileNameW dialog. nMaxFile is a TCHAR count, not a byte count.
bool pickSaveFile(HWND owner, const std::wstring& initialDir,
                  std::wstring& out);

} // namespace win
