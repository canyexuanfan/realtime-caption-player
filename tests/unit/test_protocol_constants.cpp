#include <QTest>
#include <QObject>
#include "ipc/Protocol.h"

using namespace rcp::ipc;

class TestProtocolConstants : public QObject {
    Q_OBJECT
private slots:
    void commandWireNames();
    void eventWireNames();
    void roundTripNames();
    void envelopeValid();
    void constants();
};

void TestProtocolConstants::commandWireNames() {
    QCOMPARE(wireTypeForCommand(CommandType::OpenMedia), QStringLiteral("command.open_media"));
    QCOMPARE(wireTypeForCommand(CommandType::Seek), QStringLiteral("command.seek"));
    QCOMPARE(wireTypeForCommand(CommandType::Shutdown), QStringLiteral("command.shutdown"));
}

void TestProtocolConstants::eventWireNames() {
    QCOMPARE(wireTypeForEvent(EventType::CaptionPartial), QStringLiteral("event.caption_partial"));
    QCOMPARE(wireTypeForEvent(EventType::CaptionFinal), QStringLiteral("event.caption_final"));
    QCOMPARE(wireTypeForEvent(EventType::Heartbeat), QStringLiteral("event.heartbeat"));
}

void TestProtocolConstants::roundTripNames() {
    QCOMPARE(commandTypeFromName(wireTypeForCommand(CommandType::SetSpeed)), CommandType::SetSpeed);
    QCOMPARE(eventTypeFromName(wireTypeForEvent(EventType::Error)), EventType::Error);
    // Unknown names resolve to the first enum value (invalid sentinel).
    QCOMPARE(commandTypeFromName(QStringLiteral("command.unknown")), CommandType::Hello);
    QCOMPARE(eventTypeFromName(QStringLiteral("")), EventType::Ready);
}

void TestProtocolConstants::envelopeValid() {
    Envelope ok;
    ok.version = kProtocolVersion;
    ok.type = QStringLiteral("command.open_media");
    ok.id = QStringLiteral("id-1");
    QVERIFY(ok.isValid());

    Envelope noType = ok;
    noType.type.clear();
    QVERIFY(!noType.isValid());

    Envelope noId = ok;
    noId.id.clear();
    QVERIFY(!noId.isValid());

    Envelope badVer = ok;
    badVer.version = 99;
    QVERIFY(!badVer.isValid());
}

void TestProtocolConstants::constants() {
    QCOMPARE(kProtocolVersion, quint8(1));
    QCOMPARE(kMaxFrameBytes, quint32(1 * 1024 * 1024));
}

QTEST_GUILESS_MAIN(TestProtocolConstants)
#include "test_protocol_constants.moc"
