// SenseVoiceProbe — T0021 探针：Silero VAD 分句 + SenseVoice 终稿（sherpa-onnx C-API）。
//
// 注意：SenseVoice 在 sherpa-onnx 中是 *离线* 模型（SherpaOnnxOfflineRecognizer）。
// Silero VAD 走专用 VoiceActivityDetector API（SherpaOnnxVadModelConfig 含 silero_vad），
// 并非离线 recognizer 的子字段。
//
// 目的（编译/链接级验证 + 管线 smoke；真正分句/识别正确性需真机音频）：
//   1. 用 Silero VAD（VoiceActivityDetector）对音频做分段，证明 VAD 加载与分段链路接通。
//   2. 加载 SenseVoice int8 模型（离线 recognizer），证明终稿识别链路接通。
//
// 用法：sensevoice_probe.exe [silero_dir] [sensevoice_dir]
//   默认 .tools/models/silero 与 .tools/models/sensevoice
//
// 链接目标：rcp::sherpa-onnx -> sherpa-onnx-c-api.dll

#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

static void feed_offline(const SherpaOnnxOfflineRecognizer* rec,
                         const char* path, int32_t sr) {
    const SherpaOnnxOfflineStream* s = SherpaOnnxCreateOfflineStream(rec);
    // 合成 4 秒音频：前 1.5s 220Hz 语音，0.7s 静音，后 1.5s 330Hz 语音
    const int32_t total = sr * 4;
    const int32_t chunk = 3200;
    for (int32_t start = 0; start < total; start += chunk) {
        int32_t n = (start + chunk <= total) ? chunk : (total - start);
        float* buf = new float[n];
        for (int32_t i = 0; i < n; i++) {
            double t = (double)(start + i) / sr;
            double freq = (t < 1.5) ? 220.0 : (t > 2.2 ? 330.0 : 0.0);
            buf[i] = (freq == 0.0) ? 0.0f : 0.3f * (float)sin(2.0 * 3.141592653589793 * freq * t);
        }
        SherpaOnnxAcceptWaveformOffline(s, sr, buf, n);
        delete[] buf;
    }
    SherpaOnnxDecodeOfflineStream(rec, s);
    const SherpaOnnxOfflineRecognizerResult* r = SherpaOnnxGetOfflineStreamResult(s);
    (void)path;
    SherpaOnnxDestroyOfflineStream(s);
    (void)r; // 合成音频无语音内容；仅验证链路
}

int main(int argc, char** argv) {
    std::string silero_dir = (argc > 1) ? argv[1] : ".tools/models/silero";
    std::string sv_dir     = (argc > 2) ? argv[2] : ".tools/models/sensevoice";
    std::string vad_model  = silero_dir + "/silero_vad.onnx";
    std::string sv_model   = sv_dir + "/model.int8.onnx";
    std::string sv_tokens  = sv_dir + "/tokens.txt";

    // ---- 1) Silero VAD 分句（VoiceActivityDetector）----
    SherpaOnnxVadModelConfig vad_cfg;
    memset(&vad_cfg, 0, sizeof(vad_cfg));
    vad_cfg.silero_vad.model = vad_model.c_str();
    vad_cfg.silero_vad.threshold = 0.5f;
    vad_cfg.silero_vad.min_silence_duration = 0.5f;
    vad_cfg.silero_vad.min_speech_duration = 0.25f;
    vad_cfg.silero_vad.max_speech_duration = 20.0f;
    vad_cfg.sample_rate = 16000;
    vad_cfg.num_threads = 1;
    vad_cfg.provider = "cpu";

    const SherpaOnnxVoiceActivityDetector* vad = SherpaOnnxCreateVoiceActivityDetector(&vad_cfg, 30.0f);
    if (!vad) { std::printf("sensevoice_probe: VAD detector FAILED (silero 路径/权重错误?)\n"); return 1; }
    std::printf("sensevoice_probe: Silero VAD detector created (threshold=0.5)\n");

    const int32_t sr = 16000;
    const int32_t total = sr * 4;
    const int32_t chunk = 3200;
    int seg_count = 0;
    for (int32_t start = 0; start < total; start += chunk) {
        int32_t n = (start + chunk <= total) ? chunk : (total - start);
        float* buf = new float[n];
        for (int32_t i = 0; i < n; i++) {
            double t = (double)(start + i) / sr;
            double freq = (t < 1.5) ? 220.0 : (t > 2.2 ? 330.0 : 0.0);
            buf[i] = (freq == 0.0) ? 0.0f : 0.3f * (float)sin(2.0 * 3.141592653589793 * freq * t);
        }
        SherpaOnnxVoiceActivityDetectorAcceptWaveform(vad, buf, n);
        delete[] buf;
        while (SherpaOnnxVoiceActivityDetectorDetected(vad)) {
            const SherpaOnnxSpeechSegment* seg = SherpaOnnxVoiceActivityDetectorFront(vad);
            std::printf("sensevoice_probe: [VAD] segment start=%d n=%d (%.2fs-%.2fs)\n",
                        seg->start, seg->n, (double)seg->start / sr, (double)(seg->start + seg->n) / sr);
            seg_count++;
            SherpaOnnxVoiceActivityDetectorPop(vad);
            SherpaOnnxDestroySpeechSegment(seg);
        }
    }
    std::printf("sensevoice_probe: VAD 检测到语音段数=%d (合成音频含 2 段语音，预期出现分句边界)\n", seg_count);
    SherpaOnnxDestroyVoiceActivityDetector(vad);

    // ---- 2) SenseVoice 终稿识别（离线 recognizer）----
    SherpaOnnxOfflineRecognizerConfig sv_cfg;
    memset(&sv_cfg, 0, sizeof(sv_cfg));
    sv_cfg.feat_config.sample_rate = 16000;
    sv_cfg.feat_config.feature_dim = 80;
    sv_cfg.model_config.tokens = sv_tokens.c_str();
    sv_cfg.model_config.sense_voice.model = sv_model.c_str();
    sv_cfg.model_config.sense_voice.language = "auto";
    sv_cfg.model_config.sense_voice.use_itn = 1;

    const SherpaOnnxOfflineRecognizer* sv = SherpaOnnxCreateOfflineRecognizer(&sv_cfg);
    if (!sv) { std::printf("sensevoice_probe: SenseVoice recognizer FAILED (模型路径/权重错误?)\n"); return 1; }
    std::printf("sensevoice_probe: SenseVoice recognizer created (int8, lang=auto, itn=on)\n");
    feed_offline(sv, sv_model.c_str(), sr);
    std::printf("sensevoice_probe: SenseVoice 离线识别链路已跑通\n");
    SherpaOnnxDestroyOfflineRecognizer(sv);

    std::printf("sensevoice_probe: done (合成音频验证管线；真机需真实语音验证分句/识别正确性)\n");
    return 0;
}
