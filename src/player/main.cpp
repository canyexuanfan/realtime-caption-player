// src/player/main.cpp
// 播放器应用入口（P2 基础窗口）。后续里程碑接入字幕 worker 与叠加层。
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
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
