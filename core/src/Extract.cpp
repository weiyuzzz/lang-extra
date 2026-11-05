#include "langtext/Extract.h"
#include "langtext/Filter.h"
#include "langtext/Scanner.h"
#include "langtext/Xml.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>

namespace fs = std::filesystem;

namespace ltx {
namespace {

Issue makeIssue(IssueKind kind, const std::string& group,
                const std::string& language, const std::string& key,
                const std::string& detail, const std::string& source,
                std::size_t line = 0) {
    Issue issue;
    issue.kind = kind;
    issue.group = group;
    issue.language = language;
    issue.key = key;
    issue.detail = detail;
    issue.source = source;
    issue.line = line;
    return issue;
}

std::size_t referenceIndex(const std::vector<Language>& languages) {
    for (std::size_t i = 0; i < languages.size(); ++i) {
        if (languages[i].reference) return i;
    }
    return 0;
}

std::string groupTitle(const std::map<std::string, std::string>& titles,
                       const std::string& relative) {
    if (relative.empty()) return "(root)";
    auto found = titles.find(relative);
    if (found != titles.end()) return found->second;
    return relative;
}

// Makes duplicate keys unique inside one file and records them as issues.
std::vector<Entry> normalizeEntries(const std::vector<TextNode>& nodes,
                                    const std::string& source,
                                    const std::string& group,
                                    const std::string& language,
                                    std::vector<Issue>& issues) {
    std::map<std::string, int> seen;
    std::vector<Entry> entries;
    entries.reserve(nodes.size());
    for (const TextNode& node : nodes) {
        const int count = ++seen[node.key];
        Entry entry;
        entry.key = count == 1 ? node.key
                               : node.key + "~" + std::to_string(count);
        entry.text = node.text;
        entry.source = source;
        entry.line = node.line;
        if (count > 1) {
            issues.push_back(makeIssue(IssueKind::DuplicateKey, group, language,
                                       node.key,
                                       "duplicate key in " + source +
                                       ", renamed to " + entry.key,
                                       source, node.line));
        }
        entries.push_back(entry);
    }
    return entries;
}

} // namespace

Result extract(const ExtractOptions& options) {
    Result result;
    result.languages = options.settings.languages;
    const std::size_t languageCount = result.languages.size();
    const std::size_t refIndex = referenceIndex(result.languages);

    auto progress = [&](float value, const std::string& message) {
        if (options.onProgress) options.onProgress(value, message);
    };
    auto cancelled = [&]() {
        return options.isCancelled ? options.isCancelled() : false;
    };

    // Group titles are loaded once, not re-parsed for every folder.
    std::map<std::string, std::string> titles;
    std::string mapFile = options.groupMapFileUtf8;
    if (mapFile.empty()) {
        mapFile = findConfigFile(options.inputRootUtf8,
                                 options.settings.groupMapFile);
    }
    if (mapFile.empty()) {
        std::error_code ec;
        mapFile = findConfigFile(fs::current_path(ec).u8string(),
                                 options.settings.groupMapFile);
    }
    if (!mapFile.empty()) {
        std::vector<std::pair<std::string, std::string>> pairs;
        std::string error;
        if (loadGroupMap(mapFile, pairs, error)) {
            for (const auto& pair : pairs) titles[pair.first] = pair.second;
        } else {
            result.globalIssues.push_back(makeIssue(
                IssueKind::IoError, "", "", "",
                "cannot load group map " + mapFile + ": " + error, mapFile));
        }
    } else {
        result.globalIssues.push_back(makeIssue(
            IssueKind::IoError, "", "", "",
            "group map not found: " + options.settings.groupMapFile +
            " (folder titles fall back to relative paths)", ""));
    }

    std::vector<Folder> folders;
    std::string error;
    if (!scanFolders(options.inputRootUtf8, result.languages, folders,
                     result.globalIssues, error)) {
        result.globalIssues.push_back(makeIssue(
            IssueKind::IoError, "", "", "", error, options.inputRootUtf8));
        ++result.stats.errors;
        return result;
    }

    for (std::size_t folderIndex = 0; folderIndex < folders.size(); ++folderIndex) {
        if (cancelled()) {
            result.globalIssues.push_back(makeIssue(
                IssueKind::IoError, "", "", "", "cancelled by user", ""));
            break;
        }
        const Folder& folder = folders[folderIndex];

        Table table;
        table.folder = folder.relative;
        table.title = groupTitle(titles, folder.relative);

        std::vector<std::vector<Entry>> perLanguage(languageCount);
        for (const SourceFile& file : folder.files) {
            const Language& language = result.languages[
                static_cast<std::size_t>(file.languageIndex)];
            std::vector<TextNode> nodes;
            std::string parseError;
            if (!parseTextNodes(file.path, options.settings.textTags,
                                options.settings.specialTags,
                                options.settings.keyAttributes,
                                nodes, parseError)) {
                table.issues.push_back(makeIssue(
                    IssueKind::ParseError, folder.relative, language.code, "",
                    parseError, file.relative));
                ++result.stats.errors;
                continue;
            }
            ++result.stats.sourceFiles;
            perLanguage[static_cast<std::size_t>(file.languageIndex)] =
                normalizeEntries(nodes, file.relative, folder.relative,
                                 language.code, table.issues);
        }

        // The reference language defines the canonical key order. Filtering is
        // applied ONLY here, so all languages are judged by the same rule.
        std::vector<std::string> keys;
        std::set<std::string> skippedKeys;
        std::set<std::string> canonicalKeys;
        for (const Entry& entry : perLanguage[refIndex]) {
            canonicalKeys.insert(entry.key);
            if (shouldSkip(entry.text, options.settings.filter)) {
                skippedKeys.insert(entry.key);
                ++result.stats.skipped;
                table.issues.push_back(makeIssue(
                    IssueKind::Skipped, folder.relative,
                    result.languages[refIndex].code, entry.key,
                    "non-translatable text skipped: " + entry.text,
                    entry.source, entry.line));
                continue;
            }
            keys.push_back(entry.key);
        }

        table.keys = keys;
        table.rows.assign(keys.size(), std::vector<std::string>(languageCount));
        std::map<std::string, std::string> referenceValues;
        for (const Entry& entry : perLanguage[refIndex]) {
            referenceValues[entry.key] = entry.text;
        }
        for (std::size_t r = 0; r < keys.size(); ++r) {
            table.rows[r][refIndex] = referenceValues[keys[r]];
        }

        for (std::size_t lang = 0; lang < languageCount; ++lang) {
            if (lang == refIndex) continue;
            const Language& language = result.languages[lang];
            std::map<std::string, std::string> values;
            for (const Entry& entry : perLanguage[lang]) {
                values[entry.key] = entry.text;
            }
            for (std::size_t r = 0; r < keys.size(); ++r) {
                auto found = values.find(keys[r]);
                if (found == values.end() || found->second.empty()) {
                    ++result.stats.missing;
                    table.issues.push_back(makeIssue(
                        IssueKind::MissingTranslation, folder.relative,
                        language.code, keys[r],
                        "missing translation for key " + keys[r], folder.relative));
                    continue;
                }
                table.rows[r][lang] = found->second;
            }
            for (const auto& pair : values) {
                if (canonicalKeys.count(pair.first) == 0) {
                    ++result.stats.orphan;
                    table.issues.push_back(makeIssue(
                        IssueKind::OrphanText, folder.relative, language.code,
                        pair.first, "key has no reference-language entry",
                        folder.relative));
                }
            }
        }

        for (const Issue& issue : table.issues) {
            if (issue.kind == IssueKind::DuplicateKey) ++result.stats.duplicate;
        }
        result.stats.groups += 1;
        result.stats.entries += keys.size();
        result.tables.push_back(std::move(table));

        progress(static_cast<float>(folderIndex + 1) /
                     static_cast<float>(folders.size()),
                 "processed " + folder.relative);
    }

    return result;
}

} // namespace ltx
