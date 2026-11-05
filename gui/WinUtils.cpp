#include "WinUtils.h"

#include <commdlg.h>
#include <objbase.h>
#include <shlobj.h>

#include <algorithm>
#include <vector>

namespace win {

std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    const int length = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (length <= 0) return {};
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                        static_cast<int>(utf8.size()), &result[0], length);
    return result;
}

std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    const int length = WideCharToMultiByte(
        CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
        nullptr, 0, nullptr, nullptr);
    if (length <= 0) return {};
    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                        static_cast<int>(wide.size()), &result[0], length,
                        nullptr, nullptr);
    return result;
}

std::wstring exeDirectory() {
    std::vector<wchar_t> buffer(MAX_PATH * 2, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                             static_cast<DWORD>(buffer.size()));
    if (length == 0) return {};
    std::wstring full(buffer.data(), length);
    const std::size_t slash = full.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return {};
    return full.substr(0, slash);
}

void enablePerMonitorDpi() {
    using SetDpiFn = BOOL(WINAPI*)(HANDLE);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        auto fn = reinterpret_cast<SetDpiFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 == (HANDLE)-4
        if (fn && fn(reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4)))) return;
    }
    SetProcessDPIAware();
}

OleGuard::OleGuard() {
    const HRESULT hr = OleInitialize(nullptr);
    initialized_ = SUCCEEDED(hr);
}

OleGuard::~OleGuard() {
    if (initialized_) OleUninitialize();
}

bool pickFolder(HWND owner, std::wstring& out) {
    wchar_t displayName[MAX_PATH] = {0};
    BROWSEINFOW bi = {};
    bi.hwndOwner = owner;
    bi.pidlRoot = nullptr;
    bi.pszDisplayName = displayName;
    bi.lpszTitle = L"请选择需要提取文案的目录";
    bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
    bi.lpfn = nullptr;
    bi.lParam = 0;
    bi.iImage = 0;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return false;

    wchar_t path[MAX_PATH] = {0};
    const bool ok = SHGetPathFromIDListW(pidl, path) == TRUE;
    CoTaskMemFree(pidl);   // fixed: the original code leaked both PIDLs
    if (!ok) return false;
    out = path;
    return true;
}

bool pickSaveFile(HWND owner, const std::wstring& initialDir,
                  std::wstring& out) {
#ifdef LANGTEXT_HAS_XLSXWRITER
    wchar_t file[MAX_PATH] = L"translations.xlsx";
    const wchar_t* filter = L"Excel 工作簿 (*.xlsx)\0*.xlsx\0"
                            L"CSV 文件 (*.csv)\0*.csv\0"
                            L"所有文件 (*.*)\0*.*\0";
    const DWORD defaultFilter = 1;
#else
    wchar_t file[MAX_PATH] = L"translations.csv";
    const wchar_t* filter = L"CSV 文件 (*.csv)\0*.csv\0"
                            L"所有文件 (*.*)\0*.*\0";
    const DWORD defaultFilter = 1;
#endif
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);   // fixed: was sizeof(szFile) (bytes)
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = defaultFilter;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = initialDir.empty() ? nullptr : initialDir.c_str();
    ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR |
                OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (GetSaveFileNameW(&ofn) != TRUE) return false;   // cancelled -> abort
    out = file;
    const std::size_t dot = out.find_last_of(L'.');
    const std::size_t slash = out.find_last_of(L"\\/");
    const bool hasExtension = dot != std::wstring::npos &&
                              (slash == std::wstring::npos || slash < dot);
    if (!hasExtension) {
#ifdef LANGTEXT_HAS_XLSXWRITER
        out += (ofn.nFilterIndex == 2) ? L".csv" : L".xlsx";
#else
        out += L".csv";
#endif
    }
    return true;
}

} // namespace win
