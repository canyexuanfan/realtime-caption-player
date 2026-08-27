#include <QTest>
#include <QObject>
#include "settings/KeymapService.h"
#include <QJsonObject>

using namespace rcp::settings;

class TestKeymapConflicts : public QObject {
    Q_OBJECT
private slots:
    void noConflictByDefault();
    void detectsConflict();
    void ignoresDistinctBindings();
};

void TestKeymapConflicts::noConflictByDefault() {
    KeymapService km;
    km.loadDefaults();
    QVERIFY(km.conflictBindings().isEmpty());
}

void TestKeymapConflicts::detectsConflict() {
    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 1);
    QJsonObject bindings;
    bindings.insert(QStringLiteral("a.one"), QStringLiteral("F1"));
    bindings.insert(QStringLiteral("a.two"), QStringLiteral("F1")); // collision
    bindings.insert(QStringLiteral("a.three"), QStringLiteral("F2"));
    root.insert(QStringLiteral("bindings"), bindings);
    KeymapService km;
    QVERIFY(km.loadFromObject(root).isOk());
    QStringList conflicts = km.conflictBindings();
    QCOMPARE(conflicts.size(), 1);
    QCOMPARE(conflicts.first(), QStringLiteral("F1"));
}

void TestKeymapConflicts::ignoresDistinctBindings() {
    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 1);
    QJsonObject bindings;
    bindings.insert(QStringLiteral("a.one"), QStringLiteral("F1"));
    bindings.insert(QStringLiteral("a.two"), QStringLiteral("F2"));
    root.insert(QStringLiteral("bindings"), bindings);
    KeymapService km;
    QVERIFY(km.loadFromObject(root).isOk());
    QVERIFY(km.conflictBindings().isEmpty());
}

QTEST_GUILESS_MAIN(TestKeymapConflicts)
#include "test_keymap_conflicts.moc"
