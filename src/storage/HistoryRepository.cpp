// src/storage/HistoryRepository.cpp
#include "HistoryRepository.h"
#include "Database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

namespace rcp::storage {

namespace {
qint64 nowMs() { return QDateTime::currentMSecsSinceEpoch(); }
} // namespace

Result<void> HistoryRepository::upsertPlayback(const QString& cacheKey, qint64 size,
                                               qint64 mtimeMs, const QString& displayName,
                                               qint64 positionMs, qint64 durationMs,
                                               const QString& lastPath) {
    // Qt 会把 null QString 绑成 SQL NULL（撞 NOT NULL 列），统一规范化为空串。
    const QString m_lastPath = lastPath.isNull() ? QString::fromLatin1("") : lastPath;
    const QString m_display = displayName.isNull() ? QString::fromLatin1("") : displayName;
    QSqlDatabase db = Database::threadConnection();
    // 同 cacheKey = 同一媒体（指纹+大小）；mtime 仅作记录列（不参与匹配，
    // touch 不会制造重复行——SELECT 命中即 UPDATE，未命中才 INSERT）。
    {
        QSqlQuery find(db);
        find.prepare(QStringLiteral(
            "SELECT id FROM media WHERE canonical_url = ? LIMIT 1"));
        find.addBindValue(cacheKey);
        if (find.exec() && find.next()) {
            QSqlQuery upd(db);
            upd.prepare(QStringLiteral(
                "UPDATE media SET last_position_ms = ?, last_played_at_ms = ?, "
                "file_size = COALESCE(NULLIF(file_size, 0), NULLIF(?, 0)), "
                "modified_time_ms = COALESCE(modified_time_ms, ?), "
                "duration_ms = COALESCE(?, duration_ms), "
                "display_name = CASE WHEN ? = '' THEN display_name ELSE ? END, "
                "last_path = CASE WHEN ? = '' THEN last_path ELSE ? END "
                "WHERE canonical_url = ?"));
            upd.addBindValue(positionMs);
            upd.addBindValue(nowMs());
            upd.addBindValue(size);
            upd.addBindValue(mtimeMs);
            upd.addBindValue(durationMs > 0 ? QVariant(durationMs) : QVariant());
            upd.addBindValue(m_display);
            upd.addBindValue(m_display);
            upd.addBindValue(m_lastPath);
            upd.addBindValue(m_lastPath);
            upd.addBindValue(cacheKey);
            if (!upd.exec())
                return Result<void>::fail(AppError(ErrorDomain::Storage,
                    QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("历史更新失败"),
                    upd.lastError().text()));
            return Result<void>::ok();
        }
    }
    QSqlQuery ins(db);
    if (!ins.prepare(QStringLiteral(
            "INSERT INTO media (canonical_url, file_size, modified_time_ms, duration_ms, "
            "last_position_ms, last_played_at_ms, created_at_ms, updated_at_ms, display_name, last_path) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")))
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("历史写入失败"),
            QStringLiteral("prepare: %1").arg(ins.lastError().text())));
    const qint64 t = nowMs();
    ins.addBindValue(cacheKey);
    ins.addBindValue(size);
    ins.addBindValue(mtimeMs);
    ins.addBindValue(durationMs > 0 ? QVariant(durationMs) : QVariant());
    ins.addBindValue(positionMs);
    ins.addBindValue(t);
    ins.addBindValue(t);
    ins.addBindValue(t);
    ins.addBindValue(m_display);
    ins.addBindValue(m_lastPath);
    if (!ins.exec())
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("历史写入失败"),
            QStringLiteral("%1 | binds=%2 | lastQuery=%3")
                .arg(ins.lastError().text()).arg(ins.boundValues().size()).arg(ins.executedQuery())));
    return Result<void>::ok();
}

Result<long long> HistoryRepository::lastPositionMs(const QString& cacheKey) {
    QSqlDatabase db = Database::threadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT last_position_ms FROM media WHERE canonical_url = ? LIMIT 1"));
    q.addBindValue(cacheKey);
    if (!q.exec())
        return Result<long long>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("历史查询失败"),
            q.lastError().text()));
    if (!q.next()) return Result<long long>::ok(-1LL);
    return Result<long long>::ok(q.value(0).toLongLong());
}

Result<QList<HistoryEntry>> HistoryRepository::recent(int limit) {
    QSqlDatabase db = Database::threadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT canonical_url, display_name, last_position_ms, duration_ms, last_played_at_ms, "
        "last_path FROM media WHERE last_played_at_ms IS NOT NULL "
        "ORDER BY last_played_at_ms DESC LIMIT ?"));
    q.addBindValue(limit);
    QList<HistoryEntry> out;
    if (!q.exec())
        return Result<QList<HistoryEntry>>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("历史查询失败"),
            q.lastError().text()));
    while (q.next()) {
        HistoryEntry e;
        e.cacheKey = q.value(0).toString();
        e.displayName = q.value(1).toString();
        e.positionMs = q.value(2).toLongLong();
        e.durationMs = q.value(3).toLongLong();
        e.lastPlayedAtMs = q.value(4).toLongLong();
        e.path = q.value(5).toString();
        out.append(e);
    }
    return Result<QList<HistoryEntry>>::ok(out);
}

Result<void> HistoryRepository::remove(const QString& cacheKey) {
    QSqlDatabase db = Database::threadConnection();
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM media WHERE canonical_url = ?"));
    q.addBindValue(cacheKey);
    if (!q.exec())
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("历史删除失败"),
            q.lastError().text()));
    return Result<void>::ok();
}

} // namespace rcp::storage
