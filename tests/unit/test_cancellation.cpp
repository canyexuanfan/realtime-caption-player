#include <QTest>
#include <QObject>
#include "core/Cancellation.h"

using namespace rcp;

class TestCancellation : public QObject {
    Q_OBJECT
private slots:
    void sourceCancelsToken();
    void independentTokens();
    void defaultNotCanceled();
};

void TestCancellation::sourceCancelsToken() {
    CancellationSource src;
    CancellationToken tok = src.token();
    QVERIFY(!tok.isCanceled());
    src.cancel();
    QVERIFY(tok.isCanceled());
    QVERIFY(src.isCanceled());
}

void TestCancellation::independentTokens() {
    CancellationSource a, b;
    CancellationToken ta = a.token();
    CancellationToken tb = b.token();
    a.cancel();
    QVERIFY(ta.isCanceled());
    QVERIFY(!tb.isCanceled());
}

void TestCancellation::defaultNotCanceled() {
    CancellationToken tok;
    QVERIFY(!tok.isCanceled());
}

QTEST_GUILESS_MAIN(TestCancellation)
#include "test_cancellation.moc"
