#include "core/Logging.h"

#include <cstdio>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <QMessageLogContext>

namespace rcp::logging {

QString levelName(Level l) {
    switch (l) {
    case Level::Trace: return QStringLiteral("TRACE");
    case Level::Debug: return QStringLiteral("DEBUG");
    case Level::Info:  return QStringLiteral("INFO");
    case Level::Warn:  return QStringLiteral("WARN");
    case Level::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("INFO");
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::~Logger() {
    if (file_) {
        file_->flush();
        delete file_;
        file_ = nullptr;
    }
}

void Logger::init(const QString& logDir, bool console) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_ = console;
    logDir_ = logDir;
    if (logDir_.isEmpty()) return;

    QDir d(logDir_);
    d.mkpath(QStringLiteral("."));
    currentPath_ = d.filePath(QStringLiteral("app.log"));

    if (file_) { file_->flush(); delete file_; file_ = nullptr; }
    file_ = new QFile(currentPath_);
    // Append so we keep history across restarts; rotation trims old data.
    file_->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    bytesWritten_ = file_->size();
}

void Logger::rotateIfNeeded() {
    if (!file_ || bytesWritten_ < kMaxFileBytes) return;
    file_->flush();
    file_->close();
    delete file_;
    file_ = nullptr;

    // Shift previous generations: app.log.2 -> app.log.3, app.log.1 -> app.log.2
    QDir d(logDir_);
    for (int i = kMaxFiles - 1; i >= 1; --i) {
        const QString src = d.filePath(QStringLiteral("app.log.%1").arg(i));
        const QString dst = d.filePath(QStringLiteral("app.log.%1").arg(i + 1));
        if (QFile::exists(src)) QFile::remove(dst), QFile::rename(src, dst);
    }
    const QString prev = d.filePath(QStringLiteral("app.log.1"));
    if (QFile::exists(currentPath_)) QFile::rename(currentPath_, prev);

    file_ = new QFile(currentPath_);
    file_->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    bytesWritten_ = 0;
}

void Logger::log(Level l, const QString& category, const QString& message) {
    if (l < level_) return;
    const QString line = QStringLiteral("%1 [%2] [%3] %4")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")))
        .arg(levelName(l))
        .arg(category)
        .arg(message);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_) {
            QTextStream ts(file_);
            ts << line << Qt::endl;
            bytesWritten_ += line.size() + 1;
            rotateIfNeeded();
        }
    }
    if (console_) {
        fprintf(stderr, "%s\n", line.toUtf8().constData());
        fflush(stderr);
    }
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_) file_->flush();
}

QString Logger::currentFilePath() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentPath_;
}

void Logger::installQtHandler() {
    if (qtHandlerInstalled_) return;
    qtHandlerInstalled_ = true;
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
        Level l = Level::Info;
        switch (type) {
        case QtDebugMsg: l = Level::Debug; break;
        case QtInfoMsg:  l = Level::Info;  break;
        case QtWarningMsg:l = Level::Warn;  break;
        case QtCriticalMsg:
        case QtFatalMsg:  l = Level::Error; break;
        }
        const char* cat = ctx.category ? ctx.category : "qt";
        Logger::instance().log(l, QString::fromUtf8(cat), msg);
    });
}

void initLogging(const QString& logDir, bool console) { Logger::instance().init(logDir, console); }
void setLogLevel(Level l) { Logger::instance().setLevel(l); }
void log(Level l, const QString& category, const QString& message) {
    Logger::instance().log(l, category, message);
}

} // namespace rcp::logging
