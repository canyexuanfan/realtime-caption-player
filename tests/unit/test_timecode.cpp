#include <QTest>
#include <QObject>
#include "core/Timecode.h"
#include <chrono>

using namespace std::chrono_literals;
using namespace rcp;

class TestTimecode : public QObject {
    Q_OBJECT
private slots:
    void formatSrt();
    void parseSrt();
    void clock();
    void clamp();
};

void TestTimecode::formatSrt() {
    QCOMPARE(formatSrtTime(0ms), QStringLiteral("00:00:00,000"));
    QCOMPARE(formatSrtTime(1000ms), QStringLiteral("00:00:01,000"));
    QCOMPARE(formatSrtTime(65432ms), QStringLiteral("00:01:05,432"));
    QCOMPARE(formatSrtTime(3661000ms), QStringLiteral("01:01:01,000"));
    // Hours are NOT zero-padded beyond two digits (tech plan).
    QCOMPARE(formatSrtTime(100h), QStringLiteral("100:00:00,000"));
}

void TestTimecode::parseSrt() {
    auto a = parseSrtTime(QStringLiteral("00:01:05,432"));
    QCOMPARE(a, 65432ms);
    auto b = parseSrtTime(QStringLiteral("01:01:01.000")); // dot separator accepted
    QCOMPARE(b, 3661000ms);
    QVERIFY(parseSrtTime(QStringLiteral("garbage")) < 0ms);
    QVERIFY(parseSrtTime(QStringLiteral("")) < 0ms);
}

void TestTimecode::clock() {
    QCOMPARE(formatClock(0ms), QStringLiteral("0:00"));
    QCOMPARE(formatClock(65432ms), QStringLiteral("1:05"));
    QCOMPARE(formatClock(3661000ms), QStringLiteral("1:01:01"));
}

void TestTimecode::clamp() {
    QCOMPARE(clampTime(-500ms, 1000ms), 0ms);
    QCOMPARE(clampTime(500ms, 1000ms), 500ms);
    QCOMPARE(clampTime(1500ms, 1000ms), 1000ms);
}

QTEST_GUILESS_MAIN(TestTimecode)
#include "test_timecode.moc"
