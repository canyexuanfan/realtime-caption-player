#include <QTest>
#include <QObject>
#include "core/Error.h"

using namespace rcp;

class TestError : public QObject {
    Q_OBJECT
private slots:
    void domainName();
    void factoryFunctions();
    void toString();
};

void TestError::domainName() {
    AppError e(ErrorDomain::Playback, QStringLiteral("PLY-OPEN-FAILED"), QStringLiteral("无法打开"));
    QCOMPARE(e.domainName(), QStringLiteral("Playback"));
    QCOMPARE(e.code, QStringLiteral("PLY-OPEN-FAILED"));
    QCOMPARE(e.userMessage, QStringLiteral("无法打开"));
    QVERIFY(!e.retryable);
}

void TestError::factoryFunctions() {
    AppError open = errc::playbackOpenFailed(QStringLiteral("detail-123"));
    QCOMPARE(open.domain, ErrorDomain::Playback);
    QCOMPARE(open.code, QStringLiteral("PLY-OPEN-FAILED"));
    QVERIFY(!open.userMessage.isEmpty());

    AppError worker = errc::ipcWorkerDied();
    QCOMPARE(worker.domain, ErrorDomain::Ipc);
    QCOMPARE(worker.code, QStringLiteral("IPC-WORKER-DIED"));

    AppError model = errc::modelMissing();
    QCOMPARE(model.domain, ErrorDomain::Model);
    QCOMPARE(model.code, QStringLiteral("MOD-MISSING"));
}

void TestError::toString() {
    AppError e(ErrorDomain::Storage, QStringLiteral("DB-MIGRATION-FAILED"),
               QStringLiteral("数据库升级失败"), QStringLiteral("technical detail"));
    QString s = e.toString();
    QVERIFY(s.contains(QStringLiteral("DB-MIGRATION-FAILED")));
    QVERIFY(s.contains(QStringLiteral("Storage")));
}

QTEST_GUILESS_MAIN(TestError)
#include "test_error.moc"
