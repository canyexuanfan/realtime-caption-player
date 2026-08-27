#include <QTest>
#include <QObject>
#include <QThread>
#include <chrono>
#include "worker/BoundedQueue.h"
#include "core/Cancellation.h"

using namespace rcp;
using namespace rcp::worker;

class TestBoundedQueue : public QObject {
    Q_OBJECT
private slots:
    void pushPop();
    void fifoOrder();
    void capacityBlocks();
    void cancellationUnblocks();
    void shutdownUnblocks();
};

void TestBoundedQueue::pushPop() {
    BoundedQueue<int> q(2);
    QVERIFY(q.empty());
    QVERIFY(q.push(42));
    QCOMPARE(q.size(), 1);
    auto v = q.pop();
    QVERIFY(v.has_value());
    QCOMPARE(*v, 42);
    QVERIFY(q.empty());
}

void TestBoundedQueue::fifoOrder() {
    BoundedQueue<int> q(4);
    for (int i = 0; i < 4; ++i) QVERIFY(q.push(i));
    for (int i = 0; i < 4; ++i) {
        auto v = q.pop();
        QVERIFY(v.has_value());
        QCOMPARE(*v, i);
    }
}

void TestBoundedQueue::capacityBlocks() {
    BoundedQueue<int> q(1);
    QVERIFY(q.push(1));
    // A second push in another thread should block until we pop.
    std::thread producer([&]() {
        QVERIFY(q.push(2)); // blocks until pop below
    });
    // Give the producer time to block.
    QThread::msleep(50);
    QCOMPARE(q.size(), 1);
    auto v = q.pop();
    QVERIFY(v.has_value());
    QCOMPARE(*v, 1);
    producer.join();
    auto v2 = q.pop();
    QVERIFY(v2.has_value());
    QCOMPARE(*v2, 2);
}

void TestBoundedQueue::cancellationUnblocks() {
    BoundedQueue<int> q(1);
    CancellationSource src;
    std::thread consumer([&]() {
        auto v = q.pop(src.token()); // blocks until cancel/shutdown
        Q_UNUSED(v);
    });
    QThread::msleep(50);
    src.cancel();
    consumer.join();
    // After cancel, push should report false.
    QVERIFY(!q.push(99, src.token()));
}

void TestBoundedQueue::shutdownUnblocks() {
    BoundedQueue<int> q(1);
    std::thread consumer([&]() {
        auto v = q.pop();
        Q_UNUSED(v);
    });
    QThread::msleep(50);
    q.shutdown();
    consumer.join();
    auto v = q.pop();
    QVERIFY(!v.has_value());
}

QTEST_GUILESS_MAIN(TestBoundedQueue)
#include "test_bounded_queue.moc"
