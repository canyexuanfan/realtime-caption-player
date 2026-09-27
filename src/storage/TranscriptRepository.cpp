// src/storage/TranscriptRepository.cpp
#include "TranscriptRepository.h"
#include "Database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDateTime>

namespace rcp::storage {

namespace {
qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }

Result<long long> mediaIdFor(QSqlDatabase& db, const QString& cacheKey) {
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id FROM media WHERE canonical_url = ? LIMIT 1"));
    q.addBindValue(cacheKey);
    if (!q.exec())
        return Result<long long>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("字幕缓存查询失败"),
            q.lastError().text()));
    if (!q.next()) return Result<long long>::ok(-1LL);
    return Result<long long>::ok(q.value(0).toLongLong());
}
} // namespace

Result<QString> TranscriptRepository::saveSession(const QString& cacheKey, int audioFfIndex,
                                                  const QString& language, const QString& profile,
                                                  const QVector<rcp::CaptionSegment>& finals) {
    QSqlDatabase db = Database::threadConnection();
    auto media = mediaIdFor(db, cacheKey);
    if (media.isError()) return Result<QString>::fail(media.error());
    if (media.value() < 0)
        return Result<QString>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("字幕缓存保存失败"),
            QStringLiteral("media row not found（先写播放历史再存字幕）")));

    const QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const qint64 t = nowMs();
    {
        QSqlQuery ins(db);
        ins.prepare(QStringLiteral(
            "INSERT INTO caption_session (id, media_id, audio_ff_index, model_bundle_id, "
            "model_bundle_version, language, profile, status, covered_until_ms, created_at_ms, updated_at_ms) "
            "VALUES (?, ?, ?, 'local', '1', ?, ?, 'completed', 0, ?, ?)"));
        ins.addBindValue(sessionId);
        ins.addBindValue(media.value());
        ins.addBindValue(audioFfIndex);
        ins.addBindValue(language);
        ins.addBindValue(profile);
        ins.addBindValue(t);
        ins.addBindValue(t);
        if (!ins.exec())
            return Result<QString>::fail(AppError(ErrorDomain::Storage,
                QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("字幕缓存保存失败"),
                ins.lastError().text()));
    }
    for (const auto& seg : finals) {
        if (seg.text.trimmed().isEmpty()) continue;
        if (seg.endMs <= seg.startMs) continue;   // schema CHECK(end_ms > start_ms)
        QSqlQuery ins(db);
        ins.prepare(QStringLiteral(
            "INSERT INTO caption_segment (session_id, segment_id, revision, start_ms, end_ms, "
            "text, language, source, created_at_ms) VALUES (?, ?, 1, ?, ?, ?, ?, 'asr', ?) "
            "ON CONFLICT(session_id, segment_id) DO UPDATE SET revision = revision + 1, "
            "text = excluded.text, end_ms = excluded.end_ms"));
        ins.addBindValue(sessionId);
        ins.addBindValue(seg.id);
        ins.addBindValue(seg.startMs);
        ins.addBindValue(seg.endMs);
        ins.addBindValue(seg.text);
        ins.addBindValue(seg.language);
        ins.addBindValue(t);
        if (!ins.exec())
            return Result<QString>::fail(AppError(ErrorDomain::Storage,
                QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("字幕缓存保存失败"),
                ins.lastError().text()));
    }
    return Result<QString>::ok(sessionId);
}

Result<QVector<rcp::CaptionSegment>> TranscriptRepository::loadLatestFinals(const QString& cacheKey) {
    QSqlDatabase db = Database::threadConnection();
    auto media = mediaIdFor(db, cacheKey);
    if (media.isError()) return Result<QVector<rcp::CaptionSegment>>::fail(media.error());
    QVector<rcp::CaptionSegment> out;
    if (media.value() < 0) return Result<QVector<rcp::CaptionSegment>>::ok(out);

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT cs.id FROM caption_session cs WHERE cs.media_id = ? AND cs.status = 'completed' "
        "ORDER BY cs.updated_at_ms DESC LIMIT 1"));
    q.addBindValue(media.value());
    if (!q.exec())
        return Result<QVector<rcp::CaptionSegment>>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("字幕缓存查询失败"),
            q.lastError().text()));
    if (!q.next()) return Result<QVector<rcp::CaptionSegment>>::ok(out);
    const QString sessionId = q.value(0).toString();

    QSqlQuery seg(db);
    seg.prepare(QStringLiteral(
        "SELECT segment_id, start_ms, end_ms, text, language FROM caption_segment "
        "WHERE session_id = ? ORDER BY start_ms ASC"));
    seg.addBindValue(sessionId);
    if (!seg.exec())
        return Result<QVector<rcp::CaptionSegment>>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("字幕缓存查询失败"),
            seg.lastError().text()));
    while (seg.next()) {
        rcp::CaptionSegment s;
        s.id = seg.value(0).toString();
        s.startMs = seg.value(1).toLongLong();
        s.endMs = seg.value(2).toLongLong();
        s.text = seg.value(3).toString();
        s.language = seg.value(4).toString();
        s.kind = rcp::CaptionKind::Final;
        out.append(s);
    }
    return Result<QVector<rcp::CaptionSegment>>::ok(out);
}

} // namespace rcp::storage
