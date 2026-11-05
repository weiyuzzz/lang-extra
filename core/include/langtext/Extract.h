#pragma once

#include "Model.h"
#include "Settings.h"

#include <functional>
#include <string>

namespace ltx {

struct ExtractOptions {
    std::string inputRootUtf8;
    Settings settings;
    std::string groupMapFileUtf8;   // empty = resolve next to settings/root
    std::function<void(float progress, const std::string& message)> onProgress;
    std::function<bool()> isCancelled;
};

// Top-level pipeline: scan -> parse -> align by key -> build tables + issues.
// Never throws; all failures are reported through Result::globalIssues.
Result extract(const ExtractOptions& options);

} // namespace ltx
