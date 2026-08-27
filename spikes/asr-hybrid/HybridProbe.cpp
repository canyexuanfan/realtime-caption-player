// HybridProbe — T0022 探针：实时逐字 partial（Zipformer2-CTC 流式）
//                + 精准终稿（Silero VAD 分句 × SenseVoice 离线）。
//
// 用户硬要求：实时逐字 partial 必须可用，不可降级。
// 原 Paraformer 流式在 ORT1.27.1 下崩溃（BLOCKER-2：2023-02 双语 Paraformer 模型图
// 与 ORT 流式路径不兼容，1.13.6/1.12.1 均崩）→ 改用现代 ORT-1.27 兼容的
// Zipformer2-CTC 作实时引擎；SenseVoice 仍负责精准终稿 + ITN（数字/标点规整）。
//
// 架构：VAD 分句 → 每段①Zipformer2-CTC 流式实时 partial（逐字上屏）
//                    ②SenseVoice 离线精准识别（终稿 SRT，带 ITN）
//
// 用法：hybrid_probe.exe [zipformer_ctc_dir] [silero_dir] [sensevoice_dir] [model_basename]
//   默认 .tools/models/{zipformer-ctc,silero,sensevoice}；model_basename 默认 model.int8.onnx
//
// 链接目标：rcp::sherpa-onnx -> sherpa-onnx-c-api.dll

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);  // 无缓冲：崩溃前也能看到定位输出
    std::printf("hybrid_probe: start\n");
    std::string zf_dir  = (argc > 1) ? argv[1] : ".tools/models/zipformer-ctc";
    std::string vad_dir = (argc > 2) ? argv[2] : ".tools/models/silero";
    std::string sv_dir  = (argc > 3) ? argv[3] : ".tools/models/sensevoice";
    std::string model_base = (argc > 4) ? argv[4] : "model.int8.onnx";
    std::string zf_model = zf_dir + "/" + model_base;
    std::string zf_tokens = zf_dir + "/tokens.txt";
    std::string vad_model = vad_dir + "/silero_vad.onnx";
    std::string sv_model  = sv_dir  + "/model.int8.onnx";
    std::string sv_tokens = sv_dir  + "/tokens.txt";

    // ---------- 1) Silero VAD 分句 ----------
    SherpaOnnxVadModelConfig vad_cfg;
    memset(&vad_cfg, 0, sizeof(vad_cfg));
    vad_cfg.silero_vad.model = vad_model.c_str();
    vad_cfg.silero_vad.threshold = 0.5f;
    vad_cfg.silero_vad.min_silence_duration = 0.5f;
    vad_cfg.silero_vad.min_speech_duration = 0.25f;
    vad_cfg.silero_vad.max_speech_duration = 20.0f;
    vad_cfg.silero_vad.window_size = 512;  // 必须 512/1024/1536；漏设=0 会致 VAD 加载/推理异常
    vad_cfg.sample_rate = 16000;
    vad_cfg.num_threads = 1;
    vad_cfg.provider = "cpu";
    // ten_vad 必须给空串而非 NULL（memset 后=NULL），否则 dll 内 std::string(NULL) 触发 UB
    vad_cfg.ten_vad.model = "";
    vad_cfg.ten_vad.threshold = 0.5f;
    vad_cfg.ten_vad.min_silence_duration = 0.5f;
    vad_cfg.ten_vad.min_speech_duration = 0.25f;
    vad_cfg.ten_vad.max_speech_duration = 20.0f;
    vad_cfg.ten_vad.window_size = 256;

    const SherpaOnnxVoiceActivityDetector* vad = SherpaOnnxCreateVoiceActivityDetector(&vad_cfg, 30.0f);
    if (!vad) { std::printf("hybrid_probe: VAD detector FAILED (silero 路径/权重错误?)\n"); return 1; }
    std::printf("hybrid_probe: Silero VAD detector created (threshold=0.5)\n");

    // ---------- 2) Zipformer2-CTC 在线识别器（实时逐字 partial 引擎）----------
    SherpaOnnxOnlineRecognizerConfig zf_cfg;
    memset(&zf_cfg, 0, sizeof(zf_cfg));
    zf_cfg.feat_config.sample_rate = 16000;
    zf_cfg.feat_config.feature_dim = 80;
    zf_cfg.model_config.zipformer2_ctc.model = zf_model.c_str();
    zf_cfg.model_config.tokens = zf_tokens.c_str();
    zf_cfg.model_config.provider = "cpu";
    zf_cfg.model_config.num_threads = 1;
    zf_cfg.decoding_method = "greedy_search";
    const SherpaOnnxOnlineRecognizer* zf = SherpaOnnxCreateOnlineRecognizer(&zf_cfg);
    if (!zf) { std::printf("hybrid_probe: Zipformer2-CTC FAILED (模型路径/权重错误?)\n"); return 1; }
    std::printf("hybrid_probe: Zipformer2-CTC 在线识别器 created (实时逐字引擎就绪)\n");
    const SherpaOnnxOnlineStream* zs = SherpaOnnxCreateOnlineStream(zf);

    // ---------- 3) SenseVoice 离线识别器（精准终稿 + ITN）----------
    SherpaOnnxOfflineRecognizerConfig sv_cfg;
    memset(&sv_cfg, 0, sizeof(sv_cfg));
    sv_cfg.feat_config.sample_rate = 16000;
    sv_cfg.feat_config.feature_dim = 80;
    sv_cfg.model_config.tokens = sv_tokens.c_str();
    sv_cfg.model_config.sense_voice.model = sv_model.c_str();
    sv_cfg.model_config.sense_voice.language = "auto";
    sv_cfg.model_config.sense_voice.use_itn = 1;

    const SherpaOnnxOfflineRecognizer* sv = SherpaOnnxCreateOfflineRecognizer(&sv_cfg);
    if (!sv) { std::printf("hybrid_probe: SenseVoice recognizer FAILED (模型路径/权重错误?)\n"); return 1; }
    std::printf("hybrid_probe: SenseVoice recognizer created (int8, lang=auto, itn=on)\n");

    // ---------- 4) 合成音频 → 分块喂入：实时 partial + VAD 分句后 SenseVoice 终稿 ----------
    const int32_t sr = 16000;
    const int32_t total = sr * 4;
    const int32_t chunk = 3200;
    std::vector<float> all_samples;   // 累积全部样本，供按 VAD 段边界切片送 SenseVoice
    all_samples.reserve((size_t)total);
    int seg_idx = 1;
    std::printf("\n--- 实时逐字(partial) + 精准终稿(final) 融合演示（合成音频无语音，文本为空属正常）---\n");
    for (int32_t start = 0; start < total; start += chunk) {
        int32_t n = (start + chunk <= total) ? chunk : (total - start);
        float* buf = new float[n];
        for (int32_t i = 0; i < n; i++) {
            double t = (double)(start + i) / sr;
            double freq = (t < 1.5) ? 220.0 : (t > 2.2 ? 330.0 : 0.0);
            buf[i] = (freq == 0.0) ? 0.0f : 0.3f * (float)sin(2.0 * 3.141592653589793 * freq * t);
        }
        // 实时 partial：zipformer2-ctc 逐块解码（单一 stream 累积上下文，产出递增 partial）
        SherpaOnnxOnlineStreamAcceptWaveform(zs, sr, buf, n);
        while (SherpaOnnxIsOnlineStreamReady(zf, zs)) {
            SherpaOnnxDecodeOnlineStream(zf, zs);
            const SherpaOnnxOnlineRecognizerResult* r = SherpaOnnxGetOnlineStreamResult(zf, zs);
            if (r && r->text && r->text[0]) std::printf("  [partial] %s\n", r->text);
            SherpaOnnxDestroyOnlineRecognizerResult(r);
        }
        // VAD 分段
        SherpaOnnxVoiceActivityDetectorAcceptWaveform(vad, buf, n);
        all_samples.insert(all_samples.end(), buf, buf + n);
        delete[] buf;
        while (SherpaOnnxVoiceActivityDetectorDetected(vad)) {
            const SherpaOnnxSpeechSegment* seg = SherpaOnnxVoiceActivityDetectorFront(vad);
            const float* seg_samples = all_samples.data() + seg->start;
            const SherpaOnnxOfflineStream* os = SherpaOnnxCreateOfflineStream(sv);
            SherpaOnnxAcceptWaveformOffline(os, sr, seg_samples, seg->n);
            SherpaOnnxDecodeOfflineStream(sv, os);
            const SherpaOnnxOfflineRecognizerResult* r = SherpaOnnxGetOfflineStreamResult(os);
            const char* text = (r && r->text) ? r->text : "";
            int32_t a0 = seg->start, a1 = seg->start + seg->n;
            std::printf("  [final %d] %02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d  %s\n",
                        seg_idx,
                        a0/sr/60, (a0/sr)%60, (a0%sr)/1000, a0%1000,
                        a1/sr/60, (a1/sr)%60, (a1%sr)/1000, a1%1000, text);
            seg_idx++;
            SherpaOnnxDestroyOfflineStream(os);
            if (r) SherpaOnnxDestroyOfflineRecognizerResult(r);
            SherpaOnnxVoiceActivityDetectorPop(vad);
            SherpaOnnxDestroySpeechSegment(seg);
        }
    }
    std::printf("hybrid_probe: 融合链路已接通（实时逐字 Zipformer2-CTC + 精准终稿 SenseVoice）\n");
    SherpaOnnxDestroyOnlineStream(zs);
    SherpaOnnxDestroyOnlineRecognizer(zf);
    SherpaOnnxDestroyOfflineRecognizer(sv);
    SherpaOnnxDestroyVoiceActivityDetector(vad);
    std::printf("hybrid_probe: done（真实中文分句/识别正确性需真机音频回填）\n");
    return 0;
}
