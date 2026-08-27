#pragma once

#include "core/Result.h"
#include <QString>

namespace rcp::storage {

/// Stable identity for a media file, used as the transcript cache key.
/// Privacy-safe: derived from metadata + a cheap content fingerprint of the
/// file head, never from the full path (tech plan 9.x / rule 47).
struct MediaIdentity {
    QString id;            ///< stable cache key
    QString fingerprint;   ///< content fingerprint (head hash + size + mtime)
    qint64 size = 0;
    qint64 mtimeMs = 0;
    QString displayName;   ///< basename only (redacted path)

    bool isValid() const { return !id.isEmpty(); }
};

class MediaIdentityUtil {
public:
    /// Compute identity for a media file on disk.
    static Result<MediaIdentity> compute(const QString& path);

    /// Cache key derived purely from an identity (no PII).
    static QString cacheKey(const MediaIdentity& id);

    /// Convenience: compute identity and return its cache key.
    static Result<QString> cacheKeyForPath(const QString& path);

private:
    static Result<QString> fingerprint(const QString& path, qint64 size, qint64 mtimeMs);
};

} // namespace rcp::storage
