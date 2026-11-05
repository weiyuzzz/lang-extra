#include "MainWindow.h"
#include "WinUtils.h"

#include <UIlib.h>

#include <string>

// Unicode entry point. The window is a stack object: the previous code did
// `delete this` in OnFinalMessage and `delete pFrame` here, i.e. a double free.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    win::enablePerMonitorDpi();
    win::OleGuard ole;

    DuiLib::CPaintManagerUI::SetInstance(hInstance);

    // Resolve skins relative to the executable, not the process working
    // directory, so launching from a shortcut or another folder still works.
    const std::wstring exeDir = win::exeDirectory();
    if (!exeDir.empty()) {
        DuiLib::CPaintManagerUI::SetResourcePath(
            (exeDir + L"\\skin\\").c_str());
    }

    {
        MainWindow frame;
        frame.Create(nullptr, _T("langTextExtra"), UI_WNDSTYLE_FRAME,
                     WS_EX_WINDOWEDGE);
        frame.CenterWindow();
        frame.ShowWindow(true);
        DuiLib::CPaintManagerUI::MessageLoop();
        // frame is destroyed here, after the message loop has ended. Its
        // destructor cancels and joins any still-running worker thread.
    }
    return 0;
}
