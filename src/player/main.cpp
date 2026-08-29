// src/player/main.cpp
// 播放器应用入口（P2 基础窗口）。后续里程碑接入字幕 worker 与叠加层。
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QSvgRenderer>
#include <QTimer>
#include "MainWindow.h"
#include "MpvTrace.h"

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
    // 视觉自验证：RCP_SNAPSHOT=<png路径> 时正常显示窗口（真实 GL 渲染，
    // 可验证视频画面），默认 5s/16s 两次 grab 存 PNG 后退出；
    // RCP_SNAP_T1/T2（毫秒）可覆盖（慢介质/晚开口的验证场景）。
    const QString snapPath = qEnvironmentVariable("RCP_SNAPSHOT");
    if (!snapPath.isEmpty()) {
        int t1 = qEnvironmentVariableIntValue("RCP_SNAP_T1");
        if (t1 <= 0) t1 = 5000;
        int t2 = qEnvironmentVariableIntValue("RCP_SNAP_T2");
        if (t2 <= 0) t2 = 16000;
        QTimer::singleShot(t1, &window, [&window, snapPath] {
            window.grab().save(snapPath);
        });
        QTimer::singleShot(t2, &window, [&window, snapPath] {
            const int dot = snapPath.lastIndexOf(QLatin1Char('.'));
            window.grab().save(snapPath.left(dot) + QStringLiteral(".b") + snapPath.mid(dot));
            QApplication::quit();
        });
    }
    window.show();
    rcpMark("main:shown");
    // 快照/示例模式：统一 1280x800 逻辑尺寸（与 Edge 无头渲染同宽，像素比对用），
    // 不恢复上次的窗口几何/最大化状态。
    if (!snapPath.isEmpty() || !qEnvironmentVariableIsEmpty("RCP_DEMO_ONLY")) {
        window.showNormal();
        window.resize(1280, 800);
    }
    // 快照模式：5s 首验 + 16s 复验（证明持续播放非单帧）后退出。
    // （旧 2.5s 二分定时器已删：它 grab 后直接 quit，会截断 16s 复验。）

    if (argc > 1) {
        // 命令行传入媒体路径。延迟 1.5s：等窗口完成首次绘制（渲染上下文就绪），
        // 否则 loadfile 的 VO 初始化会因 "No render context set" 失败 → 永久黑屏。
        const QString mediaPath = QString::fromLocal8Bit(argv[1]);
        window.setStartupMedia(mediaPath);   // 恢复会话让位，杜绝双 loadfile 竞态
        QTimer::singleShot(1500, &window, [&window, mediaPath] {
            rcpTrace(QStringLiteral("open-timer[argv] fire"));
            window.openFile(mediaPath);
        });
    }

    return app.exec();
}
