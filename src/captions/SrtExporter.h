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

    /// WebVTT 导出（C1：自动导出格式设置真实生效）。
    static QString exportVtt(const QList<CaptionSegment>& segments);
    static Result<void> writeVtt(const QString& path, const QList<CaptionSegment>& segments);

    /// 纯文本导出（仅句子文本，无时间轴）。
    static QString exportTxt(const QList<CaptionSegment>& segments);
    static Result<void> writeTxt(const QString& path, const QList<CaptionSegment>& segments);
};

} // namespace rcp::captions
