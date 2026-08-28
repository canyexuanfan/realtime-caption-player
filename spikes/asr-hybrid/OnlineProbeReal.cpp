// OnlineProbeReal — 最小复现探针：真实语音 PCM 喂 Zipformer2-CTC 在线解码。
// 用途：判定"真实语音首解码崩溃"是 sherpa/模型层问题还是 worker 工程问题。
// 用法：online_probe_real.exe <model_dir> <wav_path(s16le mono)>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    std::string dir = (argc > 1) ? argv[1] : ".tools/models/zipformer-ctc";
    std::string wav  = (argc > 2) ? argv[2] : "out/e2e-verify/speech16k.wav";
    std::printf("probe_real: dir=%s wav=%s\n", dir.c_str(), wav.c_str());

    // ---- 读 wav（跳到 data 块）----
    FILE* f = std::fopen(wav.c_str(), "rb");
    if (!f) { std::printf("probe_real: open wav failed\n"); return 1; }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> raw(static_cast<size_t>(size));
    std::fread(raw.data(), 1, raw.size(), f);
    std::fclose(f);
    // 找 data 块
    size_t pos = 12;
    int16_t* samples = nullptr;
    size_t nSamples = 0;
    while (pos + 8 <= raw.size()) {
        uint32_t id = *reinterpret_cast<uint32_t*>(&raw[pos]);
        uint32_t sz = *reinterpret_cast<uint32_t*>(&raw[pos + 4]);
        if (id == 0x61746164) { // 'data'
            samples = reinterpret_cast<int16_t*>(&raw[pos + 8]);
            nSamples = sz / 2;
            break;
        }
        pos += 8 + sz + (sz & 1);
    }
    if (!samples) { std::printf("probe_real: no data chunk\n"); return 1; }
    std::printf("probe_real: %zu samples (%.2fs)\n", nSamples, nSamples / 16000.0);

    // ---- 识别器（与 AsrEngine/probe 相同配置）----
    SherpaOnnxOnlineRecognizerConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.feat_config.sample_rate = 16000;
    cfg.feat_config.feature_dim = 80;
    std::string model = dir + "/model.int8.onnx";
    std::string tok   = dir + "/tokens.txt";
    cfg.model_config.zipformer2_ctc.model = model.c_str();
    cfg.model_config.tokens = tok.c_str();
    cfg.model_config.provider = "cpu";
    cfg.model_config.num_threads = 1;
    cfg.decoding_method = "greedy_search";
    const SherpaOnnxOnlineRecognizer* rec = SherpaOnnxCreateOnlineRecognizer(&cfg);
    if (!rec) { std::printf("probe_real: create failed\n"); return 1; }
    std::printf("probe_real: recognizer created\n");
    const SherpaOnnxOnlineStream* stream = SherpaOnnxCreateOnlineStream(rec);
    std::printf("probe_real: stream created\n");

    const int32_t sr = 16000;
    const int32_t chunk = 1600;
    std::vector<float> buf;
    for (size_t start = 0; start < nSamples; start += chunk) {
        size_t n = (start + chunk <= nSamples) ? chunk : (nSamples - start);
        buf.resize(n);
        for (size_t i = 0; i < n; i++) buf[i] = samples[start + i] / 32768.0f;
        SherpaOnnxOnlineStreamAcceptWaveform(stream, sr, buf.data(), static_cast<int32_t>(n));
        std::printf("feed %zu\n", start / chunk + 1);
        while (SherpaOnnxIsOnlineStreamReady(rec, stream)) {
            std::printf("  decode...\n");
            SherpaOnnxDecodeOnlineStream(rec, stream);
            std::printf("  get result...\n");
            const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(rec, stream);
            if (r && r->text && r->text[0]) std::printf("  [partial] %s\n", r->text);
            SherpaOnnxDestroyOnlineRecognizerResult(r);
            std::printf("  ok\n");
        }
    }
    std::printf("probe_real: done, no crash\n");
    return 0;
}
