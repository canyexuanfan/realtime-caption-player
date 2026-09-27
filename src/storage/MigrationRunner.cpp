// src/storage/MigrationRunner.cpp
#include "MigrationRunner.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QResource>
#include <QFile>
#include <QStringList>

namespace rcp::storage {

Result<int> MigrationRunner::run(QSqlDatabase db) {
    // 版本探测：schema_meta 由 001 迁移自身创建（不能在这里预建，否则 001 的
    // CREATE TABLE 会因表已存在而失败）。
    QSqlQuery q(db);
    int version = 0;
    if (q.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name='schema_meta'")) &&
        q.next()) {
        QSqlQuery qv(db);
        if (qv.exec(QStringLiteral("SELECT MAX(version) FROM schema_meta")) && qv.next())
            version = qv.value(0).isNull() ? 0 : qv.value(0).toInt();
    }

    // 迁移清单：版本号 -> qrc 脚本（与仓库 migrations/ 同源，禁止双份维护）。
    const QVector<QPair<int, QString>> migrations = {
        {1, QStringLiteral(":/migrations/001_initial.sql")},
        {2, QStringLiteral(":/migrations/002_add_display_name.sql")},
        {3, QStringLiteral(":/migrations/003_add_last_path.sql")},
    };

    for (const auto& m : migrations) {
        if (m.first <= version) continue;
        QFile f(m.second);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return Result<int>::fail(AppError(ErrorDomain::Storage,
                QStringLiteral("DB-MIGRATE-FAILED"), QStringLiteral("数据库迁移失败"),
                QStringLiteral("migration %1 not found: %2").arg(m.first).arg(m.second)));
        const QStringList statements = QString::fromUtf8(f.readAll())
                                           .split(QLatin1Char(';'), Qt::SkipEmptyParts);
        for (const QString& stmtRaw : statements) {
            // 逐行剥离 "--" 注释（文件头注释不能导致整条语句被跳过——003 教训）。
            QString original;
            const QStringList lines = stmtRaw.split(QLatin1Char('\n'));
            for (const QString& line : lines) {
                const QString trimmed = line.trimmed();
                if (trimmed.startsWith(QStringLiteral("--"))) continue;
                original += line + QLatin1Char('\n');
            }
            original = original.trimmed();
            if (original.isEmpty()) continue;
            QSqlQuery mq(db);
            if (!mq.exec(original))
                return Result<int>::fail(AppError(ErrorDomain::Storage,
                    QStringLiteral("DB-MIGRATE-FAILED"), QStringLiteral("数据库迁移失败"),
                    QStringLiteral("v%1: %2").arg(m.first).arg(mq.lastError().text())));
        }
        QSqlQuery rec(db);
        rec.prepare(QStringLiteral("INSERT INTO schema_meta (version) VALUES (?)"));
        rec.addBindValue(m.first);
        if (!rec.exec())
            return Result<int>::fail(AppError(ErrorDomain::Storage,
                QStringLiteral("DB-MIGRATE-FAILED"), QStringLiteral("数据库迁移失败"),
                rec.lastError().text()));
        version = m.first;
    }
    return Result<int>::ok(version);
}

} // namespace rcp::storage
