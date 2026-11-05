#include "langtext/Xml.h"
#include "langtext/Filter.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "tinyxml.h"

namespace fs = std::filesystem;

namespace ltx {
namespace {

bool contains(const std::vector<std::string>& values, const std::string& needle) {
    for (const std::string& value : values) {
        if (value == needle) return true;
    }
    return false;
}

std::string keySuffix(const TiXmlElement* element,
                      const std::vector<std::string>& keyAttributes,
                      int siblingIndex, bool hasSiblings) {
    for (const std::string& name : keyAttributes) {
        const char* value = element->Attribute(name.c_str());
        if (value && *value) return "#" + std::string(value);
    }
    if (hasSiblings) return "[" + std::to_string(siblingIndex) + "]";
    return {};
}

void collectFrom(const TiXmlNode* container, const std::string& containerPath,
                 bool inheritedCollect, bool thisSpecial,
                 const std::vector<std::string>& textTags,
                 const std::vector<std::string>& specialTags,
                 const std::vector<std::string>& keyAttributes,
                 std::vector<TextNode>& out) {
    std::map<std::string, int> tagCounts;
    for (const TiXmlNode* node = container->FirstChild(); node;
         node = node->NextSibling()) {
        if (node->Type() == TiXmlNode::TINYXML_ELEMENT) {
            ++tagCounts[std::string(node->Value())];
        }
    }

    std::map<std::string, int> tagSeen;
    int textIndex = 0;
    for (const TiXmlNode* node = container->FirstChild(); node;
         node = node->NextSibling()) {
        if (node->Type() == TiXmlNode::TINYXML_TEXT) {
            if (inheritedCollect || thisSpecial) {
                TextNode item;
                item.key = containerPath + "#" + std::to_string(textIndex);
                item.text = trimText(node->Value() ? node->Value() : "");
                item.line = node->Row() > 0 ? static_cast<std::size_t>(node->Row()) : 0;
                out.push_back(item);
            }
            ++textIndex;
            continue;
        }
        if (node->Type() != TiXmlNode::TINYXML_ELEMENT) continue;

        const TiXmlElement* element = node->ToElement();
        const std::string tag = node->Value();
        const int index = tagSeen[tag]++;
        const bool repeated = tagCounts[tag] > 1;
        const std::string segment = tag +
            keySuffix(element, keyAttributes, index, repeated);
        const std::string childPath = containerPath + "/" + segment;
        const bool childInherited = inheritedCollect || contains(textTags, tag);
        const bool childSpecial = contains(specialTags, tag);
        collectFrom(element, childPath, childInherited, childSpecial,
                    textTags, specialTags, keyAttributes, out);
    }
}

void walkGroup(const TiXmlElement* element, const std::string& prefix,
               std::vector<std::pair<std::string, std::string>>& out) {
    const std::string name = element->Value();
    const std::string path = prefix.empty() ? name : prefix + "/" + name;

    for (const TiXmlNode* node = element->FirstChild(); node;
         node = node->NextSibling()) {
        if (node->Type() == TiXmlNode::TINYXML_TEXT) {
            const std::string title = trimText(node->Value() ? node->Value() : "");
            if (!title.empty()) {
                out.emplace_back(path, title);
                break;
            }
        }
    }
    for (const TiXmlNode* node = element->FirstChild(); node;
         node = node->NextSibling()) {
        if (node->Type() == TiXmlNode::TINYXML_ELEMENT) {
            walkGroup(node->ToElement(), path, out);
        }
    }
}

} // namespace

bool readFileUtf8(const std::string& pathUtf8, std::string& bytes,
                  std::string& error) {
    std::ifstream in(fs::u8path(pathUtf8), std::ios::binary);
    if (!in) {
        error = "cannot open file: " + pathUtf8;
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    bytes = buffer.str();
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF) {
        bytes.erase(0, 3);
    }
    return true;
}

bool parseTextNodes(const std::string& fileUtf8,
                    const std::vector<std::string>& textTags,
                    const std::vector<std::string>& specialTags,
                    const std::vector<std::string>& keyAttributes,
                    std::vector<TextNode>& out,
                    std::string& error) {
    std::string bytes;
    if (!readFileUtf8(fileUtf8, bytes, error)) return false;

    TiXmlDocument doc;
    doc.Parse(bytes.c_str(), nullptr, TIXML_ENCODING_UTF8);
    if (doc.Error()) {
        error = "XML parse error at line " + std::to_string(doc.ErrorRow()) +
                ": " + doc.ErrorDesc();
        return false;
    }
    if (!doc.RootElement()) {
        error = "XML document has no root element";
        return false;
    }
    collectFrom(&doc, "", false, false, textTags, specialTags,
                keyAttributes, out);
    return true;
}

bool loadGroupMap(const std::string& fileUtf8,
                  std::vector<std::pair<std::string, std::string>>& out,
                  std::string& error) {
    std::string bytes;
    if (!readFileUtf8(fileUtf8, bytes, error)) return false;

    TiXmlDocument doc;
    doc.Parse(bytes.c_str(), nullptr, TIXML_ENCODING_UTF8);
    if (doc.Error()) {
        error = "group map parse error at line " +
                std::to_string(doc.ErrorRow()) + ": " + doc.ErrorDesc();
        return false;
    }
    const TiXmlElement* root = doc.RootElement();
    if (!root) {
        error = "group map has no root element";
        return false;
    }
    for (const TiXmlNode* node = root->FirstChild(); node;
         node = node->NextSibling()) {
        if (node->Type() != TiXmlNode::TINYXML_ELEMENT) continue;
        const TiXmlElement* child = node->ToElement();
        // Flat form: <group path="misc/alert/login" title="..."/>
        const char* path = child->Attribute("path");
        if (std::string(child->Value()) == "group" && path && *path) {
            const char* title = child->Attribute("title");
            const char* text = child->GetText();
            std::string value = title ? title : (text ? text : "");
            value = trimText(value);
            if (!value.empty()) out.emplace_back(path, value);
            continue;
        }
        // Legacy nested form: <misc><alert><login>title</login></alert></misc>
        walkGroup(child, "", out);
    }
    return true;
}

} // namespace ltx
