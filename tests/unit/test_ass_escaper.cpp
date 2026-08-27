#include <QTest>
#include <QObject>
#include "captions/AssEscaper.h"

using namespace rcp::captions;

class TestAssEscaper : public QObject {
    Q_OBJECT
private slots:
    void escapeSpecials();
    void newlines();
    void stripNewlines();
    void plainPassthrough();
};

void TestAssEscaper::escapeSpecials() {
    QCOMPARE(AssEscaper::escape(QStringLiteral("a{b}c")), QStringLiteral("a\\{b\\}c"));
    QCOMPARE(AssEscaper::escape(QStringLiteral("x\\y")), QStringLiteral("x\\\\y"));
}

void TestAssEscaper::newlines() {
    QCOMPARE(AssEscaper::escape(QStringLiteral("line1\nline2")), QStringLiteral("line1\\Nline2"));
    // carriage return dropped
    QCOMPARE(AssEscaper::escape(QStringLiteral("a\rb")), QStringLiteral("ab"));
}

void TestAssEscaper::stripNewlines() {
    QCOMPARE(AssEscaper::toAssText(QStringLiteral("a\nb"), true), QStringLiteral("a b"));
    QCOMPARE(AssEscaper::toAssText(QStringLiteral("a\nb"), false), QStringLiteral("a\\Nb"));
}

void TestAssEscaper::plainPassthrough() {
    QCOMPARE(AssEscaper::escape(QStringLiteral("你好 world 123")), QStringLiteral("你好 world 123"));
}

QTEST_GUILESS_MAIN(TestAssEscaper)
#include "test_ass_escaper.moc"
