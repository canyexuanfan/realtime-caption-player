// spikes/seek-e2e/SeekE2EProbe.cpp
// 阶段 B 验收探针（worker 层）：打开长媒体 -> seekToSec(SeekSec) -> 解码喂入
// 双引擎 -> 校验产出字幕时间戳落在 seek 附近（PTS 锚定 + seek 生效）。
// 用法：seek_e2e_probe <media> [seekSec=1800] [modelsRoot=.tools/models]
// PASS 判定：存在非空字幕事件其 startMs 落在 [seek-5s, seek+45s]。
#include "audio/AudioExtractor.h"
#include "asr/AsrEngine.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("usage: %s <media> [seekSec] [modelsRoot]\n", argv[0]);
        return 2;
    }
    const QString media = QString::fromLocal8Bit(argv[1]);
    const double seekSec = argc > 2 ? std::atof(argv[2]) : 1800.0;
    const QString models = argc > 3 ? QString::fromLocal8Bit(argv[3])
                                    : QStringLiteral(".tools/models");

    rcp::audio::AudioExtractor ex;
    if (!ex.open(media, -1)) {
        std::printf("OPEN-FAILED %s\n", ex.lastError().toUtf8().constData());
        return 1;
    }
    std::printf("duration=%.1fs\n", ex.durationSec());

    rcp::asr::AsrEngine eng;
    if (!eng.load(models + "/zipformer-ctc", models + "/silero", models + "/sensevoice")) {
        std::printf("ASR-LOAD-FAILED %s\n", eng.lastError().toUtf8().constData());
        return 1;
    }

    struct Ev { bool partial; long long s, e; QString text; };
    std::vector<Ev> evs;
    eng.setCaptionCallback([&](const rcp::asr::CaptionUtterance& u) {
        evs.push_back({u.isPartial, u.startMs, u.endMs, u.text});
    });

    if (!ex.seekToSec(seekSec)) {
        std::printf("SEEK-FAILED %s\n", ex.lastError().toUtf8().constData());
        return 1;
    }
    ex.extract(0.1, [&](const float* s, int n, double t) {
        eng.feed(s, n, static_cast<long long>(std::llround(t * 1000.0)));
    });
    eng.flush();

    const long long lo = static_cast<long long>((seekSec - 5.0) * 1000.0);
    const long long hi = static_cast<long long>((seekSec + 45.0) * 1000.0);
    bool ok = false;
    for (const auto& e : evs) {
        if (!e.text.isEmpty() && e.s >= lo && e.s <= hi) ok = true;
    }
    std::printf("events=%d\n", static_cast<int>(evs.size()));
    for (const auto& e : evs) {
        std::printf("[%s] %lld-%lldms %s\n", e.partial ? "partial" : "final",
                    e.s, e.e, e.text.toUtf8().constData());
    }
    std::printf("%s\n", ok ? "SEEK-E2E-PASS" : "SEEK-E2E-FAIL");
    return ok ? 0 : 3;
}
