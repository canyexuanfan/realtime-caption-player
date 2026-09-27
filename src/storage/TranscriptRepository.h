// src/storage/TranscriptRepository.h
// 字幕缓存仓库（D1）：caption_session / caption_segment 读写。
// 同一媒体重看时可直接预载终稿时间线（"重看秒出"）。
#pragma once

#include "captions/CaptionTypes.h"
#include "core/Result.h"
#include <QString>
#include <QVector>

namespace rcp::storage {

class TranscriptRepository {
public:
    /// 保存/更新一次识别会话（终稿集合）。返回会话 id。
    static Result<QString> saveSession(const QString& cacheKey, int audioFfIndex,
                                       const QString& language, const QString& profile,
                                       const QVector<rcp::CaptionSegment>& finals);

    /// 载入该媒体最近一次完成会话的终稿（无则返回空集）。
    static Result<QVector<rcp::CaptionSegment>> loadLatestFinals(const QString& cacheKey);
};

} // namespace rcp::storage
