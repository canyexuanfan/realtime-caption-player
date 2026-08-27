#pragma once

#include <QString>
#include <QFile>
#include <mutex>

namespace rcp::logging {

enum class Level { Trace, Debug, Info, Warn, Error };

QString levelName(Level l);

/// Minimal structured file logger with size-based rotation.
/// No external dependency: writes plain text lines to <logDir>/app.log*
class Logger {
public:
    static Logger& instance();

    /// Open the log file under `logDir`. `console` mirrors lines to stderr.
    void init(const QString& logDir, bool console = true);
    /// Route qDebug/qWarning/... through this logger as well.
    void installQtHandler();
    void setLevel(Level l) { level_ = l; }
    Level level() const { return level_; }

    void log(Level l, const QString& category, const QString& message);
    void flush();

    /// Path of the currently open log file (for tests / diagnostics).
    QString currentFilePath() const;

    static const qint64 kMaxFileBytes = 5 * 1024 * 1024; ///< rotate at 5 MiB
    static const int kMaxFiles = 3;                      ///< keep 3 generations

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void rotateIfNeeded();

    mutable std::mutex mutex_;
    QString logDir_;
    QString currentPath_;
    QFile* file_ = nullptr;
    qint64 bytesWritten_ = 0;
    Level level_ = Level::Info;
    bool console_ = true;
    bool qtHandlerInstalled_ = false;
};

/// Convenience free functions.
void initLogging(const QString& logDir, bool console = true);
void setLogLevel(Level l);
void log(Level l, const QString& category, const QString& message);

} // namespace rcp::logging
