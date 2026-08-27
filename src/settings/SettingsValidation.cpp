#include "settings/Settings.h"

#include <QJsonValue>
#include <QSet>

namespace rcp::settings {

int currentSchemaVersion() { return 1; }

namespace {

AppError invalid(const QString& field, const QString& reason) {
    return AppError(ErrorDomain::App,
        QStringLiteral("SETTINGS-INVALID"),
        QStringLiteral("设置项无效：") + field,
        QStringLiteral("validation failed: ") + field + QStringLiteral(" (") + reason + QStringLiteral(")"));
}

bool inSet(const QString& v, std::initializer_list<const char*> opts) {
    for (const char* o : opts)
        if (v == QLatin1String(o)) return true;
    return false;
}

double getDouble(const QJsonObject& o, const QString& k, double fallback) {
    const QJsonValue v = o.value(k);
    if (!v.isDouble()) return fallback;
    return v.toDouble();
}

int getInt(const QJsonObject& o, const QString& k, int fallback) {
    const QJsonValue v = o.value(k);
    if (!v.isDouble()) return fallback;
    return static_cast<int>(v.toDouble());
}

QString getString(const QJsonObject& o, const QString& k, const QString& fallback) {
    const QJsonValue v = o.value(k);
    if (!v.isString()) return fallback;
    return v.toString();
}

bool getBool(const QJsonObject& o, const QString& k, bool fallback) {
    const QJsonValue v = o.value(k);
    if (!v.isBool()) return fallback;
    return v.toBool();
}

} // namespace

Result<Settings> parseSettings(const QJsonObject& root) {
    if (!root.contains(QStringLiteral("schema_version")) ||
        root.value(QStringLiteral("schema_version")).toInt() != currentSchemaVersion()) {
        return Result<Settings>::fail(invalid(QStringLiteral("schema_version"),
            QStringLiteral("expected %1").arg(currentSchemaVersion())));
    }

    Settings s;
    s.schemaVersion = currentSchemaVersion();

    const QJsonObject pb = root.value(QStringLiteral("playback")).toObject();
    s.playback.hardwareDecode = getString(pb, QStringLiteral("hardware_decode"), s.playback.hardwareDecode);
    if (!inSet(s.playback.hardwareDecode, { "off", "auto-safe", "auto-aggressive" }))
        return Result<Settings>::fail(invalid(QStringLiteral("playback.hardware_decode"),
            QStringLiteral("unknown preset")));
    s.playback.rememberPosition = getBool(pb, QStringLiteral("remember_position"), s.playback.rememberPosition);
    s.playback.folderContinuousPlay = getBool(pb, QStringLiteral("folder_continuous_play"), s.playback.folderContinuousPlay);
    s.playback.defaultSpeed = getDouble(pb, QStringLiteral("default_speed"), s.playback.defaultSpeed);
    if (s.playback.defaultSpeed < 0.5 || s.playback.defaultSpeed > 4.0)
        return Result<Settings>::fail(invalid(QStringLiteral("playback.default_speed"),
            QStringLiteral("outside 0.5..4.0")));
    s.playback.pitchCorrection = getBool(pb, QStringLiteral("pitch_correction"), s.playback.pitchCorrection);

    const QJsonObject cap = root.value(QStringLiteral("captions")).toObject();
    s.captions.enabledByDefault = getBool(cap, QStringLiteral("enabled_by_default"), s.captions.enabledByDefault);
    s.captions.profile = getString(cap, QStringLiteral("profile"), s.captions.profile);
    if (!inSet(s.captions.profile, { "lite", "balanced", "auto" }))
        return Result<Settings>::fail(invalid(QStringLiteral("captions.profile"),
            QStringLiteral("unknown profile")));
    s.captions.language = getString(cap, QStringLiteral("language"), s.captions.language);
    if (!inSet(s.captions.language, { "zh", "en", "auto", "ja", "ko", "yue", "auto-exp" }))
        return Result<Settings>::fail(invalid(QStringLiteral("captions.language"),
            QStringLiteral("unknown language")));
    s.captions.fontSize = getInt(cap, QStringLiteral("font_size"), s.captions.fontSize);
    if (s.captions.fontSize < 10 || s.captions.fontSize > 200)
        return Result<Settings>::fail(invalid(QStringLiteral("captions.font_size"),
            QStringLiteral("outside 10..200")));
    s.captions.bottomMarginPercent = getInt(cap, QStringLiteral("bottom_margin_percent"), s.captions.bottomMarginPercent);
    if (s.captions.bottomMarginPercent < 0 || s.captions.bottomMarginPercent > 50)
        return Result<Settings>::fail(invalid(QStringLiteral("captions.bottom_margin_percent"),
            QStringLiteral("outside 0..50")));
    s.captions.showBackground = getBool(cap, QStringLiteral("show_background"), s.captions.showBackground);
    s.captions.delayMs = getInt(cap, QStringLiteral("delay_ms"), s.captions.delayMs);
    if (s.captions.delayMs < -10000 || s.captions.delayMs > 10000)
        return Result<Settings>::fail(invalid(QStringLiteral("captions.delay_ms"),
            QStringLiteral("outside -10000..10000")));
    s.captions.autoExportSrt = getBool(cap, QStringLiteral("auto_export_srt"), s.captions.autoExportSrt);

    const QJsonObject mdl = root.value(QStringLiteral("models")).toObject();
    s.models.bundleId = getString(mdl, QStringLiteral("bundle_id"), s.models.bundleId);
    if (s.models.bundleId.isEmpty())
        return Result<Settings>::fail(invalid(QStringLiteral("models.bundle_id"),
            QStringLiteral("empty")));
    s.models.verifyOnStartup = getString(mdl, QStringLiteral("verify_on_startup"), s.models.verifyOnStartup);
    if (!inSet(s.models.verifyOnStartup, { "off", "incremental", "full" }))
        return Result<Settings>::fail(invalid(QStringLiteral("models.verify_on_startup"),
            QStringLiteral("unknown mode")));

    const QJsonObject pri = root.value(QStringLiteral("privacy")).toObject();
    s.privacy.allowNetwork = getBool(pri, QStringLiteral("allow_network"), s.privacy.allowNetwork);
    s.privacy.telemetry = getBool(pri, QStringLiteral("telemetry"), s.privacy.telemetry);
    s.privacy.logCaptionText = getBool(pri, QStringLiteral("log_caption_text"), s.privacy.logCaptionText);
    s.privacy.logFullPaths = getBool(pri, QStringLiteral("log_full_paths"), s.privacy.logFullPaths);

    const QJsonObject dia = root.value(QStringLiteral("diagnostics")).toObject();
    s.diagnostics.logLevel = getString(dia, QStringLiteral("log_level"), s.diagnostics.logLevel);
    if (!inSet(s.diagnostics.logLevel, { "trace", "debug", "info", "warn", "error" }))
        return Result<Settings>::fail(invalid(QStringLiteral("diagnostics.log_level"),
            QStringLiteral("unknown level")));
    s.diagnostics.keepLogDays = getInt(dia, QStringLiteral("keep_log_days"), s.diagnostics.keepLogDays);
    if (s.diagnostics.keepLogDays < 1 || s.diagnostics.keepLogDays > 365)
        return Result<Settings>::fail(invalid(QStringLiteral("diagnostics.keep_log_days"),
            QStringLiteral("outside 1..365")));

    return Result<Settings>::ok(s);
}

QJsonObject settingsToJson(const Settings& s) {
    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), s.schemaVersion);

    QJsonObject pb;
    pb.insert(QStringLiteral("hardware_decode"), s.playback.hardwareDecode);
    pb.insert(QStringLiteral("remember_position"), s.playback.rememberPosition);
    pb.insert(QStringLiteral("folder_continuous_play"), s.playback.folderContinuousPlay);
    pb.insert(QStringLiteral("default_speed"), s.playback.defaultSpeed);
    pb.insert(QStringLiteral("pitch_correction"), s.playback.pitchCorrection);
    root.insert(QStringLiteral("playback"), pb);

    QJsonObject cap;
    cap.insert(QStringLiteral("enabled_by_default"), s.captions.enabledByDefault);
    cap.insert(QStringLiteral("profile"), s.captions.profile);
    cap.insert(QStringLiteral("language"), s.captions.language);
    cap.insert(QStringLiteral("font_size"), s.captions.fontSize);
    cap.insert(QStringLiteral("bottom_margin_percent"), s.captions.bottomMarginPercent);
    cap.insert(QStringLiteral("show_background"), s.captions.showBackground);
    cap.insert(QStringLiteral("delay_ms"), s.captions.delayMs);
    cap.insert(QStringLiteral("auto_export_srt"), s.captions.autoExportSrt);
    root.insert(QStringLiteral("captions"), cap);

    QJsonObject mdl;
    mdl.insert(QStringLiteral("bundle_id"), s.models.bundleId);
    mdl.insert(QStringLiteral("verify_on_startup"), s.models.verifyOnStartup);
    root.insert(QStringLiteral("models"), mdl);

    QJsonObject pri;
    pri.insert(QStringLiteral("allow_network"), s.privacy.allowNetwork);
    pri.insert(QStringLiteral("telemetry"), s.privacy.telemetry);
    pri.insert(QStringLiteral("log_caption_text"), s.privacy.logCaptionText);
    pri.insert(QStringLiteral("log_full_paths"), s.privacy.logFullPaths);
    root.insert(QStringLiteral("privacy"), pri);

    QJsonObject dia;
    dia.insert(QStringLiteral("log_level"), s.diagnostics.logLevel);
    dia.insert(QStringLiteral("keep_log_days"), s.diagnostics.keepLogDays);
    root.insert(QStringLiteral("diagnostics"), dia);

    return root;
}

} // namespace rcp::settings
