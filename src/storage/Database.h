// src/storage/Database.h
// SQLite 持久化（D1）：WAL 模式 + 外键开启；每个线程独立的 QSqlDatabase
// 连接（Qt SQL 不允许跨线程共享连接），供仓库层在后台线程读写。
#pragma once

#include "core/Result.h"
#include <QString>
#include <QSqlDatabase>

namespace rcp::storage {

class Database {
public:
    /// 打开全局数据库文件（主线程调用一次；文件位于应用数据目录）。
    static Result<void> open(const QString& filePath);

    /// 数据库文件路径（未 open 时为空）。
    static QString filePath();

    /// 当前线程的连接（按线程命名懒创建；主线程连接由 open() 建立）。
    static QSqlDatabase threadConnection();

    /// 快捷执行（当前线程连接）。
    static Result<void> execute(const QString& sql);

private:
    Database() = default;
};

} // namespace rcp::storage
