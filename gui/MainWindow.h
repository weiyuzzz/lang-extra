#pragma once

#include <UIlib.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "WinUtils.h"

struct UiMessage {
    int code = 0;       // 0 = progress, 1 = finished, 2 = error
    float progress = 0.0f;
    std::wstring text;
    std::vector<std::wstring> lines;
};

class MainWindow : public DuiLib::WindowImplBase {
public:
    MainWindow() = default;
    ~MainWindow();

    LPCTSTR GetWindowClassName() const override { return _T("langTextExtraFrameWnd"); }
    DuiLib::CDuiString GetSkinFolder() override { return _T("skin\\"); }
    DuiLib::CDuiString GetSkinFile() override { return _T("duilib.xml"); }

    void InitWindow() override;
    void Notify(DuiLib::TNotifyUI& msg) override;
    // Fixed: the old code did `delete this` here AND deleted the frame again in
    // WinMain -> double free. The frame now lives on the stack; we only forward
    // to the base class so DuiLib can release its notifier/filter/objects.
    void OnFinalMessage(HWND hWnd) override {
        WindowImplBase::OnFinalMessage(hWnd);
    }
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    struct WorkerArgs {
        std::string input;
        std::string output;
        std::string report;
        std::string settings;
        std::string config;
    };

    static constexpr UINT kUiMessage = WM_APP + 7;

    void onImport();
    void onStart();
    void onCancel();
    void worker(const WorkerArgs& args);
    void post(UiMessage* payload) const;
    void addLine(const std::wstring& text);
    void applyMessage(const UiMessage& message);
    void finishWorker();

    HWND m_hwnd = nullptr;
    DuiLib::CEditUI* m_path = nullptr;
    DuiLib::CListUI* m_list = nullptr;
    DuiLib::CLabelUI* m_status = nullptr;
    DuiLib::CProgressUI* m_progress = nullptr;
    DuiLib::CButtonUI* m_startBtn = nullptr;
    DuiLib::CButtonUI* m_cancelBtn = nullptr;

    std::atomic<bool> m_cancel{false};
    std::thread m_worker;
};
