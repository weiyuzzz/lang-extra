#include "langtext/Filter.h"

#include <algorithm>

namespace ltx {
namespace {

bool isAsciiSpace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

bool isAsciiLetter(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

} // namespace

std::string trimText(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && isAsciiSpace(static_cast<unsigned char>(text[begin]))) ++begin;
    while (end > begin && isAsciiSpace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(begin, end - begin);
}

bool shouldSkip(const std::string& utf8Text, const FilterRules& rules) {
    const std::string text = trimText(utf8Text);

    if (text.empty()) return rules.skipBlank;

    if (std::find(rules.skipExact.begin(), rules.skipExact.end(), text) !=
        rules.skipExact.end()) {
        return true;
    }

    if (!rules.skipPureNumeric) return false;

    // A string is translatable as soon as it contains a letter: an ASCII
    // letter or any non-ASCII byte (CJK, Arabic, Hebrew, Cyrillic, ...).
    // Length is intentionally ignored, so long sentences are never dropped.
    for (unsigned char c : text) {
        if (c >= 0x80 || isAsciiLetter(c)) return false;
    }
    return true;
}

} // namespace ltx
