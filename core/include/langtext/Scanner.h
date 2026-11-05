#pragma once

#include "Model.h"

#include <string>
#include <vector>

namespace ltx {

struct SourceFile {
    std::string path;           // UTF-8 generic path
    std::string relative;       // relative to the input root
    int languageIndex = -1;     // index into Settings::languages
    int rowLine = 0;            // reserved
};

struct Folder {
    std::string relative;       // "" for the input root itself
    std::vector<SourceFile> files;
};

// Single pass over recursive_directory_iterator. No nested recursion, no
// duplicate entries, deterministic ordering, no uncaught filesystem errors.
bool scanFolders(const std::string& rootUtf8,
                 const std::vector<Language>& languages,
                 std::vector<Folder>& out,
                 std::vector<Issue>& issues,
                 std::string& error);

// Exact, case-insensitive match against a language file name.
int matchLanguageFile(const std::string& fileName,
                      const std::vector<Language>& languages);

} // namespace ltx
