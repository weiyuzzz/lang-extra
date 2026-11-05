#include "MainWindow.h"

#include "langtext/Extract.h"
#include "langtext/Settings.h"
#include "langtext/TableWriter.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <sstream>

using namespace DuiLib;
namespace fs = std::filesystem;

MainWindow::~MainWindow() {
    m_cancel.store(true);
    if (m_worker.joinable()) m_worker.join();
}

void MainWindow::InitWindow() {
    m_hwnd = m_PaintManager.GetPaintWindow();
    m_path = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit1")));
    m_list = static_cast<CListUI*>(m_PaintManager.FindControl(_T("list1")));
    m_status = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("label_status")));
    m_progress = static_cast<CProgressUI*>(m_PaintManager.FindControl(_T("progress1")));
    m_startBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_start")));
    m_cancelBtn = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_cancel")));

    if (!m_path || !m_list || !m_status || !m_progress ||
        !m_startBtn || !m_cancelBtn) {
        ::MessageBoxW(m_hwnd, L"界面资源 duilib.xml 加载失败，缺少必要控件。",
                      L"langTextExtra", MB_ICONERROR | MB_OK);
    }
    if (m_cancelBtn) m_cancelBtn->SetEnabled(false);
    if (m_status) m_status->SetText(_T("就绪：请选择目录，然后点击开始"));
}

void MainWindow::Notify(TNotifyUI& msg) {
    if (msg.sType != _T("click")) {
        WindowImplBase::Notify(msg);
        return;
    }
    const CDuiString name = msg.pSender ? msg.pSender->GetName() : _T("");
    if (name == _T("btn_import")) onImport();
    else if (name == _T("btn_start")) onStart();
    else if (name == _T("btn_cancel")) onCancel();
    else if (name == _T("minbtn")) {
        ::SendMessageW(m_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    } else if (name == _T("closebtn")) {
        ::PostQuitMessage(0);
    } else {
        WindowImplBase::Notify(msg);
    }
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == kUiMessage) {
        std::unique_ptr<UiMessage> message(reinterpret_cast<UiMessage*>(lParam));
        if (message) applyMessage(*message);
        return 0;
    }
    return WindowImplBase::HandleMessage(uMsg, wParam, lParam);
}

void MainWindow::post(UiMessage* payload) const {
    if (m_hwnd && ::IsWindow(m_hwnd) &&
        ::PostMessageW(m_hwnd, kUiMessage, 0,
                       reinterpret_cast<LPARAM>(payload))) {
        return;
    }
    delete payload;
}

void MainWindow::addLine(const std::wstring& text) {
    if (!m_list) return;
    CListLabelElementUI* item = new CListLabelElementUI;
    item->SetText(text.c_str());
    item->SetFixedHeight(22);
    m_list->Add(item);
}

void MainWindow::onImport() {
    std::wstring folder;
    if (!win::pickFolder(m_hwnd, folder)) return;
    if (m_path) m_path->SetText(folder.c_str());
    if (m_status) m_status->SetText(_T("目录已选择，点击开始提取"));
}

void MainWindow::onCancel() {
    m_cancel.store(true);
    if (m_status) m_status->SetText(_T("正在取消…"));
}

void MainWindow::onStart() {
    if (!m_path || !m_list || !m_status || !m_progress ||
        !m_startBtn || !m_cancelBtn) {
        return;
    }
    if (m_worker.joinable()) return;

    const std::wstring inputWide = m_path->GetText().GetData();
    if (inputWide.empty()) {
        ::MessageBoxW(m_hwnd, L"请先选择要提取的目录。", L"提示", MB_OK);
        return;
    }

    std::wstring outputWide;
    // A single save dialog. Cancelling aborts the whole operation (fixed: the
    // old code still created a workbook with an empty path).
    if (!win::pickSaveFile(m_hwnd, win::exeDirectory(), outputWide)) return;

    const std::string exeDirUtf8 = win::wideToUtf8(win::exeDirectory());
    WorkerArgs args;
    args.input = win::wideToUtf8(inputWide);
    args.output = win::wideToUtf8(outputWide);
    args.settings = ltx::findConfigFile(exeDirUtf8, "settings.xml", 3);
    args.config = ltx::findConfigFile(exeDirUtf8, "config.xml", 3);

    fs::path reportPath = fs::u8path(args.output);
    reportPath.replace_extension(".issues.json");
    args.report = reportPath.u8string();

    m_list->RemoveAll();
    m_status->SetText(_T("正在提取…"));
    m_progress->SetValue(0);
    m_startBtn->SetEnabled(false);
    m_cancelBtn->SetEnabled(true);
    m_cancel.store(false);

    m_worker = std::thread([this, args]() { worker(args); });
}

