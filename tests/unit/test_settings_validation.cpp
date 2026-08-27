#include <QTest>
#include <QObject>
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include "settings/Settings.h"

using namespace rcp::settings;

class TestSettingsValidation : public QObject {
    Q_OBJECT
private slots:
    void defaultsFileParses();
    void badSpeedRejected();
    void badFontSizeRejected();
    void missingSchemaVersionRejected();
    void roundTripPreservesValues();
};

static QJsonObject loadDefaults() {
    QFile f(QStringLiteral("%1/resources/defaults/settings.json").arg(QStringLiteral(RCP_SOURCE_DIR)));
    if (!f.open(QIODevice::ReadOnly | QIODevice::ReadOnly)) return QJsonObject();
    return QJsonDocument::fromJson(f.readAll()).object();
}

void TestSettingsValidation::defaultsFileParses() {
    QJsonObject root = loadDefaults();
    QVERIFY(!root.isEmpty());
    auto r = parseSettings(root);
    QVERIFY(r.isOk());
    QCOMPARE(r.value().captions.fontSize, 38);
    QCOMPARE(r.value().playback.defaultSpeed, 1.0);
    QCOMPARE(r.value().models.bundleId, QStringLiteral("balanced-multilingual-v1"));
    QVERIFY(!r.value().privacy.allowNetwork);
}

void TestSettingsValidation::badSpeedRejected() {
    QJsonObject root = loadDefaults();
    QJsonObject playback = root.value(QStringLiteral("playback")).toObject();
    playback.insert(QStringLiteral("default_speed"), 9.0);
    root.insert(QStringLiteral("playback"), playback);
    auto r = parseSettings(root);
    QVERIFY(r.isError());
    QCOMPARE(r.error().code, QStringLiteral("SETTINGS-INVALID"));
}

void TestSettingsValidation::badFontSizeRejected() {
    QJsonObject root = loadDefaults();
    QJsonObject captions = root.value(QStringLiteral("captions")).toObject();
    captions.insert(QStringLiteral("font_size"), 999);
    root.insert(QStringLiteral("captions"), captions);
    auto r = parseSettings(root);
    QVERIFY(r.isError());
}

void TestSettingsValidation::missingSchemaVersionRejected() {
    QJsonObject root = loadDefaults();
    root.remove(QStringLiteral("schema_version"));
    auto r = parseSettings(root);
    QVERIFY(r.isError());
}

void TestSettingsValidation::roundTripPreservesValues() {
    QJsonObject root = loadDefaults();
    auto parsed = parseSettings(root);
    QVERIFY(parsed.isOk());
    QJsonObject out = settingsToJson(parsed.value());
    auto reparsed = parseSettings(out);
    QVERIFY(reparsed.isOk());
    QCOMPARE(reparsed.value().captions.language, parsed.value().captions.language);
    QCOMPARE(reparsed.value().models.verifyOnStartup, parsed.value().models.verifyOnStartup);
}

QTEST_GUILESS_MAIN(TestSettingsValidation)
#include "test_settings_validation.moc"
