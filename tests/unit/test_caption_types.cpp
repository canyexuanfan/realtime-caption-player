#include <QTest>
#include <QObject>
#include "captions/CaptionTypes.h"

using namespace rcp;

class TestCaptionTypes : public QObject {
    Q_OBJECT
private slots:
    void kindNames();
    void validity();
    void equality();
};

void TestCaptionTypes::kindNames() {
    QCOMPARE(captionKindName(CaptionKind::Partial), QStringLiteral("partial"));
    QCOMPARE(captionKindName(CaptionKind::Final), QStringLiteral("final"));
    QCOMPARE(captionKindName(CaptionKind::Revision), QStringLiteral("revision"));
}

void TestCaptionTypes::validity() {
    CaptionSegment s;
    s.startMs = 0; s.endMs = 1000;
    QVERIFY(s.isValid());
    CaptionSegment bad;
    bad.startMs = 2000; bad.endMs = 1000; // end before start
    QVERIFY(!bad.isValid());
    CaptionSegment neg;
    neg.startMs = -10; neg.endMs = 100;
    QVERIFY(!neg.isValid());
}

void TestCaptionTypes::equality() {
    CaptionSegment a; a.startMs = 0; a.endMs = 100; a.text = QStringLiteral("x");
    CaptionSegment b = a;
    QVERIFY(a == b);
    b.text = QStringLiteral("y");
    QVERIFY(!(a == b));
}

QTEST_GUILESS_MAIN(TestCaptionTypes)
#include "test_caption_types.moc"
