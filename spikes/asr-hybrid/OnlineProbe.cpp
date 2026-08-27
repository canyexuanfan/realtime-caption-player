// OnlineProbe — T0020 探针：Online Zipformer2-CTC 流式 partial 识别（实时逐字）。
//
// 取代原崩溃的 int8 双语 Paraformer（BLOCKER-2 根因：该 2023-02 Paraformer 模型图
// 与 ORT 流式路径不兼容——1.13.6/ORT1.27.1 与 1.12.1/旧 ORT 均崩，官方二进制复现）。
// Zipformer2-CTC 是现代 ORT-1.27 兼容架构，单文件 model.int8.onnx + tokens.txt。
//
// 用法：online_probe.exe [zipformer_ctc_dir] [model_basename]
//   zipformer_ctc_dir 默认 .tools/models/zipformer-ctc
//   model_basename    默认 model.int8.onnx（可传 model.onnx 复测 fp32）
//
// 链接目标：rcp::sherpa-onnx -> sherpa-onnx-c-api.dll

#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);  // 无缓冲：崩溃前也能看到定位输出
    std::printf("online_probe: start (zipformer2-ctc streaming partial)\n");
    std::string dir = (argc > 1) ? argv[1] : ".tools/models/zipformer-ctc";
    std::string model_base = (argc > 2) ? argv[2] : "model.int8.onnx";
    std::string model = dir + "/" + model_base;
    std::string tok = dir + "/tokens.txt";

    SherpaOnnxOnlineRecognizerConfig config;
    memset(&config, 0, sizeof(config));
    config.feat_config.sample_rate = 16000;
    config.feat_config.feature_dim = 80;
    config.model_config.zipformer2_ctc.model = model.c_str();
    config.model_config.tokens = tok.c_str();
    config.model_config.provider = "cpu";
    config.model_config.num_threads = 1;
    config.decoding_method = "greedy_search";

    const SherpaOnnxOnlineRecognizer* rec = SherpaOnnxCreateOnlineRecognizer(&config);
    if (!rec) { std::printf("online_probe: create recognizer FAILED (模型路径/权重错误?)\n"); return 1; }
    std::printf("online_probe: recognizer created (zipformer2_ctc model=%s)\n", model_base.c_str());

    const SherpaOnnxOnlineStream* stream = SherpaOnnxCreateOnlineStream(rec);
    std::printf("online_probe: stream created\n");

    // 合成 3 秒 16kHz 单声道音频（220Hz 正弦），分块喂入，演示逐块 partial。
    // 真机应替换为真实语音 PCM。
    const int32_t sr = 16000;
    const int32_t total = sr * 3;
    const int32_t chunk = 3200; // 0.2s
    int32_t produced = 0;
    for (int32_t start = 0; start < total; start += chunk) {
        int32_t n = (start + chunk <= total) ? chunk : (total - start);
        float* buf = new float[n];
        for (int32_t i = 0; i < n; i++) {
            double t = (double)(start + i) / sr;
            buf[i] = 0.3f * (float)sin(2.0 * 3.141592653589793 * 220.0 * t);
        }
        SherpaOnnxOnlineStreamAcceptWaveform(stream, sr, buf, n);
        delete[] buf;
        while (SherpaOnnxIsOnlineStreamReady(rec, stream)) {
            SherpaOnnxDecodeOnlineStream(rec, stream);
            const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(rec, stream);
            if (r && r->text && r->text[0]) {
                std::printf("online_probe: [partial] %s\n", r->text);
            }
            SherpaOnnxDestroyOnlineRecognizerResult(r);
        }
        produced += n;
    }
    // flush：持续 decode 直到不再 ready
    while (SherpaOnnxIsOnlineStreamReady(rec, stream)) {
        SherpaOnnxDecodeOnlineStream(rec, stream);
    }
    const SherpaOnnxOnlineRecognizerResult* fin = SherpaOnnxGetOnlineStreamResult(rec, stream);
    std::printf("online_probe: [final] %s\n", (fin && fin->text) ? fin->text : "(empty)");
    if (fin) SherpaOnnxDestroyOnlineRecognizerResult(fin);

    SherpaOnnxDestroyOnlineStream(stream);
    SherpaOnnxDestroyOnlineRecognizer(rec);
    std::printf("online_probe: done (fed %d samples of synthetic 220Hz tone)\n", produced);
    return 0;
}
