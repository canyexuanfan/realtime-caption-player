#include <QTest>
#include <QObject>
#include <QDir>
#include <QTemporaryDir>
#include "core/AppPaths.h"

using namespace rcp;

class TestAppPaths : public QObject {
    Q_OBJECT
private slots:
    void standardDirsNonEmpty();
    void ensureDirCreates();
    void ensureDirEmptyPathFails();
    void ensureAll();
};

void TestAppPaths::standardDirsNonEmpty() {
    QVERIFY(!AppPaths::dataDir().isEmpty());
    QVERIFY(!AppPaths::cacheDir().isEmpty());
    QVERIFY(!AppPaths::logDir().isEmpty());
    QVERIFY(!AppPaths::modelsDir().isEmpty());
    QVERIFY(!AppPaths::transcriptCacheDir().isEmpty());
}

void TestAppPaths::ensureDirCreates() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QString sub = QDir(tmp.path()).filePath(QStringLiteral("a/b/c"));
    auto r = AppPaths::ensureDir(sub);
    QVERIFY(r.isOk());
    QVERIFY(QDir(sub).exists());
}

void TestAppPaths::ensureDirEmptyPathFails() {
    auto r = AppPaths::ensureDir(QString());
    QVERIFY(r.isError());
}

void TestAppPaths::ensureAll() {
    // ensureAll targets real AppData/Cache locations; should succeed on Windows.
    auto r = AppPaths::ensureAll();
    QVERIFY(r.isOk());
    QVERIFY(QDir(AppPaths::logDir()).exists());
}

QTEST_GUILESS_MAIN(TestAppPaths)
#include "test_app_paths.moc"
