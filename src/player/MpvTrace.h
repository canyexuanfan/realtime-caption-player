// src/player/MpvTrace.h
// 诊断 trace（P7 诊断能力的一部分）：设置环境变量 RCP_TRACE=<文件路径> 时，
// 将播放/渲染事件流写入该文件。
// 实现为每次调用独立开关文件（诊断用途，性能不敏感）：杜绝静态 QFile 实例
// 按 MSVC 编译单元分裂、句柄互踩导致 trace 行丢失的问题（实测多次坑）。
#pragma once

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QString>
#include <cstdio>

inline void rcpTrace(const QString& line) {
    const QString path = qEnvironmentVariable("RCP_TRACE");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append)) return;
    f.write(QStringLiteral("[%1 pid=%2] %3\n")
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")))
                .arg(QCoreApplication::applicationPid())
                .arg(line)
                .toUtf8());
    f.close();
}

// C 标准库直写标记（绕开 Qt 文件层，用于定位挂死/静默退出点）。
// RCP_MARK=<文件路径> 时启用。
inline void rcpMark(const char* tag) {
    const QString path = qEnvironmentVariable("RCP_MARK");
    if (path.isEmpty()) return;
    FILE* fp = nullptr;
    fopen_s(&fp, path.toLocal8Bit().constData(), "a");
    if (!fp) return;
    fprintf(fp, "[%ld] %s\n", static_cast<long>(QCoreApplication::applicationPid()), tag);
    fclose(fp);
}
