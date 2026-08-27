#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include "settings/SettingsService.h"
#include "settings/Settings.h"

using namespace rcp::settings;

class TestSettingsService : public QObject {
    Q_OBJECT
private slots:
    void loadMissingReturnsDefaults();
    void saveThenLoadRoundTrip();
    void loadStrictMissingFails();
    void atomicWriteNoPartialFile();
};

void TestSettingsService::loadMissingReturnsDefaults() {
    QTemporaryDir tmp;
    QString p = QDir(tmp.path()).filePath(QStringLiteral("settings.json"));
    auto r = SettingsService::load(p);
    QVERIFY(r.isOk());
    // Defaults match the struct defaults.
    QCOMPARE(r.value().captions.fontSize, 38);
}

void TestSettingsService::saveThenLoadRoundTrip() {
    QTemporaryDir tmp;
    QString p = QDir(tmp.path()).filePath(QStringLiteral("settings.json"));
    Settings s;
    s.captions.fontSize = 50;
    s.captions.language = QStringLiteral("en");
    s.playback.defaultSpeed = 1.5;
    auto save = SettingsService::save(p, s);
    QVERIFY(save.isOk());
    QVERIFY(QFile::exists(p));

    auto loaded = SettingsService::load(p);
    QVERIFY(loaded.isOk());
    QCOMPARE(loaded.value().captions.fontSize, 50);
    QCOMPARE(loaded.value().captions.language, QStringLiteral("en"));
    QCOMPARE(loaded.value().playback.defaultSpeed, 1.5);
}

void TestSettingsService::loadStrictMissingFails() {
    QTemporaryDir tmp;
    QString p = QDir(tmp.path()).filePath(QStringLiteral("nope.json"));
    auto r = SettingsService::loadStrict(p);
    QVERIFY(r.isError());
    QCOMPARE(r.error().code, QStringLiteral("SETTINGS-MISSING"));
}

void TestSettingsService::atomicWriteNoPartialFile() {
    QTemporaryDir tmp;
    QString p = QDir(tmp.path()).filePath(QStringLiteral("settings.json"));
    Settings s;
    s.captions.fontSize = 42;
    auto save = SettingsService::save(p, s);
    QVERIFY(save.isOk());
    QFile f(p);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    QVERIFY(doc.isObject());
    QCOMPARE(doc.object().value(QStringLiteral("captions")).toObject()
                 .value(QStringLiteral("font_size")).toInt(), 42);
}

QTEST_GUILESS_MAIN(TestSettingsService)
#include "test_settings_service.moc"
