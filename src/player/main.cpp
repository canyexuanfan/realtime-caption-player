// src/player/main.cpp
// 播放器应用入口（P2 基础窗口）。后续里程碑接入字幕 worker 与叠加层。
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QSvgRenderer>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    // libmpv 的 GL 渲染器依赖兼容 profile 的旧式 GL；Qt6 默认在支持的
    // 驱动上申请 Core profile（实测 4.6 Core → mpv 建纹理 INVALID_ENUM，
    // VO 等不到首帧导致播放停滞）。必须在 QApplication 创建前设置默认格式。
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setVersion(2, 0);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("RealtimeCaptionPlayer"));
    QApplication::setOrganizationName(QStringLiteral("RealtimeCaption"));

    // 资源自检（图标/logo 空白排查）：启动时记录 :/ 资源可达性到诊断文件。
    {
        const QStringList probes = {
            QStringLiteral(":/logo.png"),
            QStringLiteral(":/icons/play.svg"),
            QStringLiteral(":/icons/settings.svg"),
            QStringLiteral(":/icons/subtitle.svg")};
        QStringList lines;
        for (const QString& p : probes)
            lines << p + QStringLiteral(" = ") + (QFile::exists(p) ? QStringLiteral("OK") : QStringLiteral("MISSING"));
        // 渲染级自检：QSvgRenderer 能否解析（防颜色格式/内容问题导致空图标）
        QFile svgf(QStringLiteral(":/icons/play.svg"));
        const QByteArray svgBytes = svgf.open(QIODevice::ReadOnly) ? svgf.readAll() : QByteArray();
        QSvgRenderer r(svgBytes);
        lines << QStringLiteral("svg-bytes = %1 svg-render(play) = %2")
                     .arg(svgBytes.size())
                     .arg(r.isValid() ? QStringLiteral("OK") : QStringLiteral("INVALID"));
        QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
        QFile f(QCoreApplication::applicationDirPath() + QStringLiteral("/resource-check.txt"));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.setFileName(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                          + QStringLiteral("/resource-check.txt"));
            f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        }
        f.write(lines.join(QLatin1Char('\n')).toUtf8());
        f.close();
    }

    MainWindow window;
    window.show();

    if (argc > 1) {
        // 命令行传入媒体路径（本地 8-bit 路径）。
        window.openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
