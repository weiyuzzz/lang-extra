#include "langtext/Extract.h"
#include "langtext/Settings.h"
#include "langtext/TableWriter.h"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace fs = std::filesystem;

namespace {

struct Options {
    std::string input;
    std::string output;
    std::string settingsFile;
    std::string groupMapFile;
    std::string report;
    bool includeKeyColumn = true;
    bool markMissing = true;
    bool failOnMissing = false;
    bool quiet = false;
    bool help = false;
    bool valid = true;
    std::string error;
};

void printHelp() {
    std::cout
        << "langTextExtra CLI - extract DuiLib style*.xml text into a translation table\n"
        << "\n"
        << "Usage:\n"
        << "  langtext --input DIR --output FILE [options]\n"
        << "\n"
        << "Options:\n"
        << "  --input DIR          Directory tree containing style*.xml files\n"
        << "  --output FILE        Output table (.csv, or .xlsx when built with XLSX support)\n"
        << "  --settings FILE      Language/rule configuration (default: settings.xml)\n"
        << "  --config FILE        Folder-title map (default: from settings.xml)\n"
        << "  --report FILE        Also write a machine-readable JSON issue report\n"
        << "  --no-key-column      Hide the leading key column\n"
        << "  --no-missing-marker  Leave missing cells empty instead of [MISSING]\n"
        << "  --fail-on-missing    Exit with code 2 when translations are missing (for CI)\n"
        << "  --quiet              Suppress progress output\n"
        << "  --help               Show this message\n"
        << "\n"
        << "Exit codes: 0 success, 1 error, 2 missing translations (with --fail-on-missing)\n";
}

#ifdef _WIN32
std::string wideToUtf8(const wchar_t* wide) {
    if (!wide) return {};
    const int length = WideCharToMultiByte(
        CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) return {};
    std::string out(static_cast<std::size_t>(length - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &out[0], length,
                        nullptr, nullptr);
    return out;
}
#endif

// On Windows the CRT hands `char** argv` to us in the process ANSI code page
// (GBK/CP1252/...), NOT UTF-8. Passing those bytes to std::filesystem::u8path
// produces an invalid path (and with some standard libraries a hard crash),
// so read the native UTF-16 command line and convert to UTF-8 ourselves.
std::vector<std::string> commandLineUtf8(int argc, char** argv) {
#ifdef _WIN32
    (void)argc;
    (void)argv;
    int count = 0;
    LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count);
    std::vector<std::string> out;
    if (wide) {
        out.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) out.push_back(wideToUtf8(wide[i]));
        LocalFree(wide);
    }
    if (out.empty()) out.emplace_back();
#else
    std::vector<std::string> out;
    out.reserve(static_cast<std::size_t>(argc > 0 ? argc : 0));
    for (int i = 0; i < argc; ++i) out.emplace_back(argv[i] ? argv[i] : "");
#endif
    return out;
}

Options parseArgs(const std::vector<std::string>& args) {
    Options options;
    const std::size_t count = args.size();
    for (std::size_t i = 1; i < count; ++i) {
        const std::string& arg = args[i];
        auto value = [&](const std::string& name, std::string& target) {
            if (i + 1 >= count) {
                options.valid = false;
                options.error = "missing value for " + name;
                return;
            }
            target = args[++i];
        };
        if (arg == "--input") value(arg, options.input);
        else if (arg == "--output") value(arg, options.output);
        else if (arg == "--settings") value(arg, options.settingsFile);
        else if (arg == "--config") value(arg, options.groupMapFile);
        else if (arg == "--report") value(arg, options.report);
        else if (arg == "--no-key-column") options.includeKeyColumn = false;
        else if (arg == "--no-missing-marker") options.markMissing = false;
        else if (arg == "--fail-on-missing") options.failOnMissing = true;
        else if (arg == "--quiet") options.quiet = true;
        else if (arg == "--help" || arg == "-h") options.help = true;
        else {
            options.valid = false;
            options.error = "unknown argument: " + arg;
        }
        if (!options.valid) break;
    }
    if (options.valid && !options.help) {
        if (options.input.empty()) {
            options.valid = false;
            options.error = "--input is required";
        } else if (options.output.empty()) {
            options.valid = false;
            options.error = "--output is required";
        }
    }
    return options;
}

