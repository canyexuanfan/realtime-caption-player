// src/worker/main.cpp
// caption-worker 独立进程（P4 文件模式）：从媒体抽音轨 -> 双引擎 ASR
// （Zipformer2-CTC 实时 partial + SenseVoice 精准终稿）-> 导出 SRT。
// 后续里程碑将在此进程内增加 IPC server（QLocalSocket）以接入主播放器进程。
//
// 用法：
//   caption-worker.exe --file video.mkv [--track N] [--zf dir] [--vad dir] [--sv dir] [--out out.srt]
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QList>
#include <cstdio>

#include "audio/AudioExtractor.h"
#include "asr/AsrEngine.h"
#include "captions/SrtExporter.h"
#include "captions/CaptionTypes.h"
#include "worker/IpcServer.h"

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("caption-worker"));
    // stdout 无缓冲：崩溃前已产生的 partial/final 输出不丢失。
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList{"f", "file"}, "media file", "path"));
    parser.addOption(QCommandLineOption(QStringList{"t", "track"}, "audio track index (-1=first)", "idx", "-1"));
    parser.addOption(QCommandLineOption(QStringList{"zf"}, "zipformer-ctc model dir", "dir", ".tools/models/zipformer-ctc"));
    parser.addOption(QCommandLineOption(QStringList{"vad"}, "silero vad dir", "dir", ".tools/models/silero"));
    parser.addOption(QCommandLineOption(QStringList{"sv"}, "sensevoice dir", "dir", ".tools/models/sensevoice"));
    parser.addOption(QCommandLineOption(QStringList{"o", "out"}, "output srt path", "path", "out.srt"));
    parser.addOption(QCommandLineOption(QStringList{"s", "servername"}, "IPC server socket name (service mode)", "name"));
    parser.addOption(QCommandLineOption(QStringList{"m", "models"}, "models root dir (zipformer-ctc/silero/sensevoice)", "dir", ".tools/models"));
    parser.process(app);

    // IPC 服务模式：作为后台进程监听 QLocalServer，由主播放器进程连接并下发命令。
    const QString serverName = parser.value("servername");
    if (!serverName.isEmpty()) {
        rcp::worker::IpcServer srv;
        srv.setModelsRoot(parser.value("models"));
        if (!srv.listen(serverName)) {
            std::fprintf(stderr, "caption-worker: listen failed: %s\n", qPrintable(serverName));
            return 1;
        }
        std::printf("caption-worker: IPC server listening on %s\n", qPrintable(serverName));
        return app.exec();
    }

    const QString file = parser.value("file");
    if (file.isEmpty()) {
        std::fprintf(stderr, "caption-worker: --file required\n");
        return 2;
    }
    const int track = parser.value("track").toInt();

    rcp::audio::AudioExtractor ex;
    if (!ex.open(file, track)) {
        std::fprintf(stderr, "caption-worker: open failed: %s\n", ex.lastError().toUtf8().constData());
        return 1;
    }

    rcp::asr::AsrEngine engine;
    if (!engine.load(parser.value("zf"), parser.value("vad"), parser.value("sv"))) {
        std::fprintf(stderr, "caption-worker: asr load failed: %s\n", engine.lastError().toUtf8().constData());
        return 1;
    }

    QList<rcp::CaptionSegment> finals;
    engine.setCaptionCallback([&](const rcp::asr::CaptionUtterance& u) {
        if (u.isPartial) {
            std::printf("[partial] %s\n", u.text.toUtf8().constData());
        } else {
            std::printf("[final] %lld-%lld %s\n", u.startMs, u.endMs, u.text.toUtf8().constData());
            rcp::CaptionSegment seg;
            seg.startMs = u.startMs;
            seg.endMs   = u.endMs;
            seg.text    = u.text;
            seg.kind    = rcp::CaptionKind::Final;
            finals.append(seg);
        }
    });

    const long long total = ex.extract(0.1, [&](const float* s, int n, double /*t*/) {
        engine.feed(s, n);
    });
    engine.flush();

    const auto res = rcp::captions::SrtExporter::writeSrt(parser.value("out"), finals);
    if (res.isError()) {
        std::fprintf(stderr, "caption-worker: write srt failed\n");
        return 3;
    }
    std::printf("caption-worker: processed %lld samples, wrote %d segments -> %s\n",
                total, finals.size(), parser.value("out").toUtf8().constData());
    return 0;
}
