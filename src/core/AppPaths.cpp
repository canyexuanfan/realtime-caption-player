#include "core/AppPaths.h"

#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

namespace rcp {

QString AppPaths::dataDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString AppPaths::configDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString AppPaths::cacheDir() {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString AppPaths::logDir() {
    return subDir(cacheDir(), QStringLiteral("logs"));
}

QString AppPaths::modelsDir() {
    return subDir(dataDir(), QStringLiteral("models"));
}

QString AppPaths::transcriptCacheDir() {
    return subDir(cacheDir(), QStringLiteral("transcripts"));
}

QString AppPaths::tempDir() {
    return subDir(cacheDir(), QStringLiteral("tmp"));
}

QString AppPaths::subDir(const QString& base, const QString& name) {
    if (base.isEmpty()) return QString();
    QDir d(base);
    return d.filePath(name);
}

Result<void> AppPaths::ensureDir(const QString& dir) {
    if (dir.isEmpty()) {
        return Result<void>::fail(AppError(ErrorDomain::App,
            QStringLiteral("APP-PATH-EMPTY"),
            QStringLiteral("无法解析应用目录路径"),
            QStringLiteral("empty directory path")));
    }
    QDir d;
    if (!d.mkpath(dir)) {
        // Redact the concrete path; surface only that creation failed.
        return Result<void>::fail(AppError(ErrorDomain::App,
            QStringLiteral("APP-PATH-CREATE-FAILED"),
            QStringLiteral("无法创建应用目录"),
            QStringLiteral("QDir::mkpath returned false")));
    }
    return Result<void>::ok();
}

Result<void> AppPaths::ensureAll() {
    for (const QString& dir : { dataDir(), configDir(), cacheDir(),
                                logDir(), modelsDir(),
                                transcriptCacheDir(), tempDir() }) {
        auto r = ensureDir(dir);
        if (r.isError()) return r;
    }
    return Result<void>::ok();
}

} // namespace rcp
