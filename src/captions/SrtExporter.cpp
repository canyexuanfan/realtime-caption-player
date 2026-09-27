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

/// WebVTT 时间戳 HH:MM:SS.mmm。
QString formatVttTime(std::chrono::milliseconds ms) {
    const auto total = static_cast<long long>(ms.count());
    const int h = static_cast<int>(total / 3600000LL);
    const int m = static_cast<int>((total / 60000LL) % 60LL);
    const int s = static_cast<int>((total / 1000LL) % 60LL);
    const int msec = static_cast<int>(total % 1000LL);
    return QStringLiteral("%1:%2:%3.%4")
        .arg(h, 2, 10, QLatin1Char('0')).arg(m, 2, 10, QLatin1Char('0'))
        .arg(s, 2, 10, QLatin1Char('0')).arg(msec, 3, 10, QLatin1Char('0'));
}

QString buildVtt(const QList<CaptionSegment>& segments) {
    QString out;
    out.reserve(segments.size() * 64);
    out.append(QStringLiteral("WEBVTT\n\n"));
    int index = 1;
    for (const CaptionSegment& seg : segments) {
        if (seg.text.trimmed().isEmpty()) continue;
        if (seg.endMs < seg.startMs) continue;
        out.append(QString::number(index++)).append(QLatin1Char('\n'));
        out.append(formatVttTime(std::chrono::milliseconds(seg.startMs)))
            .append(QStringLiteral(" --> "))
            .append(formatVttTime(std::chrono::milliseconds(seg.endMs)))
            .append(QLatin1Char('\n'));
        out.append(seg.text).append(QLatin1Char('\n')).append(QLatin1Char('\n'));
    }
    return out;
}

QString buildTxt(const QList<CaptionSegment>& segments) {
    QString out;
    for (const CaptionSegment& seg : segments) {
        const QString t = seg.text.trimmed();
        if (t.isEmpty()) continue;
        out.append(t).append(QLatin1Char('\n'));
    }
    return out;
}

/// 原子写（QSaveFile）：SRT/VTT 带 UTF-8 BOM，TXT 不带。
Result<void> writeAtomic(const QString& path, const QString& body, bool withBom) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return Result<void>::fail(AppError(ErrorDomain::Export,
            QStringLiteral("SRT-WRITE-FAILED"),
            QStringLiteral("无法写入字幕文件"),
            QStringLiteral("QSaveFile::open failed")));
    if (withBom) {
        const QByteArray bom = QByteArray::fromHex("efbbbf");
        if (f.write(bom) != bom.size()) return Result<void>::fail(AppError(ErrorDomain::Export,
            QStringLiteral("SRT-WRITE-FAILED"), QStringLiteral("无法写入字幕文件"),
            QStringLiteral("short BOM write")));
    }
    const QByteArray data = body.toUtf8();
    if (f.write(data) != data.size()) return Result<void>::fail(AppError(ErrorDomain::Export,
        QStringLiteral("SRT-WRITE-FAILED"), QStringLiteral("无法写入字幕文件"),
        QStringLiteral("short body write")));
    if (!f.commit()) return Result<void>::fail(AppError(ErrorDomain::Export,
        QStringLiteral("SRT-WRITE-FAILED"), QStringLiteral("无法写入字幕文件"),
        QStringLiteral("QSaveFile::commit failed")));
    return Result<void>::ok();
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
    return writeAtomic(path, exportSrt(segments), true);
}

QString SrtExporter::exportVtt(const QList<CaptionSegment>& segments) {
    return buildVtt(finalsSorted(segments));
}

Result<void> SrtExporter::writeVtt(const QString& path, const QList<CaptionSegment>& segments) {
    return writeAtomic(path, exportVtt(segments), true);
}

QString SrtExporter::exportTxt(const QList<CaptionSegment>& segments) {
    return buildTxt(finalsSorted(segments));
}

Result<void> SrtExporter::writeTxt(const QString& path, const QList<CaptionSegment>& segments) {
    return writeAtomic(path, exportTxt(segments), false);
}

} // namespace rcp::captions
