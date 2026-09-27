// src/worker/main.cpp
// caption-worker 独立进程（P4 文件模式）：从媒体抽音轨 -> 双引擎 ASR
// （Zipformer2-CTC 实时 partial + SenseVoice 精准终稿）-> 导出 SRT。
// 后续里程碑将在此进程内增加 IPC server（QLocalSocket）以接入主播放器进程。
//
// 用法：
//   caption-worker.exe --file video.mkv [--track N] [--zf dir] [--vad dir] [--sv dir] [--out out.srt]
#include <QCoreApplication>
#include <QTimer>
#include <qt_windows.h>
#include <cstdio>
#include <string>
#include <exception>
#include <QTimer>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QList>
#include <cstdio>

#include "audio/AudioExtractor.h"
#include "asr/AsrEngine.h"
#include "captions/SrtExporter.h"
#include "captions/CaptionTypes.h"
#include "worker/IpcServer.h"

// 崩溃定位：SEH 顶层过滤器 → stderr 异常码 + MiniDump（RCP_CRASH_DIR 指定目录）。
#include <dbghelp.h>
static LONG WINAPI workerCrashFilter(EXCEPTION_POINTERS* info) {
    if (info && info->ExceptionRecord) {
        std::fprintf(stderr, "[worker-crash] code=0x%08lx addr=%p\n",
                     info->ExceptionRecord->ExceptionCode,
                     static_cast<void*>(info->ExceptionRecord->ExceptionAddress));
        std::fflush(stderr);
    }
    const char* dumpDir = std::getenv("RCP_CRASH_DIR");
    if (dumpDir) {
        const std::string path = std::string(dumpDir) + "/worker-crash.dmp";
        HANDLE f = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION di{GetCurrentThreadId(), info, FALSE};
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), f,
                              MiniDumpNormal, info ? &di : nullptr, nullptr, nullptr);
            CloseHandle(f);
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

// terminate 钩子：未捕获 C++ 异常走 std::terminate（不经 SEH filter），此处取证。
static void workerTerminateHandler() {
    const char* cd = std::getenv("RCP_CRASH_DIR");
    FILE* lg = cd ? std::fopen((std::string(cd) + "/worker-boot.log").c_str(), "a") : nullptr;
    if (auto ex = std::current_exception()) {
        try { std::rethrow_exception(ex); }
        catch (const std::exception& e) {
            std::fprintf(stderr, "[worker-terminate] exception: %s\n", e.what());
            if (lg) std::fprintf(lg, "terminate-exception: %s\n", e.what());
        }
        catch (...) {
            std::fprintf(stderr, "[worker-terminate] unknown exception\n");
            if (lg) std::fprintf(lg, "terminate-exception: unknown\n");
        }
    } else {
        std::fprintf(stderr, "[worker-terminate] no active exception\n");
        if (lg) std::fprintf(lg, "terminate: no active exception\n");
    }
    std::fflush(stderr);
    if (lg) std::fclose(lg);
    std::abort();
}

int main(int argc, char** argv) {
    SetUnhandledExceptionFilter(workerCrashFilter);
    std::set_terminate(workerTerminateHandler);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("caption-worker"));
    if (const char* cd = std::getenv("RCP_CRASH_DIR")) {
        FILE* lg = std::fopen((std::string(cd) + "/worker-boot.log").c_str(), "a");
        if (lg) { std::fprintf(lg, "boot pid=%ld\n", static_cast<long>(GetCurrentProcessId())); std::fclose(lg); }
    }
    std::fprintf(stderr, "[worker] boot pid=%ld\n", static_cast<long>(GetCurrentProcessId()));
    std::fflush(stderr);
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
    parser.addOption(QCommandLineOption(QStringList{"pipetest"}, "run pipeline directly on <media> (crash diagnosis)", "media"));
    parser.process(app);

    // B2 崩溃定位：--pipetest <media> 直跑 CaptionPipeline（不经 IPC/QProcess），
    // 与 spikes/pipeline-probe 等价，用于隔离 worker 链接/环境差异。
    const QString pipeTest = parser.value("pipetest");
    if (!pipeTest.isEmpty()) {
        rcp::worker::CaptionPipeline pipe;
        pipe.setModelsRoot(parser.value("models"));
        QObject::connect(&pipe, &rcp::worker::CaptionPipeline::mediaOpened,
                         [](const QString& p) { std::printf("[pt] mediaOpened %s\n", p.toUtf8().constData()); });
        QObject::connect(&pipe, &rcp::worker::CaptionPipeline::pipelineError,
                         [](const QString& m) { std::printf("[pt] error %s\n", m.toUtf8().constData()); });
        QTimer drain;
        QObject::connect(&drain, &QTimer::timeout, [&pipe] {
            rcp::CaptionSegment seg;
            while (pipe.popOutbound(seg)) {
                std::printf("[pt] %s %lld-%lld %s\n",
                            seg.kind == rcp::CaptionKind::Partial ? "partial" : "final",
                            seg.startMs, seg.endMs, seg.text.toUtf8().constData());
            }
        });
        drain.start(100);
        std::printf("[pt] start\n");
        if (!pipe.start(pipeTest, -1, 1)) return 1;
        QTimer::singleShot(10000, &app, [&app] { QCoreApplication::exit(0); });
        const int rc = app.exec();
        pipe.stop();
        std::printf("[pt] done rc=%d\n", rc);
        return rc;
    }

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
