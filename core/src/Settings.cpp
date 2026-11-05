#include "langtext/Settings.h"
#include "langtext/Xml.h"

#include <algorithm>
#include <filesystem>

#include "tinyxml.h"

namespace fs = std::filesystem;

namespace ltx {
namespace {

std::string attr(const TiXmlElement* element, const char* name,
                 const std::string& fallback = "") {
    if (!element) return fallback;
    const char* value = element->Attribute(name);
    return value ? std::string(value) : fallback;
}

bool attrBool(const TiXmlElement* element, const char* name, bool fallback) {
    const std::string value = attr(element, name);
    if (value.empty()) return fallback;
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

void collectChildValues(const TiXmlElement* parent, const char* childName,
                        std::vector<std::string>& out) {
    if (!parent) return;
    for (const TiXmlNode* node = parent->FirstChild(); node;
         node = node->NextSibling()) {
        const TiXmlElement* child = node->ToElement();
        if (!child || std::string(child->Value()) != childName) continue;
        const char* text = child->GetText();
        if (text && *text) out.push_back(text);
    }
}

} // namespace

Settings defaultSettings() {
    Settings s;
    s.languages = {
        {"style.xml",      "zh-CN", "简体中文",   true },
        {"style_ar.xml",   "ar",    "Arabic",     false},
        {"style_cht.xml",  "zh-TW", "繁體中文",   false},
        {"style_de.xml",   "de",    "German",     false},
        {"style_en.xml",   "en",    "English",    false},
        {"style_es.xml",   "es",    "Spanish",    false},
        {"style_fr.xml",   "fr",    "French",     false},
        {"style_he.xml",   "he",    "Hebrew",     false},
        {"style_it.xml",   "it",    "Italian",    false},
        {"style_nl.xml",   "nl",    "Dutch",      false},
        {"style_pl.xml",   "pl",    "Polish",     false},
        {"style_pt.xml",   "pt",    "Portuguese", false},
        {"style_ru.xml",   "ru",    "Russian",    false},
        {"style_slo.xml",  "sk",    "Slovak",     false},
        {"style_tr.xml",   "tr",    "Turkish",    false},
        {"style_vi.xml",   "vi",    "Vietnamese", false},
    };
    s.textTags = {"text"};
    s.specialTags = {"panel", "wall", "center", "person", "busy"};
    s.keyAttributes = {"name", "id", "key"};
    return s;
}

bool loadSettings(const std::string& pathUtf8, Settings& out, std::string& error) {
    std::string bytes;
    if (!readFileUtf8(pathUtf8, bytes, error)) return false;

    TiXmlDocument doc;
    doc.Parse(bytes.c_str(), nullptr, TIXML_ENCODING_UTF8);
    if (doc.Error()) {
        error = "settings XML parse error at line " +
                std::to_string(doc.ErrorRow()) + ": " + doc.ErrorDesc();
        return false;
    }
    const TiXmlElement* root = doc.RootElement();
    if (!root || std::string(root->Value()) != "settings") {
        error = "settings XML root must be <settings>";
        return false;
    }

    Settings parsed = defaultSettings();
    std::vector<Language> languages;
    const TiXmlElement* languageList = root->FirstChildElement("languages");
    if (languageList) {
        for (const TiXmlNode* node = languageList->FirstChild(); node;
             node = node->NextSibling()) {
            const TiXmlElement* child = node->ToElement();
            if (!child || std::string(child->Value()) != "language") continue;
            Language lang;
            lang.file = attr(child, "file");
            lang.code = attr(child, "code");
            lang.name = attr(child, "name", lang.code);
            lang.reference = attrBool(child, "reference", false);
            if (!lang.file.empty() && !lang.code.empty()) languages.push_back(lang);
        }
    }
    if (languages.empty()) {
        error = "settings XML contains no valid <language> entries";
        return false;
    }
    if (std::none_of(languages.begin(), languages.end(),
                     [](const Language& l) { return l.reference; })) {
        languages.front().reference = true;
    }
    parsed.languages = languages;

    const TiXmlElement* extract = root->FirstChildElement("extract");
    if (extract) {
        std::vector<std::string> textTags;
        std::vector<std::string> specialTags;
        std::vector<std::string> keyAttributes;
        collectChildValues(extract->FirstChildElement("textTags"), "tag", textTags);
        collectChildValues(extract->FirstChildElement("specialTags"), "tag", specialTags);
        collectChildValues(extract->FirstChildElement("keyAttributes"), "attr", keyAttributes);
        if (!textTags.empty()) parsed.textTags = textTags;
        if (!specialTags.empty()) parsed.specialTags = specialTags;
        if (!keyAttributes.empty()) parsed.keyAttributes = keyAttributes;

        const TiXmlElement* filter = extract->FirstChildElement("filter");
        if (filter) {
            parsed.filter.skipBlank = attrBool(filter, "skipBlank", true);
            parsed.filter.skipPureNumeric = attrBool(filter, "skipPureNumeric", true);
            collectChildValues(filter, "skipExact", parsed.filter.skipExact);
        }
    }

    const TiXmlElement* groupMap = root->FirstChildElement("groupMap");
    if (groupMap) {
        const std::string file = attr(groupMap, "file");
        if (!file.empty()) parsed.groupMapFile = file;
    }

    out = parsed;
    return true;
}

std::string findConfigFile(const std::string& startDirUtf8,
                           const std::string& fileName, int maxLevels) {
    std::error_code ec;
    fs::path dir = fs::u8path(startDirUtf8);
    if (dir.empty()) dir = fs::current_path(ec);
    for (int level = 0; level <= maxLevels; ++level) {
        fs::path candidate = dir / fs::u8path(fileName);
        if (fs::is_regular_file(candidate, ec)) return candidate.u8string();
        fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

} // namespace ltx
