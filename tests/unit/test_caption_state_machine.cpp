#include <QTest>
#include <QObject>
#include "captions/CaptionTypes.h"
#include "captions/CaptionStateMachine.h"

using namespace rcp;
using namespace rcp::captions;

class TestCaptionStateMachine : public QObject {
    Q_OBJECT
private slots:
    void initialState();
    void lifecycle();
    void generationIsolation();
};

void TestCaptionStateMachine::initialState() {
    CaptionStateMachine m;
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Idle));
    QVERIFY(!m.hasActiveCaption());
    QCOMPARE(m.generation(), quint64(0));
}

void TestCaptionStateMachine::lifecycle() {
    CaptionStateMachine m;
    m.reset(7);
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Listening));
    QCOMPARE(m.generation(), quint64(7));
    QVERIFY(m.hasActiveCaption());

    CaptionSegment p;
    p.id = QStringLiteral("s1"); p.text = QStringLiteral("你好"); p.startMs = 0; p.endMs = 500;
    p.kind = CaptionKind::Partial; p.generation = 7;
    m.onPartial(p, 7);
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Partial));
    QCOMPARE(m.currentLine().text, QStringLiteral("你好"));

    CaptionSegment fin = p;
    fin.text = QStringLiteral("你好世界"); fin.kind = CaptionKind::Final; fin.endMs = 1200;
    m.onFinal(fin, 7);
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Final));
    QCOMPARE(m.currentLine().text, QStringLiteral("你好世界"));

    // revision updates text while in Final
    CaptionSegment rev = fin;
    rev.text = QStringLiteral("你好，世界"); rev.kind = CaptionKind::Revision;
    m.onRevision(rev, 7);
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Final));
    QCOMPARE(m.currentLine().text, QStringLiteral("你好，世界"));
    QCOMPARE(int(m.currentLine().kind), int(CaptionKind::Revision));

    // revision ignored before any final (state Listening)
    CaptionStateMachine m2;
    m2.reset(1);
    CaptionSegment r2; r2.text = QStringLiteral("x"); r2.kind = CaptionKind::Revision;
    m2.onRevision(r2, 1); // state is Listening, not Final -> ignored
    QCOMPARE(int(m2.state()), int(CaptionDisplayState::Listening));

    // stop
    m.onStop(7);
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Idle));
    QVERIFY(!m.hasActiveCaption());
}

void TestCaptionStateMachine::generationIsolation() {
    CaptionStateMachine m;
    m.reset(10);
    CaptionSegment p; p.id = QStringLiteral("a"); p.text = QStringLiteral("old");
    p.kind = CaptionKind::Partial; p.generation = 10;
    m.onPartial(p, 10);
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Partial));

    // stale generation event must be discarded
    CaptionSegment stale; stale.id = QStringLiteral("b"); stale.text = QStringLiteral("stale");
    stale.kind = CaptionKind::Partial; stale.generation = 9;
    m.onPartial(stale, 9);
    QCOMPARE(m.currentLine().text, QStringLiteral("old")); // unchanged

    // new generation resets
    m.reset(11);
    QCOMPARE(int(m.state()), int(CaptionDisplayState::Listening));
    QVERIFY(m.currentLine().text.isEmpty());
}

QTEST_GUILESS_MAIN(TestCaptionStateMachine)
#include "test_caption_state_machine.moc"
