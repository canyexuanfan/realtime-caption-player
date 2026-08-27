#include <QTest>
#include <QObject>
#include <QFile>
#include <QJsonDocument>
#include "settings/Settings.h"
#include "settings/SettingsService.h"

using namespace rcp::settings;

class TestDefaultSettings : public QObject {
    Q_OBJECT
private slots:
    void roundTripDefault();
    void bundledDefaultsParse();
};

void TestDefaultSettings::roundTripDefault() {
    Settings s;
    QJsonObject j = settingsToJson(s);
    auto r = parseSettings(j);
    QVERIFY(r.isOk());
    QCOMPARE(r.value(), s);
}

void TestDefaultSettings::bundledDefaultsParse() {
    const QString path = QString::fromUtf8(RCP_SOURCE_DIR) + QStringLiteral("/resources/defaults/settings.json");
    QFile f(path);
    QVERIFY2(f.exists(), qPrintable("default settings file missing: " + path));
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QByteArray raw = f.readAll();
    f.close();
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);
    QVERIFY(perr.error == QJsonParseError::NoError);
    auto r = parseSettings(doc.object());
    QVERIFY2(r.isOk(), qPrintable(r.isError() ? r.error().technicalMessage : QStringLiteral("ok")));
    QCOMPARE(r.value().captions.fontSize, 38);
    QCOMPARE(r.value().models.bundleId, QStringLiteral("balanced-multilingual-v1"));
    QCOMPARE(r.value().privacy.allowNetwork, false);
}

QTEST_GUILESS_MAIN(TestDefaultSettings)
#include "test_default_settings.moc"
