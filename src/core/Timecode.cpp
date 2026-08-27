#include "core/Timecode.h"
#include <QRegularExpression>

#include <cmath>

namespace rcp {

QString formatSrtTime(Milliseconds ms) {
    if (ms < Milliseconds(0)) ms = Milliseconds(0);
    const qint64 total = ms.count();
    const qint64 hours = total / 3'600'000;
    const qint64 minutes = (total % 3'600'000) / 60'000;
    const qint64 seconds = (total % 60'000) / 1'000;
    const qint64 millis = total % 1'000;
    // Hours use a 2-digit minimum so normal SRT stays "HH:MM:SS,mmm", while
    // values > 99h still render their full width (e.g. "100:00:00,000").
    return QString::asprintf("%02lld:%02lld:%02lld,%03lld",
                              static_cast<long long>(hours),
                              static_cast<long long>(minutes),
                              static_cast<long long>(seconds),
                              static_cast<long long>(millis));
}

QString formatClock(Milliseconds ms) {
    if (ms < Milliseconds(0)) ms = Milliseconds(0);
    const qint64 total = ms.count();
    const qint64 hours = total / 3'600'000;
    const qint64 minutes = (total % 3'600'000) / 60'000;
    const qint64 seconds = (total % 60'000) / 1'000;
    if (hours > 0)
        return QString::asprintf("%lld:%02lld:%02lld",
                                  static_cast<long long>(hours),
                                  static_cast<long long>(minutes),
                                  static_cast<long long>(seconds));
    // Minutes are NOT zero-padded when they are the leading field
    // (tech plan: "MM:SS" under one hour -> "0:00", "1:05").
    return QString::asprintf("%lld:%02lld",
                             static_cast<long long>(minutes),
                             static_cast<long long>(seconds));
}

Milliseconds parseSrtTime(const QString& text) {
    // Accept "HH:MM:SS,mmm" or "HH:MM:SS.mmm"
    const QString t = text.trimmed();
    QRegularExpression re(QStringLiteral(R"(^(\d+):([0-5]?\d):([0-5]?\d)[,.](\d{1,3})$)"));
    const QRegularExpressionMatch m = re.match(t);
    if (!m.hasMatch()) return Milliseconds(-1);
    const qint64 h = m.captured(1).toLongLong();
    const qint64 mi = m.captured(2).toLongLong();
    const qint64 s = m.captured(3).toLongLong();
    const qint64 ms = m.captured(4).toLongLong();
    return Milliseconds(h * 3'600'000 + mi * 60'000 + s * 1'000 + ms);
}

Milliseconds clampTime(Milliseconds ms, Milliseconds maxMs) {
    if (ms < Milliseconds(0)) return Milliseconds(0);
    if (maxMs >= Milliseconds(0) && ms > maxMs) return maxMs;
    return ms;
}

} // namespace rcp
