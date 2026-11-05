#include "langtext/Extract.h"
#include "langtext/Filter.h"
#include "langtext/Scanner.h"
#include "langtext/Settings.h"
#include "langtext/TableWriter.h"
#include "langtext/Xml.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef LANGTEXT_FIXTURES_DIR
#define LANGTEXT_FIXTURES_DIR "tests/fixtures"
#endif

namespace fs = std::filesystem;

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(condition, message)                                              \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(condition)) {                                                    \
            ++g_failures;                                                      \
            std::cerr << "FAIL line " << __LINE__ << ": " << (message) << "\n";\
        }                                                                      \
    } while (false)

std::string fixture(const std::string& name) {
    return std::string(LANGTEXT_FIXTURES_DIR) + "/" + name;
}

std::string tempFile(const std::string& name) {
    std::error_code ec;
    fs::path dir = fs::temp_directory_path(ec) / "langtext_tests";
    fs::create_directories(dir, ec);
    return (dir / name).u8string();
}

std::string readAll(const std::string& path) {
    std::ifstream in(fs::u8path(path), std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
    return bytes;
}

void testDefaults() {
    const ltx::Settings settings = ltx::defaultSettings();
    CHECK(settings.languages.size() == 16, "16 built-in languages");
    int references = 0;
    for (const ltx::Language& language : settings.languages) {
        if (language.reference) ++references;
    }
    CHECK(references == 1, "exactly one reference language");
    CHECK(settings.languages[0].code == "zh-CN", "zh-CN is the reference");
    CHECK(settings.languages[13].code == "sk", "Slovakia corrected to Slovak/sk");
    CHECK(settings.textTags.size() == 1, "one default text tag");
    CHECK(settings.specialTags.size() == 5, "five default special tags");
}

void testLoadSettings() {
    ltx::Settings settings;
    std::string error;
    CHECK(ltx::loadSettings(fixture("settings.xml"), settings, error),
          "settings.xml loads: " + error);
    CHECK(settings.languages.size() == 2, "fixture has 2 languages");
    CHECK(settings.languages[0].reference, "fixture reference language");
    CHECK(settings.groupMapFile == "config.xml", "group map file from settings");
    CHECK(settings.filter.skipExact.size() == 1, "skipExact parsed");

    ltx::Settings broken;
    std::string brokenError;
    CHECK(!ltx::loadSettings(fixture("does_not_exist.xml"), broken, brokenError),
          "missing settings file reports an error");
    CHECK(!brokenError.empty(), "missing settings error is descriptive");
}

void testGroupMap() {
    std::vector<std::pair<std::string, std::string>> pairs;
    std::string error;
    CHECK(ltx::loadGroupMap(fixture("config.xml"), pairs, error),
          "config.xml loads: " + error);
    bool flatMain = false;
    bool flatSub = false;
    bool legacyNested = false;
    for (const auto& pair : pairs) {
        if (pair.first == "main" && pair.second == "Main module") flatMain = true;
        if (pair.first == "sub" && pair.second == "Sub module") flatSub = true;
        if (pair.first == "legacy/nested" && pair.second == "42") legacyNested = true;
    }
    CHECK(flatMain, "flat <group path=...> form works");
    CHECK(flatSub, "second flat group works");
    CHECK(legacyNested, "legacy nested form still works");
}

void testFilter() {
    const ltx::FilterRules rules = ltx::defaultSettings().filter;
    CHECK(ltx::shouldSkip("", rules), "empty is skipped");
    CHECK(ltx::shouldSkip("   \t\n ", rules), "whitespace is skipped");
    CHECK(ltx::shouldSkip("12:30", rules), "time-like text is skipped");
    CHECK(ltx::shouldSkip("2024-01-01", rules), "date-like text is skipped");
    CHECK(!ltx::shouldSkip("A", rules), "single letter is kept (old bug)");
    CHECK(!ltx::shouldSkip("确定", rules), "CJK text is kept");
    CHECK(!ltx::shouldSkip("OK", rules), "short word is kept");
    const std::string longText(80, 'a');
    CHECK(!ltx::shouldSkip(longText, rules),
          "long text is kept (old >=32 char drop bug)");
    CHECK(!ltx::shouldSkip("%s", rules),
          "placeholders are kept unless explicitly configured");

    ltx::Settings configured;
    std::string loadError;
    CHECK(ltx::loadSettings(fixture("settings.xml"), configured, loadError),
          "fixture settings load for filter test: " + loadError);
    CHECK(ltx::shouldSkip("%s", configured.filter),
          "explicit skipExact value is skipped");
    CHECK(ltx::trimText("  hi \n") == "hi", "trimText works");
}

void testJsonEscape() {
    CHECK(ltx::jsonEscape("a\"b") == "a\\\"b", "quote escaped");
    CHECK(ltx::jsonEscape("a\\b") == "a\\\\b", "backslash escaped");
    CHECK(ltx::jsonEscape("a\nb") == "a\\nb", "newline escaped");
    CHECK(ltx::jsonEscape("中文") == "中文", "UTF-8 text preserved");
}

void testLanguageMatching() {
    const std::vector<ltx::Language> languages = ltx::defaultSettings().languages;
    CHECK(ltx::matchLanguageFile("style.xml", languages) == 0,
          "exact style.xml match");
    CHECK(ltx::matchLanguageFile("Style_En.XML", languages) == 4,
          "case-insensitive match");
    CHECK(ltx::matchLanguageFile("style_en.xml.bak", languages) == -1,
          "backup file is not matched (old substring bug)");
    CHECK(ltx::matchLanguageFile("my_style_en.xml", languages) == -1,
          "prefixed file is not matched");
}

void testXmlEntities() {
    std::vector<ltx::TextNode> nodes;
    std::string error;
    const std::vector<std::string> textTags = {"text"};
    const std::vector<std::string> specialTags = {"panel"};
    const std::vector<std::string> keyAttributes = {"name", "id", "key"};
    CHECK(ltx::parseTextNodes(fixture("entities.xml"), textTags, specialTags,
                              keyAttributes, nodes, error),
          "entities.xml parses: " + error);
    CHECK(nodes.size() == 3, "three text nodes");
    bool amp = false;
    bool hex = false;
    bool decimal = false;
    for (const ltx::TextNode& node : nodes) {
        if (node.text == "A & B") amp = true;
        if (node.text == "中文") {
            if (node.key.find("numeric") != std::string::npos) hex = true;
            if (node.key.find("decimal") != std::string::npos) decimal = true;
        }
    }
    CHECK(amp, "&amp; decoded");
    CHECK(hex, "&#x....; decoded to UTF-8 (old LEGACY bug)");
    CHECK(decimal, "&#.....; decoded to UTF-8");
}

void testScanner() {
    const ltx::Settings settings = ltx::defaultSettings();
    std::vector<ltx::Folder> folders;
    std::vector<ltx::Issue> issues;
    std::string error;
    CHECK(ltx::scanFolders(fixture("input"), settings.languages, folders,
                           issues, error),
          "scanFolders succeeds: " + error);
    CHECK(folders.size() == 2, "two folders found");
    CHECK(folders[0].relative == "main", "folders sorted deterministically");
    CHECK(folders[0].files.size() == 2,
          "only the 2 real language files, .bak ignored");
    CHECK(folders[1].files.size() == 2, "sub folder has 2 files");
    CHECK(folders[0].files[0].relative == "main/style.xml",
          "files sorted by relative path");
}

void testExtractAlignment() {
    ltx::Settings settings;
    std::string error;
    CHECK(ltx::loadSettings(fixture("settings.xml"), settings, error),
          "fixture settings load");

    ltx::ExtractOptions options;
    options.inputRootUtf8 = fixture("input");
    options.settings = settings;
    options.groupMapFileUtf8 = fixture("config.xml");

    const ltx::Result result = ltx::extract(options);
    CHECK(result.tables.size() == 2, "two group tables");
    CHECK(result.stats.errors == 0, "no parse/io errors");
    CHECK(result.stats.sourceFiles == 4, "four source files, .bak excluded");
    CHECK(result.tables[0].title == "Main module", "group title from config.xml");

    const ltx::Table& main = result.tables[0];
    CHECK(main.keys.size() == 4, "reference language defines 4 rows (version skipped)");
    CHECK(main.rows.size() == 4, "row count equals key count (old last-file bug)");

    // English has an extra leading entry and a missing 'cancel'. With key-based
    // alignment the existing rows must stay in place, not shift by one.
    bool okRow = false;
    bool greetingRow = false;
    bool cancelMissing = false;
    for (std::size_t r = 0; r < main.keys.size(); ++r) {
        if (main.rows[r][0] == "确定") {
            okRow = (main.rows[r][1] == "OK");
        }
        if (main.rows[r][0].find("欢迎使用") != std::string::npos) {
            greetingRow = main.rows[r][1].find("Welcome") == 0;
        }
        if (main.rows[r][0] == "取消") {
            cancelMissing = main.rows[r][1].empty();
        }
    }
    CHECK(okRow, "OK stays on the 确定 row despite the extra English entry");
    CHECK(greetingRow, "long greeting kept and aligned");
    CHECK(cancelMissing, "missing English translation is an empty cell");
    CHECK(result.stats.missing == 1, "exactly one missing translation");
    CHECK(result.stats.orphan == 1, "exactly one orphan key reported");
    CHECK(result.stats.skipped == 1, "pure numeric version string skipped");

    bool hasMissingIssue = false;
    bool hasOrphanIssue = false;
    for (const ltx::Issue& issue : main.issues) {
        if (issue.kind == ltx::IssueKind::MissingTranslation) hasMissingIssue = true;
        if (issue.kind == ltx::IssueKind::OrphanText) hasOrphanIssue = true;
    }
    CHECK(hasMissingIssue, "missing translation reported as an issue");
    CHECK(hasOrphanIssue, "orphan key reported as an issue");

    const ltx::Table& sub = result.tables[1];
    CHECK(sub.rows.size() == 1 && sub.rows[0][1] == "Sub module",
          "second group extracted correctly");
}

void testWriters() {
    ltx::Settings settings;
    std::string error;
    ltx::loadSettings(fixture("settings.xml"), settings, error);

    ltx::ExtractOptions options;
    options.inputRootUtf8 = fixture("input");
    options.settings = settings;
    options.groupMapFileUtf8 = fixture("config.xml");
    const ltx::Result result = ltx::extract(options);

    const std::string csvPath = tempFile("out.csv");
    ltx::WriteOptions writeOptions;
    CHECK(ltx::writeCsv(result, csvPath, writeOptions, error),
          "CSV written: " + error);
    const std::string csv = readAll(csvPath);
    CHECK(csv.size() > 3 && static_cast<unsigned char>(csv[0]) == 0xEF,
          "CSV starts with UTF-8 BOM");
    CHECK(csv.find("key,简体中文,English") != std::string::npos,
          "CSV header contains key + language names");
    CHECK(csv.find("[MISSING]") != std::string::npos,
          "CSV marks missing translations");
    CHECK(csv.find("取消") != std::string::npos, "CJK text survives round-trip");

    const std::string jsonPath = tempFile("report.json");
    CHECK(ltx::writeJsonReport(result, jsonPath, error),
          "JSON report written: " + error);
    const std::string json = readAll(jsonPath);
    CHECK(json.find("\"missing\"") != std::string::npos, "JSON has stats");
    CHECK(json.find("missing_translation") != std::string::npos,
          "JSON has issue entries");
    CHECK(json.find("\"issues\"") != std::string::npos, "JSON has issues array");

    const std::string xlsxPath = tempFile("out.xlsx");
    std::string xlsxError;
    const bool xlsxOk = ltx::writeXlsx(result, xlsxPath, writeOptions, xlsxError);
#ifdef LANGTEXT_HAS_XLSXWRITER
    CHECK(xlsxOk, "XLSX written: " + xlsxError);
#else
    CHECK(!xlsxOk, "XLSX path reports a clear error when support is disabled");
    CHECK(xlsxError.find("XLSX support") != std::string::npos,
          "XLSX error message is actionable");
#endif
}

} // namespace

int main() {
    testDefaults();
    testLoadSettings();
    testGroupMap();
    testFilter();
    testJsonEscape();
    testLanguageMatching();
    testXmlEntities();
    testScanner();
    testExtractAlignment();
    testWriters();

    std::cout << "langtext tests: " << g_checks << " checks, "
              << g_failures << " failures\n";
    return g_failures == 0 ? 0 : 1;
}
