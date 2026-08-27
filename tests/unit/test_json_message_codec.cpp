#include <QTest>
#include <QObject>
#include "ipc/JsonMessageCodec.h"
#include "ipc/FrameCodec.h"

using namespace rcp;

using namespace rcp::ipc;

class TestJsonMessageCodec : public QObject {
    Q_OBJECT
private slots:
    void roundTrip();
    void invalidJson();
    void missingRequiredField();
    void badVersion();
    void wireFrameDecode();
};

void TestJsonMessageCodec::roundTrip() {
    Envelope env;
    env.version = kProtocolVersion;
    env.type = wireTypeForCommand(CommandType::OpenMedia);
    env.id = QStringLiteral("abc-123");
    env.generation = 7;
    env.payload = QJsonObject{{QStringLiteral("path"), QStringLiteral("x.mp4")}};

    QByteArray frame = JsonMessageCodec::serialize(env);
    QVERIFY(!frame.isEmpty());

    // Strip the 4-byte length prefix (FrameDecoder would do this).
    QByteArray payload = frame.mid(4);
    auto r = JsonMessageCodec::parse(payload);
    QVERIFY(r.isOk());
    QCOMPARE(r.value().type, env.type);
    QCOMPARE(r.value().id, QStringLiteral("abc-123"));
    QCOMPARE(r.value().generation, quint64(7));
    QCOMPARE(r.value().payload.value(QStringLiteral("path")).toString(), QStringLiteral("x.mp4"));
}

void TestJsonMessageCodec::invalidJson() {
    QByteArray bad = QByteArray("not json at all {{{");
    auto r = JsonMessageCodec::parse(bad);
    QVERIFY(r.isError());
    QCOMPARE(r.error().domain, ErrorDomain::Ipc);
}

void TestJsonMessageCodec::missingRequiredField() {
    // Valid JSON but missing required "type" and "id".
    QByteArray partial = QByteArray("{\"v\":1,\"generation\":1}");
    auto r = JsonMessageCodec::parse(partial);
    QVERIFY(r.isError());
    QCOMPARE(r.error().code, QStringLiteral("IPC-PROTOCOL-MISMATCH"));
}

void TestJsonMessageCodec::badVersion() {
    QByteArray wrong = QByteArray("{\"v\":99,\"type\":\"command.open_media\",\"id\":\"x\"}");
    auto r = JsonMessageCodec::parse(wrong);
    QVERIFY(r.isError());
}

void TestJsonMessageCodec::wireFrameDecode() {
    Envelope env;
    env.version = kProtocolVersion;
    env.type = wireTypeForEvent(EventType::CaptionFinal);
    env.id = QStringLiteral("evt-1");
    env.generation = 2;
    env.payload = QJsonObject{{QStringLiteral("text"), QStringLiteral("你好")}};

    QByteArray wire = JsonMessageCodec::serialize(env);
    FrameDecoder dec;
    QList<QByteArray> frames;
    QString err;
    QVERIFY(dec.feed(wire, frames, &err));
    QCOMPARE(frames.size(), 1);
    auto r = JsonMessageCodec::parse(frames.first());
    QVERIFY(r.isOk());
    QCOMPARE(r.value().type, wireTypeForEvent(EventType::CaptionFinal));
    QCOMPARE(r.value().payload.value(QStringLiteral("text")).toString(), QStringLiteral("你好"));
}

QTEST_GUILESS_MAIN(TestJsonMessageCodec)
#include "test_json_message_codec.moc"
