#include <QTest>
#include <QObject>
#include "ipc/FrameCodec.h"

using namespace rcp::ipc;

class TestFrameCodec : public QObject {
    Q_OBJECT
private slots:
    void roundTrip();
    void splitAcrossFeeds();
    void coalescedFrames();
    void oversizeRejected();
    void emptyPayload();
};

static QByteArray makeFrame(const QByteArray& payload) {
    return FrameDecoder::encodeFrame(payload);
}

void TestFrameCodec::roundTrip() {
    FrameDecoder dec;
    QList<QByteArray> frames;
    QString err;
    QByteArray data = makeFrame("{\"a\":1}");
    QVERIFY(dec.feed(data, frames, &err));
    QVERIFY(err.isEmpty());
    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames.first(), QByteArray("{\"a\":1}"));
}

void TestFrameCodec::splitAcrossFeeds() {
    FrameDecoder dec;
    QList<QByteArray> frames;
    QString err;
    QByteArray full = makeFrame("{\"hello\":\"world\"}");
    // Feed in two chunks.
    QVERIFY(dec.feed(full.left(5), frames, &err));
    QCOMPARE(frames.size(), 0); // incomplete
    QVERIFY(dec.feed(full.mid(5), frames, &err));
    QCOMPARE(frames.size(), 1);
    QCOMPARE(frames.first(), QByteArray("{\"hello\":\"world\"}"));
}

void TestFrameCodec::coalescedFrames() {
    FrameDecoder dec;
    QList<QByteArray> frames;
    QString err;
    QByteArray data = makeFrame("A") + makeFrame("BB") + makeFrame("CCC");
    QVERIFY(dec.feed(data, frames, &err));
    QCOMPARE(frames.size(), 3);
    QCOMPARE(frames.at(0), QByteArray("A"));
    QCOMPARE(frames.at(1), QByteArray("BB"));
    QCOMPARE(frames.at(2), QByteArray("CCC"));
}

void TestFrameCodec::oversizeRejected() {
    FrameDecoder dec;
    QList<QByteArray> frames;
    QString err;
    // Claim a length of 2 MiB but send nothing -> must be rejected.
    QByteArray header;
    header.resize(4);
    quint32 len = 2 * 1024 * 1024;
    header[0] = len & 0xFF; header[1] = (len >> 8) & 0xFF;
    header[2] = (len >> 16) & 0xFF; header[3] = (len >> 24) & 0xFF;
    QVERIFY(!dec.feed(header, frames, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(err.contains(QStringLiteral("TOO-LARGE")));
}

void TestFrameCodec::emptyPayload() {
    // A zero-length payload carries no JSON, so the codec must reject it.
    FrameDecoder dec;
    QList<QByteArray> frames;
    QString err;
    QByteArray data = makeFrame(QByteArray());
    QVERIFY(!dec.feed(data, frames, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(err.contains(QStringLiteral("EMPTY")));
}

QTEST_GUILESS_MAIN(TestFrameCodec)
#include "test_frame_codec.moc"
