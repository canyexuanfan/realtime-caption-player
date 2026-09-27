// src/storage/Database.cpp
#include "Database.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QMutexLocker>

// 静态库资源初始化：qrc 编进 rcp_core 后其初始化对象可能被链接器丢弃
// （同 rcp_player 当年 qt_add_resources 教训），需显式 Q_INIT_RESOURCE 拉入。
static void initCoreResources() { Q_INIT_RESOURCE(rcp); }

namespace rcp::storage {

namespace {
QMutex g_mutex;
QString g_filePath;
constexpr auto kMainConnection = "rcp-db-main";
}

Result<void> Database::open(const QString& filePath) {
    initCoreResources();
    QMutexLocker lock(&g_mutex);
    g_filePath = filePath;
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kMainConnection);
    db.setDatabaseName(filePath);
    if (!db.open())
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-OPEN-FAILED"), QStringLiteral("无法打开本地数据库"),
            QStringLiteral("sqlite open failed: %1").arg(db.lastError().text())));
    QSqlQuery q(db);
    // WAL：读写并发；外键：caption_session -> media 级联。
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    return Result<void>::ok();
}

QString Database::filePath() {
    QMutexLocker lock(&g_mutex);
    return g_filePath;
}

QSqlDatabase Database::threadConnection() {
    const QString name = QStringLiteral("rcp-db-%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    {
        QMutexLocker lock(&g_mutex);
        if (QSqlDatabase::contains(name)) return QSqlDatabase::database(name, true);
    }
    QString path;
    { QMutexLocker lock(&g_mutex); path = g_filePath; }
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    db.setDatabaseName(path);
    db.open();
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    return db;
}

Result<void> Database::execute(const QString& sql) {
    QSqlQuery q(threadConnection());
    if (!q.exec(sql))
        return Result<void>::fail(AppError(ErrorDomain::Storage,
            QStringLiteral("DB-EXEC-FAILED"), QStringLiteral("数据库操作失败"),
            q.lastError().text()));
    return Result<void>::ok();
}

} // namespace rcp::storage
