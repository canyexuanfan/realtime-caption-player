#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include "storage/Database.h"
#include "storage/MigrationRunner.h"
#include "storage/HistoryRepository.h"
#include "storage/TranscriptRepository.h"
#include "captions/CaptionTypes.h"

using namespace rcp;

// D1 持久化集成测试：迁移（v1..v3）+ 播放历史续播 + 字幕缓存往返。
class TestSqlPersistence : public QObject {
    Q_OBJECT
private slots:
    void migrationHistoryTranscriptRoundTrip();
};

void TestSqlPersistence::migrationHistoryTranscriptRoundTrip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto opened = rcp::storage::Database::open(dir.path() + QStringLiteral("/t.db"));
    QVERIFY2(!opened.isError(), "db open failed");
    auto mig = rcp::storage::MigrationRunner::run(rcp::storage::Database::threadConnection());
    QVERIFY2(!mig.isError(), qPrintable(mig.isError() ? mig.error().technicalMessage : QStringLiteral("ok")));
    QVERIFY(mig.value() >= 3);
    for (const char* r : {":/migrations/001_initial.sql", ":/migrations/002_add_display_name.sql",
                          ":/migrations/003_add_last_path.sql"}) {
        QFile rf(r);
        qWarning() << r << "size=" << (rf.open(QIODevice::ReadOnly) ? rf.size() : -1);
    }

    // 历史：插入 → 查询续播 → 更新（mtime 变化不产生重复行）→ recent。
    auto up1 = rcp::storage::HistoryRepository::upsertPlayback(
        QStringLiteral("key1"), 123, 456, QStringLiteral("a.mp4"), 1000, 60000,
        QStringLiteral("F:/x/a.mp4"));
    QVERIFY2(!up1.isError(), qPrintable(up1.isError() ? up1.error().technicalMessage + " | " + up1.error().userMessage : QStringLiteral("ok")));
    auto pos = rcp::storage::HistoryRepository::lastPositionMs(QStringLiteral("key1"));
    QVERIFY(!pos.isError());
    QCOMPARE(pos.value(), 1000LL);

    auto up2 = rcp::storage::HistoryRepository::upsertPlayback(
        QStringLiteral("key1"), 123, 999, QStringLiteral("a.mp4"), 2000, 60000);
    QVERIFY2(!up2.isError(), qPrintable(up2.isError() ? up2.error().technicalMessage : QStringLiteral("ok")));
    auto recent = rcp::storage::HistoryRepository::recent(10);
    QVERIFY(!recent.isError());
    QCOMPARE(recent.value().size(), 1);
    QCOMPARE(recent.value().first().displayName, QStringLiteral("a.mp4"));
    QCOMPARE(recent.value().first().path, QStringLiteral("F:/x/a.mp4"));
    auto pos2 = rcp::storage::HistoryRepository::lastPositionMs(QStringLiteral("key1"));
    QCOMPARE(pos2.value(), 2000LL);

    // 字幕缓存：保存两个终稿 → 载入往返。
    QVector<rcp::CaptionSegment> finals;
    rcp::CaptionSegment s1;
    s1.id = QStringLiteral("u1");
    s1.startMs = 0;
    s1.endMs = 1500;
    s1.text = QStringLiteral("你好");
    s1.kind = rcp::CaptionKind::Final;
    rcp::CaptionSegment s2 = s1;
    s2.id = QStringLiteral("u2");
    s2.startMs = 2000;
    s2.endMs = 3500;
    s2.text = QStringLiteral("世界");
    finals << s1 << s2;
    auto saved = rcp::storage::TranscriptRepository::saveSession(
        QStringLiteral("key1"), 0, QStringLiteral("auto"), QStringLiteral("balanced"), finals);
    QVERIFY2(!saved.isError(), "saveSession failed");
    auto loaded = rcp::storage::TranscriptRepository::loadLatestFinals(QStringLiteral("key1"));
    QVERIFY(!loaded.isError());
    QCOMPARE(loaded.value().size(), 2);
    QCOMPARE(loaded.value().first().text, QStringLiteral("你好"));
    QCOMPARE(loaded.value().last().text, QStringLiteral("世界"));
}

QTEST_GUILESS_MAIN(TestSqlPersistence)
#include "test_sql_persistence.moc"
