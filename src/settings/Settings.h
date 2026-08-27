#pragma once

#include "core/Result.h"
#include <QString>
#include <QJsonObject>

namespace rcp::settings {

struct PlaybackSettings {
    QString hardwareDecode = QStringLiteral("auto-safe");
    bool rememberPosition = true;
    bool folderContinuousPlay = true;
    double defaultSpeed = 1.0;
    bool pitchCorrection = true;
    bool operator==(const PlaybackSettings&) const = default;
};

struct CaptionSettings {
    bool enabledByDefault = false;
    QString profile = QStringLiteral("auto");
    QString language = QStringLiteral("zh");
    int fontSize = 38;
    int bottomMarginPercent = 8;
    bool showBackground = true;
    int delayMs = 0;
    bool autoExportSrt = false;
    bool operator==(const CaptionSettings&) const = default;
};

struct ModelSettings {
    QString bundleId = QStringLiteral("balanced-multilingual-v1");
    QString verifyOnStartup = QStringLiteral("incremental");
    bool operator==(const ModelSettings&) const = default;
};

struct PrivacySettings {
    bool allowNetwork = false;
    bool telemetry = false;
    bool logCaptionText = false;
    bool logFullPaths = false;
    bool operator==(const PrivacySettings&) const = default;
};

struct DiagnosticsSettings {
    QString logLevel = QStringLiteral("info");
    int keepLogDays = 14;
    bool operator==(const DiagnosticsSettings&) const = default;
};

struct Settings {
    int schemaVersion = 1;
    PlaybackSettings playback;
    CaptionSettings captions;
    ModelSettings models;
    PrivacySettings privacy;
    DiagnosticsSettings diagnostics;
    bool operator==(const Settings&) const = default;
};

/// Parse + validate a settings JSON object. Returns a redacted error on any
/// schema violation (no caption text or paths are ever echoed back).
Result<Settings> parseSettings(const QJsonObject& root);

/// Serialize back to a JSON object (lossless round-trip with parseSettings).
QJsonObject settingsToJson(const Settings& s);

/// Current on-disk schema version this build understands.
int currentSchemaVersion();

} // namespace rcp::settings
