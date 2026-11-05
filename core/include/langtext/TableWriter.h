#pragma once

#include "Model.h"

#include <string>

namespace ltx {

struct WriteOptions {
    bool includeKeyColumn = true;   // leading "key" column for round-trip
    bool markMissing = true;        // write "[MISSING]" into empty cells
    bool includeGroupRow = true;    // merged folder title row
    bool includeIssues = true;      // extra sheet / section with the issue list
};

// UTF-8 CSV with BOM, RFC 4180 quoting. Zero third-party dependencies.
bool writeCsv(const Result& result, const std::string& pathUtf8,
              const WriteOptions& options, std::string& error);

// XLSX via libxlsxwriter. When the library is not compiled in, this returns
// false with a clear error instead of producing a corrupt/empty file.
bool writeXlsx(const Result& result, const std::string& pathUtf8,
               const WriteOptions& options, std::string& error);

// Dispatches by extension (.xlsx / .csv); unknown extension falls back to CSV.
bool writeTable(const Result& result, const std::string& pathUtf8,
                const WriteOptions& options, std::string& error);

// Writes a machine-readable UTF-8 JSON report for CI consumption.
bool writeJsonReport(const Result& result, const std::string& pathUtf8,
                     std::string& error);

std::string jsonEscape(const std::string& text);

} // namespace ltx
