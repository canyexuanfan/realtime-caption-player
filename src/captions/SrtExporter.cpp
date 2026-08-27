#include "captions/SrtExporter.h"

#include "core/Timecode.h"
#include <QFile>
#include <QSaveFile>
#include <QList>
#include <algorithm>
#include <chrono>

namespace rcp::captions {

namespace {
bool segmentLess(const CaptionSegment& a, const CaptionSegment& b) {
    if (a.startMs != b.startMs) return a.startMs < b.startMs;
    return a.endMs < b.endMs;
}

/// Build SRT body from an already-filtered, sorted list of final segments.
QString buildSrt(const QList<CaptionSegment>& segments) {
    QString out;
    out.reserve(segments.size() * 64);
    int index = 1;
    for (const CaptionSegment& seg : segments) {
        if (seg.text.trimmed().isEmpty()) continue; // skip blank lines
        if (seg.endMs < seg.startMs) continue;       // invalid span
        out.append(QString::number(index++)).append(QLatin1Char('\n'));
        const QString start = formatSrtTime(std::chrono::milliseconds(seg.startMs));
        const QString end = formatSrtTime(std::chrono::milliseconds(seg.endMs));
        out.append(start).append(QStringLiteral(" --> ")).append(end).append(QLatin1Char('\n'));
        out.append(seg.text).append(QLatin1Char('\n')).append(QLatin1Char('\n'));
    }
    return out;
}

QList<CaptionSegment> finalsSorted(const QList<CaptionSegment>& segments) {
    QList<CaptionSegment> finals;
    for (const CaptionSegment& s : segments) {
        if (s.kind == CaptionKind::Partial) continue; // only commit final/revision
        finals.append(s);
    }
    std::sort(finals.begin(), finals.end(), segmentLess);
    return finals;
}
} // namespace

QString SrtExporter::exportSrt(const QList<CaptionSegment>& segments) {
    return buildSrt(finalsSorted(segments));
}

QString SrtExporter::exportRange(const QList<CaptionSegment>& segments,
                                 long long fromMs, long long toMs) {
    QList<CaptionSegment> inRange;
    for (const CaptionSegment& s : finalsSorted(segments)) {
        // Keep segments that overlap [fromMs, toMs].
        if (s.endMs < fromMs) continue;
        if (s.startMs > toMs) continue;
        CaptionSegment clipped = s;
        clipped.startMs = std::max(s.startMs, fromMs);
        clipped.endMs = std::min(s.endMs, toMs);
        inRange.append(clipped);
    }
    return buildSrt(inRange);
}

Result<void> SrtExporter::writeSrt(const QString& path, const QList<CaptionSegment>& segments) {
    const QString body = exportSrt(segments);
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return Result<void>::fail(AppError(ErrorDomain::Export,
            QStringLiteral("SRT-WRITE-FAILED"),
            QStringLiteral("无法写入 SRT 文件"),
            QStringLiteral("QSaveFile::open failed")));
    // SRT files are UTF-8 with BOM for maximal player compatibility.
    const QByteArray bom = QByteArray::fromHex("efbbbf");
    if (f.write(bom) != bom.size()) return Result<void>::fail(AppError(ErrorDomain::Export,
        QStringLiteral("SRT-WRITE-FAILED"), QStringLiteral("无法写入 SRT 文件"),
        QStringLiteral("short BOM write")));
    const QByteArray data = body.toUtf8();
    if (f.write(data) != data.size()) return Result<void>::fail(AppError(ErrorDomain::Export,
        QStringLiteral("SRT-WRITE-FAILED"), QStringLiteral("无法写入 SRT 文件"),
        QStringLiteral("short body write")));
    if (!f.commit()) return Result<void>::fail(AppError(ErrorDomain::Export,
        QStringLiteral("SRT-WRITE-FAILED"), QStringLiteral("无法写入 SRT 文件"),
        QStringLiteral("QSaveFile::commit failed")));
    return Result<void>::ok();
}

} // namespace rcp::captions
