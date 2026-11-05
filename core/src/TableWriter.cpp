#include "langtext/TableWriter.h"
#include "langtext/Model.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef LANGTEXT_HAS_XLSXWRITER
#include <chrono>
#include <xlsxwriter.h>
#endif

namespace fs = std::filesystem;

namespace ltx {
namespace {

std::vector<std::string> headerRow(const Result& result,
                                   const WriteOptions& options) {
    std::vector<std::string> row;
    if (options.includeKeyColumn) row.push_back("key");
    for (const Language& language : result.languages) row.push_back(language.name);
    return row;
}

std::string csvCell(const std::string& value) {
    const bool needsQuote = value.find_first_of(",\"\r\n") != std::string::npos;
    if (!needsQuote) return value;
    std::string out = "\"";
    for (char c : value) {
        out.push_back(c);
        if (c == '"') out.push_back('"');
    }
    out.push_back('"');
    return out;
}

void writeCsvRow(std::ostream& stream, const std::vector<std::string>& cells) {
    for (std::size_t i = 0; i < cells.size(); ++i) {
        if (i) stream << ',';
        stream << csvCell(cells[i]);
    }
    stream << "\r\n";
}

std::vector<Issue> allIssues(const Result& result) {
    std::vector<Issue> issues = result.globalIssues;
    for (const Table& table : result.tables) {
        issues.insert(issues.end(), table.issues.begin(), table.issues.end());
    }
    return issues;
}

bool openOutput(const std::string& pathUtf8, std::ofstream& stream,
                std::string& error) {
    const fs::path path = fs::u8path(pathUtf8);
    std::error_code ec;
    if (path.has_parent_path()) fs::create_directories(path.parent_path(), ec);
    stream.open(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        error = "cannot create output file: " + pathUtf8;
        return false;
    }
    return true;
}

#ifdef LANGTEXT_HAS_XLSXWRITER
fs::path temporaryXlsxPath() {
    const auto now = std::chrono::high_resolution_clock::now()
                         .time_since_epoch().count();
    std::error_code ec;
    fs::path dir = fs::temp_directory_path(ec);
    if (ec || dir.empty()) dir = fs::current_path(ec);
    return dir / ("langtext_" + std::to_string(now) + ".xlsx");
}

// libxlsxwriter calls fopen(), which on Windows interprets narrow paths using
// the process ANSI code page (ACP), not UTF-8. std::filesystem::path::string()
// yields exactly that ACP encoding, so a temp directory such as
// C:\Users\中文\AppData\Local\Temp still works. On POSIX, string() is already
// UTF-8 and is byte-identical to u8string().
std::string fopenPath(const fs::path& path) {
    return path.string();
}
#endif

} // namespace

std::string jsonEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    static const char* hex = "0123456789abcdef";
    for (unsigned char c : text) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    out += "\\u00";
                    out.push_back(hex[(c >> 4) & 0x0F]);
                    out.push_back(hex[c & 0x0F]);
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    return out;
}

bool writeCsv(const Result& result, const std::string& pathUtf8,
              const WriteOptions& options, std::string& error) {
    std::ofstream stream;
    if (!openOutput(pathUtf8, stream, error)) return false;
    stream << "\xEF\xBB\xBF";

    const std::size_t columnCount = result.languages.size() +
                                    (options.includeKeyColumn ? 1 : 0);
    writeCsvRow(stream, headerRow(result, options));

    const std::string missingMark = options.markMissing ? "[MISSING]" : "";
    for (const Table& table : result.tables) {
        if (options.includeGroupRow) {
            std::vector<std::string> groupRow(columnCount);
            groupRow[0] = "# " + table.title;
            writeCsvRow(stream, groupRow);
        }
        for (std::size_t r = 0; r < table.rows.size(); ++r) {
            std::vector<std::string> row;
            row.reserve(columnCount);
            if (options.includeKeyColumn) row.push_back(table.keys[r]);
            for (std::size_t c = 0; c < result.languages.size(); ++c) {
                const std::string& value = table.rows[r][c];
                row.push_back(value.empty() ? missingMark : value);
            }
            writeCsvRow(stream, row);
        }
        stream << "\r\n";
    }

    if (options.includeIssues) {
        std::vector<std::string> section(columnCount);
        section[0] = "# issues";
        writeCsvRow(stream, section);
        writeCsvRow(stream, {"kind", "group", "language", "key", "detail",
                             "source", "line"});
        for (const Issue& issue : allIssues(result)) {
            writeCsvRow(stream, {issueKindName(issue.kind), issue.group,
                                 issue.language, issue.key, issue.detail,
                                 issue.source, std::to_string(issue.line)});
        }
    }

    if (!stream.good()) {
        error = "failed while writing CSV: " + pathUtf8;
        return false;
    }
    return true;
}

