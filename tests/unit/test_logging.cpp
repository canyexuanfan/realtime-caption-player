#include <QTest>
#include <QObject>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTemporaryDir>
#include "core/Logging.h"

using namespace rcp::logging;

class TestLogging : public QObject {
    Q_OBJECT
private slots:
    void writesToFile();
    void levelFilter();
    void rotatesOnSize();
};

void TestLogging::writesToFile() {
    Logger& log = Logger::instance();
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    log.init(QDir(tmp.path()).filePath(QStringLiteral("logs")), /*console=*/false);
    log.setLevel(Level::Info);
    log.log(Level::Info, QStringLiteral("caption"), QStringLiteral("hello 世界"));
    log.flush();

    QString path = log.currentFilePath();
    QVERIFY(QFile::exists(path));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(f.readAll());
    f.close();
    QVERIFY(content.contains(QStringLiteral("hello 世界")));
    QVERIFY(content.contains(QStringLiteral("caption")));
}

void TestLogging::levelFilter() {
    Logger& log = Logger::instance();
    QTemporaryDir tmp;
    log.init(QDir(tmp.path()).filePath(QStringLiteral("logs")), false);
    log.setLevel(Level::Warn); // Debug/Info suppressed
    log.log(Level::Info, QStringLiteral("cap"), QStringLiteral("should-be-filtered"));
    log.log(Level::Error, QStringLiteral("cap"), QStringLiteral("should-appear"));
    log.flush();

    QFile f(log.currentFilePath());
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QString::fromUtf8(f.readAll());
    f.close();
    QVERIFY(!content.contains(QStringLiteral("should-be-filtered")));
    QVERIFY(content.contains(QStringLiteral("should-appear")));
}

void TestLogging::rotatesOnSize() {
    Logger& log = Logger::instance();
    QTemporaryDir tmp;
    log.init(QDir(tmp.path()).filePath(QStringLiteral("logs")), false);
    log.setLevel(Level::Info);
    // Write enough to exceed the 5 MiB rotation threshold.
    for (int i = 0; i < 120000; ++i)
        log.log(Level::Info, QStringLiteral("cap"), QStringLiteral("line-number-") + QString::number(i));
    log.flush();
    QDir d(QDir(tmp.path()).filePath(QStringLiteral("logs")));
    QVERIFY(QFile::exists(d.filePath(QStringLiteral("app.log.1"))));
}

QTEST_GUILESS_MAIN(TestLogging)
#include "test_logging.moc"
