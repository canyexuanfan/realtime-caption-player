#include <QTest>
#include <QObject>
#include "worker/SegmentAssembler.h"
#include "captions/CaptionTypes.h"

using namespace rcp;
using namespace rcp::worker;

class TestSegmentAssembler : public QObject {
    Q_OBJECT
private slots:
    void partialThenFinal();
    void revisionUpdatesCommitted();
    void timeoutForceCommits();
    void generationIsolation();
};

static CaptionSegment seg(CaptionKind k, const QString& text, quint64 gen,
                          long long s, long long e, const QString& id = QStringLiteral("s1")) {
    CaptionSegment c;
    c.kind = k; c.text = text; c.generation = gen; c.startMs = s; c.endMs = e; c.id = id;
    return c;
}

void TestSegmentAssembler::partialThenFinal() {
    SegmentAssembler a;
    a.reset(1);
    a.addPartial(seg(CaptionKind::Partial, QStringLiteral("你好"), 1, 0, 1000));
    QVERIFY(a.hasPending());
    AssembleResult r = a.addFinal(seg(CaptionKind::Final, QStringLiteral("你好世界"), 1, 0, 1200));
    QCOMPARE(int(r.action), int(AssembleResult::Action::Committed));
    QCOMPARE(r.segment.text, QStringLiteral("你好世界"));
    QCOMPARE(r.segment.kind, CaptionKind::Final);
    QVERIFY(!a.hasPending());
}

void TestSegmentAssembler::revisionUpdatesCommitted() {
    SegmentAssembler a;
    a.reset(1);
    a.addFinal(seg(CaptionKind::Final, QStringLiteral("原始"), 1, 0, 1000, QStringLiteral("s9")));
    AssembleResult r = a.addRevision(seg(CaptionKind::Final, QStringLiteral("修正后"), 1, 0, 1000, QStringLiteral("s9")));
    QCOMPARE(int(r.action), int(AssembleResult::Action::Revised));
    QCOMPARE(r.segment.text, QStringLiteral("修正后"));
    QCOMPARE(r.segment.kind, CaptionKind::Revision);
}

void TestSegmentAssembler::timeoutForceCommits() {
    SegmentAssembler a;
    a.reset(1);
    a.addPartial(seg(CaptionKind::Partial, QStringLiteral("进行中"), 1, 0, 1000));
    // No final arrives; tick past timeout should force-commit.
    AssembleResult r = a.tick(5000, /*timeoutMs=*/2500);
    QCOMPARE(int(r.action), int(AssembleResult::Action::Committed));
    QCOMPARE(r.segment.text, QStringLiteral("进行中"));
    QVERIFY(!a.hasPending());
}

void TestSegmentAssembler::generationIsolation() {
    SegmentAssembler a;
    a.reset(1);
    a.addPartial(seg(CaptionKind::Partial, QStringLiteral("旧"), 1, 0, 1000));
    // A final from a different (stale) generation must be ignored.
    AssembleResult r = a.addFinal(seg(CaptionKind::Final, QStringLiteral("旧终稿"), 2, 0, 1000));
    QCOMPARE(int(r.action), int(AssembleResult::Action::None));
    QVERIFY(a.hasPending());
}

QTEST_GUILESS_MAIN(TestSegmentAssembler)
#include "test_segment_assembler.moc"