bool writeXlsx(const Result& result, const std::string& pathUtf8,
               const WriteOptions& options, std::string& error) {
#ifdef LANGTEXT_HAS_XLSXWRITER
    const fs::path tempPath = temporaryXlsxPath();
    lxw_workbook* workbook = workbook_new(fopenPath(tempPath).c_str());
    if (!workbook) {
        error = "libxlsxwriter could not create a workbook";
        return false;
    }
    lxw_worksheet* sheet = workbook_add_worksheet(workbook, "translations");
    if (!sheet) {
        error = "libxlsxwriter could not create a worksheet";
        workbook_close(workbook);
        return false;
    }

    lxw_format* headerFormat = workbook_add_format(workbook);
    format_set_bold(headerFormat);
    format_set_bg_color(headerFormat, 0xDDEBF7);
    format_set_border(headerFormat, LXW_BORDER_THIN);
    lxw_format* groupFormat = workbook_add_format(workbook);
    format_set_bold(groupFormat);
    format_set_align(groupFormat, LXW_ALIGN_CENTER);
    format_set_bg_color(groupFormat, 0xFFEFD5);
    lxw_format* missingFormat = workbook_add_format(workbook);
    format_set_font_color(missingFormat, LXW_COLOR_RED);

    const std::size_t offset = options.includeKeyColumn ? 1 : 0;
    const lxw_col_t lastColumn = static_cast<lxw_col_t>(
        result.languages.size() + offset - 1);
    lxw_row_t row = 0;

    const std::vector<std::string> header = headerRow(result, options);
    for (std::size_t c = 0; c < header.size(); ++c) {
        worksheet_write_string(sheet, row, static_cast<lxw_col_t>(c),
                               header[c].c_str(), headerFormat);
    }
    ++row;
    worksheet_freeze_panes(sheet, 1, 0);
    worksheet_set_column(sheet, 0, lastColumn, 26.0, nullptr);

    for (const Table& table : result.tables) {
        if (options.includeGroupRow) {
            worksheet_merge_range(sheet, row, 0, row, lastColumn,
                                  table.title.c_str(), groupFormat);
            ++row;
        }
        for (std::size_t r = 0; r < table.rows.size(); ++r) {
            if (options.includeKeyColumn) {
                worksheet_write_string(sheet, row, 0, table.keys[r].c_str(), nullptr);
            }
            for (std::size_t c = 0; c < result.languages.size(); ++c) {
                const std::string& value = table.rows[r][c];
                if (!value.empty()) {
                    worksheet_write_string(sheet, row,
                                           static_cast<lxw_col_t>(c + offset),
                                           value.c_str(), nullptr);
                } else if (options.markMissing) {
                    worksheet_write_string(sheet, row,
                                           static_cast<lxw_col_t>(c + offset),
                                           "[MISSING]", missingFormat);
                }
            }
            ++row;
        }
        ++row; // blank separator between groups
    }

    if (options.includeIssues) {
        lxw_worksheet* issuesSheet = workbook_add_worksheet(workbook, "issues");
        if (issuesSheet) {
            const char* columns[] = {"kind", "group", "language", "key",
                                     "detail", "source", "line"};
            for (lxw_col_t c = 0; c < 7; ++c) {
                worksheet_write_string(issuesSheet, 0, c, columns[c], headerFormat);
            }
            lxw_row_t issueRow = 1;
            for (const Issue& issue : allIssues(result)) {
                worksheet_write_string(issuesSheet, issueRow, 0,
                                       issueKindName(issue.kind), nullptr);
                worksheet_write_string(issuesSheet, issueRow, 1,
                                       issue.group.c_str(), nullptr);
                worksheet_write_string(issuesSheet, issueRow, 2,
                                       issue.language.c_str(), nullptr);
                worksheet_write_string(issuesSheet, issueRow, 3,
                                       issue.key.c_str(), nullptr);
                worksheet_write_string(issuesSheet, issueRow, 4,
                                       issue.detail.c_str(), nullptr);
                worksheet_write_string(issuesSheet, issueRow, 5,
                                       issue.source.c_str(), nullptr);
                worksheet_write_number(issuesSheet, issueRow, 6,
                                       static_cast<double>(issue.line), nullptr);
                ++issueRow;
            }
        }
    }

    const lxw_error closeResult = workbook_close(workbook);
    if (closeResult != LXW_NO_ERROR) {
        error = "libxlsxwriter failed with error code " +
                std::to_string(static_cast<int>(closeResult));
        std::error_code ignored;
        fs::remove(tempPath, ignored);
        return false;
    }

    // libxlsxwriter writes through fopen(), which cannot open a UTF-8 narrow
    // path on Windows. Build the workbook at an ACP-safe temp path and then
    // move it to the requested (possibly non-ASCII) UTF-8 destination using
    // std::filesystem, which handles wide paths correctly.
    std::error_code ec;
    const fs::path target = fs::u8path(pathUtf8);
    if (target.has_parent_path()) fs::create_directories(target.parent_path(), ec);
    fs::remove(target, ec);
    ec.clear();
    fs::rename(tempPath, target, ec);
    if (ec) {
        // rename() fails across volumes (temp dir vs. target drive): copy.
        fs::copy_file(tempPath, target,
                      fs::copy_options::overwrite_existing, ec);
        std::error_code cleanupError;
        fs::remove(tempPath, cleanupError);
    }
    if (ec) {
        error = "cannot move workbook to " + pathUtf8 + ": " + ec.message();
        return false;
    }
    return true;
#else
    (void)result;
    (void)options;
    (void)pathUtf8;
    error = "XLSX support was not compiled in. Rebuild with "
            "-DLANGTEXT_WITH_XLSX=ON, or choose a .csv output file.";
    return false;
#endif
}

