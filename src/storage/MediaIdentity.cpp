#include "storage/MediaIdentity.h"

#include <QFileInfo>
#include <QFile>
#include <QCryptographicHash>

namespace rcp::storage {

Result<QString> MediaIdentityUtil::fingerprint(const QString& path, qint64 size, qint64 mtimeMs) {
    // Cheap content fingerprint: hash the first 64 KiB with size + mtime.
    // Sufficient to detect edits without scanning the whole file.
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return Result<QString>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("MEDIA-READ-FAILED"),
            QStringLiteral("无法读取媒体文件以计算指纹"),
            QStringLiteral("QFile::open failed")));
    const QByteArray head = f.read(64 * 1024);
    f.close();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(reinterpret_cast<const char*>(head.constData()), head.size());
    hash.addData(QByteArray::number(size));
    hash.addData(QByteArray::number(mtimeMs));
    return Result<QString>::ok(QString::fromLatin1(hash.result().toHex()));
}

Result<MediaIdentity> MediaIdentityUtil::compute(const QString& path) {
    QFileInfo info(path);
    if (!info.exists())
        return Result<MediaIdentity>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("MEDIA-NOT-FOUND"),
            QStringLiteral("媒体文件不存在"),
            QStringLiteral("path not found")));
    const qint64 size = info.size();
    const qint64 mtimeMs = static_cast<qint64>(info.lastModified().toMSecsSinceEpoch());

    auto fp = fingerprint(path, size, mtimeMs);
    if (fp.isError()) return Result<MediaIdentity>::fail(fp.error());

    MediaIdentity id;
    id.fingerprint = fp.value();
    id.id = fp.value(); // the fingerprint itself is the stable cache key
    id.size = size;
    id.mtimeMs = mtimeMs;
    id.displayName = info.fileName(); // basename only
    return Result<MediaIdentity>::ok(id);
}

QString MediaIdentityUtil::cacheKey(const MediaIdentity& id) {
    return id.id;
}

Result<QString> MediaIdentityUtil::cacheKeyForPath(const QString& path) {
    auto r = compute(path);
    if (r.isError()) return Result<QString>::fail(r.error());
    return Result<QString>::ok(r.value().id);
}

} // namespace rcp::storage
