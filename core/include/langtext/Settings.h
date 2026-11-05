#pragma once

#include "Model.h"

#include <string>
#include <vector>

namespace ltx {

struct FilterRules {
    bool skipBlank = true;
    bool skipPureNumeric = true;
    std::vector<std::string> skipExact;
};

struct Settings {
    std::vector<Language> languages;
    std::vector<std::string> textTags;        // elements whose text is always collected
    std::vector<std::string> specialTags;     // elements whose text is collected too
    std::vector<std::string> keyAttributes;   // attributes used to build stable keys
    FilterRules filter;
    std::string groupMapFile = "config.xml";
};

// Built-in fallback: used when settings.xml cannot be found, so the tool still works.
Settings defaultSettings();

// Loads a UTF-8 XML settings file. Returns false and fills 'error' on failure.
bool loadSettings(const std::string& pathUtf8, Settings& out, std::string& error);

// Looks for <fileName> next to 'startDir' and up to 'maxLevels' parents.
// Returns an empty string when nothing is found.
std::string findConfigFile(const std::string& startDirUtf8,
                           const std::string& fileName,
                           int maxLevels = 3);

} // namespace ltx
