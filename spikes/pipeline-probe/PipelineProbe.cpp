// spikes/pipeline-probe/PipelineProbe.cpp
// B2 管线崩溃定位探针：直接实例化 CaptionPipeline（不经 IPC），
// 模拟播放头推进，输出全部事件。崩溃点即栈现场（控制台程序直接可见）。
// 用法: pipeline_probe <media> <modelsRoot> [seconds=15]
#include "worker/CaptionPipeline.h"

#include <QCoreApplication>
#include <QTimer>
#include <cstdio>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 3) {
        std::printf("usage: %s <media> <modelsRoot> [seconds]\n", argv[0]);
        return 2;
    }
    const QString media = QString::fromLocal8Bit(argv[1]);
    const QString models = QString::fromLocal8Bit(argv[2]);
    const int seconds = argc > 3 ? atoi(argv[3]) : 15;

    rcp::worker::CaptionPipeline pipe;
    QObject::connect(&pipe, &rcp::worker::CaptionPipeline::mediaOpened, [](const QString& p) {
        std::printf("[event] mediaOpened %s\n", p.toUtf8().constData());
    });
    QObject::connect(&pipe, &rcp::worker::CaptionPipeline::pipelineError, [](const QString& m) {
        std::printf("[event] pipelineError %s\n", m.toUtf8().constData());
    });
    QObject::connect(&pipe, &rcp::worker::CaptionPipeline::overloadDetected, [](double rtf) {
        std::printf("[event] overload rtf=%.2f\n", rtf);
    });

    QTimer drain;
    QObject::connect(&drain, &QTimer::timeout, [&pipe] {
        rcp::CaptionSegment seg;
        while (pipe.popOutbound(seg)) {
            std::printf("[out] %s %lld-%lld %s\n",
                        seg.kind == rcp::CaptionKind::Partial ? "partial" : "final",
                        seg.startMs, seg.endMs, seg.text.toUtf8().constData());
        }
    });
    drain.start(100);

    std::printf("[probe] start\n");
    pipe.setModelsRoot(models);
    if (!pipe.start(media, -1, 1)) {
        std::printf("[probe] start FAILED\n");
        return 1;
    }
    // 模拟播放头推进（0 → seconds，每 100ms +100ms）。
    qint64 head = 0;
    QTimer play;
    QObject::connect(&play, &QTimer::timeout, [&pipe, &head] {
        pipe.updatePlayback(head, 1.0, false);
        head += 100;
    });
    play.start(100);

    QTimer::singleShot(seconds * 1000, &app, [&app] {
        std::printf("[probe] stopping\n");
        QCoreApplication::exit(0);
    });
    const int rc = app.exec();
    pipe.stop();
    std::printf("[probe] done rc=%d\n", rc);
    return rc;
}
