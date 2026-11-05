#include "langtext/Model.h"

namespace ltx {

const char* issueKindName(IssueKind kind) {
    switch (kind) {
        case IssueKind::MissingTranslation: return "missing_translation";
        case IssueKind::OrphanText:         return "orphan_text";
        case IssueKind::DuplicateKey:       return "duplicate_key";
        case IssueKind::ParseError:         return "parse_error";
        case IssueKind::Skipped:            return "skipped";
        case IssueKind::IoError:            return "io_error";
    }
    return "unknown";
}

} // namespace ltx
