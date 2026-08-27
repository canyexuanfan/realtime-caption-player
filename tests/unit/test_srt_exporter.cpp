#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include "captions/SrtExporter.h"
#include "captions/CaptionTypes.h"

using namespace rcp;
using namespace rcp::captions;

static CaptionSegment mk(qint64 s, qint64 e, const QString& t, CaptionKind k = CaptionKind::Final) {
    CaptionSegment seg;
    seg.startMs = s; seg.endMs = e; seg.text = t; seg.kind = k;
    return seg;
}

class TestSrtExporter : public QObject {
    Q_OBJECT
private slots:
    void exportSrtBasic();
    void skipsPartialsAndBlanks();
    void exportRange();
    void writeSrtAtomic();
};

void TestSrtExporter::exportSrtBasic() {
    QList<CaptionSegment> segs;
    segs.append(mk(1000, 2000, QStringLiteral("second")));
    segs.append(mk(0, 1000, QStringLiteral("first"))); // out of order
    const QString s = SrtExporter::exportSrt(segs);
    // first must be first (sorted), index 1
    QVERIFY(s.startsWith(QStringLiteral("1\n00:00:00,000 --> 00:00:01,000")));
    QVERIFY(s.contains(QStringLiteral("first")));
    QVERIFY(s.contains(QStringLiteral("second")));
    QVERIFY(s.indexOf(QStringLiteral("first")) < s.indexOf(QStringLiteral("second")));
}

void TestSrtExporter::skipsPartialsAndBlanks() {
    QList<CaptionSegment> segs;
    segs.append(mk(0, 1000, QStringLiteral("a")));
    segs.append(mk(1000, 2000, QStringLiteral(""), CaptionKind::Final)); // blank -> skipped
    segs.append(mk(2000, 3000, QStringLiteral("b"), CaptionKind::Partial)); // partial -> skipped
    const QString s = SrtExporter::exportSrt(segs);
    QVERIFY(s.contains(QStringLiteral("a")));
    QVERIFY(!s.contains(QStringLiteral("b")));
    QCOMPARE(s.count(QStringLiteral(" --> ")), 1);
}

void TestSrtExporter::exportRange() {
    QList<CaptionSegment> segs;
    segs.append(mk(0, 1000, QStringLiteral("a")));
    segs.append(mk(5000, 6000, QStringLiteral("b")));
    segs.append(mk(10000, 11000, QStringLiteral("c")));
    const QString s = SrtExporter::exportRange(segs, 4000, 9000);
    QVERIFY(s.contains(QStringLiteral("b")));
    QVERIFY(!s.contains(QStringLiteral("a")));
    QVERIFY(!s.contains(QStringLiteral("c")));
}

void TestSrtExporter::writeSrtAtomic() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/out.srt";
    QList<CaptionSegment> segs;
    segs.append(mk(0, 1000, QStringLiteral("hello")));
    auto r = SrtExporter::writeSrt(path, segs);
    QVERIFY(r.isOk());
    QVERIFY(QFile::exists(path));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray data = f.readAll();
    f.close();
    // UTF-8 BOM present
    QVERIFY(data.startsWith(QByteArray::fromHex("efbbbf")));
    QVERIFY(QString::fromUtf8(data.mid(3)).contains(QStringLiteral("hello")));
}

QTEST_GUILESS_MAIN(TestSrtExporter)
#include "test_srt_exporter.moc"
