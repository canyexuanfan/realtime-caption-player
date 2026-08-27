#pragma once

#include <QString>
#include <utility>

namespace rcp {

/// Kind of a caption utterance delivered by the worker.
enum class CaptionKind {
    Partial,  ///< interim hypothesis, may still change
    Final,    ///< committed sentence (SenseVoice final or Lite final)
    Revision  ///< late correction of an already-final segment
};

inline QString captionKindName(CaptionKind k) {
    switch (k) {
    case CaptionKind::Partial: return QStringLiteral("partial");
    case CaptionKind::Final:   return QStringLiteral("final");
    case CaptionKind::Revision:return QStringLiteral("revision");
    }
    return QStringLiteral("unknown");
}

/// Stable id assigned by the assembler.
using CaptionId = QString;

/// A single committed or interim caption utterance.
struct CaptionSegment {
    CaptionId id;                 ///< stable id assigned by the assembler
    long long startMs = 0;        ///< inclusive start, milliseconds
    long long endMs = 0;          ///< inclusive end, milliseconds
    QString text;                 ///< decoded text (may be empty for partials)
    CaptionKind kind = CaptionKind::Final;
    double confidence = 1.0;      ///< 0..1, -1 if unknown
    QString language;             ///< ISO-ish code ("zh", "en", ...)
    quint64 generation = 0;       ///< owning session generation (isolation, rule 28)

    bool isValid() const {
        return endMs >= startMs && startMs >= 0;
    }

    bool operator==(const CaptionSegment&) const = default;
};

/// Display snapshot consumed by the UI overlay.
struct CaptionLine {
    CaptionId id;
    QString text;
    CaptionKind kind = CaptionKind::Final;
    long long startMs = 0;
    long long endMs = 0;

    bool operator==(const CaptionLine&) const = default;
};

} // namespace rcp
