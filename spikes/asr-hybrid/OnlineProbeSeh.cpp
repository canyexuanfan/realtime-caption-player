// OnlineProbeSeh — T0020 诊断变体：用 SEH 捕获 CreateOnlineRecognizer 的崩溃异常码/地址。
#include <cstdio>
#include <cstring>
#include <cmath>
#include <windows.h>
extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

static const SherpaOnnxOnlineRecognizer* make_rec(const SherpaOnnxOnlineRecognizerConfig* cfg) {
    return SherpaOnnxCreateOnlineRecognizer(cfg);
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    std::printf("online_probe_seh: start\n");
    char enc[512], dec[512], tok[512];
    const char* dir = (argc > 1) ? argv[1] : ".tools/models/paraformer";
    sprintf(enc, "%s/encoder.int8.onnx", dir);
    sprintf(dec, "%s/decoder.onnx", dir);
    sprintf(tok, "%s/tokens.txt", dir);
    std::printf("online_probe_seh: enc=%s\n", enc);
    std::printf("online_probe_seh: dec=%s\n", dec);
    std::printf("online_probe_seh: tok=%s\n", tok);

    SherpaOnnxOnlineRecognizerConfig config;
    memset(&config, 0, sizeof(config));
    config.feat_config.sample_rate = 16000;
    config.feat_config.feature_dim = 80;
    config.model_config.paraformer.encoder = enc;
    config.model_config.paraformer.decoder = dec;
    config.model_config.tokens = tok;
    config.model_config.provider = "cpu";
    config.model_config.num_threads = 1;
    config.decoding_method = "greedy_search";

    const SherpaOnnxOnlineRecognizer* rec = nullptr;
    EXCEPTION_POINTERS* ep = nullptr;
    __try {
        rec = make_rec(&config);
    } __except(ep = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
        std::printf("online_probe_seh: SEH exception code=0x%08X addr=%p\n",
                    ep->ExceptionRecord->ExceptionCode,
                    ep->ExceptionRecord->ExceptionAddress);
        return 2;
    }
    if (!rec) { std::printf("online_probe_seh: recognizer returned NULL (模型/tokens 错误)\n"); return 1; }
    std::printf("online_probe_seh: recognizer created OK\n");

    const SherpaOnnxOnlineStream* stream = SherpaOnnxCreateOnlineStream(rec);
    std::printf("online_probe_seh: stream created\n");
    const int32_t sr = 16000, total = sr * 3, chunk = 3200;
    for (int32_t s = 0; s < total; s += chunk) {
        int32_t n = (s + chunk <= total) ? chunk : (total - s);
        float* buf = new float[n];
        for (int32_t i = 0; i < n; i++) {
            double t = (double)(s + i) / sr;
            buf[i] = 0.3f * (float)sin(2.0 * 3.141592653589793 * 220.0 * t);
        }
        SherpaOnnxOnlineStreamAcceptWaveform(stream, sr, buf, n);
        delete[] buf;
        while (SherpaOnnxIsOnlineStreamReady(rec, stream)) SherpaOnnxDecodeOnlineStream(rec, stream);
    }
    const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(rec, stream);
    std::printf("online_probe_seh: final=[%s]\n", (r && r->text) ? r->text : "(empty)");
    if (r) SherpaOnnxDestroyOnlineRecognizerResult(r);
    SherpaOnnxDestroyOnlineStream(stream);
    SherpaOnnxDestroyOnlineRecognizer(rec);
    std::printf("online_probe_seh: done\n");
    return 0;
}
