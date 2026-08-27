#include <QTest>
#include <QObject>
#include "settings/SettingsMigration.h"
#include "settings/Settings.h"

using namespace rcp::settings;

class TestSettingsMigration : public QObject {
    Q_OBJECT
private slots:
    void currentVersionOne();
    void currentPassthrough();
    void v0GainsSchemaVersion();
    void v0FillsMissingContainers();
    void futureVersionRejected();
};

void TestSettingsMigration::currentVersionOne() {
    QCOMPARE(SettingsMigration::currentVersion(), 1);
}

void TestSettingsMigration::currentPassthrough() {
    QJsonObject root = settingsToJson(Settings{});
    auto r = SettingsMigration::migrate(root);
    QVERIFY(r.isOk());
    QCOMPARE(r.value(), root);
}

void TestSettingsMigration::v0GainsSchemaVersion() {
    QJsonObject v0; // schema_version absent (treated as 0)
    v0.insert(QStringLiteral("captions"), QJsonObject{{QStringLiteral("font_size"), 20}});
    auto r = SettingsMigration::migrate(v0);
    QVERIFY(r.isOk());
    QCOMPARE(r.value().value(QStringLiteral("schema_version")).toInt(), 1);
}

void TestSettingsMigration::v0FillsMissingContainers() {
    QJsonObject v0;
    v0.insert(QStringLiteral("schema_version"), 0);
    v0.insert(QStringLiteral("captions"), QJsonObject{{QStringLiteral("font_size"), 20}});
    auto r = SettingsMigration::migrate(v0);
    QVERIFY(r.isOk());
    QJsonObject out = r.value();
    // Missing top-level containers are filled with defaults.
    QVERIFY(out.contains(QStringLiteral("playback")));
    QVERIFY(out.contains(QStringLiteral("models")));
    QVERIFY(out.contains(QStringLiteral("privacy")));
    QVERIFY(out.contains(QStringLiteral("diagnostics")));
    QCOMPARE(out.value(QStringLiteral("captions")).toObject()
                 .value(QStringLiteral("font_size")).toInt(), 20);
}

void TestSettingsMigration::futureVersionRejected() {
    QJsonObject future;
    future.insert(QStringLiteral("schema_version"), 5);
    auto r = SettingsMigration::migrate(future);
    QVERIFY(r.isError());
    QCOMPARE(r.error().code, QStringLiteral("SETTINGS-SCHEMA-UNSUPPORTED"));
}

QTEST_GUILESS_MAIN(TestSettingsMigration)
#include "test_settings_migration.moc"