bool writeTable(const Result& result, const std::string& pathUtf8,
                const WriteOptions& options, std::string& error) {
    const std::size_t dot = pathUtf8.find_last_of('.');
    const std::string extension = dot == std::string::npos
        ? "" : pathUtf8.substr(dot);
    std::string lowered = extension;
    for (char& c : lowered) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    if (lowered == ".xlsx") return writeXlsx(result, pathUtf8, options, error);
    return writeCsv(result, pathUtf8, options, error);
}

bool writeJsonReport(const Result& result, const std::string& pathUtf8,
                     std::string& error) {
    std::ofstream stream;
    if (!openOutput(pathUtf8, stream, error)) return false;

    stream << "{\n  \"stats\": {\n"
           << "    \"groups\": " << result.stats.groups << ",\n"
           << "    \"sourceFiles\": " << result.stats.sourceFiles << ",\n"
           << "    \"entries\": " << result.stats.entries << ",\n"
           << "    \"missing\": " << result.stats.missing << ",\n"
           << "    \"orphan\": " << result.stats.orphan << ",\n"
           << "    \"duplicate\": " << result.stats.duplicate << ",\n"
           << "    \"skipped\": " << result.stats.skipped << ",\n"
           << "    \"errors\": " << result.stats.errors << "\n  },\n";

    stream << "  \"languages\": [";
    for (std::size_t i = 0; i < result.languages.size(); ++i) {
        if (i) stream << ", ";
        stream << "\"" << jsonEscape(result.languages[i].code) << "\"";
    }
    stream << "],\n  \"issues\": [\n";

    const std::vector<Issue> issues = allIssues(result);
    for (std::size_t i = 0; i < issues.size(); ++i) {
        const Issue& issue = issues[i];
        stream << "    {\"kind\": \"" << issueKindName(issue.kind) << "\", "
               << "\"group\": \"" << jsonEscape(issue.group) << "\", "
               << "\"language\": \"" << jsonEscape(issue.language) << "\", "
               << "\"key\": \"" << jsonEscape(issue.key) << "\", "
               << "\"detail\": \"" << jsonEscape(issue.detail) << "\", "
               << "\"source\": \"" << jsonEscape(issue.source) << "\", "
               << "\"line\": " << issue.line << "}";
        if (i + 1 < issues.size()) stream << ",";
        stream << "\n";
    }
    stream << "  ]\n}\n";

    if (!stream.good()) {
        error = "failed while writing JSON report: " + pathUtf8;
        return false;
    }
    return true;
}

} // namespace ltx
