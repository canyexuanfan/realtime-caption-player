#pragma once

#include "captions/CaptionTypes.h"
#include "core/Result.h"
#include <QString>
#include <QList>

namespace rcp::captions {

/// Exports caption segments to SubRip (SRT) format (tech plan 6.x / T0188).
class SrtExporter {
public:
    /// Export all committed (final/revision) segments, sorted by start time.
    static QString exportSrt(const QList<CaptionSegment>& segments);

    /// Export only segments overlapping [fromMs, toMs] (T0190 partial export).
    static QString exportRange(const QList<CaptionSegment>& segments,
                               long long fromMs, long long toMs);

    /// Write SRT atomically (temp file + atomic rename) with UTF-8 BOM.
    static Result<void> writeSrt(const QString& path, const QList<CaptionSegment>& segments);
};

} // namespace rcp::captions
