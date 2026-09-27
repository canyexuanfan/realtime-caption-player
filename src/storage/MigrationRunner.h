// src/storage/MigrationRunner.h
// schema 迁移（D1）：按 schema_meta.version 顺序应用内嵌迁移脚本
// （qrc :/migrations/001_initial.sql，来自仓库 migrations/）。
#pragma once

#include "core/Result.h"
#include <QSqlDatabase>

namespace rcp::storage {

class MigrationRunner {
public:
    /// 在给定连接上应用所有未执行的迁移（幂等）。
    static Result<int> run(QSqlDatabase db);
};

} // namespace rcp::storage
