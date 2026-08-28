// src/player/main.cpp
// 播放器应用入口（P2 基础窗口）。后续里程碑接入字幕 worker 与叠加层。
#include <QApplication>
#include <QSurfaceFormat>
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

    MainWindow window;
    window.show();

    if (argc > 1) {
        // 命令行传入媒体路径（本地 8-bit 路径）。
        window.openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
