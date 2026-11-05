#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ltx {

enum class IssueKind {
    MissingTranslation,
    OrphanText,
    DuplicateKey,
    ParseError,
    Skipped,
    IoError
};

const char* issueKindName(IssueKind kind);

struct Issue {
    IssueKind kind = IssueKind::IoError;
    std::string group;      // relative folder, generic separators
    std::string language;   // language code
    std::string key;        // entry key, when applicable
    std::string detail;
    std::string source;     // source file relative to input root
    std::size_t line = 0;
};

struct Language {
    std::string file;       // exact file name, matched case-insensitively
    std::string code;
    std::string name;
    bool reference = false; // base language: defines the canonical key order
};

struct Entry {
    std::string key;
    std::string text;       // UTF-8
    std::string source;     // relative source path
    std::size_t line = 0;
};

struct Table {
    std::string folder;                         // relative folder, generic separators
    std::string title;                          // group title from config.xml
    std::vector<std::string> keys;               // ordered by the reference language
    std::vector<std::vector<std::string>> rows;  // rows[r][c], empty = missing translation
    std::vector<Issue> issues;
};

struct Stats {
    std::size_t groups = 0;
    std::size_t sourceFiles = 0;
    std::size_t entries = 0;
    std::size_t missing = 0;
    std::size_t orphan = 0;
    std::size_t duplicate = 0;
    std::size_t skipped = 0;
    std::size_t errors = 0;
};

struct Result {
    std::vector<Table> tables;
    std::vector<Issue> globalIssues;
    std::vector<Language> languages;
    Stats stats;
};

} // namespace ltx
