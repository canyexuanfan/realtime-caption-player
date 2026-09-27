#include "settings/SettingsService.h"

#include "settings/SettingsMigration.h"

#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDir>

namespace rcp::settings {

Result<Settings> SettingsService::load(const QString& path) {
    QFile f(path);
    if (!f.exists()) return Result<Settings>::ok(Settings{}); // defaults
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return Result<Settings>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-READ-FAILED"),
            QStringLiteral("无法读取设置文件"),
            QStringLiteral("QFile::open failed")));
    const QByteArray raw = f.readAll();
    f.close();

    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
    if (doc.isNull())
        return Result<Settings>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-PARSE-FAILED"),
            QStringLiteral("设置文件格式损坏"),
            QStringLiteral("JSON parse error at %1").arg(perr.offset)));
    if (!doc.isObject())
        return Result<Settings>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-PARSE-FAILED"),
            QStringLiteral("设置文件格式损坏"), QStringLiteral("root not object")));

    // D3：旧版本设置对象先迁移到当前 schema 再解析（审查 §3：load 从不调用迁移）。
    auto migrated = SettingsMigration::migrate(doc.object());
    if (migrated.isError()) return Result<Settings>::fail(migrated.error());
    return parseSettings(migrated.value());
}

Result<Settings> SettingsService::loadStrict(const QString& path) {
    QFile f(path);
    if (!f.exists())
        return Result<Settings>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-MISSING"),
            QStringLiteral("未找到设置文件"),
            QStringLiteral("file does not exist")));
    return load(path);
}

Result<void> SettingsService::save(const QString& path) {
    return save(path, Settings{});
}

Result<void> SettingsService::save(const QString& path, const Settings& s) {
    const QJsonDocument doc(settingsToJson(s));
    const QByteArray data = doc.toJson(QJsonDocument::Indented);

    QDir().mkpath(QFileInfo(path).absoluteDir().absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-WRITE-FAILED"),
            QStringLiteral("无法写入设置文件"),
            QStringLiteral("QSaveFile::open failed")));
    if (f.write(data) != data.size())
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-WRITE-FAILED"),
            QStringLiteral("无法写入设置文件"),
            QStringLiteral("short write")));
    if (!f.commit())
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-WRITE-FAILED"),
            QStringLiteral("无法写入设置文件"),
            QStringLiteral("QSaveFile::commit failed")));
    return Result<void>::ok();
}

} // namespace rcp::settings