void MainWindow::worker(const WorkerArgs& args) {
    ltx::Settings settings = ltx::defaultSettings();
    std::string error;
    if (!args.settings.empty() &&
        !ltx::loadSettings(args.settings, settings, error)) {
        UiMessage* message = new UiMessage;
        message->code = 2;
        message->text = win::utf8ToWide("配置读取失败: " + error);
        post(message);
        return;
    }

    std::string configPath = args.config;
    if (configPath.empty() && !args.settings.empty()) {
        const std::string settingsDir =
            fs::u8path(args.settings).parent_path().u8string();
        configPath = ltx::findConfigFile(settingsDir, settings.groupMapFile, 2);
    }

    ltx::ExtractOptions options;
    options.inputRootUtf8 = args.input;
    options.settings = settings;
    options.groupMapFileUtf8 = configPath;
    options.isCancelled = [this]() { return m_cancel.load(); };
    options.onProgress = [this](float progress, const std::string& text) {
        UiMessage* message = new UiMessage;
        message->code = 0;
        message->progress = progress;
        message->text = win::utf8ToWide(text);
        post(message);
    };

    const ltx::Result result = ltx::extract(options);

    UiMessage* message = new UiMessage;
    message->progress = 1.0f;

    if (m_cancel.load()) {
        message->code = 2;
        message->text = L"已取消，未生成输出文件";
        post(message);
        return;
    }

    ltx::WriteOptions writeOptions;
    std::string writeError;
    const bool wrote = ltx::writeTable(result, args.output, writeOptions,
                                       writeError);
    std::string reportError;
    const bool reported = args.report.empty() ||
        ltx::writeJsonReport(result, args.report, reportError);

    std::wostringstream summary;
    summary << L"分组 " << result.stats.groups
            << L" / 源文件 " << result.stats.sourceFiles
            << L" / 词条 " << result.stats.entries
            << L" / 缺译 " << result.stats.missing
            << L" / 多余 " << result.stats.orphan
            << L" / 错误 " << result.stats.errors;
    message->text = summary.str();
    message->lines.push_back(L"[结果] " + win::utf8ToWide(args.output));
    if (reported && !args.report.empty()) {
        message->lines.push_back(L"[报告] " + win::utf8ToWide(args.report));
    }
    if (!wrote) {
        message->code = 2;
        message->lines.push_back(L"[错误] " + win::utf8ToWide(writeError));
    } else {
        message->code = 1;
    }
    if (!reported) {
        message->lines.push_back(L"[警告] 报告写入失败: " +
                                 win::utf8ToWide(reportError));
    }
    message->lines.push_back(
        L"[统计] 跳过 " + std::to_wstring(result.stats.skipped) +
        L" / 重复 key " + std::to_wstring(result.stats.duplicate));

    std::vector<ltx::Issue> issues = result.globalIssues;
    for (const ltx::Table& table : result.tables) {
        issues.insert(issues.end(), table.issues.begin(), table.issues.end());
    }
    const std::size_t limit = std::min<std::size_t>(issues.size(), 300);
    for (std::size_t i = 0; i < limit; ++i) {
        const ltx::Issue& issue = issues[i];
        std::wstring line = L"[" +
            win::utf8ToWide(ltx::issueKindName(issue.kind)) + L"] " +
            win::utf8ToWide(issue.group) + L" / " +
            win::utf8ToWide(issue.language) + L" / " +
            win::utf8ToWide(issue.key);
        if (!issue.detail.empty()) {
            line += L" - " + win::utf8ToWide(issue.detail);
        }
        message->lines.push_back(line);
    }
    if (issues.size() > limit) {
        message->lines.push_back(L"… 其余 " +
            std::to_wstring(issues.size() - limit) +
            L" 条请查看 JSON 报告");
    }

    post(message);
}

void MainWindow::applyMessage(const UiMessage& message) {
    const int percent = static_cast<int>(message.progress * 100.0f);
    if (m_progress) {
        m_progress->SetValue(message.code == 0 ? percent : 100);
    }
    if (m_status) {
        std::wstring status = message.text;
        if (message.code == 0) {
            status = std::to_wstring(percent) + L"%  " + status;
        }
        m_status->SetText(status.c_str());
    }
    for (const std::wstring& line : message.lines) addLine(line);

    if (message.code != 0) finishWorker();
}

void MainWindow::finishWorker() {
    if (m_worker.joinable()) m_worker.join();
    if (m_startBtn) m_startBtn->SetEnabled(true);
    if (m_cancelBtn) m_cancelBtn->SetEnabled(false);
}