std::string resolveConfig(const std::string& explicitPath,
                          const std::string& fileName,
                          const std::string& inputRoot,
                          const std::string& programPath) {
    if (!explicitPath.empty()) return explicitPath;
    std::string found = ltx::findConfigFile(inputRoot, fileName);
    if (!found.empty()) return found;
    std::error_code ec;
    found = ltx::findConfigFile(fs::current_path(ec).u8string(), fileName);
    if (!found.empty()) return found;
    const fs::path programDir = fs::u8path(programPath).parent_path();
    if (!programDir.empty()) {
        found = ltx::findConfigFile(programDir.u8string(), fileName);
    }
    return found;
}

int run(int argc, char** argv) {
    const std::vector<std::string> args = commandLineUtf8(argc, argv);
    const std::string programPath = args.empty() ? std::string() : args[0];
    const Options options = parseArgs(args);
    if (options.help) {
        printHelp();
        return 0;
    }
    if (!options.valid) {
        std::cerr << "langtext: " << options.error << "\n"
                  << "Run 'langtext --help' for usage.\n";
        return 1;
    }

    {
        std::error_code ec;
        if (!fs::is_directory(fs::u8path(options.input), ec)) {
            std::cerr << "langtext: input directory does not exist or is not a "
                         "directory: " << options.input << "\n";
            return 1;
        }
    }

    const std::string settingsPath = resolveConfig(
        options.settingsFile, "settings.xml", options.input, programPath);

    ltx::Settings settings = ltx::defaultSettings();
    if (!settingsPath.empty()) {
        std::string error;
        if (!ltx::loadSettings(settingsPath, settings, error)) {
            std::cerr << "langtext: cannot load " << settingsPath
                      << ": " << error << "\n";
            return 1;
        }
    } else if (!options.quiet) {
        std::cout << "langtext: settings.xml not found, using built-in defaults\n";
    }

    ltx::ExtractOptions extractOptions;
    extractOptions.inputRootUtf8 = options.input;
    extractOptions.settings = settings;
    extractOptions.groupMapFileUtf8 = options.groupMapFile;
    if (extractOptions.groupMapFileUtf8.empty()) {
        const std::string mapPath = resolveConfig(
            "", settings.groupMapFile, options.input, programPath);
        extractOptions.groupMapFileUtf8 = mapPath;
    }
    if (!options.quiet) {
        extractOptions.onProgress = [](float progress, const std::string& message) {
            std::cout << "[" << static_cast<int>(progress * 100) << "%] "
                      << message << "\n";
        };
    }

    const ltx::Result result = ltx::extract(extractOptions);

    ltx::WriteOptions writeOptions;
    writeOptions.includeKeyColumn = options.includeKeyColumn;
    writeOptions.markMissing = options.markMissing;

    std::string error;
    if (!ltx::writeTable(result, options.output, writeOptions, error)) {
        std::cerr << "langtext: " << error << "\n";
        return 1;
    }
    if (!options.report.empty()) {
        if (!ltx::writeJsonReport(result, options.report, error)) {
            std::cerr << "langtext: " << error << "\n";
            return 1;
        }
    }

    if (!options.quiet) {
        std::cout << "langtext: wrote " << options.output << "\n"
                  << "  groups=" << result.stats.groups
                  << " files=" << result.stats.sourceFiles
                  << " entries=" << result.stats.entries
                  << " missing=" << result.stats.missing
                  << " orphan=" << result.stats.orphan
                  << " duplicate=" << result.stats.duplicate
                  << " skipped=" << result.stats.skipped
                  << " errors=" << result.stats.errors << "\n";
    }

    if (result.stats.errors > 0) return 1;
    if (options.failOnMissing &&
        (result.stats.missing > 0 || result.stats.orphan > 0)) {
        return 2;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "langtext: unexpected error: " << e.what() << "\n";
        return 1;
    }
}
