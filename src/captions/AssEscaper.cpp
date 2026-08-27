#include "captions/AssEscaper.h"

#include <QChar>

namespace rcp::captions {

QString AssEscaper::escape(const QString& text) {
    QString out;
    out.reserve(text.size() + 16);
    for (QChar c : text) {
        switch (c.unicode()) {
        case '\n': out.append(QStringLiteral("\\N")); break;
        case '\\': out.append(QStringLiteral("\\\\")); break;
        case '{':  out.append(QStringLiteral("\\{")); break;
        case '}':  out.append(QStringLiteral("\\}")); break;
        default:
            // Drop raw carriage returns; they are not valid in ASS text.
            if (c.unicode() != '\r') out.append(c);
            break;
        }
    }
    return out;
}

QString AssEscaper::toAssText(const QString& text, bool stripNewlines) {
    if (stripNewlines)
        return escape(text).replace(QStringLiteral("\\N"), QStringLiteral(" "));
    return escape(text);
}

} // namespace rcp::captions
