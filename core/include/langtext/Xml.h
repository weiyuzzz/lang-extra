#pragma once

#include "Model.h"

#include <string>
#include <utility>
#include <vector>

namespace ltx {

struct TextNode {
    std::string key;        // stable key derived from the element path
    std::string text;       // UTF-8, entities already decoded
    std::size_t line = 0;
};

// Reads the whole file as UTF-8 bytes (wide paths work on Windows) and parses
// with TIXML_ENCODING_UTF8 so &#x....; / &amp; entities are decoded correctly.
bool readFileUtf8(const std::string& pathUtf8, std::string& bytes, std::string& error);

bool parseTextNodes(const std::string& fileUtf8,
                    const std::vector<std::string>& textTags,
                    const std::vector<std::string>& specialTags,
                    const std::vector<std::string>& keyAttributes,
                    std::vector<TextNode>& out,
                    std::string& error);

// Loads the nested <root><a><b>title</b></a></root> group map into flat
// "a/b" -> "title" pairs. Whitespace around titles is trimmed.
bool loadGroupMap(const std::string& fileUtf8,
                  std::vector<std::pair<std::string, std::string>>& out,
                  std::string& error);

} // namespace ltx
