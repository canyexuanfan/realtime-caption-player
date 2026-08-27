#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include "storage/MediaIdentity.h"

using namespace rcp::storage;

class TestMediaIdentity : public QObject {
    Q_OBJECT
private slots:
    void computeValid();
    void missingFile();
    void cacheKeyStable();
};

void TestMediaIdentity::computeValid() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/sample.mp4";
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("fake media content for fingerprint test 12345"));
        f.close();
    }
    auto r = MediaIdentityUtil::compute(path);
    QVERIFY(r.isOk());
    MediaIdentity id = r.value();
    QVERIFY(id.isValid());
    QVERIFY(!id.fingerprint.isEmpty());
    QCOMPARE(id.id, id.fingerprint);
    QCOMPARE(id.displayName, QStringLiteral("sample.mp4"));
    QVERIFY(id.size > 0);
}

void TestMediaIdentity::missingFile() {
    auto r = MediaIdentityUtil::compute(QStringLiteral("C:/no/such/file-xyz.mp4"));
    QVERIFY(r.isError());
    QCOMPARE(r.error().code, QStringLiteral("MEDIA-NOT-FOUND"));
}

void TestMediaIdentity::cacheKeyStable() {
    QTemporaryDir dir;
    const QString path = dir.path() + "/m.mkv";
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("content-a"));
        f.close();
    }
    auto k1 = MediaIdentityUtil::cacheKeyForPath(path);
    QVERIFY(k1.isOk());
    auto k2 = MediaIdentityUtil::cacheKeyForPath(path);
    QVERIFY(k2.isOk());
    QCOMPARE(k1.value(), k2.value()); // stable for same content
    auto idr = MediaIdentityUtil::compute(path);
    QCOMPARE(MediaIdentityUtil::cacheKey(idr.value()), k1.value());
}

QTEST_GUILESS_MAIN(TestMediaIdentity)
#include "test_media_identity.moc"
