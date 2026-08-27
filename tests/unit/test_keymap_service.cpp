#include <QTest>
#include <QObject>
#include "settings/KeymapService.h"
#include <QJsonObject>

using namespace rcp::settings;

class TestKeymapService : public QObject {
    Q_OBJECT
private slots:
    void defaultsLoad();
    void resolveKnownAction();
    void resolveUnknownActionEmpty();
    void loadFromObject();
    void loadFromFile();
};

void TestKeymapService::defaultsLoad() {
    KeymapService km;
    km.loadDefaults();
    QVERIFY(km.hasAction(QStringLiteral("playback.toggle_pause")));
    QCOMPARE(km.keyForAction(QStringLiteral("playback.toggle_pause")), QStringLiteral("Space"));
    QVERIFY(!km.sequenceForAction(QStringLiteral("playback.toggle_pause")).isEmpty());
}

void TestKeymapService::resolveKnownAction() {
    KeymapService km;
    km.loadDefaults();
    QKeySequence seq = km.sequenceForAction(QStringLiteral("caption.export_srt"));
    QCOMPARE(seq.toString(), QStringLiteral("Ctrl+Shift+S"));
}

void TestKeymapService::resolveUnknownActionEmpty() {
    KeymapService km;
    km.loadDefaults();
    QVERIFY(km.keyForAction(QStringLiteral("does.not.exist")).isEmpty());
    QVERIFY(km.sequenceForAction(QStringLiteral("does.not.exist")).isEmpty());
}

void TestKeymapService::loadFromObject() {
    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 1);
    QJsonObject bindings;
    bindings.insert(QStringLiteral("playback.toggle_pause"), QStringLiteral("K"));
    root.insert(QStringLiteral("bindings"), bindings);
    KeymapService km;
    auto r = km.loadFromObject(root);
    QVERIFY(r.isOk());
    QCOMPARE(km.keyForAction(QStringLiteral("playback.toggle_pause")), QStringLiteral("K"));
    QCOMPARE(km.actions().size(), 1);
}

void TestKeymapService::loadFromFile() {
    QTemporaryDir tmp;
    QDir d(tmp.path());
    QFile f(d.filePath(QStringLiteral("keymap.json")));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(R"({"schema_version":1,"bindings":{"window.fullscreen":"F11"}})");
    f.close();
    KeymapService km;
    auto r = km.load(d.filePath(QStringLiteral("keymap.json")));
    QVERIFY(r.isOk());
    QCOMPARE(km.keyForAction(QStringLiteral("window.fullscreen")), QStringLiteral("F11"));
}

QTEST_GUILESS_MAIN(TestKeymapService)
#include "test_keymap_service.moc"
