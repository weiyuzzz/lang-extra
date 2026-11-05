#include "langtext/Scanner.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <system_error>

namespace fs = std::filesystem;

namespace ltx {
namespace {

std::string lowerAscii(std::string value) {
    for (char& c : value) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return value;
}

std::string genericUtf8(const fs::path& path) {
    std::string value = path.u8string();
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

std::string relativeGeneric(const fs::path& path, const fs::path& root,
                            std::error_code& ec) {
    fs::path rel = fs::relative(path, root, ec);
    if (ec) return {};
    return genericUtf8(rel);
}

} // namespace

int matchLanguageFile(const std::string& fileName,
                      const std::vector<Language>& languages) {
    const std::string needle = lowerAscii(fs::path(fileName).filename().u8string());
    for (std::size_t i = 0; i < languages.size(); ++i) {
        if (lowerAscii(languages[i].file) == needle) return static_cast<int>(i);
    }
    return -1;
}

bool scanFolders(const std::string& rootUtf8,
                 const std::vector<Language>& languages,
                 std::vector<Folder>& out,
                 std::vector<Issue>& issues,
                 std::string& error) {
    std::error_code ec;
    const fs::path root = fs::u8path(rootUtf8);
    if (!fs::is_directory(root, ec) || ec) {
        error = "input is not a readable directory: " + rootUtf8;
        return false;
    }

    std::map<std::string, Folder> folders;
    const fs::directory_options options =
        fs::directory_options::skip_permission_denied;
    fs::recursive_directory_iterator it(root, options, ec);
    if (ec) {
        error = "cannot start directory scan: " + ec.message();
        return false;
    }

    const fs::recursive_directory_iterator end;
    while (it != end) {
        const fs::directory_entry& entry = *it;
        bool isFile = entry.is_regular_file(ec);
        if (ec) {
            Issue issue;
            issue.kind = IssueKind::IoError;
            issue.detail = "cannot stat entry: " + ec.message();
            issue.source = genericUtf8(entry.path());
            issues.push_back(issue);
            ec.clear();
        }
        if (isFile) {
            const int langIndex = matchLanguageFile(
                entry.path().filename().u8string(), languages);
            if (langIndex >= 0) {
                const std::string relative = relativeGeneric(entry.path(), root, ec);
                const std::string folderRelative =
                    relativeGeneric(entry.path().parent_path(), root, ec);
                if (ec) {
                    Issue issue;
                    issue.kind = IssueKind::IoError;
                    issue.detail = "cannot resolve relative path: " + ec.message();
                    issue.source = genericUtf8(entry.path());
                    issues.push_back(issue);
                    ec.clear();
                } else {
                    Folder& folder = folders[folderRelative];
                    folder.relative = folderRelative;
                    SourceFile file;
                    file.path = genericUtf8(entry.path());
                    file.relative = relative;
                    file.languageIndex = langIndex;
                    folder.files.push_back(file);
                }
            }
        }

        it.increment(ec);
        if (ec) {
            Issue issue;
            issue.kind = IssueKind::IoError;
            issue.detail = "directory traversal stopped: " + ec.message();
            issues.push_back(issue);
            ec.clear();
            break;
        }
    }

    out.clear();
    for (auto& pair : folders) {
        Folder& folder = pair.second;
        std::sort(folder.files.begin(), folder.files.end(),
                  [](const SourceFile& a, const SourceFile& b) {
                      return a.relative < b.relative;
                  });
        if (!folder.files.empty()) out.push_back(folder);
    }
    return true;
}

} // namespace ltx
