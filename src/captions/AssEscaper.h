#pragma once

#include <QString>

namespace rcp::captions {

/// Escapes plain text into ASS (SubStation Alpha) override-safe text.
/// ASS reserves `{` `}` for overrides, `\` for escapes, and `\N` for newlines.
class AssEscaper {
public:
    /// Escape a single string fragment for safe inclusion in an ASS line.
    static QString escape(const QString& text);

    /// Convenience: escape and optionally collapse newlines into spaces.
    static QString toAssText(const QString& text, bool stripNewlines = false);
};

} // namespace rcp::captions
