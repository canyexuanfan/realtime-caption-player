#include "settings/SettingsMigration.h"
#include "settings/Settings.h"

namespace rcp::settings {

Result<QJsonObject> SettingsMigration::migrate(const QJsonObject& in) {
    int v = in.value(QStringLiteral("schema_version")).toInt(0);
    if (v == currentVersion()) return Result<QJsonObject>::ok(in);
    if (v > currentVersion()) {
        return Result<QJsonObject>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("SETTINGS-SCHEMA-UNSUPPORTED"),
            QStringLiteral("设置文件来自更新的程序版本，无法向下兼容"),
            QStringLiteral("found version %1, supported %2").arg(v).arg(currentVersion())));
    }

    QJsonObject out = in;
    // Forward migration steps. Only v0->v1 exists today.
    while (v < currentVersion()) {
        out = migrateStep0To1(out);
        v = out.value(QStringLiteral("schema_version")).toInt(0);
    }
    return Result<QJsonObject>::ok(out);
}

QJsonObject SettingsMigration::migrateStep0To1(const QJsonObject& in) {
    // v0 had no schema_version and may miss newer top-level containers.
    // Merge defaults for any missing keys so the result validates as v1.
    QJsonObject defaults = settingsToJson(Settings{});
    QJsonObject out = in;
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        if (it.key() == QStringLiteral("schema_version")) continue;
        if (!out.contains(it.key())) out.insert(it.key(), it.value());
    }
    out.insert(QStringLiteral("schema_version"), currentVersion());
    return out;
}

} // namespace rcp::settings
