// src/storage/HistoryRepository.h
// 播放历史/续播位置仓库（D1）：media 表读写。
// canonical_url 存媒体缓存键（MediaIdentity：头部哈希+大小，不含路径/不含
// mtime——touch 不再使缓存键漂移），不落任何明文本机路径（隐私规则）。
#pragma once

#include "core/Result.h"
#include <QString>
#include <QList>
#include <QVector>

namespace rcp::storage {

struct HistoryEntry {
    QString cacheKey;        // 媒体缓存键（非路径）
    QString displayName;     // 仅文件名（脱敏）
    QString path;            // 本机路径（仅本机数据库内，用于历史重开；不进日志）
    qint64 positionMs = 0;
    qint64 durationMs = 0;
    qint64 lastPlayedAtMs = 0;
};

class HistoryRepository {
public:
    /// 插入/更新播放记录（存在同 cacheKey 行则原地更新，含续播位置）。
    static Result<void> upsertPlayback(const QString& cacheKey, qint64 size,
                                       qint64 mtimeMs, const QString& displayName,
                                       qint64 positionMs, qint64 durationMs,
                                       const QString& lastPath = QString());

    /// 查询续播位置；无记录返回 -1。
    static Result<long long> lastPositionMs(const QString& cacheKey);

    /// 最近播放（按 last_played_at 倒序，默认 200 条上限）。
    static Result<QList<HistoryEntry>> recent(int limit = 200);

    /// 删除单条历史。
    static Result<void> remove(const QString& cacheKey);
};

} // namespace rcp::storage
