#pragma once

#include "core/Result.h"
#include <QJsonObject>

namespace rcp::settings {

/// Schema migration for persisted settings (tech plan 10.x).
/// Current version is 1. Unknown/future versions are rejected.
class SettingsMigration {
public:
    static int currentVersion() { return 1; }

    /// Migrate a (possibly older) settings object to the current schema.
    /// Missing schema_version is treated as version 0 and migrated forward.
    static Result<QJsonObject> migrate(const QJsonObject& in);

private:
    static QJsonObject migrateStep0To1(const QJsonObject& in);
};

} // namespace rcp::settings
