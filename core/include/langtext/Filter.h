#pragma once

#include "Settings.h"

#include <string>

namespace ltx {

// Trims leading/trailing ASCII whitespace. Does not alter inner text.
std::string trimText(const std::string& text);

// Returns true only for genuinely non-translatable content (blank, pure
// numbers/placeholders/punctuation, or an explicit skipExact value).
// Length is deliberately NOT a criterion: long sentences must be kept.
bool shouldSkip(const std::string& utf8Text, const FilterRules& rules);

} // namespace ltx
