// src/player/MpvTrace.h
// 诊断 trace（P7 诊断能力的一部分）：设置环境变量 RCP_TRACE=<文件路径> 时，
// 将播放/渲染事件流写入该文件。inline 函数保证多编译单元共享同一静态实例，
// 避免多句柄写同一文件互相覆盖。
#pragma once

#include <QDateTime>
#include <QFile>
#include <QString>

inline void rcpTrace(const QString& line) {
    static QFile f;
    static bool initialized = [] {
        const QString path = qEnvironmentVariable("RCP_TRACE");
        if (path.isEmpty()) return false;
        f.setFileName(path);
        return f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    }();
    if (!initialized) return;
    f.write(QStringLiteral("[%1] %2\n")
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")), line)
                .toUtf8());
    f.flush();
}
